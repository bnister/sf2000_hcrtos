#ifndef __GE_DRAW_SUBTITLE__
#define __GE_DRAW_SUBTITLE__

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdlib.h>
#include <hcge/ge_api.h>
#include "dis_test.h"
#include <hcuapi/dis.h>

#define HCGE_RED 0
#define HCGE_GREEN 1
#define HCGE_BLUE 2
#define HCGE_BLACK 3
#define HCGE_WHITE 4
#define HCGE_TRANSPARENT 5

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

typedef struct HCGeDrawSubtitle
{
	int fbdev;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	hcge_context *ctx;
	int w;
	int h;
	int x;
	int y;
	int screen_w;
	int screen_h;
	int base_size;
	int buffer_num;
	uint32_t line_width;
	uint32_t pixel_width;
	uint32_t screen_size;
	uint8_t *bg_picture;
	uint8_t *free_ptr;
	uint8_t *screen_buffer[2];
	void *fb_base;
	uint32_t tar_size;
	void * tar_buf;
	int format;
}HCGeDrawSubtitle_t;

int init_fb_device(struct HCGeDrawSubtitle *ge_info);
void deinit_fb_device(struct HCGeDrawSubtitle *ge_info);
void draw_background(uint8_t *buf, struct HCGeDrawSubtitle *ge_info);
int ge_stop_draw_subtitle(struct HCGeDrawSubtitle *ge_info);

#endif
