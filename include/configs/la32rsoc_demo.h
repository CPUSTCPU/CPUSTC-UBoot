#ifndef __TRIVIALMIPS_NSCSCC_CONFIG_H
#define __TRIVIALMIPS_NSCSCC_CONFIG_H

/* BootROM + MIG is pretty smart. DDR and Cache initialized */
#define CONFIG_SKIP_LOWLEVEL_INIT

/*--------------------------------------------
 * CPU configuration
 */
/* CPU cycle counter rate in Hz; may be overridden by the build parameter. */
#ifndef CPUSTC_CPU_FREQ_HZ
#define CPUSTC_CPU_FREQ_HZ		50000000UL
#endif
#define CONFIG_SYS_LOONGARCH_TIMER_FREQ	CPUSTC_CPU_FREQ_HZ

/* Cache Configuration */
#define CONFIG_SYS_MIPS_CACHE_MODE	CONF_CM_CACHABLE_NONCOHERENT

/*----------------------------------------------------------------------
 * Memory Layout
 */

/* SDRAM Configuration (for final code, data, stack, heap) */
#define CONFIG_SYS_SDRAM_BASE		0xa0000000
#define CONFIG_SYS_SDRAM_SIZE		0x08000000	/* 128 Mbytes */
#define CONFIG_SYS_INIT_SP_ADDR		\
	(CONFIG_SYS_SDRAM_BASE + CONFIG_SYS_SDRAM_SIZE - 0x1000)

#define CONFIG_SYS_MEMTEST_START    0xa0000000
#define CONFIG_SYS_MEMTEST_END      0xb0000000

#define CONFIG_SYS_MALLOC_LEN		(256 << 10)
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_LOAD_ADDR		0x90000000 /* default load address */
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1

/*----------------------------------------------------------------------
 * Commands
 */
//#define CONFIG_SYS_LONGHELP		/* undef to save memory */
//#define CONFIG_CMDLINE_EDITING

/*------------------------------------------------------------
 * Console Configuration
 */
#define CONFIG_SYS_CBSIZE		1024 /* Console I/O Buffer Size   */
#define CONFIG_SYS_MAXARGS		16   /* max number of command args*/


// serial configuration
#define CONFIG_CONS_INDEX 1
#define CONFIG_SYS_NS16550_COM1 0x9fe001e0
#define CONFIG_SYS_NS16550_CLK 33000000
#define CONFIG_SYS_NS16550_IER 0


/* -------------------------------------------------
 * Environment
 */
#define CONFIG_ENV_SIZE		0x4000
/* Keep the environment after the 48 MiB physical NAND kernel window. */
#define CONFIG_ENV_OFFSET		0x03000000
#define CONFIG_ENV_RANGE		0x00200000
#define CPUSTC_BOOTCOMMAND \
    "usb start; if ext4load usb 0:1 0xa3000000 /vmlinux; " \
    "then usb stop; bootelf 0xa3000000 console=ttyS0,115200 rdinit=/init; " \
    "else usb stop; nand read 0xa3000000 0 0x2800000 && " \
    "bootelf 0xa3000000 console=ttyS0,115200 rdinit=/init; fi"
#define CONFIG_EXTRA_ENV_SETTINGS \
    "bootcmd=" CPUSTC_BOOTCOMMAND "\0" \
    "autoload=no\0" \
    "stdin=serial\0" \
    "stdout=serial\0" \
    "stderr=serial\0" \
    "bootlogo=1\0" \
    "serverip=169.254.150.45\0" \
    "ipaddr=169.254.150.46\0" \
    "netmask=255.255.255.0\0" \
    "ethaddr=00:98:76:64:32:19\0"



/* ---------------------------------------------------------------------
 * Board boot configuration
 */

#define CONFIG_TIMESTAMP	/* Print image info with timestamp */


/* Flash */
#define CONFIG_SYS_MAX_FLASH_BANKS_DETECT   1
#define CONFIG_SYS_MAX_FLASH_SECT           64

/* Raw NAND */
#define CONFIG_SYS_MAX_NAND_DEVICE          1
#define CONFIG_SYS_NAND_MAX_CHIPS           1


#endif /* __TRIVIALMIPS_NSCSCC_CONFIG_H */
