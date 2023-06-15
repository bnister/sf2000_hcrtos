#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_BT_UNFOCUS
#define LV_ATTRIBUTE_IMG_BT_UNFOCUS
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_BT_UNFOCUS uint8_t bt_unfocus_map[] = {
#if LV_COLOR_DEPTH == 1 || LV_COLOR_DEPTH == 8
  /*Pixel format: Red: 3 bit, Green: 3 bit, Blue: 2 bit*/
  0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 
  0xe3, 0xe3, 0xe3, 0x49, 0x69, 0x69, 0xe3, 0xe3, 0xe3, 0xe3, 
  0xe3, 0xe3, 0xe3, 0x49, 0x49, 0x69, 0x49, 0xe3, 0xe3, 0xe3, 
  0xe3, 0x6a, 0x49, 0x69, 0x45, 0x6a, 0x6d, 0x49, 0xe3, 0xe3, 
  0xe3, 0xe3, 0x6d, 0x6d, 0x6d, 0x69, 0x45, 0xe3, 0xe3, 0xe3, 
  0xe3, 0x45, 0x6d, 0x6d, 0x6d, 0x49, 0xe3, 0xe3, 0xe3, 0xe3, 
  0xe3, 0x49, 0x45, 0x69, 0x69, 0x69, 0x6d, 0x45, 0xe3, 0xe3, 
  0xe3, 0xe3, 0xe3, 0x49, 0xe3, 0x49, 0x6d, 0x45, 0xe3, 0xe3, 
  0xe3, 0xe3, 0xe3, 0x49, 0x6d, 0x49, 0xe3, 0xe3, 0xe3, 0xe3, 
  0xe3, 0xe3, 0xe3, 0x6a, 0x65, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 
#endif
#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0
  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit*/
  0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x2a, 0x52, 0x6b, 0x5a, 0x8b, 0x59, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x29, 0x4a, 0xe8, 0x41, 0x2b, 0x5a, 0x69, 0x4a, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x4d, 0x6a, 0xc8, 0x41, 0x4a, 0x52, 0x47, 0x39, 0x8e, 0x71, 0xca, 0x52, 0xc7, 0x39, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x1f, 0xf8, 0xab, 0x5a, 0xab, 0x5a, 0xab, 0x5a, 0x4a, 0x52, 0x29, 0x49, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x48, 0x41, 0x8c, 0x62, 0xab, 0x5a, 0xab, 0x5a, 0x09, 0x4a, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x0a, 0x52, 0x8a, 0x51, 0x4a, 0x52, 0xea, 0x51, 0x2a, 0x52, 0xab, 0x5a, 0x29, 0x49, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x29, 0x4a, 0x1f, 0xf8, 0xaa, 0x51, 0xab, 0x5a, 0x67, 0x39, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x29, 0x4a, 0x8c, 0x62, 0x48, 0x42, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
  0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0xae, 0x71, 0x8b, 0x59, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 
#endif
#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP != 0
  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit BUT the 2 bytes are swapped*/
  0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0x52, 0x2a, 0x5a, 0x6b, 0x59, 0x8b, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0x4a, 0x29, 0x41, 0xe8, 0x5a, 0x2b, 0x4a, 0x69, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0x6a, 0x4d, 0x41, 0xc8, 0x52, 0x4a, 0x39, 0x47, 0x71, 0x8e, 0x52, 0xca, 0x39, 0xc7, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0xf8, 0x1f, 0x5a, 0xab, 0x5a, 0xab, 0x5a, 0xab, 0x52, 0x4a, 0x49, 0x29, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0x41, 0x48, 0x62, 0x8c, 0x5a, 0xab, 0x5a, 0xab, 0x4a, 0x09, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0x52, 0x0a, 0x51, 0x8a, 0x52, 0x4a, 0x51, 0xea, 0x52, 0x2a, 0x5a, 0xab, 0x49, 0x29, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0x4a, 0x29, 0xf8, 0x1f, 0x51, 0xaa, 0x5a, 0xab, 0x39, 0x67, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0x4a, 0x29, 0x62, 0x8c, 0x42, 0x48, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 
  0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0x71, 0xae, 0x59, 0x8b, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 
#endif
#if LV_COLOR_DEPTH == 32
  /*Pixel format: Fix 0xFF: 8 bit, Red: 8 bit, Green: 8 bit, Blue: 8 bit*/
  0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x4f, 0x42, 0x4f, 0xff, 0x55, 0x4d, 0x55, 0xff, 0x57, 0x30, 0x57, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x4a, 0x45, 0x4a, 0xff, 0x3c, 0x3a, 0x3c, 0xff, 0x57, 0x42, 0x57, 0xff, 0x49, 0x4d, 0x49, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0x67, 0x48, 0x67, 0xff, 0x3e, 0x36, 0x3e, 0xff, 0x52, 0x48, 0x52, 0xff, 0x39, 0x26, 0x39, 0xff, 0x6c, 0x31, 0x6c, 0xff, 0x53, 0x56, 0x53, 0xff, 0x39, 0x38, 0x39, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x5b, 0x52, 0x5b, 0xff, 0x55, 0x55, 0x55, 0xff, 0x59, 0x53, 0x59, 0xff, 0x53, 0x48, 0x53, 0xff, 0x45, 0x24, 0x45, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0x41, 0x26, 0x41, 0xff, 0x5c, 0x51, 0x5c, 0xff, 0x55, 0x55, 0x55, 0xff, 0x57, 0x54, 0x57, 0xff, 0x4a, 0x3e, 0x4a, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0x4e, 0x40, 0x4e, 0xff, 0x4c, 0x2f, 0x4c, 0xff, 0x52, 0x48, 0x52, 0xff, 0x50, 0x3b, 0x50, 0xff, 0x50, 0x42, 0x50, 0xff, 0x5b, 0x52, 0x5b, 0xff, 0x47, 0x23, 0x47, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x4a, 0x45, 0x4a, 0xff, 0xff, 0x00, 0xff, 0xff, 0x4e, 0x35, 0x4e, 0xff, 0x56, 0x54, 0x56, 0xff, 0x38, 0x2b, 0x38, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x4a, 0x45, 0x4a, 0xff, 0x5d, 0x51, 0x5d, 0xff, 0x43, 0x49, 0x43, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
  0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x6d, 0x33, 0x6d, 0xff, 0x5b, 0x2e, 0x5b, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 
#endif
};

const lv_img_dsc_t bt_unfocus = {
  .header.cf = LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 10,
  .header.h = 10,
  .data_size = 100 * LV_COLOR_SIZE / 8,
  .data = bt_unfocus_map,
};