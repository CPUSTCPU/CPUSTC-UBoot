// SPDX-License-Identifier: GPL-2.0+
/*
 * Loongson LS1A-compatible NAND controller support.
 *
 * The register programming and DMA descriptor format follow the matching
 * Linux ls1a_nand driver and the platform MMC DMA driver.
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <memalign.h>
#include <nand.h>
#include <asm/cache.h>
#include <asm/cacheops.h>
#include <asm/io.h>
#include <linux/io.h>
#include <linux/ioport.h>

#define LS1A_NAND_CMD           0x00
#define LS1A_NAND_ADDR_LOW      0x04
#define LS1A_NAND_ADDR_HIGH     0x08
#define LS1A_NAND_TIMING        0x0c
#define LS1A_NAND_ID_LOW        0x10
#define LS1A_NAND_STATUS_IDHIGH 0x14
#define LS1A_NAND_PARAM         0x18
#define LS1A_NAND_OP_NUM        0x1c
#define LS1A_NAND_CE_MAP0       0x20

#define LS1A_CMD_VALID          BIT(0)
#define LS1A_CMD_READ           BIT(1)
#define LS1A_CMD_WRITE          BIT(2)
#define LS1A_CMD_ERASE_ONE      BIT(3)
#define LS1A_CMD_READ_ID        BIT(5)
#define LS1A_CMD_RESET          BIT(6)
#define LS1A_CMD_OP_MAIN        BIT(8)
#define LS1A_CMD_OP_SPARE       BIT(9)
#define LS1A_CMD_DONE           BIT(10)

#define LS1A_DMA_INT_MASK       BIT(0)
#define LS1A_DMA_START          BIT(3)
#define LS1A_DMA_WRITE          BIT(12)
#define LS1A_DMA_ACCESS_OFFSET  0x40

#define LS1A_NAND_PARAM_1G      0x08005300
#define LS1A_NAND_CS_RDY_MAP    0x88442200
#define LS1A_NAND_TIMING_VALUE  0x00000412
#define LS1A_NAND_TIMEOUT_MS    2000
#define LS1A_NAND_DIAG_TIMEOUT_MS 50
#define LS1A_NAND_BUFFER_SIZE   4096
#define LS1A_DMA_DESC_HW_ALIGN  32
#define LS1A_DMA_DESC_ALIGN     max(LS1A_DMA_DESC_HW_ALIGN, ARCH_DMA_MINALIGN)
#define LS1A_L2_CACHE_LINE_SIZE 64

enum ls1a_wait_stage {
	LS1A_WAIT_NONE,
	LS1A_WAIT_DMA_START,
	LS1A_WAIT_NAND_DONE,
};

enum ls1a_dma_mode {
	LS1A_DMA_BASELINE,
	LS1A_DMA_DBAR,
	LS1A_DMA_L2,
	LS1A_DMA_UNCACHED,
	LS1A_DMA_MODE_COUNT,
};

struct ls1a_dma_desc {
	u32 order_addr;
	u32 source_addr;
	u32 dest_addr;
	u32 length;
	u32 step_length;
	u32 step_times;
	u32 command;
};

struct ls1a_nand_priv {
	struct nand_chip chip;
	void __iomem *nand_base;
	void __iomem *dma_order_reg;
	u32 dma_access_addr;
	u8 data[8];
	u8 *buffer;
	struct ls1a_dma_desc *desc;
	unsigned int pos;
	unsigned int count;
	unsigned int seqin_column;
	unsigned int seqin_page;
	unsigned int last_command;
	int last_error;
	bool buffer_active;
	enum ls1a_dma_mode dma_mode;
	enum ls1a_wait_stage wait_stage;
	ulong wait_elapsed_ms;
	u32 wait_value;
	u32 wait_dma_order;
	u32 wait_nand_cmd;
};

static struct ls1a_nand_priv *ls1a_diag_priv;

static void ls1a_write(struct ls1a_nand_priv *priv, u32 value,
		       unsigned int reg)
{
	writel(value, priv->nand_base + reg);
}

static u32 ls1a_read(struct ls1a_nand_priv *priv, unsigned int reg)
{
	return readl(priv->nand_base + reg);
}

static const char *ls1a_wait_stage_name(enum ls1a_wait_stage stage)
{
	switch (stage) {
	case LS1A_WAIT_DMA_START:
		return "DMA_START_TIMEOUT";
	case LS1A_WAIT_NAND_DONE:
		return "NAND_DONE_TIMEOUT";
	default:
		return "NONE";
	}
}

static const char *ls1a_dma_mode_name(enum ls1a_dma_mode mode)
{
	switch (mode) {
	case LS1A_DMA_BASELINE:
		return "baseline";
	case LS1A_DMA_DBAR:
		return "dbar";
	case LS1A_DMA_L2:
		return "l2";
	case LS1A_DMA_UNCACHED:
		return "uncached";
	default:
		return "unknown";
	}
}

static void ls1a_data_barrier(void)
{
	sync();
}

static void *ls1a_uncached_alias(void *addr)
{
	return (void *)PHYS_TO_UNCACHED(virt_to_phys(addr));
}

static void ls1a_l1_wbinv_range(void *buffer, unsigned int length)
{
	ulong addr = (ulong)buffer & ~(ARCH_DMA_MINALIGN - 1);
	ulong end = roundup((ulong)buffer + length, ARCH_DMA_MINALIGN);

	for (; addr < end; addr += ARCH_DMA_MINALIGN)
		la32r_cache(HIT_WRITEBACK_INV_D,
			     (const void *)addr);
}

#ifdef CONFIG_CPUSTC_L2_CACHE
static void ls1a_l2_wbinv_range(void *buffer, unsigned int length)
{
	ulong addr = (ulong)buffer & ~(LS1A_L2_CACHE_LINE_SIZE - 1);
	ulong end = roundup((ulong)buffer + length, LS1A_L2_CACHE_LINE_SIZE);

	for (; addr < end; addr += LS1A_L2_CACHE_LINE_SIZE)
		la32r_cache(HIT_WRITEBACK_INV_L2,
			     (const void *)addr);
}
#endif

static int ls1a_wait_mask(struct ls1a_nand_priv *priv, void __iomem *addr,
			  u32 mask, bool set, enum ls1a_wait_stage stage,
			  ulong timeout_ms)
{
	ulong start = get_timer(0);

	while (!!(readl(addr) & mask) != set) {
		if (get_timer(start) >= timeout_ms) {
			priv->wait_stage = stage;
			priv->wait_elapsed_ms = get_timer(start);
			priv->wait_value = readl(addr);
			priv->wait_dma_order = readl(priv->dma_order_reg);
			priv->wait_nand_cmd = ls1a_read(priv, LS1A_NAND_CMD);
			return -ETIMEDOUT;
		}
	}

	priv->wait_stage = LS1A_WAIT_NONE;
	priv->wait_elapsed_ms = get_timer(start);
	priv->wait_value = readl(addr);
	return 0;
}

static int ls1a_wait_done(struct ls1a_nand_priv *priv, ulong timeout_ms)
{
	return ls1a_wait_mask(priv, priv->nand_base + LS1A_NAND_CMD,
			      LS1A_CMD_DONE, true, LS1A_WAIT_NAND_DONE,
			      timeout_ms);
}

static void ls1a_start_command(struct ls1a_nand_priv *priv, u32 command)
{
	ls1a_write(priv, 0, LS1A_NAND_CMD);
	ls1a_write(priv, command | LS1A_CMD_VALID, LS1A_NAND_CMD);
}

static void ls1a_set_operation(struct ls1a_nand_priv *priv,
			       unsigned int page, unsigned int column,
			       unsigned int length, u32 command)
{
	u32 param = ls1a_read(priv, LS1A_NAND_PARAM);

	ls1a_write(priv, 0, LS1A_NAND_CMD);
	ls1a_write(priv, column, LS1A_NAND_ADDR_LOW);
	ls1a_write(priv, page, LS1A_NAND_ADDR_HIGH);
	ls1a_write(priv, length, LS1A_NAND_OP_NUM);
	param = (param & 0xc000ffff) | (length << 16);
	ls1a_write(priv, param, LS1A_NAND_PARAM);
	ls1a_write(priv, command | LS1A_CMD_VALID, LS1A_NAND_CMD);
}

static void ls1a_flush_buffer(void *buffer, unsigned int length)
{
	ls1a_l1_wbinv_range(buffer, length);
}

static void ls1a_cache_before_dma(void *buffer, unsigned int length,
				  enum ls1a_dma_mode mode)
{
	ls1a_flush_buffer(buffer, length);
#ifdef CONFIG_CPUSTC_L2_CACHE
	if (mode == LS1A_DMA_L2 || mode == LS1A_DMA_UNCACHED)
		ls1a_l2_wbinv_range(buffer, length);
#endif
	if (mode != LS1A_DMA_BASELINE)
		ls1a_data_barrier();
}

static void ls1a_cache_after_dma(void *buffer, unsigned int length,
				 enum ls1a_dma_mode mode)
{
	ls1a_l1_wbinv_range(buffer, length);
	if (mode != LS1A_DMA_BASELINE)
		ls1a_data_barrier();
}

static int ls1a_dma_transfer_mode(struct ls1a_nand_priv *priv,
				  unsigned int length, bool write,
				  enum ls1a_dma_mode mode,
				  ulong timeout_ms)
{
	struct ls1a_dma_desc *desc;
	void *buffer;
	int ret;

	if (mode == LS1A_DMA_UNCACHED && write)
		return -ENOTSUPP;

	if (mode == LS1A_DMA_UNCACHED) {
		/* Remove cached aliases before the diagnostic writes through KSEG1. */
		ls1a_cache_before_dma(priv->buffer, length, mode);
		ls1a_cache_before_dma(priv->desc, sizeof(*priv->desc), mode);
		desc = ls1a_uncached_alias(priv->desc);
		buffer = ls1a_uncached_alias(priv->buffer);
	} else {
		desc = priv->desc;
		buffer = priv->buffer;
	}

	desc->order_addr = 0;
	desc->source_addr = virt_to_phys(priv->buffer);
	desc->dest_addr = priv->dma_access_addr;
	desc->length = DIV_ROUND_UP(length, sizeof(u32));
	desc->step_length = 0;
	desc->step_times = 1;
	desc->command = LS1A_DMA_INT_MASK | (write ? LS1A_DMA_WRITE : 0);

	if (mode != LS1A_DMA_UNCACHED) {
		ls1a_cache_before_dma(buffer, length, mode);
		ls1a_cache_before_dma(desc, sizeof(*desc), mode);
	} else {
		ls1a_data_barrier();
	}
	writel(virt_to_phys(desc) | LS1A_DMA_START, priv->dma_order_reg);
	if (mode != LS1A_DMA_BASELINE)
		ls1a_data_barrier();

	ret = ls1a_wait_mask(priv, priv->dma_order_reg, LS1A_DMA_START,
			     false, LS1A_WAIT_DMA_START, timeout_ms);
	if (ret)
		return ret;

	ret = ls1a_wait_done(priv, timeout_ms);
	if (ret)
		return ret;

	if (!write && mode != LS1A_DMA_UNCACHED)
		ls1a_cache_after_dma(priv->buffer, length, mode);

	return 0;
}

static int ls1a_dma_transfer(struct ls1a_nand_priv *priv,
			     unsigned int length, bool write)
{
	return ls1a_dma_transfer_mode(priv, length, write, priv->dma_mode,
				      LS1A_NAND_TIMEOUT_MS);
}

static void ls1a_print_failure(struct ls1a_nand_priv *priv)
{
	printf("stage=%s elapsed=%lums order=%08x cmd=%08x mode=%s",
	       ls1a_wait_stage_name(priv->wait_stage), priv->wait_elapsed_ms,
	       priv->wait_dma_order, priv->wait_nand_cmd,
	       ls1a_dma_mode_name(priv->dma_mode));
}

static void ls1a_read_id(struct ls1a_nand_priv *priv)
{
	u32 id_low;
	u32 id_high;

	ls1a_start_command(priv, LS1A_CMD_READ_ID);
	udelay(1);
	id_low = ls1a_read(priv, LS1A_NAND_ID_LOW);
	id_high = ls1a_read(priv, LS1A_NAND_STATUS_IDHIGH);

	priv->data[0] = id_high & 0xff;
	priv->data[1] = (id_low >> 24) & 0xff;
	priv->data[2] = (id_low >> 16) & 0xff;
	priv->data[3] = (id_low >> 8) & 0xff;
	priv->data[4] = id_low & 0xff;
	priv->pos = 0;
	priv->count = 5;
	priv->buffer_active = false;
}

static void ls1a_read_page(struct mtd_info *mtd,
			   struct ls1a_nand_priv *priv, int page_addr,
			   bool oob_only)
{
	unsigned int column = oob_only ? mtd->writesize : 0;
	unsigned int length = oob_only ? mtd->oobsize :
						mtd->writesize + mtd->oobsize;
	u32 command = LS1A_CMD_READ | LS1A_CMD_OP_SPARE;

	if (!oob_only)
		command |= LS1A_CMD_OP_MAIN;

	priv->pos = 0;
	priv->count = length;
	priv->buffer_active = true;
	priv->last_error = 0;
	ls1a_set_operation(priv, page_addr, column, length, command);
	priv->last_error = ls1a_dma_transfer(priv, length, false);
	if (priv->last_error) {
		memset(priv->buffer, 0xff, length);
		printf("LS1A NAND: page %d read failed (%d, ",
		       page_addr, priv->last_error);
		ls1a_print_failure(priv);
		puts(")\n");
	}
}

static void ls1a_program_page(struct mtd_info *mtd,
			      struct ls1a_nand_priv *priv)
{
	unsigned int length = priv->pos;
	u32 command = LS1A_CMD_WRITE | LS1A_CMD_OP_SPARE;

	if (!length) {
		priv->last_error = -EINVAL;
		return;
	}

	if (priv->seqin_column < mtd->writesize)
		command |= LS1A_CMD_OP_MAIN;

	ls1a_set_operation(priv, priv->seqin_page, priv->seqin_column,
			   length, command);
	priv->last_error = ls1a_dma_transfer(priv, length, true);
	if (priv->last_error) {
		printf("LS1A NAND: page %u program failed (%d, ",
		       priv->seqin_page, priv->last_error);
		ls1a_print_failure(priv);
		puts(")\n");
	}
}

static void ls1a_erase_block(struct ls1a_nand_priv *priv, int page_addr)
{
	priv->last_error = 0;
	ls1a_set_operation(priv, page_addr, 0, 0, LS1A_CMD_ERASE_ONE);
	priv->last_error = ls1a_wait_done(priv, LS1A_NAND_TIMEOUT_MS);
	if (priv->last_error)
		printf("LS1A NAND: block at page %d erase failed (%d)\n",
		       page_addr, priv->last_error);
}

static void ls1a_cmdfunc(struct mtd_info *mtd, unsigned int command,
			 int column, int page_addr)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct ls1a_nand_priv *priv = nand_get_controller_data(chip);

	priv->last_command = command;

	switch (command) {
	case NAND_CMD_RESET:
		priv->buffer_active = false;
		priv->last_error = 0;
		ls1a_start_command(priv, LS1A_CMD_RESET);
		priv->last_error = ls1a_wait_done(priv,
						 LS1A_NAND_TIMEOUT_MS);
		if (priv->last_error)
			printf("LS1A NAND: reset timed out\n");
		break;
	case NAND_CMD_READID:
		ls1a_read_id(priv);
		break;
	case NAND_CMD_STATUS:
		priv->data[0] =
			(ls1a_read(priv, LS1A_NAND_STATUS_IDHIGH) >> 16) |
			NAND_STATUS_WP;
		priv->pos = 0;
		priv->count = 1;
		priv->buffer_active = false;
		break;
	case NAND_CMD_READ0:
		ls1a_read_page(mtd, priv, page_addr, false);
		break;
	case NAND_CMD_READOOB:
		ls1a_read_page(mtd, priv, page_addr, true);
		break;
	case NAND_CMD_RNDOUT:
		priv->pos = column;
		break;
	case NAND_CMD_SEQIN:
		priv->seqin_column = column;
		priv->seqin_page = page_addr;
		priv->pos = 0;
		priv->count = mtd->writesize + mtd->oobsize - column;
		priv->buffer_active = true;
		priv->last_error = 0;
		memset(priv->buffer, 0xff, LS1A_NAND_BUFFER_SIZE);
		break;
	case NAND_CMD_PAGEPROG:
		ls1a_program_page(mtd, priv);
		break;
	case NAND_CMD_ERASE1:
		ls1a_erase_block(priv, page_addr);
		break;
	case NAND_CMD_ERASE2:
	case NAND_CMD_READ1:
		break;
	default:
		printf("LS1A NAND: unsupported command 0x%x\n", command);
		break;
	}
}

static u8 ls1a_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct ls1a_nand_priv *priv = nand_get_controller_data(chip);
	u8 *buffer = priv->buffer_active ? priv->buffer : priv->data;

	if (priv->pos >= priv->count)
		return 0xff;

	return buffer[priv->pos++];
}

static void ls1a_read_buf(struct mtd_info *mtd, u8 *buf, int len)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct ls1a_nand_priv *priv = nand_get_controller_data(chip);
	unsigned int available = priv->pos < priv->count ?
				 priv->count - priv->pos : 0;
	unsigned int copy = min_t(unsigned int, len, available);

	memcpy(buf, priv->buffer + priv->pos, copy);
	if (copy < len)
		memset(buf + copy, 0xff, len - copy);
	priv->pos += copy;
}

static void ls1a_write_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct ls1a_nand_priv *priv = nand_get_controller_data(chip);
	unsigned int available = priv->pos < priv->count ?
				 priv->count - priv->pos : 0;
	unsigned int copy = min_t(unsigned int, len, available);

	memcpy(priv->buffer + priv->pos, buf, copy);
	priv->pos += copy;
}

static int ls1a_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct ls1a_nand_priv *priv = nand_get_controller_data(chip);

	return !!(ls1a_read(priv, LS1A_NAND_CMD) & LS1A_CMD_DONE);
}

static int ls1a_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct ls1a_nand_priv *priv = nand_get_controller_data(chip);

	if (priv->last_error ||
	    ls1a_wait_done(priv, LS1A_NAND_TIMEOUT_MS))
		return NAND_STATUS_FAIL;

	return (ls1a_read(priv, LS1A_NAND_STATUS_IDHIGH) >> 16) |
		NAND_STATUS_WP;
}

static void ls1a_select_chip(struct mtd_info *mtd, int chipnr)
{
}

static int ls1a_nand_probe(struct udevice *dev)
{
	struct ls1a_nand_priv *priv = dev_get_priv(dev);
	struct nand_chip *chip = &priv->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct resource nand_res;
	struct resource dma_order_res;
	int ret;

	ret = dev_read_resource_byname(dev, "nand", &nand_res);
	if (ret)
		return ret;
	ret = dev_read_resource_byname(dev, "dma-order", &dma_order_res);
	if (ret)
		return ret;

	priv->nand_base = devm_ioremap(dev, nand_res.start,
					resource_size(&nand_res));
	priv->dma_order_reg = devm_ioremap(dev, dma_order_res.start,
					    resource_size(&dma_order_res));
	if (!priv->nand_base || !priv->dma_order_reg)
		return -ENOMEM;
	priv->dma_access_addr = nand_res.start + LS1A_DMA_ACCESS_OFFSET;

	priv->buffer = memalign(ARCH_DMA_MINALIGN, LS1A_NAND_BUFFER_SIZE);
	priv->desc = memalign(LS1A_DMA_DESC_ALIGN,
			      roundup(sizeof(*priv->desc), ARCH_DMA_MINALIGN));
	if (!priv->buffer || !priv->desc) {
		free(priv->desc);
		free(priv->buffer);
		return -ENOMEM;
	}
	memset(priv->buffer, 0xff, LS1A_NAND_BUFFER_SIZE);
	memset(priv->desc, 0, sizeof(*priv->desc));
#ifdef CONFIG_CPUSTC_L2_CACHE
	priv->dma_mode = LS1A_DMA_L2;
#else
	priv->dma_mode = LS1A_DMA_DBAR;
#endif

	ls1a_write(priv, LS1A_NAND_TIMING_VALUE, LS1A_NAND_TIMING);
	ls1a_write(priv, LS1A_NAND_PARAM_1G, LS1A_NAND_PARAM);
	ls1a_write(priv, LS1A_NAND_CS_RDY_MAP, LS1A_NAND_CE_MAP0);
	ls1a_write(priv, 0, LS1A_NAND_OP_NUM);

	nand_set_controller_data(chip, priv);
	chip->IO_ADDR_R = chip->IO_ADDR_W = priv->nand_base;
	chip->cmdfunc = ls1a_cmdfunc;
	chip->read_byte = ls1a_read_byte;
	chip->read_buf = ls1a_read_buf;
	chip->write_buf = ls1a_write_buf;
	chip->select_chip = ls1a_select_chip;
	chip->dev_ready = ls1a_dev_ready;
	chip->waitfunc = ls1a_waitfunc;
	chip->chip_delay = 50;
	chip->options |= NAND_NO_SUBPAGE_WRITE;
	chip->ecc.mode = NAND_ECC_SOFT;

	ret = nand_scan(mtd, CONFIG_SYS_NAND_MAX_CHIPS);
	if (ret)
		return ret;
	ret = nand_register(0, mtd);
	if (ret)
		return ret;

	ls1a_diag_priv = priv;

	return 0;
}

static const struct udevice_id ls1a_nand_ids[] = {
	{ .compatible = "loongson,ls1a-nand" },
	{ .compatible = "ls1a-nand" },
	{ }
};

U_BOOT_DRIVER(ls1a_nand) = {
	.name = "ls1a-nand",
	.id = UCLASS_MTD,
	.of_match = ls1a_nand_ids,
	.probe = ls1a_nand_probe,
	.priv_auto_alloc_size = sizeof(struct ls1a_nand_priv),
};

void board_nand_init(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_driver(UCLASS_MTD,
					  DM_GET_DRIVER(ls1a_nand), &dev);
	if (ret && ret != -ENODEV)
		printf("LS1A NAND: probe failed (%d)\n", ret);
}

#ifdef CONFIG_CMD_LS1A_NAND_DIAG
struct ls1a_diag_stats {
	unsigned int pass;
	unsigned int fail;
	unsigned int dma_timeout;
	unsigned int nand_timeout;
};

static int ls1a_diag_parse_mode(const char *name, enum ls1a_dma_mode *mode)
{
	if (!strcmp(name, "baseline"))
		*mode = LS1A_DMA_BASELINE;
	else if (!strcmp(name, "dbar"))
		*mode = LS1A_DMA_DBAR;
	else if (!strcmp(name, "l2") || !strcmp(name, "l1l2"))
		*mode = LS1A_DMA_L2;
	else if (!strcmp(name, "uncached"))
		*mode = LS1A_DMA_UNCACHED;
	else
		return -EINVAL;

	return 0;
}

static u32 ls1a_diag_hash(const volatile u8 *data, unsigned int length)
{
	u32 hash = 2166136261U;
	unsigned int i;

	for (i = 0; i < length; i++) {
		hash ^= data[i];
		hash *= 16777619U;
	}

	return hash;
}

static void ls1a_diag_dump_status(struct mtd_info *mtd,
				  struct ls1a_nand_priv *priv)
{
	volatile struct ls1a_dma_desc *uncached_desc =
		ls1a_uncached_alias(priv->desc);

	printf("NANDDIAG config mode=%s normal_timeout_ms=%u "
	       "diag_timeout_ms=%u uboot_cacheline=%u l2_cacheline=%u\n",
	       ls1a_dma_mode_name(priv->dma_mode), LS1A_NAND_TIMEOUT_MS,
	       LS1A_NAND_DIAG_TIMEOUT_MS, ARCH_DMA_MINALIGN,
	       LS1A_L2_CACHE_LINE_SIZE);
	printf("NANDDIAG geometry size=0x%llx erase=0x%x page=%u oob=%u\n",
	       mtd->size, mtd->erasesize, mtd->writesize, mtd->oobsize);
	printf("NANDDIAG memory buffer_v=%08lx buffer_p=%08lx align64=%lu "
	       "desc_v=%08lx desc_p=%08lx align32=%lu align64=%lu\n",
	       (ulong)priv->buffer, virt_to_phys(priv->buffer),
	       (ulong)priv->buffer & (LS1A_L2_CACHE_LINE_SIZE - 1),
	       (ulong)priv->desc, virt_to_phys(priv->desc),
	       (ulong)priv->desc & (LS1A_DMA_DESC_HW_ALIGN - 1),
	       (ulong)priv->desc & (LS1A_L2_CACHE_LINE_SIZE - 1));
	printf("NANDDIAG regs cmd=%08x addr_lo=%08x addr_hi=%08x timing=%08x "
	       "param=%08x op_num=%08x ce_map=%08x order=%08x\n",
	       ls1a_read(priv, LS1A_NAND_CMD),
	       ls1a_read(priv, LS1A_NAND_ADDR_LOW),
	       ls1a_read(priv, LS1A_NAND_ADDR_HIGH),
	       ls1a_read(priv, LS1A_NAND_TIMING),
	       ls1a_read(priv, LS1A_NAND_PARAM),
	       ls1a_read(priv, LS1A_NAND_OP_NUM),
	       ls1a_read(priv, LS1A_NAND_CE_MAP0),
	       readl(priv->dma_order_reg));
	printf("NANDDIAG desc_cached order=%08x src=%08x dst=%08x len=%08x "
	       "step_len=%08x step_times=%08x command=%08x\n",
	       priv->desc->order_addr, priv->desc->source_addr,
	       priv->desc->dest_addr, priv->desc->length,
	       priv->desc->step_length, priv->desc->step_times,
	       priv->desc->command);
	printf("NANDDIAG desc_uncached order=%08x src=%08x dst=%08x len=%08x "
	       "step_len=%08x step_times=%08x command=%08x\n",
	       uncached_desc->order_addr, uncached_desc->source_addr,
	       uncached_desc->dest_addr, uncached_desc->length,
	       uncached_desc->step_length, uncached_desc->step_times,
	       uncached_desc->command);
}

static int ls1a_diag_recover(struct ls1a_nand_priv *priv)
{
	int ret;

	writel(0, priv->dma_order_reg);
	ls1a_data_barrier();
	ls1a_start_command(priv, LS1A_CMD_RESET);
	ls1a_data_barrier();
	ret = ls1a_wait_done(priv, LS1A_NAND_DIAG_TIMEOUT_MS);
	if (ret)
		printf("NANDDIAG recover result=FAIL stage=%s elapsed=%lums "
		       "order=%08x cmd=%08x\n",
		       ls1a_wait_stage_name(priv->wait_stage),
		       priv->wait_elapsed_ms, priv->wait_dma_order,
		       priv->wait_nand_cmd);

	return ret;
}

static int ls1a_diag_cache_check(struct ls1a_nand_priv *priv)
{
	u32 *cached = (u32 *)priv->buffer;
	volatile u32 *uncached = ls1a_uncached_alias(priv->buffer);
	const unsigned int words = LS1A_L2_CACHE_LINE_SIZE / sizeof(u32);
	unsigned int l1_bad = 0;
	unsigned int l2_bad = 0;
	unsigned int i;

	/* Start from a clean line, then make DDR contents deterministic. */
	ls1a_cache_before_dma(cached, LS1A_L2_CACHE_LINE_SIZE, LS1A_DMA_L2);
	for (i = 0; i < words; i++)
		uncached[i] = 0;
	ls1a_data_barrier();

	for (i = 0; i < words; i++)
		cached[i] = 0x4c310000U ^ i;
	ls1a_cache_before_dma(cached, LS1A_L2_CACHE_LINE_SIZE, LS1A_DMA_DBAR);
	for (i = 0; i < words; i++) {
		if (uncached[i] != (0x4c310000U ^ i))
			l1_bad++;
	}
	printf("NANDDIAG cache mode=l1_only result=%s mismatches=%u "
	       "cached0=%08x uncached0=%08x\n",
	       l1_bad ? "FAIL" : "PASS", l1_bad,
	       cached[0], uncached[0]);

	for (i = 0; i < words; i++)
		cached[i] = 0x4c320000U ^ i;
	ls1a_cache_before_dma(cached, LS1A_L2_CACHE_LINE_SIZE, LS1A_DMA_L2);
	for (i = 0; i < words; i++) {
		if (uncached[i] != (0x4c320000U ^ i))
			l2_bad++;
	}
	printf("NANDDIAG cache mode=l2 result=%s mismatches=%u "
	       "cached0=%08x uncached0=%08x\n",
	       l2_bad ? "FAIL" : "PASS", l2_bad,
	       cached[0], uncached[0]);

	return l2_bad ? -EIO : 0;
}

static int ls1a_diag_read_once(struct mtd_info *mtd,
			       struct ls1a_nand_priv *priv,
			       unsigned int page, bool full,
			       enum ls1a_dma_mode mode,
			       struct ls1a_diag_stats *stats)
{
	const volatile u8 *view;
	unsigned int column = full ? 0 : mtd->writesize;
	unsigned int length = full ? mtd->writesize + mtd->oobsize :
				     mtd->oobsize;
	u32 command = LS1A_CMD_READ | LS1A_CMD_OP_SPARE;
	u32 hash = 0;
	int ret;

	if (full)
		command |= LS1A_CMD_OP_MAIN;

	ret = ls1a_diag_recover(priv);
	if (!ret) {
		ls1a_set_operation(priv, page, column, length, command);
		ret = ls1a_dma_transfer_mode(priv, length, false, mode,
					     LS1A_NAND_DIAG_TIMEOUT_MS);
	}

	view = mode == LS1A_DMA_UNCACHED ?
		ls1a_uncached_alias(priv->buffer) : priv->buffer;
	if (!ret)
		hash = ls1a_diag_hash(view, length);

	printf("NANDDIAG read page=%u kind=%s mode=%s result=%s stage=%s "
	       "elapsed=%lums order=%08x cmd=%08x hash=%08x first=%02x%02x%02x%02x\n",
	       page, full ? "full" : "oob", ls1a_dma_mode_name(mode),
	       ret ? "FAIL" : "PASS", ls1a_wait_stage_name(priv->wait_stage),
	       priv->wait_elapsed_ms, readl(priv->dma_order_reg),
	       ls1a_read(priv, LS1A_NAND_CMD), hash,
	       view[0], view[1], view[2], view[3]);

	if (!stats)
		return ret;
	if (!ret) {
		stats->pass++;
	} else {
		stats->fail++;
		if (priv->wait_stage == LS1A_WAIT_DMA_START)
			stats->dma_timeout++;
		else if (priv->wait_stage == LS1A_WAIT_NAND_DONE)
			stats->nand_timeout++;
	}

	return ret;
}

static int ls1a_diag_run_all(struct mtd_info *mtd,
			     struct ls1a_nand_priv *priv,
			     unsigned int repeat)
{
	static const unsigned int pages[] = { 0, 1, 63, 64, 65 };
	static const enum ls1a_dma_mode modes[] = {
		LS1A_DMA_BASELINE,
		LS1A_DMA_DBAR,
		LS1A_DMA_L2,
		LS1A_DMA_UNCACHED,
	};
	enum ls1a_dma_mode saved_mode = priv->dma_mode;
	struct ls1a_diag_stats total = { 0 };
	unsigned int mode_idx;
	unsigned int page_idx;
	unsigned int iteration;
	unsigned int full;
	int l2_cache_ret;

	ls1a_diag_dump_status(mtd, priv);
	l2_cache_ret = ls1a_diag_cache_check(priv);

	for (mode_idx = 0; mode_idx < ARRAY_SIZE(modes); mode_idx++) {
		struct ls1a_diag_stats stats = { 0 };

		for (full = 0; full < 2; full++) {
			for (page_idx = 0; page_idx < ARRAY_SIZE(pages);
			     page_idx++) {
				for (iteration = 0; iteration < repeat;
				     iteration++)
					ls1a_diag_read_once(mtd, priv,
							    pages[page_idx], full,
							    modes[mode_idx], &stats);
			}
		}
		printf("NANDDIAG summary mode=%s pass=%u fail=%u "
		       "dma_timeout=%u nand_timeout=%u\n",
		       ls1a_dma_mode_name(modes[mode_idx]), stats.pass,
		       stats.fail, stats.dma_timeout, stats.nand_timeout);
		total.pass += stats.pass;
		total.fail += stats.fail;
		total.dma_timeout += stats.dma_timeout;
		total.nand_timeout += stats.nand_timeout;
	}

	priv->dma_mode = saved_mode;
	printf("NANDDIAG complete repeat=%u restored_mode=%s l2_cache=%s "
	       "pass=%u fail=%u dma_timeout=%u nand_timeout=%u\n",
	       repeat, ls1a_dma_mode_name(saved_mode),
	       l2_cache_ret ? "FAIL" : "PASS", total.pass, total.fail,
	       total.dma_timeout, total.nand_timeout);

	return l2_cache_ret || total.fail ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_ls1a_nanddiag(cmd_tbl_t *cmdtp, int flag, int argc,
			    char * const argv[])
{
	struct ls1a_nand_priv *priv = ls1a_diag_priv;
	struct mtd_info *mtd = get_nand_dev_by_index(0);
	enum ls1a_dma_mode mode;
	unsigned long page;
	unsigned long repeat;
	bool full;
	int ret = 0;

	if (!priv || !mtd) {
		puts("NANDDIAG NAND device 0 is unavailable\n");
		return CMD_RET_FAILURE;
	}
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "status")) {
		if (argc != 2)
			return CMD_RET_USAGE;
		ls1a_diag_dump_status(mtd, priv);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "cache")) {
		if (argc != 2)
			return CMD_RET_USAGE;
		return ls1a_diag_cache_check(priv) ? CMD_RET_FAILURE :
			CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "mode")) {
		if (argc != 3 || ls1a_diag_parse_mode(argv[2], &mode) ||
		    mode == LS1A_DMA_UNCACHED)
			return CMD_RET_USAGE;
		priv->dma_mode = mode;
		printf("NANDDIAG normal DMA mode=%s\n",
		       ls1a_dma_mode_name(priv->dma_mode));
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "read")) {
		if (argc < 5 || argc > 6 ||
		    ls1a_diag_parse_mode(argv[4], &mode))
			return CMD_RET_USAGE;
		page = simple_strtoul(argv[2], NULL, 0);
		full = !strcmp(argv[3], "full");
		if (!full && strcmp(argv[3], "oob"))
			return CMD_RET_USAGE;
		repeat = argc == 6 ? simple_strtoul(argv[5], NULL, 0) : 1;
		if (!repeat || repeat > 32 ||
		    page >= (mtd->size / mtd->writesize))
			return CMD_RET_USAGE;
		while (repeat--)
			if (ls1a_diag_read_once(mtd, priv, page, full, mode,
						NULL))
				ret = CMD_RET_FAILURE;
		return ret;
	}

	if (!strcmp(argv[1], "all")) {
		if (argc > 3)
			return CMD_RET_USAGE;
		repeat = argc == 3 ? simple_strtoul(argv[2], NULL, 0) : 1;
		if (!repeat || repeat > 32)
			return CMD_RET_USAGE;
		return ls1a_diag_run_all(mtd, priv, repeat);
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	nanddiag, 6, 0, do_ls1a_nanddiag,
	"run read-only LS1A NAND DMA and cache diagnostics",
	"status\n"
	"nanddiag cache\n"
	"nanddiag mode <baseline|dbar|l2>\n"
	"nanddiag read <page> <oob|full> <baseline|dbar|l2|uncached> [repeat]\n"
	"nanddiag all [repeat]\n"
	"    - all tests cache visibility plus pages 0,1,63,64,65 in OOB/full modes\n"
	"    - diagnostics only read/reset NAND; mode changes the following normal NAND DMA"
);
#endif
