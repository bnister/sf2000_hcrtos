#ifndef _HCUAPI_HCFB_H_
#define _HCUAPI_HCFB_H_

#include <hcuapi/iocbase.h>

#define HCFB_MAX_LAYER_NUM 2

#define HCFBIOSET_SCALE			_IOW(HCFB_IOCBASE, 0, hcfb_scale_t)
#define HCFBIOSET_ENHANCE		_IOW(HCFB_IOCBASE, 1, hcfb_enhance_t)
#define HCFBIOGET_ENHANCE		_IOR(HCFB_IOCBASE, 2, hcfb_enhance_t)
#define HCFBIOSET_ENHANCE_COEF		_IOC(_IOC_WRITE, HCFB_IOCBASE, 3, 13*sizeof(int))
#define HCFBIOSET_MMAP_CACHE 		_IO (HCFB_IOCBASE, 4)
#define HCFBIOSET_SET_LEFTTOP_POS	_IOW(HCFB_IOCBASE, 5, hcfb_lefttop_pos_t)

typedef enum HCFB_COLOR_FMT {
	HCFB_FMT_RGB888 = 0x00,
	HCFB_FMT_ARGB8888 = 0x01,
	HCFB_FMT_RGB444 = 0x02,
	HCFB_FMT_ARGB4444 = 0x03,
	HCFB_FMT_RGB555 = 0x04,
	HCFB_FMT_ARGB1555 = 0x05,
	HCFB_FMT_RGB565 = 0x06,
	HCFB_FMT_RESERVED0 = 0x07,
	HCFB_FMT_CLUT2 = 0x08,
	HCFB_FMT_RESERVED1 = 0x09,
	HCFB_FMT_CLUT4 = 0x0a,
	HCFB_FMT_ACLUT44 = 0x0b,
	HCFB_FMT_CLUT8 = 0x0c,
	HCFB_FMT_RESERVED2 = 0x0d,
	HCFB_FMT_ACLUT88 = 0x0e,
	HCFB_FMT_RESERVED3 = 0x0f,

	HCFB_FMT_YCbCr444 = 0x10,
	HCFB_FMT_YCbCr422 = 0x11,
	HCFB_FMT_YCbCr420 = 0x12,
	HCFB_FMT_AYCbCr8888 = 0x13,

	HCFB_FMT_MASK_A1 = 0x1c,    // For 1bpp MASK
	HCFB_FMT_MASK_A8 = 0x1d,    // For 8bpp MASK

	HCFB_FMT_MAX_VALUE
} hcfb_color_fmt_e;

typedef enum HCFB_PALETTE_TYPE {
	HCFB_PALETTE_RGB = 0x00,
	HCFB_PALETTE_YCBCR = 0x01,
} hcfb_palette_type_e;

typedef enum hcfb_rgb_order {
	HCFB_RGB_ORDER_ARGB,
	HCFB_RGB_ORDER_ABGR,
	HCFB_RGB_ORDER_RGBA,
	HCFB_RGB_ORDER_BGRA,

	HCFB_RGB_ORDER_AYCbCr = HCFB_RGB_ORDER_ARGB,
	HCFB_RGB_ORDER_ACrCbY = HCFB_RGB_ORDER_ABGR,
	HCFB_RGB_ORDER_YCbCrA = HCFB_RGB_ORDER_RGBA,
	HCFB_RGB_ORDER_CrCbYA = HCFB_RGB_ORDER_BGRA,
} hcfb_rgb_order_e;

typedef enum HCFB_ALPHA_RANGE {
	HCFB_ALPHA_RANGE_255 = 0x00,
	HCFB_ALPHA_RANGE_127 = 0x01,
	HCFB_ALPHA_RANGE_15 = 0x02,
} hcfb_alpha_range_e;

typedef enum HCFB_ALPHA_POLARITY {
	HCFB_ALPHA_POLARITY_0 = 0x00,	/* 0: transparent, 1: non-transparent */
	HCFB_ALPHA_POLARITY_1 = 0x01,	/* 0: non-transparent, 1: transparent */
} hcfb_alpha_polarity_e;

typedef enum HCFB_ENHANCE_CSCMODE {
	HCFB_ENHANCE_CSCMODE_BT601 = 0x00,
	HCFB_ENHANCE_CSCMODE_BT709 = 0x01,
} hcfb_enhance_cscmode_e;

typedef enum HCFB_MMAP_CACHE {
	HCFB_MMAP_NO_CACHE,
	HCFB_MMAP_CACHE,
} hcfb_mmap_cache_e;

typedef struct hcfb_scale {
	uint16_t h_div;
	uint16_t v_div;
	uint16_t h_mul;
	uint16_t v_mul;
} hcfb_scale_t;

typedef struct hcfb_enhance {
	int brightness;
	int contrast;
	int saturation;
	int hue;
	int sharpness;
	hcfb_enhance_cscmode_e cscmode;
} hcfb_enhance_t;

typedef struct hcfb_palette {
	uint8_t palette_type;	/* See HCFB_PALETTE_* */
	uint8_t rgb_order;	/* See HCFB_RGB_ORDER_* */
	uint8_t alpha_range;	/* See HCFB_ALPHA_RANGE_* */
	uint8_t alpha_polarity;	/* See HCFB_ALPHA_POLARITY_* */
	uint16_t ncolors;
	uint8_t *palette;
} hcfb_palette_t;

typedef struct hcfb_lefttop_pos {
	uint16_t left;
	uint16_t top;
} hcfb_lefttop_pos_t;

#endif	/* _HCUAPI_HCFB_H_ */
