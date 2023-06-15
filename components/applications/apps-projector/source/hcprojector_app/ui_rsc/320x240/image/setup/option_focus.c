#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_MAINMENU_IMG_OPTION_FOCUS1
#define LV_ATTRIBUTE_IMG_MAINMENU_IMG_OPTION_FOCUS1
#endif


const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_MAINMENU_IMG_OPTION_FOCUS1 uint8_t MAINMENU_IMG_OPTION_FOCUS_map[] = {
#if LV_COLOR_DEPTH == 1 || LV_COLOR_DEPTH == 8
  /*Pixel format: Red: 3 bit, Green: 3 bit, Blue: 2 bit*/
  0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 
  0x25, 0x25, 0x49, 0x49, 0x49, 0x49, 0x25, 0x49, 0x49, 0x49, 0x49, 0x25, 0x25, 
  0x25, 0x49, 0xdb, 0xff, 0xff, 0xff, 0x49, 0xdb, 0xff, 0xff, 0xdb, 0x49, 0x25, 
  0x25, 0x49, 0xff, 0xff, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff, 0xff, 0x6d, 0x25, 
  0x25, 0x49, 0xff, 0xff, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff, 0xff, 0x6d, 0x25, 
  0x25, 0x49, 0xff, 0xff, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff, 0xff, 0x6d, 0x25, 
  0x25, 0x25, 0x49, 0x49, 0x49, 0x49, 0x25, 0x49, 0x49, 0x49, 0x49, 0x25, 0x25, 
  0x24, 0x49, 0xdb, 0xff, 0xff, 0xff, 0x49, 0xdb, 0xff, 0xff, 0xdb, 0x49, 0x24, 
  0x24, 0x49, 0xff, 0xff, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff, 0xff, 0x6d, 0x24, 
  0x24, 0x49, 0xff, 0xff, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff, 0xff, 0x6d, 0x24, 
  0x24, 0x49, 0xdb, 0xff, 0xff, 0xff, 0x49, 0xdb, 0xff, 0xff, 0xff, 0x49, 0x24, 
  0x24, 0x25, 0x49, 0x6d, 0x6d, 0x6d, 0x25, 0x49, 0x6d, 0x6d, 0x49, 0x25, 0x24, 
  0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
#endif
#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0
  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit*/
  0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 
  0x25, 0x29, 0x65, 0x29, 0x28, 0x42, 0x49, 0x4a, 0x49, 0x4a, 0x49, 0x4a, 0x65, 0x29, 0x28, 0x42, 0x49, 0x4a, 0x49, 0x4a, 0x28, 0x42, 0x66, 0x31, 0x25, 0x29, 
  0x25, 0x29, 0x08, 0x42, 0xd7, 0xbd, 0xdb, 0xde, 0xbb, 0xde, 0xba, 0xd6, 0x28, 0x42, 0xd7, 0xbd, 0xbb, 0xde, 0xbb, 0xde, 0x18, 0xc6, 0x8a, 0x52, 0x25, 0x29, 
  0x25, 0x29, 0x49, 0x4a, 0xba, 0xd6, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x49, 0x4a, 0xdb, 0xde, 0xff, 0xff, 0xff, 0xff, 0x3d, 0xef, 0xcb, 0x5a, 0x25, 0x29, 
  0x25, 0x29, 0x49, 0x4a, 0xba, 0xd6, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x49, 0x4a, 0xdb, 0xde, 0xff, 0xff, 0xff, 0xff, 0x3d, 0xef, 0xcb, 0x5a, 0x25, 0x29, 
  0x25, 0x29, 0x49, 0x4a, 0xba, 0xd6, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x49, 0x4a, 0xdb, 0xde, 0xff, 0xff, 0xff, 0xff, 0x3d, 0xef, 0xcb, 0x5a, 0x25, 0x29, 
  0x25, 0x29, 0x65, 0x29, 0x28, 0x42, 0x49, 0x4a, 0x49, 0x4a, 0x49, 0x4a, 0x65, 0x29, 0x28, 0x42, 0x49, 0x4a, 0x49, 0x4a, 0x29, 0x4a, 0x66, 0x31, 0x25, 0x29, 
  0x04, 0x21, 0xe8, 0x41, 0xd7, 0xbd, 0xdb, 0xde, 0xdb, 0xde, 0xbb, 0xde, 0xe8, 0x41, 0xd7, 0xbd, 0xdb, 0xde, 0xdb, 0xde, 0x39, 0xce, 0x69, 0x4a, 0x04, 0x21, 
  0xe4, 0x20, 0x08, 0x42, 0xba, 0xd6, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x08, 0x42, 0xbb, 0xde, 0xff, 0xff, 0xff, 0xff, 0x3c, 0xe7, 0x8a, 0x52, 0xe4, 0x20, 
  0xe4, 0x20, 0x08, 0x42, 0xba, 0xd6, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x08, 0x42, 0xbb, 0xde, 0xff, 0xff, 0xff, 0xff, 0x3c, 0xe7, 0x8a, 0x52, 0xe4, 0x20, 
  0xe4, 0x20, 0x08, 0x42, 0x39, 0xce, 0x5d, 0xef, 0x5d, 0xef, 0x3d, 0xef, 0xe8, 0x41, 0x39, 0xce, 0x5d, 0xef, 0x5d, 0xef, 0xba, 0xd6, 0x6a, 0x52, 0xe4, 0x20, 
  0xe4, 0x20, 0x45, 0x29, 0x69, 0x4a, 0xaa, 0x52, 0xab, 0x5a, 0xaa, 0x52, 0x25, 0x29, 0x69, 0x4a, 0xaa, 0x52, 0xab, 0x5a, 0x8a, 0x52, 0x45, 0x29, 0xe4, 0x20, 
  0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 
#endif
#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP != 0
  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit BUT the 2 bytes are swapped*/
  0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 0x29, 0x25, 
  0x29, 0x25, 0x29, 0x65, 0x42, 0x28, 0x4a, 0x49, 0x4a, 0x49, 0x4a, 0x49, 0x29, 0x65, 0x42, 0x28, 0x4a, 0x49, 0x4a, 0x49, 0x42, 0x28, 0x31, 0x66, 0x29, 0x25, 
  0x29, 0x25, 0x42, 0x08, 0xbd, 0xd7, 0xde, 0xdb, 0xde, 0xbb, 0xd6, 0xba, 0x42, 0x28, 0xbd, 0xd7, 0xde, 0xbb, 0xde, 0xbb, 0xc6, 0x18, 0x52, 0x8a, 0x29, 0x25, 
  0x29, 0x25, 0x4a, 0x49, 0xd6, 0xba, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x4a, 0x49, 0xde, 0xdb, 0xff, 0xff, 0xff, 0xff, 0xef, 0x3d, 0x5a, 0xcb, 0x29, 0x25, 
  0x29, 0x25, 0x4a, 0x49, 0xd6, 0xba, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x4a, 0x49, 0xde, 0xdb, 0xff, 0xff, 0xff, 0xff, 0xef, 0x3d, 0x5a, 0xcb, 0x29, 0x25, 
  0x29, 0x25, 0x4a, 0x49, 0xd6, 0xba, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x4a, 0x49, 0xde, 0xdb, 0xff, 0xff, 0xff, 0xff, 0xef, 0x3d, 0x5a, 0xcb, 0x29, 0x25, 
  0x29, 0x25, 0x29, 0x65, 0x42, 0x28, 0x4a, 0x49, 0x4a, 0x49, 0x4a, 0x49, 0x29, 0x65, 0x42, 0x28, 0x4a, 0x49, 0x4a, 0x49, 0x4a, 0x29, 0x31, 0x66, 0x29, 0x25, 
  0x21, 0x04, 0x41, 0xe8, 0xbd, 0xd7, 0xde, 0xdb, 0xde, 0xdb, 0xde, 0xbb, 0x41, 0xe8, 0xbd, 0xd7, 0xde, 0xdb, 0xde, 0xdb, 0xce, 0x39, 0x4a, 0x69, 0x21, 0x04, 
  0x20, 0xe4, 0x42, 0x08, 0xd6, 0xba, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x42, 0x08, 0xde, 0xbb, 0xff, 0xff, 0xff, 0xff, 0xe7, 0x3c, 0x52, 0x8a, 0x20, 0xe4, 
  0x20, 0xe4, 0x42, 0x08, 0xd6, 0xba, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x42, 0x08, 0xde, 0xbb, 0xff, 0xff, 0xff, 0xff, 0xe7, 0x3c, 0x52, 0x8a, 0x20, 0xe4, 
  0x20, 0xe4, 0x42, 0x08, 0xce, 0x39, 0xef, 0x5d, 0xef, 0x5d, 0xef, 0x3d, 0x41, 0xe8, 0xce, 0x39, 0xef, 0x5d, 0xef, 0x5d, 0xd6, 0xba, 0x52, 0x6a, 0x20, 0xe4, 
  0x20, 0xe4, 0x29, 0x45, 0x4a, 0x69, 0x52, 0xaa, 0x5a, 0xab, 0x52, 0xaa, 0x29, 0x25, 0x4a, 0x69, 0x52, 0xaa, 0x5a, 0xab, 0x52, 0x8a, 0x29, 0x45, 0x20, 0xe4, 
  0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 0x20, 0xe4, 
#endif
#if LV_COLOR_DEPTH == 32
  /*Pixel format: Fix 0xFF: 8 bit, Red: 8 bit, Green: 8 bit, Blue: 8 bit*/
  0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 0x25, 0x25, 0x25, 0xff, 
  0x25, 0x25, 0x25, 0xff, 0x2a, 0x2a, 0x2a, 0xff, 0x42, 0x42, 0x42, 0xff, 0x48, 0x48, 0x48, 0xff, 0x48, 0x48, 0x48, 0xff, 0x46, 0x46, 0x46, 0xff, 0x2b, 0x2b, 0x2b, 0xff, 0x42, 0x42, 0x42, 0xff, 0x48, 0x48, 0x48, 0xff, 0x48, 0x48, 0x48, 0xff, 0x43, 0x43, 0x43, 0xff, 0x2d, 0x2d, 0x2d, 0xff, 0x25, 0x25, 0x25, 0xff, 
  0x25, 0x25, 0x25, 0xff, 0x41, 0x41, 0x41, 0xff, 0xb6, 0xb6, 0xb6, 0xff, 0xd6, 0xd6, 0xd6, 0xff, 0xd5, 0xd5, 0xd5, 0xff, 0xd2, 0xd2, 0xd2, 0xff, 0x42, 0x42, 0x42, 0xff, 0xb8, 0xb8, 0xb8, 0xff, 0xd5, 0xd5, 0xd5, 0xff, 0xd5, 0xd5, 0xd5, 0xff, 0xc1, 0xc1, 0xc1, 0xff, 0x4e, 0x4e, 0x4e, 0xff, 0x25, 0x25, 0x25, 0xff, 
  0x25, 0x25, 0x25, 0xff, 0x47, 0x47, 0x47, 0xff, 0xd3, 0xd3, 0xd3, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xf6, 0xf6, 0xf6, 0xff, 0x48, 0x48, 0x48, 0xff, 0xd6, 0xd6, 0xd6, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xe5, 0xe5, 0xe5, 0xff, 0x59, 0x59, 0x59, 0xff, 0x25, 0x25, 0x25, 0xff, 
  0x25, 0x25, 0x25, 0xff, 0x47, 0x47, 0x47, 0xff, 0xd3, 0xd3, 0xd3, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xf6, 0xf6, 0xf6, 0xff, 0x48, 0x48, 0x48, 0xff, 0xd6, 0xd6, 0xd6, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xe5, 0xe5, 0xe5, 0xff, 0x59, 0x59, 0x59, 0xff, 0x25, 0x25, 0x25, 0xff, 
  0x25, 0x25, 0x25, 0xff, 0x47, 0x47, 0x47, 0xff, 0xd3, 0xd3, 0xd3, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xf6, 0xf6, 0xf6, 0xff, 0x48, 0x48, 0x48, 0xff, 0xd6, 0xd6, 0xd6, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xe5, 0xe5, 0xe5, 0xff, 0x59, 0x59, 0x59, 0xff, 0x25, 0x25, 0x25, 0xff, 
  0x24, 0x24, 0x24, 0xff, 0x2a, 0x2a, 0x2a, 0xff, 0x42, 0x42, 0x42, 0xff, 0x49, 0x49, 0x49, 0xff, 0x49, 0x49, 0x49, 0xff, 0x48, 0x48, 0x48, 0xff, 0x2a, 0x2a, 0x2a, 0xff, 0x42, 0x42, 0x42, 0xff, 0x49, 0x49, 0x49, 0xff, 0x49, 0x49, 0x49, 0xff, 0x45, 0x45, 0x45, 0xff, 0x2d, 0x2d, 0x2d, 0xff, 0x24, 0x24, 0x24, 0xff, 
  0x1e, 0x1e, 0x1e, 0xff, 0x3d, 0x3d, 0x3d, 0xff, 0xb6, 0xb6, 0xb6, 0xff, 0xd7, 0xd7, 0xd7, 0xff, 0xd7, 0xd7, 0xd7, 0xff, 0xd4, 0xd4, 0xd4, 0xff, 0x3c, 0x3c, 0x3c, 0xff, 0xb7, 0xb7, 0xb7, 0xff, 0xd7, 0xd7, 0xd7, 0xff, 0xd7, 0xd7, 0xd7, 0xff, 0xc4, 0xc4, 0xc4, 0xff, 0x4a, 0x4a, 0x4a, 0xff, 0x1e, 0x1e, 0x1e, 0xff, 
  0x1c, 0x1c, 0x1c, 0xff, 0x41, 0x41, 0x41, 0xff, 0xd3, 0xd3, 0xd3, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xf6, 0xf6, 0xf6, 0xff, 0x40, 0x40, 0x40, 0xff, 0xd4, 0xd4, 0xd4, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xe3, 0xe3, 0xe3, 0xff, 0x51, 0x51, 0x51, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 
  0x1c, 0x1c, 0x1c, 0xff, 0x41, 0x41, 0x41, 0xff, 0xd3, 0xd3, 0xd3, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xf6, 0xf6, 0xf6, 0xff, 0x40, 0x40, 0x40, 0xff, 0xd4, 0xd4, 0xd4, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xfa, 0xfa, 0xfa, 0xff, 0xe3, 0xe3, 0xe3, 0xff, 0x51, 0x51, 0x51, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 
  0x1c, 0x1c, 0x1c, 0xff, 0x40, 0x40, 0x40, 0xff, 0xc5, 0xc5, 0xc5, 0xff, 0xe7, 0xe7, 0xe7, 0xff, 0xe8, 0xe8, 0xe8, 0xff, 0xe5, 0xe5, 0xe5, 0xff, 0x3d, 0x3d, 0x3d, 0xff, 0xc4, 0xc4, 0xc4, 0xff, 0xe7, 0xe7, 0xe7, 0xff, 0xe8, 0xe8, 0xe8, 0xff, 0xd3, 0xd3, 0xd3, 0xff, 0x4d, 0x4d, 0x4d, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 
  0x1c, 0x1c, 0x1c, 0xff, 0x26, 0x26, 0x26, 0xff, 0x4a, 0x4a, 0x4a, 0xff, 0x53, 0x53, 0x53, 0xff, 0x54, 0x54, 0x54, 0xff, 0x53, 0x53, 0x53, 0xff, 0x25, 0x25, 0x25, 0xff, 0x4a, 0x4a, 0x4a, 0xff, 0x53, 0x53, 0x53, 0xff, 0x54, 0x54, 0x54, 0xff, 0x4e, 0x4e, 0x4e, 0xff, 0x29, 0x29, 0x29, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 
  0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 0x1c, 0x1c, 0x1c, 0xff, 
#endif
};

const lv_img_dsc_t MAINMENU_IMG_OPTION_FOCUS = {
  .header.cf = LV_IMG_CF_TRUE_COLOR,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 13,
  .header.h = 13,
  .data_size = 169 * LV_COLOR_SIZE / 8,
  .data = MAINMENU_IMG_OPTION_FOCUS_map,
};

