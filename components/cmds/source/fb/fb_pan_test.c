#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>
#include <kernel/fb.h>
#include <hcuapi/fb.h>

#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif

static int fb_pan_up(int argc, char **argv)
{
	struct fb_var_screeninfo var;
	int fbfd = 0;

	fbfd = open(FBDEV_PATH, O_RDWR);
	if (fbfd == -1)
		return -1;

	ioctl(fbfd, FBIOGET_VSCREENINFO, &var);
	var.vmode |= FB_VMODE_YWRAP;
	var.yoffset += 10;
	ioctl(fbfd, FBIOPAN_DISPLAY, &var);

	close(fbfd);
	return 0;
}

static int fb_pan_down(int argc, char **argv)
{
	struct fb_var_screeninfo var;
	int fbfd = 0;

	fbfd = open(FBDEV_PATH, O_RDWR);
	if (fbfd == -1)
		return -1;

	ioctl(fbfd, FBIOGET_VSCREENINFO, &var);
	var.vmode |= FB_VMODE_YWRAP;
	if (var.yoffset >= 10)
		var.yoffset -= 10;
	ioctl(fbfd, FBIOPAN_DISPLAY, &var);

	close(fbfd);
	return 0;
}

CONSOLE_CMD(pan_down, "fb", fb_pan_down, CONSOLE_CMD_MODE_SELF, "fb pan display down")
CONSOLE_CMD(pan_up, "fb", fb_pan_up, CONSOLE_CMD_MODE_SELF, "fb pan display up")
