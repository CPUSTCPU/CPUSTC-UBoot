# SPDX-License-Identifier: GPL-2.0+

extra-y	= start.o

obj-y += time.o
obj-y += interrupts.o
obj-y += cpu.o
obj-y += dram.o
obj-$(CONFIG_BOARD_LATE_INIT) += boot_logo.o
obj-$(CONFIG_LA32R_BOOT_LOGO) += boot_logo_data.o
