#ifndef _HCFB_H
#define _HCFB_H

#include <kernel/fb.h>
#include <kernel/types.h>
#include <kernel/wait.h>
#include <hcuapi/fb.h>

#define __FB_DEBUG__ 1

#if __FB_DEBUG__
#define fb_err(info, fmt, ...) \
        printf(fmt, ##__VA_ARGS__)
#else
#define fb_err(info, fmt, ...)                                                 \
	do {                                                                   \
	} while (0)
#endif

typedef struct {
	/* Dword0 */
	uint32_t is_last_block : 1;	/* bit[0] */
	uint32_t alpha_closed : 1;	/* bit[1] */

#define SCALE_OFF (0)
#define SCALE_ON  (1)
#define SCALE_DIR(dir) (!!(dir))
	uint32_t scale_on : 1;		/* bit[2] */

#define SCALE_EP_OFF (0)
#define SCALE_EP_ON  (1)
	uint32_t edge_preserve_on : 1;	/* bit[3] */

	uint32_t gma_mode : 4;		/* bit[7:4] see HCFB_FMT_* */
	uint32_t color_by_color : 1;	/* bit[8] */

#define CLUT_MODE_PIO (0)
#define CLUT_MODE_DMA (1)
	uint32_t clut_mode : 1;		/* bit[9] */

	uint32_t clut_update : 1;	/* bit[10] */
	uint32_t pre_multiply : 1;	/* bit[11] */

	uint32_t csc_mode : 1;		/* bit[12] */

	uint32_t rsved3 : 3;		/* bit[15:13] */
	uint32_t edge_preserve_reduce_thr : 7;	/* bit[22:16], value[0,127], default 32 */
	uint32_t rsved4 : 1;		/* bit[23] */
	uint32_t edge_preserve_avg_thr : 8;	/* bit[31:24], value[0,255], default 170 */

	/* Dword1 */
	uint32_t clut_base;		/* bit[31:5], CLUT memory base address. 32Byte aligned */

	/* Dword2 */
	uint32_t sx : 12;		/* bit[11:0], start x position on screen */
	uint32_t rsved5 : 4;		/* bit[15:12] */
	uint32_t ex : 13;		/* bit[28:16], end x position on screen */
	uint32_t rsved6 : 3;		/* bit[31:29] */

	/* Dword3 */
	uint32_t sy : 12;		/* bit[10:0], start y position on screen */
	uint32_t rsved7 : 4;		/* bit[15:11] */
	uint32_t ey : 12;		/* bit[26:16], end y position on screen */
	uint32_t rsved8 : 4;		/* bit[31:27] */

	/* Dword4 */
	uint32_t src_w: 12;		/* bit[11:0], source width in pixel */
	uint32_t rsved9 : 4;		/* bit[15:11] */
	uint32_t src_h: 12;		/* bit[10:0], source height in pixel */
	uint32_t rsved10 : 4;		/* bit[31:27] */

	/* Dword5 */
	uint32_t global_alpha : 8;	/* bit[7:0] */
	uint32_t clut_segment : 4;	/* bit[11:8], color palette segment selection */
	uint32_t rgb_order : 2;		/* bit[13:12], both for CLUT and true color mode */
	uint32_t rsved11 : 2;		/* bit[15:14] */
	uint32_t pitch : 14;		/* bit[29:16], the number of bytes between the conjoint pixel */
	uint32_t rsved12 : 2; // 31:30

	/* Dword6 */
	uint32_t next;			/* bit[31:5], address of next block header, 32Byte aligned */

	/* Dword7 */
	uint32_t bitmap_addr;		/* bit[31:0], aligned in 1 pixel */

	/* Dword8 */
#define SCALE_MODE_FILTER    (0)
#define SCALE_MODE_DUPLICATE (1)
	uint32_t scale_mode : 1;	/* default: SCALE_MODE_FILTER */
	uint32_t rsved13 : 2;

#define FILTER_SELECT_TAP_2 (0)
#define FILTER_SELECT_TAP_3 (1)
	uint32_t filter_select : 1;
	uint32_t rsved14 : 28;

	/* Dword9 */
	uint32_t inc_h_frag : 12;	/* bit[11:0], horizontal scale increment frament part */
	uint32_t inc_h_int : 4;		/* bit[15:12], horizontal scale increment integer part */
	uint32_t inc_v_frag : 12;	/* bit[27:16], vertical scale increment frament part */
	uint32_t inc_v_int : 4;		/* bit[31:28], vertical scale increment integer part */
} gma_header_t;

#define NDWORDS_OF_BLOCK		((8 + 32 + 32 + 8)*2)
#define SIZE_OF_BLOCK			(sizeof(uint32_t) * NDWORDS_OF_BLOCK)
typedef union {
	gma_header_t header;
	uint32_t buf[NDWORDS_OF_BLOCK];
} gma_dmba_block_t;

struct hcfb_device {
	int open_cnt;
	int hwinited;
	void *iobase;

	unsigned char *fb_mem;
	size_t fb_len;
	unsigned long fb_phys;
	uint32_t off_pitch;

	uint32_t fb_static_phys;
	uint32_t fb_static_len;

#define HCFB_BUFFER_SOURCE_SYSTEM 0
#define HCFB_BUFFER_SOURCE_MMZ0   1
#define HCFB_BUFFER_SOURCE_NONE   2
#define HCFB_BUFFER_SOURCE_STATIC_ALLOC 3
	int buffer_source;

	wait_queue_head_t wait;
	int vblank_ntkey;
	unsigned int vblank_count;

	hcfb_enhance_t enhance;

	int enhance_coef[13];
	short filter_coef[4*64];

	uint8_t clut_segment;

	uint8_t clut_mode;
	uint8_t filter_select;
	uint8_t scale_mode;
	uint8_t scale_dir;
	uint8_t scale_ep_on;
	uint16_t h_scale;
	uint16_t v_scale;
	uint16_t h_div;
	uint16_t v_div;
	uint16_t h_mul;
	uint16_t v_mul;
	uint16_t left;
	uint16_t top;
	uint16_t sx;
	uint16_t ex;
	uint16_t sy;
	uint16_t ey;
	uint16_t src_w;
	uint16_t src_h;
	uint16_t green_length;
	uint16_t transp_length;
	uint16_t pitch;

	uint8_t inlineblock[2*SIZE_OF_BLOCK + 32];
	gma_dmba_block_t *block[2];
	uint8_t sw_block_id;

	hcfb_color_fmt_e color_format;
	uint16_t bits_per_pixel;

	uint8_t *palette_alloc;
	uint8_t *palette_hw;
	uint8_t *palette_sw;
	hcfb_palette_type_e palette_type_hw;
	hcfb_palette_type_e palette_type_sw;
	uint8_t palette_rgb_order_sw;
	uint8_t palette_alpha_range_sw;
	uint16_t palette_ncolors_sw;

	uint8_t global_alpha_on;
	uint8_t global_alpha;
	uint8_t pre_multiply;
	size_t extra_fb_len;
	hcfb_mmap_cache_e mmap_cache;
	bool gma_onoff;
};

#endif
