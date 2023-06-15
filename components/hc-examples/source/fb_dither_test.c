#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/fb.h>
#include <hcuapi/fb.h>
#include <hcuapi/dis.h>
#include <hcuapi/common.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#else
#include <kernel/lib/console.h>
#endif


#define NO_WINDOW		(0)
#define HAMMING_WINDOW		(1)
#define LANCZOS_WINDOW		(2)

#define WINDOW_TYPE		(LANCZOS_WINDOW)

#define FIXED_BITS		(32)
#define FRAC_BITS		(16)
#define FRAC_MASK		((1 << (FRAC_BITS)) - 1)
#define INT_BITS		((FIXED_BITS) - (FRAC_BITS))

#define FRAC_MAX		((int)(0x7fffffff))
#define FRAC_MIN		((int)(0x80000000))
#define FRAC_ONE		(1 << (FRAC_BITS))
#define FRAC_TWO		((FRAC_ONE) << 1)
#define FRAC_HALF		((FRAC_ONE) >> 1)
#define FRAC_QUARTER		((FRAC_ONE) >> 2)

#define FLOAT_TO_FRAC(x)	((int)((x) * (FRAC_ONE)))
#define DOUBLE_TO_FRAC(x)	((int)((x) * (FRAC_ONE)))
#define INT_TO_FRAC(x)		((x) << (FRAC_BITS))
#define FRAC_TO_DOUBLE(x)	((float)(x) / (FRAC_ONE))
#define PI 3.141592653589793238462643383279502884197169399375105820974944592
#define INV_PI (1/PI)
#define FRAC_INV_PI		(DOUBLE_TO_FRAC(INV_PI))
#define FRAC_INV_2PI_SQ		(DOUBLE_TO_FRAC(1/(4*PI*PI)))

#define MY_BITS			(16)
#define MY0			(93.484f)
#define MY1			(314.486f)
#define MY2			(31.7476f)
#define MY3			(439.7176f)

#define MC0_1			(51.542f)
#define MC0_2			(224.8784f)
#define MC1_1			(173.336f)
#define MC1_2			(204.2796f)
#define MC2_1			(224.8784f)
#define MC2_2			(20.5989f)

#define FLOOR(x) ((int)((x) + 0.5f))

#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#define FBDEV_PATH2  "/dev/fb1"
#endif

#define FB_WIDTH (1280)
#define FB_HIGTH (720)
#define COLOR_BIT (8)

static struct termios stored_settings;

#define ENTRY_BITS		(8)
#define CLUT8_ALPHA		(0xa)

static unsigned short red_16[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
	0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
	0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
	0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
	0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
	0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
	0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
	0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005,
	0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005,
	0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005,
	0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005,
	0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006,
	0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006,
	0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006,
	0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006,
	0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007,
	0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007,
	0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007,
	0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007,
};
static unsigned short green_16[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
};
static unsigned short blue_16[] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
};

static unsigned short alpha_16[] = {
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
	CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA, CLUT8_ALPHA,
};


static hcfb_scale_t scale_param = { 1280, 720, 1920, 1080 };// to do later...
static int screen_size =  0;
static int pixel_bytes =  0;
static long int screensize = 0;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static char *fb_p0 = 0;
static char *fb_p1 = 0;
static uint32_t fb_dis_buf[1280*720] = { 0 };
static int fb_fd = 0;

struct fb_cmap cmap_user = {
	.start = 0,
	.len = 256,
	.red = red_16,
	.green  = green_16,
	.blue = blue_16,
	.transp = alpha_16,
};


/** Represents an area of the screen.*/
typedef struct {
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
} fb_dis_area_t;


static int bytes_per_pixel (int bits_per_pixel) {
	switch(bits_per_pixel) {
	case 32:
	case 24:
		return 4;
	case 16:
	case 15:
		return 2;
	default:
		return 1;
	}
}
static void fbdev_calc_scale(void) {
	int disfd = -1;
	struct dis_screen_info screen = { 0 };

	disfd = open("/dev/dis", O_RDWR);
	if (disfd < 0) {
		return;
	}

	screen.distype = DIS_TYPE_HD;
	if(ioctl(disfd, DIS_GET_SCREEN_INFO, &screen)) {
		close(disfd);
		return;
	}

	scale_param.h_mul = screen.area.w;
	scale_param.v_mul = screen.area.h;
	close(disfd);
}

static int set_dis_mem_color(int index) {
	int i = 0;
	int j = 0;
	int16_t* fb_dis_16b = NULL;
	int32_t* fb_dis_32b = NULL;
	int8_t* fb_dis_8b = NULL;
	
	fb_dis_16b = (int16_t*)fb_dis_buf;
	fb_dis_8b = (int8_t*)fb_dis_buf;
	#if 1
	for (j=0;j<FB_HIGTH;j++) {
		for (i=0;i<FB_WIDTH;i++) {
			if (COLOR_BIT == 32) {
				fb_dis_buf[j*FB_WIDTH+i] = 0xffff0000;//red
			}
			else if (COLOR_BIT == 16) {
				fb_dis_16b[j*FB_WIDTH+i] = 0x07e0;
			}
			else if (COLOR_BIT == 8) {
				#if 0
				fb_dis_8b[j*FB_WIDTH+i] = 0xff;
				#else
				fb_dis_8b[j*FB_WIDTH+i] = rand()%256;//index;//rand()%256;
				#endif
			}
		}
	}
	#else
	for(j=0;j<100;j++) {
		for(i=0;i<100;i++) {
			if (COLOR_BIT == 32) {
				fb_dis_buf[j*100+i] = 0xffff0000;
			}
			else if (COLOR_BIT == 16) {
				fb_dis_16b[j*100+i] = 0x07e0;
			}
			else if (COLOR_BIT == 8) {
				#if 0
				fb_dis_8b[j*FB_WIDTH+i] = 0xff;
				#else
				fb_dis_8b[j*FB_WIDTH+i] = rand()%256;//index;//rand()%256;
				#endif
			}
		}
	}
	#endif
	
	return 0;
}

static void fb_flush(fb_dis_area_t* dis_area,uint8_t* color_p) {
	uint32_t* color_32b = NULL;
	uint16_t* color_16b = NULL;
	uint8_t* color_8b = NULL;

		/*Truncate the area to the screen*/
	int32_t act_x1 = dis_area->x1 < 0 ? 0 : dis_area->x1;
	int32_t act_y1 = dis_area->y1 < 0 ? 0 : dis_area->y1;
	int32_t act_x2 = dis_area->x2 > (int32_t)vinfo.xres - 1 ? (int32_t)vinfo.xres - 1 : dis_area->x2;
	int32_t act_y2 = dis_area->y2 > (int32_t)vinfo.yres - 1 ? (int32_t)vinfo.yres - 1 : dis_area->y2;

	int16_t w = (act_x2 - act_x1 + 1);
	long int location = 0;
	long int byte_location = 0;
	unsigned char bit_location = 0;
	
	if(vinfo.bits_per_pixel == 16 && vinfo.green.length == 5) {
		uint16_t * fbp16;
		uint16_t c;
		int32_t y;
		color_16b = color_p;
		/*printf("color_p: %p, w: %d, h: %d \n", color_p, act_x2 - act_x1, act_y2 - act_y1);*/
		for(y = act_y1; y <= act_y2; y++) {
			int i;
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
			fbp16 = (uint16_t *)fb_p0 + location;
			memcpy(fbp16,color_16b,(act_x2 - act_x1 + 1)*2);
			color_16b += w;
		}
	} else if(vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
		uint32_t * fbp32 = (uint32_t *)fb_p0;
		int32_t y;
		color_32b = color_p;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 4;
			memcpy(&fbp32[location], (uint32_t *)color_32b, (act_x2 - act_x1 + 1) * 4);
			color_32b += w;
		}
	}
	/*16 bit per pixel*/
	else if(vinfo.bits_per_pixel == 16) {
		uint16_t * fbp16 = (uint16_t *)fb_p0;
		int32_t y;
		color_16b = color_p;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
			memcpy(&fbp16[location], (uint32_t *)color_16b, (act_x2 - act_x1 + 1) * 2);
			color_16b += w;
		}
	}
	/*8 bit per pixel*/
	else if(vinfo.bits_per_pixel == 8) {
		uint8_t * fbp8 = (uint8_t *)fb_p0;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length;
			memcpy(&fbp8[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1));
			color_p += w;
		}
	}
	/*1 bit per pixel*/
	else if(vinfo.bits_per_pixel == 1) {
		uint8_t * fbp8 = (uint8_t *)fb_p0;
		int32_t x;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			for(x = act_x1; x <= act_x2; x++) {
				location = (x + vinfo.xoffset) + (y + vinfo.yoffset) * vinfo.xres;
				byte_location = location / 8; /* find the byte we need to change */
				bit_location = location % 8; /* inside the byte found, find the bit we need to change */
				fbp8[byte_location] &= ~(((uint8_t)(1)) << bit_location);
				//fbp8[byte_location] |= ((uint8_t)(color_p->full)) << bit_location;
				color_p++;
			}

			color_p += dis_area->x2 - act_x2;
		}
	} else {
		/*Not supported bit per pixel*/
	}
}

static int fb_dither_start (int argc , char *argv[]) {
	fb_dis_area_t test_dis_area;
	int index = 0;
	unsigned short red_tmp = 0;
	unsigned short blue_tmp = 0;
	unsigned short green_tmp = 0;
	unsigned short alpha_tmp = 0;
	int i = 0;
	int bit_tmp = 0;
	
	if(argc == 4) {
		index = atoi(argv[1]);
		alpha_tmp = atoi(argv[2]);
		bit_tmp = atoi(argv[3]);
		for(i=0;i<256;i++) {
			alpha_16[i] = alpha_tmp;
			//bit_tmp set to 0
			red_16[i] = (red_16[i]<<bit_tmp);
			green_16[i] = (green_16[i]<<bit_tmp);
			blue_16[i] = (blue_16[i]<<bit_tmp);
		}
	}
	else if(argc == 2){
		index = atoi(argv[1]);
	}
	
	if(index==0)
		fb_fd = open(FBDEV_PATH, O_RDWR);
	else
		fb_fd = open(FBDEV_PATH2, O_RDWR);
	
	if(fb_fd == -1) {
		perror("Error: cannot open framebuffer device");
		return -1;
	}
	printf("The framebuffer device was opened successfully.\n");

	// Make sure that the display is on.
	if (ioctl(fb_fd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
		perror("ioctl(FBIOBLANK)");
		return -1;
	}

	if(ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return -1;
	}

	vinfo.xoffset = 0;
	vinfo.yoffset = 0;
	if(COLOR_BIT==8) {
		vinfo.bits_per_pixel = 8;
		vinfo.red.length = 8;
		vinfo.green.length = 8;
		vinfo.blue.length = 8;
		vinfo.yres_virtual = vinfo.yres;
	}

	printf("bits_per_pixel: %d,red.length: %d, green.length: %d, blue.length: %d\n",
	       vinfo.bits_per_pixel, vinfo.red.length, vinfo.green.length,
	       vinfo.blue.length);
	vinfo.activate |= FB_ACTIVATE_FORCE|FB_ACTIVATE_NOW;
	printf("cmap_user red=0x%x,green=0x%x,transp=0x%x\n",(unsigned int)cmap_user.red,\
		(unsigned int)cmap_user.green,(unsigned int)cmap_user.transp);
	
	if(ioctl(fb_fd, FBIOPUTCMAP, &cmap_user) == -1) {
		perror("Error set cmap");
		return -1;
	}

	// Get variable screen information
	if(ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return -1;
	}

	// Get fixed screen information
	if(ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		return -1;
	}

	fbdev_calc_scale();

	if (ioctl(fb_fd, HCFBIOSET_SCALE, &scale_param) != 0) {
		perror("ioctl(set scale param)");
		return -1;
	}
	// Figure out the size of the screen in bytes
	screensize =  finfo.smem_len; //finfo.line_length * vinfo.yres;

	ioctl(fb_fd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_NO_CACHE);

	// Map the device to memory
	fb_p0 = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if((intptr_t)fb_p0 == -1) {
		perror("Error: failed to map framebuffer device to memory");
		return;
	}
	memset(fb_p0, 0, screensize);

	pixel_bytes = bytes_per_pixel(vinfo.bits_per_pixel);
	screen_size = vinfo.xres * vinfo.yres * pixel_bytes;
	printf("pixel_bytes: %d, screen_size: %08x,fb_p0 = 0x%x,fb_dis_buf=0x%x\n", pixel_bytes, screen_size,fb_p0,fb_dis_buf);
	printf("The framebuffer device was mapped to memory successfully.\n");
	test_dis_area.x1 = 0;
	test_dis_area.x2 = FB_WIDTH;
	test_dis_area.y1 = 0;
	test_dis_area.y2 = FB_HIGTH;
	set_dis_mem_color(index);
	fb_flush(&test_dis_area,(uint8_t*)fb_dis_buf);
	return 0;
}


static int fb_dither_stop (int argc , char *argv[]) {
	if (ioctl(fb_fd, FBIOBLANK, FB_BLANK_NORMAL) != 0) {
		perror("ioctl(FBIOBLANK)");
		return -1;
	}
	close(fb_fd);
	return 0;
}

static void exit_console (int signo) {
    (void)signo;
    tcsetattr(0 , TCSANOW , &stored_settings);
    exit(0);
}

int main (int argc , char *argv[]) {
	struct termios new_settings;

    tcgetattr(0 , &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0 , TCSANOW , &new_settings);

    signal(SIGTERM , exit_console);
    signal(SIGINT , exit_console);
    signal(SIGSEGV , exit_console);
    signal(SIGBUS , exit_console);
    console_init("hcfb:");

    console_register_cmd(NULL , "start" , fb_dither_start , CONSOLE_CMD_MODE_SELF , "start ");
    console_register_cmd(NULL , "stop" , fb_dither_stop , CONSOLE_CMD_MODE_SELF , "stop");
	
    console_start();
    exit_console(0);
    (void)argc;
    (void)argv;
    return 0;
}

