#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <hcuapi/dis.h>
#include <sys/ioctl.h>
#include "lv_hichip_conf.h"

#if LV_HC_KEYSTONE_AA_SW_FIX

#define NBIT_FRAC_KS_SUBPIX 18

static void calc_keystone_side_delta(int top_width, int bottom_width, int nline_osd,
							  int *left_delta, int *right_delta);
static void keystone_overlay_left_edge_argb8888(
		int y_start, int y_end,
		int delta, uint32_t bg_color,
		const uint32_t *last_edge,
		int stride, uint32_t *osd_buf);

static void keystone_overlay_right_edge_argb8888(
		int y_start, int y_end,
		int delta, uint32_t bg_color,
		const uint32_t *last_edge,
		int osd_width, int stride, uint32_t *osd_buf);
/**
 * @brief sw AA of keystone correction
 *
 * @param [out] the frame buffer to do keystone aa corrections
 */
void keystone_aa_sw_fix(int osd_width, int osd_height, int osd_stride, uint32_t *osd_argb8888);

static inline int calc_delta(int offset_diff, int nline) {
	return ((offset_diff << (NBIT_FRAC_KS_SUBPIX + 1)) / nline) >> 1;
}

/**
 * @brief Calculate side spans of keystone corrections
 *
 * @param top_width The top base length of trapezoid (pixels)
 * @param bottom_width The bottom base length of trapezoid (pixels)
 * @param nline_osd Number of lines in OSD layer. It might be scaled to display size.
 * @param [out] left_delta Weight delta to fix left edge
 * @param [out] right_delta Weight delta to fix right edge
 *
 * @remarks As only to blend active video with background color, sub-pixel
 *          position is not important. Delta could be non-directional.
 */
void calc_keystone_side_delta(int top_width, int bottom_width, int nline_osd,
							  int *left_delta, int *right_delta) {
	/* left & right span of keystone correction
			|<->|___________|<->|
				/			\
			   /			 \
			  / 			  \
			 /_________________\
	 */
	int init_offset, last_offset;
	if (top_width < bottom_width) {
		init_offset = (bottom_width - top_width) >> 1;
		last_offset = 0;
	} else {
		init_offset = 0;
		last_offset = (top_width - bottom_width) >> 1;
	}
	*left_delta = -calc_delta(init_offset - last_offset, nline_osd);
	*right_delta = calc_delta(init_offset - last_offset, nline_osd);
}

static inline
uint32_t mix_premult(uint32_t c_osd, uint32_t c_edge, uint32_t beta_q8) {
	// uint32_t c = c_osd * a_osd_q8 * (256 - beta_q8) + (c_edge * beta_q8 << 8);
	uint32_t c = c_osd * (256 - beta_q8) + c_edge * beta_q8;
	return c >> 8;
}

static inline uint32_t decode_alpha8(uint32_t alpha) {
	return alpha + (alpha != 0);
}

static inline uint32_t encode_alpha8(uint32_t alpha) {
	return alpha - (alpha != 0);
}

static inline
uint32_t compose_edge(uint32_t c_osd,
		uint32_t c0_bg, uint32_t c1_bg, uint32_t c2_bg, uint32_t beta) {
	const uint32_t c0_osd =  c_osd        & 0xff;
	const uint32_t c1_osd = (c_osd >>  8) & 0xff;
	const uint32_t c2_osd = (c_osd >> 16) & 0xff;
	const uint32_t c0 = mix_premult(c0_osd, c0_bg, beta);
	const uint32_t c1 = mix_premult(c1_osd, c1_bg, beta);
	const uint32_t c2 = mix_premult(c2_osd, c2_bg, beta);
	return (c2 << 16) | (c1 << 8) | c0;
}

/**
 * @brief Fix keystone left jaggy edge by overlaying a smooth version.
 * @remark Fix up to three pixels each line.
 *
 * @param y_start start of line number, inclusive
 * @param y_end end of line, exclusive
 * @param delta left edge delta, from calc_keystone_side_delta()
 * @param bg_color keystone background color
 * @param last_edge external backup of source edge pixel, a walk-around solution, to be removed
 * @param stride stride of OSD buffer
 * @param osd_buf pointer to OSD buffer
 */
static void keystone_overlay_left_edge_argb8888(
		int y_start, int y_end,
		int delta, uint32_t bg_color,
		const uint32_t *last_edge,
		int stride, uint32_t *osd_buf) {
	const int nbit_shift = NBIT_FRAC_KS_SUBPIX - 8; // 8-bit beta
	// assuming A[31:24] | COLOR[23:0]
	const uint32_t c0_bg =  bg_color        & 0xff;
	const uint32_t c1_bg = (bg_color >>  8) & 0xff;
	const uint32_t c2_bg = (bg_color >> 16) & 0xff;
	int x = delta * y_start;
	uint32_t *pixel = osd_buf + stride * y_start;
	if (last_edge)
		last_edge += y_start;
	for (int y = y_start; y < y_end; ++y) {
		const uint32_t c_osd = last_edge ? *last_edge++ : pixel[1];
		const uint32_t a_osd = decode_alpha8((c_osd >> 24) & 0xff);
		const uint32_t beta = (x >> nbit_shift) & 255;
		const uint32_t a1_q16 = 65536 - (256 - a_osd) * (256 - beta);
		const uint32_t new_color = compose_edge(c_osd, c0_bg, c1_bg, c2_bg, beta);
		const uint32_t alpha = encode_alpha8(a1_q16 >> 8);
		const uint32_t new_pixel = (alpha << 24) | new_color;
		pixel[0] = (0xff << 24) | bg_color;
		if (delta > 0 && beta == 0) {
			pixel[1] = (0xff << 24) | bg_color;
			pixel[2] = new_pixel;
		} else {
			pixel[1] = new_pixel;
		}
		x += delta;
		pixel += stride;
	}
}

/**
 * @brief Fix keystone right jaggy edge by overlaying a smooth version.
 * @remark Fix up to four pixels each line.
 *
 * @param y_start start of line number, inclusive
 * @param y_end end of line, exclusive
 * @param delta right edge delta, from calc_keystone_side_delta()
 * @param bg_color keystone background color
 * @param last_edge external backup of source edge pixel, a walk-around solution, to be removed
 * @param osd_width OSD width, in pixels
 * @param stride stride of OSD buffer
 * @param osd_buf pointer to OSD buffer
 */
static void keystone_overlay_right_edge_argb8888(
		int y_start, int y_end,
		int delta, uint32_t bg_color,
		const uint32_t *last_edge,
		int osd_width, int stride, uint32_t *osd_buf) {
	const int nbit_shift = NBIT_FRAC_KS_SUBPIX - 8; // 8-bit beta
	// assuming A[31:24] | COLOR[23:0]
	const uint32_t c0_bg =  bg_color        & 0xff;
	const uint32_t c1_bg = (bg_color >>  8) & 0xff;
	const uint32_t c2_bg = (bg_color >> 16) & 0xff;
	int x = delta * y_start;
	uint32_t *pixel = osd_buf + osd_width - 1 + stride * y_start;
	if (last_edge)
		last_edge += y_start;
	for (int y = y_start; y < y_end; ++y) {
		const uint32_t c_osd = last_edge ? *last_edge++ : pixel[-1];
		const uint32_t a_osd = decode_alpha8((c_osd >> 24) & 0xff);
		const uint32_t beta = 256 - ((x >> nbit_shift) & 255);
		const uint32_t a1_q16 = 65536 - (256 - a_osd) * (256 - beta);
		const uint32_t new_color = compose_edge(c_osd, c0_bg, c1_bg, c2_bg, beta);
		const uint32_t alpha = encode_alpha8(a1_q16 >> 8);
		const uint32_t new_pixel = (alpha << 24) | new_color;
		if (delta < 0 && beta == 1) {
			pixel[-3] = new_pixel;
			pixel[-2] = (0xff << 24) | bg_color;
		} else {
			pixel[-2] = new_pixel;
		}
		pixel[-1] = (0xff << 24) | bg_color;
		pixel[ 0] = (0xff << 24) | bg_color;
		x += delta;
		pixel += stride;
	}
}

static uint32_t *last_left_edge=NULL;
static uint32_t *last_right_edge=NULL;
static uint8_t keystone_reset_flag = 1;

static void sample_code_argb8888(int top_width, int bottom_width,
						  uint32_t keystone_bg_color,
						  int osd_width, int osd_height, int osd_stride,
						  uint32_t *osd_argb8888) {
	int left_delta, right_delta;
	//static uint32_t count_c_osd_bak = 0;
	static int last_top_width = -1;
	static int last_bottom_width = -1;
	static int last_osd_height = -1;
	/* following steps are required whenever keystone setting changes */
	const int force_refraw = last_top_width != top_width ||
							 last_bottom_width != bottom_width ||
							 last_osd_height != osd_height;
	calc_keystone_side_delta(top_width, bottom_width, osd_height,
							 &left_delta, &right_delta);

	/* fill left edge */
	/*printf("%s %d \\__/ osd_fb=%p, KS width=(%d, %d), OSD %dx%d; delta=(%d, %d)\n",
		__FUNCTION__, __LINE__,
		osd_argb8888,
		top_width, bottom_width,
		osd_width, osd_height,
		left_delta, right_delta);*/
	/* TODO find a better solution!!! */
	#define SIGNATURE 0xff010102
//	static uint32_t last_left_edge[720];
//	static uint32_t last_right_edge[720];
	uint32_t *top_left = osd_argb8888;
	uint32_t *top_right = osd_argb8888 + osd_width - 1;
	uint32_t *bottom_left = osd_argb8888 + osd_stride * (osd_height - 1);
	uint32_t *bottom_right = bottom_left + osd_width - 1;

	if (*top_left != SIGNATURE || *bottom_left != SIGNATURE) {
		const uint32_t *p = osd_argb8888;
		for (int i = 0; i < osd_height; ++i) {
			last_left_edge[i] = p[1];
			p += osd_stride;
		}
	}
	if (*top_right != SIGNATURE || *bottom_right != SIGNATURE) {
		const uint32_t *p = osd_argb8888;
		for (int i = 0; i < osd_height; ++i) {
			last_right_edge[i] = p[osd_width - 1 - 2]; // see right fixing
			p += osd_stride;
		}
	}
	/* fix left and right edges */
	if (*top_left != SIGNATURE || *bottom_left != SIGNATURE || force_refraw) {
		keystone_overlay_left_edge_argb8888(
			0, osd_height, left_delta, keystone_bg_color,
			last_left_edge, osd_stride, osd_argb8888);
		*top_left = SIGNATURE;
		*bottom_left = SIGNATURE;
	}
	if (*top_right != SIGNATURE || *bottom_right != SIGNATURE || force_refraw) {
		keystone_overlay_right_edge_argb8888(
			0, osd_height, right_delta, keystone_bg_color,
			last_right_edge, osd_width, osd_stride, osd_argb8888);
		*top_right = SIGNATURE;
		*bottom_right = SIGNATURE;
	}
	last_top_width = top_width;
	last_bottom_width = bottom_width;
	last_osd_height = osd_height;
}


static int get_keystone_top_bottom_width(uint32_t *top_w, uint32_t *bot_w, uint8_t *enable)
{
	struct dis_keystone_param pparm = { 0 };
	int ret = -1;
	int fd = open("/dev/dis" , O_RDWR);
	if(fd < 0) {
		return ret;
	}
	pparm.distype = DIS_TYPE_HD;
	ioctl(fd , DIS_GET_KEYSTONE_PARAM , &pparm);
	if(pparm.info.enable){
		*top_w = pparm.info.width_up;
		*bot_w = pparm.info.width_down;
		*enable = pparm.info.enable;
		ret = 0;
	}

	//printf(">> T: %d , B: %d , enable: %d\n", pparm.info.width_up, pparm.info.width_down, pparm.info.enable);
	close(fd);
	return ret;
}

static void recover_lr_edge(int osd_width, int osd_height, int osd_stride,uint32_t *osd_argb8888)
{
	//printf("%s\n", __FUNCTION__);
	uint32_t *p = osd_argb8888;
	for (int i = 0; i < osd_height; ++i) {
		p[1] = last_left_edge[i];
		p[osd_width - 1 - 2] = last_right_edge[i];
		p += osd_stride;
	}
}
//keystone anti-aliasing sw fix
void keystone_aa_sw_fix(int osd_width, int osd_height, int osd_stride, uint32_t *osd_argb8888)
{
	uint32_t top_width = 0;
 	uint32_t bottom_width = 0;
	uint8_t ks_enable = 0;
	
	if(get_keystone_top_bottom_width(&top_width, &bottom_width,&ks_enable) == 0){
		if(ks_enable){
			if(last_left_edge == NULL)
				last_left_edge = (uint32_t *)malloc(osd_height*sizeof(int));
			if(last_right_edge == NULL)
				last_right_edge = (uint32_t *)malloc(osd_height*sizeof(int));
			if(last_right_edge==NULL||last_left_edge==NULL)
				return ;
		
			if(top_width!=bottom_width){
				sample_code_argb8888(top_width, bottom_width,0,osd_width,osd_height,osd_width,osd_argb8888);
				keystone_reset_flag = 0;
			}
			else if(!keystone_reset_flag){// top == bottom, recover
				recover_lr_edge( osd_width,  osd_height,  osd_stride, osd_argb8888);
				keystone_reset_flag = 1;
			}
		}
	}
}

#endif

