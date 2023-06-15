#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "app_config.h"


static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+4EAE "亮" */
    0x1, 0x0, 0x6a, 0xa4, 0x19, 0x60, 0x19, 0x60,
    0xaa, 0xa8, 0x45, 0x84, 0x8, 0x80, 0x24, 0xa8,
    0x0, 0x0,

    /* U+4F4E "低" */
    0x0, 0x0, 0x16, 0xa4, 0x32, 0x20, 0x73, 0xb8,
    0x22, 0x20, 0x22, 0x20, 0x23, 0x95, 0x21, 0x49,

    /* U+5BF9 "对" */
    0x0, 0x8, 0x28, 0x8, 0x6, 0xad, 0x24, 0x8,
    0x1c, 0x88, 0x18, 0x48, 0x22, 0x8, 0x0, 0x24,

    /* U+5E73 "平" */
    0x0, 0x0, 0x1a, 0xe8, 0x4, 0x88, 0x8, 0xa0,
    0x2a, 0xe9, 0x0, 0x80, 0x0, 0x80, 0x0, 0x80,

    /* U+5EA6 "度" */
    0x0, 0x0, 0x2a, 0xa8, 0x21, 0x20, 0x27, 0x74,
    0x22, 0x60, 0x27, 0x70, 0x51, 0x90, 0x4a, 0x59,

    /* U+6670 "晰" */
    0x0, 0x0, 0x66, 0x29, 0x87, 0x60, 0xa7, 0x29,
    0x87, 0xa4, 0xaa, 0x24, 0x82, 0x54, 0x2, 0x84,
    0x0, 0x0,

    /* U+6BD4 "比" */
    0x0, 0x80, 0x20, 0x80, 0x20, 0x88, 0x39, 0xd0,
    0x20, 0x80, 0x21, 0x80, 0x39, 0x85, 0x10, 0x68,

    /* U+6E05 "清" */
    0x0, 0x50, 0x26, 0xa8, 0x1, 0xa4, 0x25, 0xa5,
    0x2, 0x58, 0x13, 0x58, 0x23, 0x58, 0x42, 0x8,
    0x0, 0x0,

    /* U+8272 "色" */
    0x0, 0x0, 0x3a, 0x2, 0x8, 0x1a, 0xec, 0x27,
    0x70, 0x80, 0x42, 0x0, 0x6, 0xaa,

    /* U+8861 "衡" */
    0x1, 0x0, 0x26, 0x84, 0x56, 0x90, 0x26, 0xad,
    0x62, 0x98, 0x26, 0x98, 0x22, 0x88, 0x28, 0x1c,
    0x0, 0x0,

    /* U+97F3 "音" */
    0x0, 0x0, 0x16, 0x94, 0x8, 0x20, 0x1a, 0x65,
    0xa, 0xa4, 0x9, 0x58, 0x8, 0x18, 0x9, 0x68,
    0x0, 0x0,

    /* U+989C "颜" */
    0x0, 0x0, 0x19, 0x78, 0x14, 0x64, 0x29, 0x54,
    0x28, 0x64, 0x19, 0x64, 0x45, 0x58, 0x41, 0x81,

    /* U+9AD8 "高" */
    0x0, 0x0, 0x2a, 0xa9, 0x9, 0x64, 0x9, 0x64,
    0x25, 0x55, 0x22, 0x62, 0x22, 0x62, 0x20, 0x9,
    0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 18, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 34, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 50, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 66, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 82, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 100, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 116, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 134, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 148, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 166, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 184, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 200, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xa0, 0xd4b, 0xfc5, 0xff8, 0x17c2, 0x1d26, 0x1f57,
    0x33c4, 0x39b3, 0x4945, 0x49ee, 0x4c2a
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 20142, .range_length = 19499, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 13, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 2,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t font_china_8 = {
#else
lv_font_t font_china_8 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 9,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



