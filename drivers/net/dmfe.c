// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2009 Michal Simek
 * (C) Copyright 2003 Xilinx Inc.
 *
 * Michal SIMEK <monstr@monstr.eu>
 */

#include <common.h>
#include <net.h>
#include <config.h>
#include <display_options.h>
#include <dm.h>
#include <console.h>
#include <malloc.h>
#include <asm/io.h>
#include <fdtdec.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <asm/addrspace.h>


#define MAC_DEBUG
#ifdef MAC_DEBUG
static int dmfe_debug;
#define dmfe_dbg(...)                           \
        do {                                    \
                if (dmfe_debug)                \
                        printf(__VA_ARGS__);   \
        } while (0)

#else
#define dmfe_dbg(...)
#endif

/*
 * NOTE!  On the Alpha, we have an alignment constraint.  The
 * card DMAs the packet immediately following the RFA.  However,
 * the first thing in the packet is a 14-byte Ethernet header.
 * This means that the packet is misaligned.  To compensate,
 * we actually offset the RFA 2 bytes into the cluster.  This
 * aligns the packet after the Ethernet header at a 32-bit
 * boundary.  HOWEVER!  This means that the RFA is misaligned!
 */

#ifdef BADPCIBRIDGE
#define BADPCIBRIDGE
#define	RFA_ALIGNMENT_FUDGE	4
#else
#define	RFA_ALIGNMENT_FUDGE	2
#endif

#undef DEBUG_SROM
#undef DEBUG_SROM2

#undef UPDATE_SROM

/* PCI Registers.
 */
#define PCI_CFDA_PSM		0x43

#define CFRV_RN		0x000000f0	/* Revision Number */

#define WAKEUP		0x00		/* Power Saving Wakeup */
#define SLEEP		0x80		/* Power Saving Sleep Mode */

#define DC2114x_BRK	0x0020		/* CFRV break between DC21142 & DC21143 */

/* Ethernet chip registers.
 */
#define DE4X5_BMR	0x000		/* Bus Mode Register, CSR0 */
#define DE4X5_TPD	0x008		/* Transmit Poll Demand Reg, CSR1 */
#define CSR2		0x010		
#define DE4X5_RRBA	0x018		/* RX Ring Base Address Reg, CSR3*/
#define DE4X5_TRBA	0x020		/* TX Ring Base Address Reg, CSR4*/
#define DE4X5_STS	0x028		/* Status Register, CSR5 */
#define DE4X5_OMR	0x030		/* Operation Mode Register, CSR6 */
#define DE4X5_IMR	0x038		/* Interrupt Enable Register, CSR7 */
#define DE4X5_MFC	0x040		/* Missed Frame Counter, CSR8 */
#define DE4X5_SICR	0x068		/* SIA Connectivity Register, */
#define DE4X5_APROM	0x048		/* Ethernet Address PROM */

/* Register bits.
 */
#define BMR_SWR		0x00000001	/* Software Reset */
#define STS_TS		0x00700000	/* Transmit Process State */
#define STS_RS		0x000e0000	/* Receive Process State */
#define OMR_ST		0x00002000	/* Start/Stop Transmission Command */
#define OMR_SR		0x00000002	/* Start/Stop Receive */
#define OMR_PS		0x00040000	/* Port Select */
#define OMR_SDP		0x02000000	/* SD Polarity - MUST BE ASSERTED */
#define OMR_PM		0x00000080	/* Pass All Multicast */
#define OMR_FD		0x00000200	/* full duplex */
#define OMR_PR		0x00000040	/* promise mode */
#define OMR_TTM   0x00400000  /* 10M */
#define OMR_SF    0x00200000  /*     */

/* Descriptor bits.
 */
#define R_OWN		0x80000000	/* Own Bit */
#define RD_FF		0x40000000	/* Filtering Fail */
#define RD_RER		0x02000000	/* Receive End Of Ring */
#define RD_DE		0x00004000	/* Descriptor Error */
#define RD_LS		0x00000100	/* Last Descriptor */
#define RD_ES		0x00008000	/* Error Summary */
#define RD_RF		0x00000800	/* Runt Frame */
#define RD_MF		0x00000400	/* Multicast Frame */
#define RD_FS		0x00000200	/* First Descriptor */
#define RD_TL		0x00000080	/* Frame Too Long */
#define RD_CS		0x00000040	/* Collision Seen */
#define RD_FT		0x00000020	/* Frame Type */
#define RD_RE		0x00000008	/* Receive Error */
#define RD_DB		0x00000004	/* Dribbling Bit */
#define RD_CE		0x00000002	/* CRC Error */
#define RD_OF		0x00000001	/* FIFO Overflow */
#define TD_TER		0x02000000	/* Transmit End Of Ring */
#define TD_TCH		0x01000000	/* Second address chained */
#define T_OWN		0x80000000	/* Own Bit */
#define TD_LS		0x40000000	/* Last Segment */
#define TD_FS		0x20000000	/* First Segment */
#define TD_ES		0x00008000	/* Error Summary */
#define TD_SET		0x08000000	/* Setup Packet */

/* */


/* CR9 definition: SROM/MII */
#define DCR9           0x48
#define CR9_SROM_READ   0x4800
#define CR9_SRCS        0x1
#define CR9_SRCLK       0x2
#define CR9_CRDOUT      0x8
#define SROM_DATA_0     0x0
#define SROM_DATA_1     0x4
#define MII_DATA_IN     0x00080000
#define MII_DIR_OUT     0x00040000
#define PHY_DATA_1      0x20000
#define PHY_DATA_0      0x00000
#define MDCLKH          0x10000

#define MII_BMCR        0
#define MII_BMSR        1
#define MII_PHYSID1     2
#define MII_PHYSID2     3
#define MII_ADVERTISE   4
#define MII_LPA         5
#define MII_DSCSR       17
#define MII_RECR        22
#define MII_DISCR       23

#define BMCR_LOOPBACK   0x4000
#define BMCR_SPEED100   0x2000
#define BMCR_ANENABLE   0x1000
#define BMCR_RESTARTAN  0x0200
#define BMCR_FULLDPLX   0x0100
#define DM9161_LOOPBACK_SETTLE_MS 800
#define PHY_POWER_DOWN  0x800

#define SROM_V41_CODE   0x14


/* The EEPROM commands include the alway-set leading bit. */
#define SROM_WRITE_CMD	5
#define SROM_READ_CMD	6
#define SROM_ERASE_CMD	7

#define SROM_HWADD	    0x0014	/* Hardware Address offset in SROM */
#define SROM_RD		0x00004000	/* Read from Boot ROM */
#define EE_DATA_WRITE	      0x04	/* EEPROM chip data in. */
#define EE_WRITE_0	    0x4801
#define EE_WRITE_1	    0x4805
#define EE_DATA_READ	      0x08	/* EEPROM chip data out. */
#define SROM_SR		0x00000800	/* Select Serial ROM when set */

#define DT_IN		0x00000004	/* Serial Data In */
#define DT_CLK		0x00000002	/* Serial ROM Clock */
#define DT_CS		0x00000001	/* Serial ROM Chip Select */

#define POLL_DEMAND	1


typedef int  s32;

struct eth_device {
	char dev_addr[6];
    u8   addr_len;   /* hardware address length  */
	u32 iobase;
	char *packet;
} ;
#define udelay udelay



#define CONFIG_TULIP_FIX_DAVICOM

#ifdef CONFIG_TULIP_FIX_DAVICOM
#define RESET_DM9102(dev) {\
    unsigned long i;\
    i=INL(dev, 0x0);\
    udelay(1000);\
    OUTL(dev, i | BMR_SWR, DE4X5_BMR);\
    udelay(1000);\
}
#else
#define RESET_DE4X5(dev) {\
    int i;\
    i=INL(dev, DE4X5_BMR);\
    udelay(1000);\
    OUTL(dev, i | BMR_SWR, DE4X5_BMR);\
    udelay(1000);\
    OUTL(dev, i, DE4X5_BMR);\
    udelay(1000);\
    for (i=0;i<5;i++) {INL(dev, DE4X5_BMR); udelay(10000);}\
    udelay(1000);\
}
#endif

#define START_DE4X5(dev) {\
    s32 omr; \
    omr = INL(dev, DE4X5_OMR);\
    omr |= OMR_ST | OMR_SR | OMR_PR | OMR_FD | OMR_TTM | OMR_SF;\
    OUTL(dev, omr, DE4X5_OMR);		/* Enable the TX and/or RX */\
}

#define STOP_DE4X5(dev) {\
    s32 omr; \
    omr = INL(dev, DE4X5_OMR);\
    omr &= ~(OMR_ST|OMR_SR);\
    OUTL(dev, omr, DE4X5_OMR);		/* Disable the TX and/or RX */ \
}

#define NUM_RX_DESC 8			/* Number of Rx descriptors */
#define NUM_TX_DESC 8			/* Number of TX descriptors */
#define RX_BUFF_SZ  1520 		//yanhua, should aligned to word
#define TX_BUFF_SZ  1520

#define TOUT_LOOP   100000

#define SETUP_FRAME_LEN 192
#define ETH_ALEN	6

struct de4x5_desc {
	volatile s32 status;
	u32 des1;
	u32 buf;
	u32 next;
};

static struct de4x5_desc _rx_ring[NUM_RX_DESC] __attribute__ ((aligned(64),section(".bss.align64"))); /* RX descriptor ring         */
static struct de4x5_desc _tx_ring[NUM_TX_DESC] __attribute__ ((aligned(64),section(".bss.align64"))); /* TX descriptor ring         */
static volatile struct de4x5_desc *rx_ring;
static volatile struct de4x5_desc *tx_ring;

char __NetRxPackets[NUM_RX_DESC][RX_BUFF_SZ] __attribute__((aligned(64),section(".bss.align64"))); //16
char (*NetRxPackets)[RX_BUFF_SZ];

char __NetTxPackets[NUM_TX_DESC][TX_BUFF_SZ] __attribute__((aligned(64),section(".bss.align64"))); //16
char (*NetTxPackets)[TX_BUFF_SZ];


static int rx_new;                             /* RX descriptor ring pointer */
static int tx_new;                             /* TX descriptor ring pointer */
static int dmfe_rx_idle_dumped;
static int dmfe_phy_addr = -1;

static char rxRingSize;
static char txRingSize;

static int   dc21x4x_init(struct eth_device* dev);
static void  read_hw_addr(struct eth_device *dev);


static int INL(struct eth_device* dev, u_long addr)
{
	return (*(volatile u_long *)(addr + dev->iobase));
}

static void OUTL(struct eth_device* dev, int command, u_long addr)
{
	*(volatile u_long *)(addr + dev->iobase) = (command);
}

#ifdef MAC_DEBUG
static void dmfe_dump_state(struct eth_device *dev, const char *tag)
{
	u32 csr0 = INL(dev, DE4X5_BMR);
	u32 csr3 = INL(dev, DE4X5_RRBA);
	u32 csr4 = INL(dev, DE4X5_TRBA);
	u32 csr5 = INL(dev, DE4X5_STS);
	u32 csr6 = INL(dev, DE4X5_OMR);
	u32 csr7 = INL(dev, DE4X5_IMR);

	dmfe_dbg("dmfe %s: CSR0=%08x CSR3=%08x CSR4=%08x CSR5=%08x CSR6=%08x CSR7=%08x\n",
		 tag, csr0, csr3, csr4, csr5, csr6, csr7);
	dmfe_dbg("  CSR5: TS=%u RS=%u TI=%u TPS=%u TU=%u UNF=%u RI=%u RU=%u RPS=%u\n",
		 (csr5 & STS_TS) >> 20, (csr5 & STS_RS) >> 17,
		 !!(csr5 & 0x00000001), !!(csr5 & 0x00000002),
		 !!(csr5 & 0x00000004), !!(csr5 & 0x00000020),
		 !!(csr5 & 0x00000040), !!(csr5 & 0x00000080),
		 !!(csr5 & 0x00000100));

	if (dmfe_debug > 1) {
		u32 csr8 = INL(dev, DE4X5_MFC);

		dmfe_dbg("  CSR8=%08x (读取会清除 FIFO overflow/missed-frame 计数)\n",
			 csr8);
	}
}

static void dmfe_dump_rx_desc(int index)
{
	volatile struct de4x5_desc *desc = &rx_ring[index];
	u32 status = desc->status;

	dmfe_dbg("RX[%d]: status=%08x des1=%08x buf=%08x next=%08x own=%u\n",
		 index, status, desc->des1, desc->buf, desc->next,
		 !!(status & R_OWN));
	if (!(status & R_OWN))
		dmfe_dbg("  RX status: len=%u ES=%u FF=%u DE=%u RF=%u MF=%u FS=%u LS=%u TL=%u CS=%u FT=%u RE=%u DB=%u CE=%u OF=%u\n",
			 (status >> 16) & 0x3fff, !!(status & RD_ES),
			 !!(status & RD_FF), !!(status & RD_DE), !!(status & RD_RF),
			 !!(status & RD_MF), !!(status & RD_FS), !!(status & RD_LS),
			 !!(status & RD_TL), !!(status & RD_CS), !!(status & RD_FT),
			 !!(status & RD_RE), !!(status & RD_DB), !!(status & RD_CE),
			 !!(status & RD_OF));
}

static void dmfe_dump_rx_ring(void)
{
	int i;

	for (i = 0; i < rxRingSize; i++)
		dmfe_dump_rx_desc(i);
}

static void dmfe_dump_tx_desc(int index)
{
	volatile struct de4x5_desc *desc = &tx_ring[index];

	dmfe_dbg("TX[%d]: status=%08x des1=%08x buf=%08x next=%08x own=%u\n",
		 index, desc->status, desc->des1, desc->buf, desc->next,
		 !!(desc->status & T_OWN));
}

static void dmfe_dump_frame(const char *tag, const void *data, int length)
{
	int dump_length;

	if (dmfe_debug < 2)
		return;

	dump_length = length > 64 ? 64 : length;
	dmfe_dbg("%s: 前 %d/%d 字节\n", tag, dump_length, length);
	print_buffer(0, data, 1, dump_length, 16);
}

/*
 * The FPGA MAC exposes a Clause 22 MDIO bit-bang interface through CSR9.
 * Keep it local to debug code: the normal dmfe path does not configure PHY
 * registers.  MDIO writes are only issued when dmfe_phy_loopback is set.
 */
static void dmfe_mdio_set(struct eth_device *dev, int data, int drive,
			  int clock)
{
	u32 csr9 = INL(dev, DCR9);

	csr9 &= ~(MII_DIR_OUT | PHY_DATA_1 | MDCLKH);
	if (drive)
		csr9 |= MII_DIR_OUT;
	if (data)
		csr9 |= PHY_DATA_1;
	if (clock)
		csr9 |= MDCLKH;
	OUTL(dev, csr9, DCR9);
	udelay(1);
}

static void dmfe_mdio_write_bit(struct eth_device *dev, int bit)
{
	dmfe_mdio_set(dev, bit, 1, 0);
	dmfe_mdio_set(dev, bit, 1, 1);
	dmfe_mdio_set(dev, bit, 1, 0);
}

static int dmfe_mdio_read_bit(struct eth_device *dev)
{
	int bit;

	dmfe_mdio_set(dev, 0, 0, 0);
	dmfe_mdio_set(dev, 0, 0, 1);
	bit = !!(INL(dev, DCR9) & MII_DATA_IN);
	dmfe_mdio_set(dev, 0, 0, 0);

	return bit;
}

static void dmfe_mdio_write_bits(struct eth_device *dev, u32 value, int bits)
{
	while (bits--)
		dmfe_mdio_write_bit(dev, !!(value & (1U << bits)));
}

static int dmfe_mdio_read(struct eth_device *dev, int phy, int reg, u16 *value)
{
	int i;
	u16 data = 0;

	/* Preamble, start (01), read opcode (10), PHY address and register. */
	dmfe_mdio_write_bits(dev, 0xffffffff, 32);
	dmfe_mdio_write_bits(dev, 0x6, 4);
	dmfe_mdio_write_bits(dev, phy, 5);
	dmfe_mdio_write_bits(dev, reg, 5);

	/* Sample the second turnaround bit: PHY must drive it low. */
	if (dmfe_mdio_read_bit(dev))
		return -ENODEV;

	for (i = 0; i < 16; i++)
		data = (data << 1) | dmfe_mdio_read_bit(dev);

	dmfe_mdio_set(dev, 0, 0, 0);
	*value = data;

	return 0;
}

static void dmfe_mdio_write(struct eth_device *dev, int phy, int reg,
			    u16 value)
{
	/* Preamble, start (01), write opcode (01), PHY address and register. */
	dmfe_mdio_write_bits(dev, 0xffffffff, 32);
	dmfe_mdio_write_bits(dev, 0x5, 4);
	dmfe_mdio_write_bits(dev, phy, 5);
	dmfe_mdio_write_bits(dev, reg, 5);

	/* Clause 22 write turnaround (10), followed by 16 data bits. */
	dmfe_mdio_write_bits(dev, 0x2, 2);
	dmfe_mdio_write_bits(dev, value, 16);
	dmfe_mdio_set(dev, 0, 0, 0);
}

static int dmfe_mdio_find_phy(struct eth_device *dev, u16 *id1)
{
	int phy;

	for (phy = 0; phy < 32; phy++) {
		if (dmfe_mdio_read(dev, phy, MII_PHYSID1, id1))
			continue;
		if (*id1 != 0x0000 && *id1 != 0xffff)
			return phy;
	}

	return -ENODEV;
}

static void dmfe_configure_phy_loopback(struct eth_device *dev)
{
	const char *loopback_env = env_get("dmfe_phy_loopback");
	const char *speed_env = env_get("dmfe_phy_loopback_speed");
	const char *speed_mode = "preserve";
	u16 bmcr, readback, id1, recr_before, recr_after;
	int recr_before_valid = 0;
	int enable;
	int phy;

	if (!loopback_env)
		return;
	if (!strcmp(loopback_env, "1"))
		enable = 1;
	else if (!strcmp(loopback_env, "0"))
		enable = 0;
	else {
		printf("dmfe: dmfe_phy_loopback 仅接受 0 或 1，当前值为 '%s'\n",
		       loopback_env);
		return;
	}

	phy = dmfe_phy_addr;
	if (phy < 0) {
		phy = dmfe_mdio_find_phy(dev, &id1);
		if (phy < 0) {
			printf("dmfe: PHY loopback 配置失败，未找到有效 PHY\n");
			return;
		}
		dmfe_phy_addr = phy;
	}

	if (dmfe_mdio_read(dev, phy, MII_BMCR, &bmcr)) {
		printf("dmfe: PHY%d BMCR 读取失败，未修改 loopback\n", phy);
		return;
	}
	if (enable && !dmfe_mdio_read(dev, phy, MII_RECR, &recr_before)) {
		recr_before_valid = 1;
		printf("dmfe: PHY%d loopback置位前 RECR=%u\n", phy,
		       recr_before);
	}

	if (enable) {
		readback = bmcr | BMCR_LOOPBACK;
		if (speed_env && !strcmp(speed_env, "auto")) {
			readback |= BMCR_ANENABLE | BMCR_RESTARTAN;
			speed_mode = "auto";
		} else if (speed_env && !strcmp(speed_env, "100")) {
			readback = BMCR_LOOPBACK | BMCR_SPEED100 | BMCR_FULLDPLX;
			speed_mode = "100-full";
		} else if (speed_env && !strcmp(speed_env, "10")) {
			readback = BMCR_LOOPBACK | BMCR_FULLDPLX;
			speed_mode = "10-full";
		} else if (speed_env) {
			printf("dmfe: dmfe_phy_loopback_speed仅接受auto、100或10，当前值为'%s'\n",
			       speed_env);
			return;
		}
	} else {
		readback = bmcr & ~BMCR_LOOPBACK;
		speed_mode = "unchanged";
	}
	dmfe_mdio_write(dev, phy, MII_BMCR, readback);
	udelay(10);

	if (dmfe_mdio_read(dev, phy, MII_BMCR, &readback)) {
		printf("dmfe: PHY%d BMCR 写后读取失败，原值=%04x\n", phy, bmcr);
		return;
	}

	printf("dmfe: PHY%d loopback=%d speed=%s BMCR %04x -> %04x%s\n",
	       phy, enable, speed_mode, bmcr, readback,
	       (!!(readback & BMCR_LOOPBACK) == enable) ? "" : "（回读不匹配）");

	if (enable && (readback & BMCR_LOOPBACK)) {
		printf("dmfe: PHY%d 等待loopback稳定 %u ms\n", phy,
		       DM9161_LOOPBACK_SETTLE_MS);
		mdelay(DM9161_LOOPBACK_SETTLE_MS);
		if (!dmfe_mdio_read(dev, phy, MII_RECR, &recr_after)) {
			if (recr_before_valid)
				printf("dmfe: PHY%d loopback稳定等待 RECR=%u -> %u\n",
				       phy, recr_before, recr_after);
			else
				printf("dmfe: PHY%d loopback稳定后 RECR=%u\n",
				       phy, recr_after);
		}
	}
}

static void dmfe_dump_phy(struct eth_device *dev)
{
	u16 bmcr, bmsr, id1, id2, anar, anlpar, dscsr;
	int phy;
	int ret;

	phy = dmfe_mdio_find_phy(dev, &id1);
	if (phy < 0) {
		dmfe_dbg("MDIO: PHY0-31 均未返回有效 ID（检查 MDC/MDIO、地址 strap 和复位）\n");
		return;
	}
	dmfe_phy_addr = phy;

	dmfe_dbg("MDIO: 候选 PHY%d，ID1=%04x\n", phy, id1);
	ret = dmfe_mdio_read(dev, phy, MII_PHYSID2, &id2);
	if (ret) {
		dmfe_dbg("MDIO: PHY%d 读取 ID2 失败\n", phy);
		return;
	}

	/* BMSR<2> is latch-low; read it twice to obtain the current link state. */
	dmfe_mdio_read(dev, phy, MII_BMCR, &bmcr);
	dmfe_mdio_read(dev, phy, MII_BMSR, &bmsr);
	dmfe_mdio_read(dev, phy, MII_BMSR, &bmsr);
	dmfe_mdio_read(dev, phy, MII_ADVERTISE, &anar);
	dmfe_mdio_read(dev, phy, MII_LPA, &anlpar);
	dmfe_mdio_read(dev, phy, MII_DSCSR, &dscsr);

	dmfe_dbg("MDIO PHY%d: ID=%04x:%04x BMCR=%04x BMSR=%04x%s ANAR=%04x LPA=%04x DSCSR=%04x\n",
		 phy, id1, id2, bmcr, bmsr,
		 (bmsr & 0x0004) ? " link-up" : " link-down",
		 anar, anlpar, dscsr);
}

static void dmfe_dump_phy_counters(struct eth_device *dev)
{
	u16 recr, discr;

	if (dmfe_phy_addr < 0)
		return;
	if (dmfe_mdio_read(dev, dmfe_phy_addr, MII_RECR, &recr) ||
	    dmfe_mdio_read(dev, dmfe_phy_addr, MII_DISCR, &discr)) {
		dmfe_dbg("MDIO PHY%d: RECR/DISCR 读取失败\n", dmfe_phy_addr);
		return;
	}

	dmfe_dbg("MDIO PHY%d: RECR=%u DISCR=%u\n", dmfe_phy_addr, recr, discr);
}
#endif

int dc21x4x_initialize(struct eth_device* dev)
{

	/* Ensure we're not sleeping. */

	read_hw_addr(dev);

	dc21x4x_init(dev);

	return 0;
}
#define next_tx(x) (((x+1)==NUM_TX_DESC)?0:(x+1))
#define next_rx(x) (((x+1)==NUM_RX_DESC)?0:(x+1))

static void send_setup_frame(struct eth_device* dev);
/*
 * init function
 */
static int dc21x4x_init(struct eth_device* dev)
{
	int		i;
    dmfe_dbg("\r\n%s, %d iobase:%02x\r\n",__func__,__LINE__,dev->iobase);
	/* Ensure we're not sleeping. */

#ifdef CONFIG_TULIP_FIX_DAVICOM
	RESET_DM9102(dev);
#else
	RESET_DE4X5(dev);
#endif

	if ((INL(dev, DE4X5_STS) & (STS_TS | STS_RS)) != 0) {
		dmfe_dbg("Error: Cannot reset ethernet controller.\n");
		dmfe_dbg("Error: read 0x%x.\n", INL(dev, DE4X5_STS));
		return 0;
	}

#ifdef CONFIG_TULIP_SELECT_MEDIA
	dc21x4x_select_media(dev);
#else
	OUTL(dev, OMR_SDP | OMR_PS , DE4X5_OMR);
#endif

#ifdef MAC_DEBUG
	if (dmfe_debug)
		dmfe_dump_phy(dev);
	dmfe_configure_phy_loopback(dev);
#endif

	/*
	 * initialise rx descriptors
	 * use it as chain structure
	 */
	rxRingSize = NUM_RX_DESC;
	txRingSize = NUM_TX_DESC;
	
	rx_ring = (struct de4x5_desc *)(CACHED_TO_UNCACHED((unsigned long)_rx_ring));
	tx_ring = (struct de4x5_desc *)(CACHED_TO_UNCACHED((unsigned long)_tx_ring));

	NetRxPackets = (char (*)[RX_BUFF_SZ])(CACHED_TO_UNCACHED((unsigned long)__NetRxPackets));
	NetTxPackets = (char (*)[TX_BUFF_SZ])(CACHED_TO_UNCACHED((unsigned long)__NetTxPackets));
	
	for (i = 0; i < NUM_RX_DESC; i++) {
		rx_ring[i].status = (R_OWN); //Initially MAC owns it.
		rx_ring[i].des1 = RX_BUFF_SZ;
		rx_ring[i].buf = CACHED_TO_UNCACHED((u32) NetRxPackets[i]);//XXX
#ifdef CONFIG_TULIP_FIX_DAVICOM
		rx_ring[i].next = CACHED_TO_UNCACHED((u32) &rx_ring[next_rx(i)]);
#else
		rx_ring[i].next = 0;
#endif
	}

	/*
	 * initialize tx descriptors
	 * use it as chain structure
	 */
	for (i=0; i < NUM_TX_DESC; i++) {
		tx_ring[i].status = 0;
		tx_ring[i].des1 = 0xe1000000;//TD_TCH;
		tx_ring[i].buf = CACHED_TO_UNCACHED((u32) NetTxPackets[i]);

#ifdef CONFIG_TULIP_FIX_DAVICOM
		tx_ring[i].next = (CACHED_TO_UNCACHED((u32) &tx_ring[next_tx(i)]));
#else
		tx_ring[i].next = 0;
#endif
	}


	/* Write the end of list marker to the descriptor lists. */
	rx_ring[rxRingSize - 1].des1 |= RD_RER; //Receive end of ring

	/* Tell the adapter where the TX/RX rings are located. */
	dmfe_dbg("rx ring %x\n", (u32)rx_ring);
	dmfe_dbg("tx ring %x\n", (u32)tx_ring);
	OUTL(dev, (u32) rx_ring, DE4X5_RRBA);
	OUTL(dev, (u32) tx_ring, DE4X5_TRBA);

	udelay(100);
	START_DE4X5(dev);

	tx_new = 0;
	rx_new = 0;
	dmfe_rx_idle_dumped = 0;

	dmfe_dbg("DE4X5_BMR= %x\n",  INL(dev, DE4X5_BMR));
	dmfe_dbg("DE4X5_TPD= %x\n",  INL(dev, DE4X5_TPD));
	dmfe_dbg("DE4X5_RRBA= %x\n", INL(dev, DE4X5_RRBA));
	dmfe_dbg("DE4X5_TRBA= %x\n", INL(dev, DE4X5_TRBA));
	dmfe_dbg("DE4X5_STS= %x\n",  INL(dev, DE4X5_STS));
	dmfe_dbg("DE4X5_OMR= %x\n",  INL(dev, DE4X5_OMR));
	send_setup_frame(dev);
    dmfe_dbg("After setup\n");
	dmfe_dbg("DE4X5_BMR= %x\n",  INL(dev, DE4X5_BMR));
	dmfe_dbg("DE4X5_TPD= %x\n",  INL(dev, DE4X5_TPD));
	dmfe_dbg("DE4X5_RRBA= %x\n", INL(dev, DE4X5_RRBA));
	dmfe_dbg("DE4X5_TRBA= %x\n", INL(dev, DE4X5_TRBA));
	dmfe_dbg("DE4X5_STS= %x\n",  INL(dev, DE4X5_STS));
	dmfe_dbg("DE4X5_OMR= %x\n",  INL(dev, DE4X5_OMR));
	dmfe_dump_state(dev, "after-setup");
	dmfe_dump_rx_ring();

	return 1;
}

/*
 *  gethex(vp,p,n) 
 *      convert n hex digits from p to binary, result in vp, 
 *      rtn 1 on success
 */
static int gethex(int32_t *vp, char *p, int n)
{
    u_long v;
    int digit;

    for (v = 0; n > 0; n--) {
        if (*p == 0)
            return (0);
        if (*p >= '0' && *p <= '9')
            digit = *p - '0';
        else if (*p >= 'a' && *p <= 'f')
            digit = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F')
            digit = *p - 'A' + 10;
        else
            return (0);

        v <<= 4;
        v |= digit;
        p++;
    }
    *vp = v;
    return (1);
}                                                                 

static void read_hw_addr(struct eth_device *dev)
{
	static char maddr[ETH_ALEN]={0x00, 0x98, 0x76, 0x64, 0x32, 0x19};
	char *p = &dev->dev_addr[0];
	int i;
	{
		int i;
		int32_t v;
	char *s=NULL;
	if(s){
		for(i = 0; i < 6; i++) {
			gethex(&v, s, 2);
			maddr[i] = v;
			s += 3;         /* Don't get to fancy here :-) */
		} 
	 }
	} 

	for (i = 0; i < ETH_ALEN; i++) 
		*p++ = maddr[i];

	return;

}

static char	setup_frame[SETUP_FRAME_LEN]
	__attribute__((aligned(64), section(".bss.align64")));
static void send_setup_frame(struct eth_device* dev)
{
	int		i;
	char		*setup_frame_uncached;
	char 	*pa;

	setup_frame_uncached = (char *)CACHED_TO_UNCACHED(
		(unsigned long)&setup_frame[0]);
	pa = setup_frame_uncached;

	memset(pa, 0x00, SETUP_FRAME_LEN);

	for (i = 0; i < ETH_ALEN; i++) {
		*(pa + (i & 1)) = dev->dev_addr[i];
		if (i & 0x01) {
			pa += 4;
		}
	}

	for(i = 0; tx_ring[tx_new].status & (T_OWN); i++) {
		if (i >= TOUT_LOOP) {
			dmfe_dbg("tx error buffer not ready tx_ring[%d]=%02x\n", i, tx_ring[tx_new].status);
			goto Done;
		}
	}

	tx_ring[tx_new].buf = (u32)setup_frame_uncached;
	tx_ring[tx_new].des1 = (TD_TCH | TD_SET| SETUP_FRAME_LEN);
	tx_ring[tx_new].status = (T_OWN);

    dmfe_dbg("\r\nbuf:%x, des1:%x, status:%x",tx_ring[tx_new].buf,tx_ring[tx_new].des1,tx_ring[tx_new].status);

	OUTL(dev, POLL_DEMAND, DE4X5_TPD);
	udelay(1000);

    dmfe_dbg("\r\nnew:%d ,status:%x\r\n",tx_new,tx_ring[tx_new].status);
	for(i = 0; tx_ring[tx_new].status & (T_OWN); i++) {
		if (i >= TOUT_LOOP) {
			dmfe_dbg("tx buffer not ready\n");
			goto Done;
		}
	}

	if ((tx_ring[tx_new].status) != 0x7FFFFFFF) {
		dmfe_dbg("TX error status2 = 0x%08X\n", (tx_ring[tx_new].status));
	}

	tx_ring[tx_new].des1 = 0xe1000000;//TD_TCH;
	tx_ring[tx_new].buf = CACHED_TO_UNCACHED((u32) NetTxPackets[tx_new]);
	tx_ring[tx_new].next = (CACHED_TO_UNCACHED((u32) &tx_ring[next_tx(tx_new)]));
	tx_new = next_tx(tx_new);

#if 0
	{
		/* Send to mac */
		int my_frame = 0x12345678;
		int len, i;

		len = sizeof(my_frame);
		tx_ring[tx_new].buf = (CACHED_TO_UNCACHED((u32) &my_frame));
		tx_ring[tx_new].des1 = 0xe1000000 | len;
		tx_ring[tx_new].status = (T_OWN);

		OUTL(dev, POLL_DEMAND, DE4X5_TPD);
		udelay(1000);

	}
#endif

Done:
	return;
}

static int dmfe_start(struct udevice *dev)
{
	struct eth_device *priv = dev_get_priv(dev);
	const char *debug_env = env_get("dmfe_debug");

	dmfe_debug = env_get_yesno("dmfe_debug") == 1;
	if (!dmfe_debug && debug_env)
		dmfe_debug = simple_strtoul(debug_env, NULL, 0);
    dc21x4x_initialize(priv);
	return 0;
}

static int dmfe_send(struct udevice *dev, void *ptr, int len)
{
	struct eth_device *priv = dev_get_priv(dev);
	int input_len;
	int tx_len;
	int sw_pad;
	
    if (len > PKTSIZE_ALIGN)
		len = PKTSIZE_ALIGN;

	input_len = len;
	tx_len = len;
	sw_pad = env_get_yesno("dmfe_sw_pad") == 1;
	if (sw_pad && tx_len < ETH_ZLEN)
		tx_len = ETH_ZLEN;

    if((tx_ring[tx_new].status & (T_OWN))) {
		dmfe_dbg("dmfe_send: TX[%d] 仍由 MAC 持有\n", tx_new);
		dmfe_dump_tx_desc(tx_new);
		dmfe_dump_state(priv, "tx-busy");
		return -1;
	}

	memset((void *)NetTxPackets[tx_new], 0 , tx_len);
    memcpy((void *)NetTxPackets[tx_new], ptr, len);
    

    tx_ring[tx_new].des1   = (0xe1000000 | tx_len); //frame in a single TD
    tx_ring[tx_new].buf = CACHED_TO_UNCACHED((u32) NetTxPackets[tx_new]);
    tx_ring[tx_new].next = (CACHED_TO_UNCACHED((u32) &tx_ring[next_tx(tx_new)]));
    tx_ring[tx_new].status = (T_OWN);
	dmfe_dump_frame("TX frame", NetTxPackets[tx_new], tx_len);


	/*
	 * command the mac to start transmit process
	 */
    OUTL(priv, POLL_DEMAND, DE4X5_TPD);
    udelay(1000);
	dmfe_dbg("dmfe_send input_len=%d tx_len=%d sw_pad=%d pad=%d desc=%d status=%08x\n",
		 input_len, tx_len, sw_pad, tx_len - input_len, tx_new,
		 tx_ring[tx_new].status);
	dmfe_dump_tx_desc(tx_new);
	dmfe_dump_state(priv, "after-tx");

    tx_new = next_tx(tx_new);
    
	return 0;
}

static int dmfe_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct eth_device *priv = dev_get_priv(dev);
	s32	 status;
	int	 length;
	int  received=0;

	for ( ; !received; ) {
		length    = 0;
			
		status = (s32)(rx_ring[rx_new].status);
		
		if (status & R_OWN) {
			if (!dmfe_rx_idle_dumped) {
				dmfe_dbg("dmfe_recv: RX[%d] 仍由 MAC 持有，尚未收到完整帧\n",
					 rx_new);
				dmfe_dump_state(priv, "rx-idle");
				dmfe_dump_phy_counters(priv);
				dmfe_dump_rx_ring();
				dmfe_rx_idle_dumped = 1;
			}
			break;
		}

		dmfe_dump_rx_desc(rx_new);

		rx_ring[rx_new].next = CACHED_TO_UNCACHED((u32) &rx_ring[next_rx(rx_new)]);

		if (status & RD_LS) {
			/* Valid frame status.
			 */
			if (status & RD_ES) {

				/* There was an error.
				 */
				dmfe_dbg("RX error status = 0x%08X\n", status);
				dmfe_dump_frame("RX error frame", NetRxPackets[rx_new],
						(status >> 16) & 0x3fff);
				dmfe_dump_state(priv, "rx-error");
			} else {
				/* A valid frame received.
				 */
				length = (rx_ring[rx_new].status >> 16) & 0x3fff;

				/* Pass the packet up to the protocol
				 * layers.
				 */
				dmfe_dbg("received a packet status %x\n", status);
				dmfe_dump_frame("RX frame", NetRxPackets[rx_new], length);
				dmfe_dump_state(priv, "rx-ok");

				{
                    *packetp = (uchar *)NetRxPackets[rx_new];
				}
				received =1;

			}

			/* Change buffer ownership for this frame, back
			 * to the adapter.
			 */
			rx_ring[rx_new].status = (R_OWN);
		}

		/* Update entry information.
		 */
		rx_new = next_rx(rx_new);
	}

	return length;
}

static void dmfe_stop(struct udevice *dev)
{
	struct eth_device *priv = dev_get_priv(dev);
	STOP_DE4X5(priv);
	OUTL(priv, 0, DE4X5_SICR);

	return ;
}

static int dmfe_probe(struct udevice *dev)
{

	return 0;
}

static int dmfe_remove(struct udevice *dev)
{
	return 0;
}

static const struct eth_ops dmfe_ops = {
	.start = dmfe_start,
	.send = dmfe_send,
	.recv = dmfe_recv,
	.stop = dmfe_stop,
};

static int dmfe_ofdata_to_platdata(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct eth_device *priv = dev_get_priv(dev);

	pdata->iobase = (phys_addr_t)devfdt_get_addr(dev);
    priv->iobase = pdata->iobase;

	return 0;
}

static const struct udevice_id dmfe_ids[] = {
	{ .compatible = "ls,ls-dmfe" },
	{ }
};

U_BOOT_DRIVER(dmfe) = {
	.name   = "dmfe",
	.id     = UCLASS_ETH,
	.of_match = dmfe_ids,
	.ofdata_to_platdata = dmfe_ofdata_to_platdata,
	.probe  = dmfe_probe,
	.remove = dmfe_remove,
	.ops    = &dmfe_ops,
	.priv_auto_alloc_size = sizeof(struct eth_device),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};
