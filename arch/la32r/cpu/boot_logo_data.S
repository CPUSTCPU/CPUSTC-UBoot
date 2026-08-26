/* SPDX-License-Identifier: GPL-2.0+ */

	.section .rodata.la32r_boot_logo, "a"
	.balign 4
	.globl la32r_boot_logo_start
la32r_boot_logo_start:
	.incbin "vga565.raw"
	.globl la32r_boot_logo_end
la32r_boot_logo_end:
	.balign 4
