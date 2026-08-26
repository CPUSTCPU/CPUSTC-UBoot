// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <iomux.h>
#include <linux/delay.h>

#define LA32R_VGA_FB_BASE	0x87e00000UL
#define LA32R_VGA_WIDTH		640
#define LA32R_VGA_HEIGHT	480
#define LA32R_VGA_BPP		2
#define LA32R_VGA_FB_SIZE	(LA32R_VGA_WIDTH * LA32R_VGA_HEIGHT * LA32R_VGA_BPP)

#ifdef CONFIG_LA32R_BOOT_LOGO
extern const unsigned char la32r_boot_logo_start[];
extern const unsigned char la32r_boot_logo_end[];

static int la32r_boot_logo_enabled(void)
{
	char *value = env_get("bootlogo");

	return !value || strcmp(value, "0");
}

static void la32r_enable_vga_console(void)
{
	const char *stdout_env = "serial,vidconsole";
	const char *stderr_env = "serial,vidconsole";

	env_set("stdout", stdout_env);
	env_set("stderr", stderr_env);
#if CONFIG_IS_ENABLED(CONSOLE_MUX)
	iomux_doenv(stdout, stdout_env);
	iomux_doenv(stderr, stderr_env);
#endif
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_LA32R_BOOT_LOGO
	unsigned char *fb = (unsigned char *)LA32R_VGA_FB_BASE;
	size_t logo_size = la32r_boot_logo_end - la32r_boot_logo_start;
	size_t copy_size = logo_size;

	if (!la32r_boot_logo_enabled())
		return 0;

	if (copy_size > LA32R_VGA_FB_SIZE)
		copy_size = LA32R_VGA_FB_SIZE;

	memcpy(fb, la32r_boot_logo_start, copy_size);
	if (copy_size < LA32R_VGA_FB_SIZE)
		memset(fb + copy_size, 0, LA32R_VGA_FB_SIZE - copy_size);

	mdelay(CONFIG_LA32R_BOOT_LOGO_DELAY_MS);
	memset(fb, 0, LA32R_VGA_FB_SIZE);
	la32r_enable_vga_console();
#endif

	return 0;
}
