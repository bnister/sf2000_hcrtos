/**
 * @file fbdev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "fbdev.h"
#ifdef __HCRTOS__
#if USE_FBDEV || USE_BSD_FBDEV

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#if USE_BSD_FBDEV
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/consio.h>
#include <sys/fbio.h>
#else  /* USE_BSD_FBDEV */
#include <kernel/fb.h>
#include <hcuapi/fb.h>
#include <hcuapi/dis.h>
#include <lv_gpu_hichip.h>
#endif /* USE_BSD_FBDEV */

/*********************
 *      DEFINES
 *********************/
#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif
#if 0
#define FBDEV_DBG printf
#else
#define FBDEV_DBG(...) do{}while(0)
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      STRUCTURES
 **********************/

struct bsd_fb_var_info {
	uint32_t xoffset;
	uint32_t yoffset;
	uint32_t xres;
	uint32_t yres;
	int bits_per_pixel;
};

struct bsd_fb_fix_info {
	long int line_length;
	long int smem_len;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
#if USE_BSD_FBDEV
static struct bsd_fb_var_info vinfo;
static struct bsd_fb_fix_info finfo;
#else
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
#endif /* USE_BSD_FBDEV */
char *fbp0 = 0;
#if LV_USE_DUAL_FRAMEBUFFER
char *fbp1 = 0;
#endif
char *fbp_swap_buf = 0;
static void *draw_fb = NULL;
static long int screensize = 0;
static int fbfd = 0;

static int screen_size =  0;
static int pixel_bytes =  0;

static int disp_rotate = 0;
static int h_flip = 0;
static int v_flip = 0;
static int fb_onoff = 0;

/**********************
 *      MACROS
 **********************/

#if USE_BSD_FBDEV
#define FBIOBLANK FBIO_BLANK
#endif /* USE_BSD_FBDEV */

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void fbdev_buffer_init(uint32_t virt_addr, uint32_t phy_addr, uint32_t size);

static int bytes_per_pixel(int bits_per_pixel)
{
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

static void fbdev_calc_scale(void)
{
	int disfd = -1;
	struct dis_screen_info screen = { 0 };
	hcfb_scale_t scale_param = { 1280, 720, 1920, 1080 };// to do later...
	int scale_enable_val =0;
	uint32_t value = 0;
#ifdef __linux__
	int fd = open("/proc/device-tree/soc/fb0@18808000/scale", O_RDONLY);
	if(fd < 0){
		return;
	}
	uint8_t buf[8];
	uint8_t *tmp = buf;
	if(read(fd, buf, 8) != 8){
		close(fd);
		return;
	}
	close(fd);
	scale_param.h_div = (tmp[0] & 0xff) << 24 | (tmp[1] & 0xff) << 16 | (tmp[2] & 0xff) << 8 | (tmp[3] & 0xff);
	tmp = buf + sizeof(uint32_t);
	scale_param.v_div = (tmp[0] & 0xff) << 24 | (tmp[1] & 0xff) << 16 | (tmp[2] & 0xff) << 8 | (tmp[3] & 0xff);
#else
	int np = -1;
	np = fdt_node_probe_by_path("/hcrtos/fb0");
	if (np >= 0){
		scale_enable_val = fdt_property_read_bool(np, "scale-enable");
		if(scale_enable_val)
		{
			fdt_get_property_data_by_name(np,"scale",&value);
			if(value == 16)
			{
				fdt_get_property_u_32_index(np, "scale", 0, &value);
				scale_param.h_div = (uint16_t)value;
				fdt_get_property_u_32_index(np, "scale", 1, &value);
				scale_param.v_div = (uint16_t)value;
				fdt_get_property_u_32_index(np, "scale", 2, &value);
				scale_param.h_mul = (uint16_t)value;
				fdt_get_property_u_32_index(np, "scale", 3, &value);
				scale_param.v_mul = (uint16_t)value;
			}
			else
			{
				scale_enable_val = 0;
			}
		}
	}
#endif

	disfd = open("/dev/dis", O_RDWR);
	if (disfd < 0) {
		return;
	}

	screen.distype = DIS_TYPE_HD;
	if(ioctl(disfd, DIS_GET_SCREEN_INFO, &screen)) {
		close(disfd);
		return;
	}

	if(scale_enable_val == 0)
	{
		scale_param.h_div = vinfo.xres;
		scale_param.v_div = vinfo.yres;
		scale_param.h_mul = screen.area.w;
		scale_param.v_mul = screen.area.h;
	}
	printf("scale_param.h_div: %d, scale_param.v_div: %d, scale_param.h_mul: %d, scale_param.v_mul:%d scale_enable_val:%d\n",
			scale_param.h_div, scale_param.v_div, scale_param.h_mul, scale_param.v_mul,scale_enable_val);

	close(disfd);
	if (ioctl(fbfd, HCFBIOSET_SCALE, &scale_param) != 0) {
		perror("ioctl(set scale param)");
	}
}
#ifdef __linux__
static int fbdev_get_param_from_dts(const char *path)
{
	int fd = open(path, O_RDONLY);
	int value = 0;;
	if(fd >= 0){
		uint8_t buf[4];
		if(read(fd, buf, 4) != 4){
			close(fd);
			return value;
		}
		close(fd);
		value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
	}
	return value;
}
#endif

void fbdev_init(void)
{
	int flip_support = 0;


	// Open the file for reading and writing
	fbfd = open(FBDEV_PATH, O_RDWR);
	if(fbfd == -1) {
		perror("Error: cannot open framebuffer device");
		return;
	}
	printf("The framebuffer device was opened successfully.\n");

#ifdef __linux__
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
	char status[16] = {0};
	fbdev_get_string_from_dts(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
	if(!strcmp(status, "okay")){
		disp_rotate = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/rotate");
		flip_support = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/flip_support");
		h_flip = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/h_flip");
		v_flip = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/v_flip");
	}
	printf("disp_rotate: %d, h_flip: %d, v_flip: %d\n", disp_rotate, h_flip, v_flip);
#else
	int np = -1;
	np = fdt_node_probe_by_path("/hcrtos/rotate");
	if (np >= 0){
		fdt_get_property_u_32_index(np, "rotate", 0, &disp_rotate);
		fdt_get_property_u_32_index(np, "flip_support", 0, &flip_support);
		fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
		fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
	}
#endif
	printf("%s:disp_rotate: %d, h_flip: %d, v_flip: %d\n", __func__, disp_rotate, h_flip, v_flip);

#if USE_BSD_FBDEV
	struct fbtype fb;
	unsigned line_length;

	//Get fb type
	if (ioctl(fbfd, FBIOGTYPE, &fb) != 0) {
		perror("ioctl(FBIOGTYPE)");
		return;
	}

	//Get screen width
	if (ioctl(fbfd, FBIO_GETLINEWIDTH, &line_length) != 0) {
		perror("ioctl(FBIO_GETLINEWIDTH)");
		return;
	}

	vinfo.xres = (unsigned) fb.fb_width;
	vinfo.yres = (unsigned) fb.fb_height;
	vinfo.bits_per_pixel = fb.fb_depth;
	vinfo.xoffset = 0;
	vinfo.yoffset = 0;
	finfo.line_length = line_length;
	finfo.smem_len = finfo.line_length * vinfo.yres;
#else /* USE_BSD_FBDEV */

	// Get variable screen information
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return;
	}

	vinfo.xoffset = 0;
	vinfo.yoffset = 0;
#if CONFIG_LV_HC_FB_COLOR_DEPTH == 32
	vinfo.bits_per_pixel = 32;
	vinfo.red.length = 8;
	vinfo.green.length = 8;
	vinfo.blue.length = 8;
#elif CONFIG_LV_HC_FB_COLOR_DEPTH_16_ARGB1555
	vinfo.bits_per_pixel = 16;
	vinfo.red.length = 5;
	vinfo.green.length = 5;
	vinfo.blue.length = 5;
	vinfo.transp.length = 1;
#else
	//ARGB4444
	vinfo.bits_per_pixel = 16;
	vinfo.red.length = 4;
	vinfo.green.length = 4;
	vinfo.blue.length = 4;
	vinfo.transp.length = 4;
#endif

#if LV_USE_DUAL_FRAMEBUFFER
	vinfo.yres_virtual = 2 * vinfo.yres;
#else
	vinfo.yres_virtual = vinfo.yres;
#endif
	printf("bits_per_pixel: %d,red.length: %d, green.length: %d, blue.length: %d\n",
	       vinfo.bits_per_pixel, vinfo.red.length, vinfo.green.length,
	       vinfo.blue.length);

#ifdef __linux__
	vinfo.activate |= FB_ACTIVATE_FORCE;
#endif

	// Get variable screen information
	if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return;
	}

	// Get fixed screen information
	if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		return;
	}
#endif /* USE_BSD_FBDEV */

	printf(" variable screen: %dx%d, %dbpp\n", (int)vinfo.xres, (int)vinfo.yres, (int)vinfo.bits_per_pixel);

	fbdev_calc_scale();
	
	// Figure out the size of the screen in bytes
	screensize =  finfo.smem_len; //finfo.line_length * vinfo.yres;

	/*ioctl(fbfd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_NO_CACHE);*/
	ioctl(fbfd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_CACHE);

	// Map the device to memory
	fbp0 = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if((intptr_t)fbp0 == -1) {
		perror("Error: failed to map framebuffer device to memory");
		return;
	}
	memset(fbp0, 0, screensize);
	cacheflush(fbp0, screensize, DCACHE);

	pixel_bytes = bytes_per_pixel(vinfo.bits_per_pixel);
	screen_size = vinfo.xres * vinfo.yres * pixel_bytes;
	draw_fb = fbp0;
	printf("pixel_bytes: %d, screen_size: %08x\n", pixel_bytes, screen_size);
	printf("The framebuffer device was mapped to memory successfully.\n");

	int buf_count = 1;
#if LV_USE_DUAL_FRAMEBUFFER
	fbp1 = fbp0 + screen_size;
	buf_count++;
#endif

	if (flip_support){
		fbp_swap_buf = fbp0 + screen_size * buf_count;
		if(screen_size * 3 > finfo.smem_len){
			printf("error: framebuffer is not enough memory\n");
			assert(false);
			return;
		}
		buf_count++;
	}
	fbdev_buffer_init((uint32_t)(fbp0 + screen_size * buf_count), finfo.smem_start + screen_size * buf_count, finfo.smem_len - screen_size * buf_count);
}

void fbdev_exit(void)
{
	close(fbfd);
}

/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixel to copy to the `area` part of the screen
 */
#if !LV_USE_GPU_HICHIP
void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
	int32_t x_limit = (int32_t)vinfo.xres;
	int32_t y_limit = (int32_t)vinfo.yres;
	void *fb_addr = NULL;

	if(disp_rotate == 90 || disp_rotate == 270){
		x_limit = (int32_t)vinfo.yres;
		y_limit = (int32_t)vinfo.xres;
	}

	if(fbp0 == NULL ||
	   area->x2 < 0 ||
	   area->y2 < 0 ||
	   area->x1 > x_limit - 1 ||
	   area->y1 > y_limit - 1) {
		lv_disp_flush_ready(drv);
		return;
	}

	/*Truncate the area to the screen*/
	int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
	int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
	int32_t act_x2 = area->x2 > (int32_t)vinfo.xres - 1 ? (int32_t)vinfo.xres - 1 : area->x2;
	int32_t act_y2 = area->y2 > (int32_t)vinfo.yres - 1 ? (int32_t)vinfo.yres - 1 : area->y2;


	lv_coord_t w = (act_x2 - act_x1 + 1);
	long int location = 0;
	long int byte_location = 0;
	unsigned char bit_location = 0;
#if 1

	/*32 or 24 bit per pixel*/
	if(vinfo.bits_per_pixel == 16 && vinfo.green.length == 5) {
		uint16_t * fbp16;
		uint16_t c;
		lv_color_t *inpixel_p;
		int32_t y;
		/*FBDEV_DBG("color_p: %p, w: %d, h: %d \n", color_p, act_x2 - act_x1, act_y2 - act_y1);*/
#if 1
		for(y = act_y1; y <= act_y2; y++) {
			int i;
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
			inpixel_p = color_p;
			fbp16 = (uint16_t *)fbp0 + location;
			for (i = 0; i < (act_x2 - act_x1 + 1); i++) {
				if (inpixel_p->ch.alpha)
					c = 1 << 15;
				else
					c = 0;

				c |= (((short)inpixel_p->ch.red >> 3)<<10)|(((short)inpixel_p->ch.green >> 3) << 5)|((short)inpixel_p->ch.blue>>3);
				*fbp16 = c;

				inpixel_p++;
				fbp16++;
			}
			color_p += w;
		}
#endif
	} else if(vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
		uint32_t * fbp32 = (uint32_t *)fbp0;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 4;
			memcpy(&fbp32[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 4);
			color_p += w;
		}
	}
	/*16 bit per pixel*/
	else if(vinfo.bits_per_pixel == 16) {
		uint16_t * fbp16 = (uint16_t *)fbp0;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
			memcpy(&fbp16[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 2);
			color_p += w;
		}
	}
	/*8 bit per pixel*/
	else if(vinfo.bits_per_pixel == 8) {
		uint8_t * fbp8 = (uint8_t *)fbp0;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length;
			memcpy(&fbp8[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1));
			color_p += w;
		}
	}
	/*1 bit per pixel*/
	else if(vinfo.bits_per_pixel == 1) {
		uint8_t * fbp8 = (uint8_t *)fbp0;
		int32_t x;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			for(x = act_x1; x <= act_x2; x++) {
				location = (x + vinfo.xoffset) + (y + vinfo.yoffset) * vinfo.xres;
				byte_location = location / 8; /* find the byte we need to change */
				bit_location = location % 8; /* inside the byte found, find the bit we need to change */
				fbp8[byte_location] &= ~(((uint8_t)(1)) << bit_location);
				fbp8[byte_location] |= ((uint8_t)(color_p->full)) << bit_location;
				color_p++;
			}

			color_p += area->x2 - act_x2;
		}
	} else {
		/*Not supported bit per pixel*/
	}
#else
	// 4. ����?a?y?��?D?? buffer
	/*FBDEV_DBG("color_p: %p, fbp0: %p, fbp1: %p\n", color_p, fbp0, fbp1);*/
#if 0
	/*FBDEV_DBG(" before display\n");*/
	/*getchar();*/
	if(color_p == fbp0) {
		vinfo.yoffset = 0;
		cacheflush(fbp0, screen_size, DCACHE);
	} else {
		vinfo.yoffset = vinfo.yres;
		cacheflush(fbp1, screen_size, DCACHE);
	}

	/*FBDEV_DBG("displaying\n");*/

	int ret;
	ret = ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
	if (ret < 0) {
		perror("ioctl() / FBIOPAN_DISPLAY");
	}

#if 0
	ret = 0;
	ioctl(fbfd, FBIO_WAITFORVSYNC, &ret);
	if (ret < 0) {
		perror("ioctl() / FBIO_WAITFORVSYNC");
	}
#endif
	/*getchar();*/
	/*FBDEV_DBG("after display\n");*/
#endif
	memcpy(fbp0, color_p, screen_size);
	cacheflush(fbp0, screen_size, BCACHE);
#endif

	//May be some direct update command is required
	//ret = ioctl(state->fd, FBIO_UPDATE, (unsigned long)((uintptr_t)rect));

	lv_disp_flush_ready(drv);
}
#else

void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
	int32_t x_limit = (int32_t)vinfo.xres;
	int32_t y_limit = (int32_t)vinfo.yres;

	if(disp_rotate == 90 || disp_rotate == 270){
		x_limit = (int32_t)vinfo.yres;
		y_limit = (int32_t)vinfo.xres;
	}

	if(draw_fb == NULL ||
	   area->x2 < 0 ||
	   area->y2 < 0 ||
	   area->x1 > x_limit - 1 ||
	   area->y1 > y_limit - 1) {
		lv_disp_flush_ready(drv);
		return;
	}

	int32_t act_x1 = area->x1;
	int32_t act_y1 = area->y1;
	int32_t act_x2 = area->x2;
	int32_t act_y2 = area->y2;

	lv_coord_t sw = (act_x2 - act_x1 + 1);
	lv_coord_t sh = (act_y2 - act_y1 + 1);

	hcge_state *state = &hcge_ctx->state;
	lv_draw_hichip_lock();

	state->render_options = HCGE_DSRO_NONE;
	state->drawingflags = HCGE_DSDRAW_NOFX;
	state->blittingflags = HCGE_DSBLIT_NOFX;

	state->src_blend = HCGE_DSBF_SRCALPHA;
	state->dst_blend = HCGE_DSBF_ZERO;

	state->destination.config.size.w = vinfo.xres;
	state->destination.config.size.h = vinfo.yres;
	state->destination.config.format = HCGE_DSPF_ARGB;
	if(fbp_swap_buf > 0 && (h_flip || v_flip))
		state->dst.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(fbp_swap_buf);
	else
		state->dst.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(draw_fb);
	state->dst.pitch = vinfo.xres * pixel_bytes;

	/*state->mod_hw = HCGE_SMF_CLIP;*/
	state->mod_hw = 0;
	state->clip.x1 = 0;
	state->clip.y1 = 0;
	state->clip.x2 = vinfo.xres - 1;
	state->clip.y2 = vinfo.yres - 1;

	state->source.config.size.w = sw;
	state->source.config.size.h = sh;
	state->source.config.format = HCGE_DSPF_ARGB;
	state->src.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(color_p);
	state->src.pitch = sw * bytes_per_pixel(LV_COLOR_DEPTH);

	/*32 or 24 bit per pixel*/
	if(vinfo.bits_per_pixel == 16 && vinfo.green.length == 5) {
		state->destination.config.format = HCGE_DSPF_ARGB1555;
	} else if(vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
		state->destination.config.format = HCGE_DSPF_ARGB;
	} else if(vinfo.bits_per_pixel == 16 || vinfo.transp.length == 4) {
		state->destination.config.format = HCGE_DSPF_ARGB4444;
	}
	/*16 bit per pixel*/
	else if(vinfo.bits_per_pixel == 16) {
	}
	/*8 bit per pixel*/
	else if(vinfo.bits_per_pixel == 8) {
	}
	/*1 bit per pixel*/
	else if(vinfo.bits_per_pixel == 1) {
	} else {
		/*Not supported bit per pixel*/
	}

	HCGERectangle srect;// = {0, 0, 1280, 720};
	state->accel = HCGE_DFXL_BLIT;
	if(disp_rotate == 270)
		state->blittingflags = HCGE_DSBLIT_ROTATE270;
	else if(disp_rotate == 180)
		state->blittingflags = HCGE_DSBLIT_ROTATE180;
	else if(disp_rotate == 90)
		state->blittingflags = HCGE_DSBLIT_ROTATE90;
	else
		state->blittingflags = 0;

	cacheflush(draw_fb, screen_size, DCACHE);
	cacheflush(color_p, sw * bytes_per_pixel(LV_COLOR_DEPTH) * act_y2 + act_x2 * sizeof(lv_color_t), DCACHE);
	hcge_set_state(hcge_ctx, &hcge_ctx->state, state->accel);
	/*FBDEV_DBG("act_x1: %d, act_y1: %d, act_x2: %d, act_y2: %d\n", act_x1, act_y1, act_x2, act_y2);*/
	if(drv->direct_mode) {
		lv_area_t *areas;
		int i = 0;
		lv_disp_t *disp = _lv_refr_get_disp_refreshing();
		FBDEV_DBG("disp->inv_p: %d\n", disp->inv_p);
		for(i = 0; i < disp->inv_p; i++) {
			if(disp->inv_area_joined[i]) {
				/*FBDEV_DBG("join: \n");*/
				continue;
			}
			areas = &disp->inv_areas[i];
			srect.x = areas->x1;
			srect.y = areas->y1;
			srect.w = areas->x2 - areas->x1 + 1;
			srect.h = areas->y2 - areas->y1 + 1;
			/*FBDEV_DBG("x:%d, y: %d, w: %d, h:%d\n", srect.x, srect.y, srect.w, srect.h);*/
			hcge_blit(hcge_ctx, &srect, srect.x, srect.y);
			/*getchar();*/
		}
	} else {
		int dx;
		int dy;
		if((disp_rotate == 270)){
			dx = vinfo.xres - act_y1 - sh + ((sh % 2 == 1)?1:0);
			dy = act_x1;
		}else if (disp_rotate == 90)  {
			dx = act_y1;
			dy = vinfo.yres - act_x1 - sw + ((sw % 2 == 1)?1:0);
		}else if(disp_rotate == 180){
			dx = vinfo.xres - act_x1 - sw;
			dy = vinfo.yres - act_y1 - sh;
		}else{
			dx = act_x1;
			dy = act_y1;
		}
		srect.x = 0;
		srect.y = 0;
		srect.w = sw;
		srect.h = sh;
		FBDEV_DBG("dx:%d, dy: %d, w: %d, h:%d, xoffset: %d, yoffset: %d\n", dx, dy, srect.w, srect.h,
				vinfo.xoffset, vinfo.yoffset);
		FBDEV_DBG("area->x1:%d, area->y1: %d, area->x2: %d, area->y2:%d\n", area->x1, area->y1, area->x2, area->y2);
		hcge_blit(hcge_ctx, &srect, dx, dy);
		hcge_engine_sync(hcge_ctx);
	}

	FBDEV_DBG("%s:disp_rotate: %d, h_flip: %d, v_flip: %d\n", __func__, disp_rotate, h_flip, v_flip);
	FBDEV_DBG("draw_fb: %p, fbp_swap_buf: %p, state->src.phys: 08x%08x, state->dst.phys: 08x%08x\n", draw_fb, fbp_swap_buf, state->src.phys,
			state->dst.phys);

	if(lv_disp_flush_is_last(drv)) {
		if(fbp_swap_buf && (h_flip || v_flip)){	
			state->destination.config.size.w = vinfo.xres;
			state->destination.config.size.h = vinfo.yres;
			state->dst.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(draw_fb);
			state->dst.pitch = vinfo.xres * pixel_bytes;

			state->source.config.size.w = vinfo.xres;
			state->source.config.size.h = vinfo.yres;
			state->source.config.format = state->destination.config.format;
			state->src.pitch = vinfo.xres * pixel_bytes;
			state->src.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(fbp_swap_buf);
			srect.x = 0;
			srect.y = 0;
			srect.w = vinfo.xres;
			srect.h = vinfo.yres;
			state->blittingflags = 0;
			if(h_flip)
				state->blittingflags |= HCGE_DSBLIT_FLIP_HORIZONTAL;
			if(v_flip)
				state->blittingflags |= HCGE_DSBLIT_FLIP_VERTICAL;
			FBDEV_DBG("state->blittingflags: 0x%08x\n", state->blittingflags);
			hcge_set_state(hcge_ctx, &hcge_ctx->state, state->accel);
			hcge_blit(hcge_ctx, &srect, 0, 0);

			FBDEV_DBG("timeout enter\n");
			hcge_engine_sync(hcge_ctx);
			FBDEV_DBG("timeout leave\n");			
		}

		/*hcge_engine_sync(hcge_ctx);*/


#if LV_USE_DUAL_FRAMEBUFFER 


		int ret;
		if(draw_fb == fbp0)
			vinfo.yoffset = 0;
		else
			vinfo.yoffset = vinfo.yres;
		ret = ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
		if (ret < 0) {
			perror("ioctl() / FBIOPAN_DISPLAY");
		}

		ret = 0;
		ioctl(fbfd, FBIO_WAITFORVSYNC, &ret);
		if (ret < 0) {
			perror("ioctl() / FBIO_WAITFORVSYNC");
		}
		state->destination.config.size.w = vinfo.xres;
		state->destination.config.size.h = vinfo.yres;
		state->dst.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy((draw_fb == fbp0)?fbp1:fbp0);
		state->dst.pitch = vinfo.xres * pixel_bytes;

		state->source.config.size.w = vinfo.xres;
		state->source.config.size.h = vinfo.yres;
		state->source.config.format = state->destination.config.format;
		state->src.pitch = vinfo.xres * pixel_bytes;
		state->src.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(draw_fb);
		srect.x = 0;
		srect.y = 0;
		srect.w = vinfo.xres;
		srect.h = vinfo.yres;
		state->blittingflags = 0;
		hcge_set_state(hcge_ctx, &hcge_ctx->state, state->accel);
		hcge_blit(hcge_ctx, &srect, 0, 0);
		draw_fb = (draw_fb == fbp0)?fbp1:fbp0;
		FBDEV_DBG("flip: vinfo.yoffset: %03d, draw_fb:%p, fbp0: %p, fbp1: %p\n", vinfo.yoffset,
				draw_fb, fbp0, fbp1);

#endif
		if(fb_onoff==0)
		{
			if (ioctl(fbfd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
				perror("ioctl(FBIOBLANK)");
				lv_draw_hichip_unlock();
				return;
			}
			fb_onoff = 1;
		}
	}
#if  LV_HC_KEYSTONE_AA_SW_FIX
	// only hor-lcd(w>h) without rotate 90/270 && 32bit ui can do keystone AA.
	if((vinfo.xres > vinfo.yres)&&(disp_rotate == 0 || disp_rotate==180)&& vinfo.bits_per_pixel==32){
		//printf("L1\n");
		keystone_aa_sw_fix(vinfo.xres,vinfo.yres,vinfo.xres,draw_fb);
	}
#endif
	lv_draw_hichip_unlock();

	lv_disp_flush_ready(drv);
}

#endif

void fbdev_get_sizes(uint32_t *width, uint32_t *height)
{
	if (width)
		*width = vinfo.xres;

	if (height)
		*height = vinfo.yres;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static uint32_t fbdev_buffer_start_virt;
static uint32_t fbdev_buffer_start_phy;
static uint32_t fbdev_buffer_off;
static uint32_t fbdev_buffer_size;
#define FBDEV_ALIGN16(addr) ((addr&0xF) ? (addr + 0xF)&0xF : (addr))
#define FBDEV_ALIGN32(addr) ((addr&0xF) ? (addr + 0xF)&0xF : (addr))
static void fbdev_buffer_init(uint32_t virt_addr, uint32_t phy_addr, uint32_t size)
{
	uint32_t addr = FBDEV_ALIGN32(virt_addr);
	int s;
	int round = addr - virt_addr;
	s = size - round;
	if(s < 0) {
		perror("Parameter error.\n");
		return;
	}
	if(addr != virt_addr && (phy_addr + round)&0x1F) {
		perror("physical address is not align with 16bytes\n");
		return;
	}

	fbdev_buffer_size = s;
	fbdev_buffer_start_phy = phy_addr + round;
	fbdev_buffer_start_virt = virt_addr + round;
	fbdev_buffer_off = 0;
	printf("fbdev_buffer_start_phy: 0x%08x, fbdev_buffer_start_virt: 0x%08x\n",
	       (int)fbdev_buffer_start_phy, (int)fbdev_buffer_start_virt);
}

void  *fbdev_buffer_addr_virt_to_phy(void *virt_addr)
{
	return (void *)(fbdev_buffer_start_phy + ((uint32_t)virt_addr - fbdev_buffer_start_virt));
}

bool fbdev_buffer_addr_virt_check(uint32_t virt_addr, uint32_t size)
{
	if(virt_addr < (uint32_t)fbp0 || (virt_addr + size) > ((uint32_t)fbp0 + finfo.smem_len)){
		FBDEV_DBG("virt_addr over range:addr = 0x%08x, size = 0x%08x\n", (int)virt_addr, (int)size);
		return false;
	}

	if(virt_addr & 0x1F){
		printf("virt_addr not align: 0x%08x\n", (int)virt_addr);
		return false;
	}

	return true;

}

void *fbdev_static_malloc_virt(int size)
{
	uint32_t addr;
	if(size & 0x1F) {
		perror("size is not align with 32bytes\n");
		return NULL;
	}

	if(fbdev_buffer_size < size) {
		perror("fbdev:not enough memory.\n");
		return NULL;
	}

	addr = fbdev_buffer_start_virt + fbdev_buffer_off;
	fbdev_buffer_off += size;
	fbdev_buffer_size -= size;

	FBDEV_DBG("return addr: 0x%08x, fbdev_buffer_off: %u\n", (int)addr, (int)fbdev_buffer_off);

	return (void *)addr;
}

uint32_t fbdev_get_buffer_size(void)
{
	return fbdev_buffer_size;
}

void fbdev_wait_cb(struct _lv_disp_drv_t * disp_drv)
{
#if 0
	int ret = 0;
	ioctl(fbfd, FBIO_WAITFORVSYNC, &ret);
	if (ret < 0) {
		perror("ioctl() / FBIO_WAITFORVSYNC");
	}
	FBDEV_DBG("%s\n", __func__);
#endif
}

void fbdev_set_offset(uint32_t xoffset, uint32_t yoffset) {
    vinfo.xoffset = xoffset;
    vinfo.yoffset = yoffset;
}

void fbdev_set_rotate(int rotate, int hor_flip, int ver_flip)
{
	lv_disp_t *disp = lv_disp_get_default();


	if((hor_flip || ver_flip || rotate == 180)&& !fbp_swap_buf){
		fbp_swap_buf = fbdev_static_malloc_virt(screen_size);
		if(!fbp_swap_buf){
			printf("Not enough memory\n");
		}
	}

	if(fbp_swap_buf){
		h_flip = hor_flip;
		v_flip = ver_flip;
	}

	disp_rotate = rotate;

	if(disp_rotate == 180){
		disp_rotate = 0;
		h_flip = (h_flip == 0)?1:0;
		v_flip = (v_flip == 0)?1:0;
	}

    lv_disp_drv_update(disp, disp->driver);
	FBDEV_DBG("%s:disp_rotate: %d, hor_flip: %d, ver_flip: %d\n", __func__, disp_rotate, h_flip, v_flip);
}

#endif
#endif
