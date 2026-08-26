// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <elf.h>
#include <nand.h>
#include <u-boot/crc.h>

#define VMLINUX_LOAD_ADDR	0xa3000000UL
#define VMLINUX_NAND_OFFSET	0
#define VMLINUX_NAND_SIZE	0x02800000UL
#define VMLINUX_NAND_LIMIT	0x03000000UL
#define VMLINUX_DEFAULT_FILE	"vmlinux"
#define VMLINUX_ELF_MACHINE	258

#if (VMLINUX_LOAD_ADDR - CONFIG_SYS_SDRAM_BASE + VMLINUX_NAND_SIZE) > \
	CONFIG_SYS_SDRAM_SIZE
#error "vmlinux buffer exceeds configured SDRAM"
#endif

static int vmlinux_range_valid(ulong offset, ulong length, ulong image_size)
{
	return offset <= image_size && length <= image_size - offset;
}

static int vmlinux_validate_elf(ulong addr, ulong image_size)
{
	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)addr;
	const Elf32_Phdr *phdr;
	const Elf32_Shdr *shdr;
	ulong table_size;
	int i;

	if (image_size < sizeof(*ehdr) || !IS_ELF(*ehdr)) {
		puts("vmlinux: invalid ELF magic\n");
		return -EINVAL;
	}

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
	    ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
	    ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
	    ehdr->e_type != ET_EXEC ||
	    ehdr->e_machine != VMLINUX_ELF_MACHINE ||
	    ehdr->e_version != EV_CURRENT) {
		puts("vmlinux: unsupported ELF type or architecture\n");
		return -EINVAL;
	}

	if (ehdr->e_ehsize != sizeof(*ehdr) ||
	    !ehdr->e_phnum || ehdr->e_phentsize != sizeof(*phdr) ||
	    !ehdr->e_shnum || ehdr->e_shentsize != sizeof(*shdr) ||
	    ehdr->e_shstrndx >= ehdr->e_shnum) {
		puts("vmlinux: invalid ELF header layout\n");
		return -EINVAL;
	}

	table_size = (ulong)ehdr->e_phnum * sizeof(*phdr);
	if (!vmlinux_range_valid(ehdr->e_phoff, table_size, image_size)) {
		puts("vmlinux: program headers exceed file size\n");
		return -EINVAL;
	}

	phdr = (const Elf32_Phdr *)(addr + ehdr->e_phoff);
	for (i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_filesz &&
		    !vmlinux_range_valid(phdr[i].p_offset,
					  phdr[i].p_filesz, image_size)) {
			puts("vmlinux: program segment exceeds file size\n");
			return -EINVAL;
		}
	}

	table_size = (ulong)ehdr->e_shnum * sizeof(*shdr);
	if (!vmlinux_range_valid(ehdr->e_shoff, table_size, image_size)) {
		puts("vmlinux: section headers exceed file size\n");
		return -EINVAL;
	}

	shdr = (const Elf32_Shdr *)(addr + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_NOBITS && shdr[i].sh_size &&
		    !vmlinux_range_valid(shdr[i].sh_offset,
					  shdr[i].sh_size, image_size)) {
			puts("vmlinux: section exceeds file size\n");
			return -EINVAL;
		}

		if (!(shdr[i].sh_flags & SHF_ALLOC) || !shdr[i].sh_size)
			continue;

		if (shdr[i].sh_addr < CONFIG_SYS_SDRAM_BASE ||
		    shdr[i].sh_addr >= VMLINUX_LOAD_ADDR ||
		    shdr[i].sh_size > VMLINUX_LOAD_ADDR - shdr[i].sh_addr) {
			puts("vmlinux: load section is outside kernel DDR area\n");
			return -EINVAL;
		}
	}

	if (ehdr->e_entry < CONFIG_SYS_SDRAM_BASE ||
	    ehdr->e_entry >= VMLINUX_LOAD_ADDR) {
		puts("vmlinux: entry point is outside kernel DDR area\n");
		return -EINVAL;
	}

	return 0;
}

static int do_vmlinux(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	const char *filename;
	struct mtd_info *mtd;
	nand_erase_options_t erase_opts;
	char *tftp_argv[3];
	char load_addr[2 * sizeof(ulong) + 3];
	ulong image_size;
	ulong write_size;
	size_t io_size;
	u32 image_crc;
	u32 verify_crc;
	int ret;

	if (argc < 2 || argc > 3 || strcmp(argv[1], "update"))
		return CMD_RET_USAGE;

	filename = argc == 3 ? argv[2] : VMLINUX_DEFAULT_FILE;
	snprintf(load_addr, sizeof(load_addr), "0x%lx", VMLINUX_LOAD_ADDR);
	tftp_argv[0] = "tftpboot";
	tftp_argv[1] = load_addr;
	tftp_argv[2] = (char *)filename;

	printf("vmlinux: TFTP %s to 0x%08lx\n", filename,
	       VMLINUX_LOAD_ADDR);
	ret = do_tftpb(cmdtp, flag, ARRAY_SIZE(tftp_argv), tftp_argv);
	if (ret)
		return CMD_RET_FAILURE;

	image_size = env_get_hex("filesize", 0);
	if (!image_size || image_size > VMLINUX_NAND_SIZE) {
		printf("vmlinux: file size 0x%lx exceeds 0x%lx-byte slot\n",
		       image_size, VMLINUX_NAND_SIZE);
		return CMD_RET_FAILURE;
	}

	if (vmlinux_validate_elf(VMLINUX_LOAD_ADDR, image_size))
		return CMD_RET_FAILURE;

	mtd = get_nand_dev_by_index(0);
	if (!mtd) {
		puts("vmlinux: NAND device 0 is unavailable\n");
		return CMD_RET_FAILURE;
	}

	write_size = ALIGN(image_size, mtd->writesize);
	if (write_size > VMLINUX_NAND_SIZE) {
		puts("vmlinux: page-aligned image exceeds NAND slot\n");
		return CMD_RET_FAILURE;
	}
	memset((void *)(VMLINUX_LOAD_ADDR + image_size), 0xff,
	       write_size - image_size);
	image_crc = crc32(0, (const u8 *)VMLINUX_LOAD_ADDR, write_size);

	memset(&erase_opts, 0, sizeof(erase_opts));
	erase_opts.offset = VMLINUX_NAND_OFFSET;
	erase_opts.length = VMLINUX_NAND_SIZE;
	erase_opts.spread = 1;
	erase_opts.lim = VMLINUX_NAND_LIMIT;

	printf("vmlinux: erase 0x%lx logical bytes from NAND offset 0\n",
	       VMLINUX_NAND_SIZE);
	ret = nand_erase_opts(mtd, &erase_opts);
	if (ret) {
		printf("vmlinux: NAND erase failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}
	printf("vmlinux: erase complete; writing and verifying 0x%lx bytes\n",
	       write_size);
	puts("vmlinux: do not reset until \"vmlinux: update complete\" appears\n");

	/* Reuse the source buffer for NAND readback. */
	io_size = write_size;
	ret = nand_write_skip_bad(mtd, VMLINUX_NAND_OFFSET, &io_size, NULL,
				  VMLINUX_NAND_LIMIT,
				  (u8 *)VMLINUX_LOAD_ADDR, WITH_WR_VERIFY);
	if (ret || io_size != write_size) {
		printf("vmlinux: NAND write failed: %d, wrote 0x%zx bytes\n",
		       ret, io_size);
		return CMD_RET_FAILURE;
	}

	io_size = write_size;
	ret = nand_read_skip_bad(mtd, VMLINUX_NAND_OFFSET, &io_size, NULL,
				 VMLINUX_NAND_LIMIT,
				 (u8 *)VMLINUX_LOAD_ADDR);
	if (ret || io_size != write_size) {
		printf("vmlinux: NAND readback failed: %d, read 0x%zx bytes\n",
		       ret, io_size);
		return CMD_RET_FAILURE;
	}

	verify_crc = crc32(0, (const u8 *)VMLINUX_LOAD_ADDR, write_size);
	if (verify_crc != image_crc) {
		printf("vmlinux: CRC mismatch, source=%08x readback=%08x\n",
		       image_crc, verify_crc);
		return CMD_RET_FAILURE;
	}

	printf("vmlinux: update complete, source=0x%lx written=0x%lx "
	       "crc32=%08x\n", image_size, write_size, image_crc);
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	vmlinux, 3, 0, do_vmlinux,
	"download and update the NAND vmlinux image",
	"update [filename]\n"
	"    - TFTP filename (default: vmlinux), validate the ELF image,\n"
	"      write it to NAND offset 0, and verify the readback CRC32"
);
