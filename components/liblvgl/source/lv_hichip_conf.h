#ifndef _LV_HICHIP_CONF_H
#define _LV_HICHIP_CONF_H
#include "lvgl/src/lv_conf_internal.h"

/* hichip ge */
#ifdef CONFIG_LV_USE_GPU_HICHIP
#define LV_USE_GPU_HICHIP 1
#else
#define LV_USE_GPU_HICHIP 0
#endif

#if CONFIG_LV_USE_DUAL_FRAMEBUFFER
//enable dual framebuffer
#define LV_USE_DUAL_FRAMEBUFFER 1
#else
#define LV_USE_DUAL_FRAMEBUFFER 0
#endif

#if CONFIG_LV_HC_SCREEN_HOR_RES > 0
//in pixel
#define LV_HC_SCREEN_HOR_RES (CONFIG_LV_HC_SCREEN_HOR_RES)
#else
#define LV_HC_SCREEN_HOR_RES (1280)
#endif

#if CONFIG_LV_HC_SCREEN_VER_RES
#define LV_HC_SCREEN_VER_RES (CONFIG_LV_HC_SCREEN_VER_RES)
#else
#define LV_HC_SCREEN_VER_RES (720)
#endif

#if CONFIG_LV_HC_DRAW_BUF_SIZE > 0
//lvgl draw buffer size, in pixel
//lvgl sugest at least 10 * 
#define LV_HC_DRAW_BUF_SIZE (CONFIG_LV_HC_DRAW_BUF_SIZE)
#else
#define LV_HC_DRAW_BUF_SIZE (LV_HC_SCREEN_HOR_RES * LV_HC_SCREEN_VER_RES)
#endif

#if CONFIG_LV_HC_DRAW_BUF_COUNT > 0
//only support 1 or 2
#define LV_HC_DRAW_BUF_COUNT (CONFIG_LV_HC_DRAW_BUF_COUNT)
#else
#define LV_HC_DRAW_BUF_COUNT (1)
#endif


#if CONFIG_LV_HC_FB_COLOR_DEPTH
#define LV_HC_FB_COLOR_DEPTH CONFIG_LV_HC_FB_COLOR_DEPTH
#else
#define LV_HC_FB_COLOR_DEPTH 16
#endif

//fix conf warning
#ifndef LV_MEM_CUSTOM 
#define LV_MEM_CUSTOM  0
#endif

#if LV_MEM_CUSTOM == 0
extern void *lv_mem_adr;
extern int lv_mem_size;
#undef LV_MEM_SIZE
#define LV_MEM_SIZE lv_mem_size
#endif

#ifndef SIYUANHEITI_LIGHT_22_1B
    #ifdef CONFIG_SIYUANHEITI_LIGHT_22_1B
        #define SIYUANHEITI_LIGHT_22_1B CONFIG_SIYUANHEITI_LIGHT_22_1B
    #else
        #define SIYUANHEITI_LIGHT_22_1B 0
    #endif
#endif

#ifndef SIYUANHEITI_LIGHT_18_1B
    #ifdef CONFIG_SIYUANHEITI_LIGHT_18_1B
        #define SIYUANHEITI_LIGHT_18_1B CONFIG_SIYUANHEITI_LIGHT_18_1B
    #else
        #define SIYUANHEITI_LIGHT_18_1B 0
    #endif
#endif

#ifndef SIYUANHEITI_LIGHT_28_1B
    #ifdef CONFIG_SIYUANHEITI_LIGHT_28_1B
        #define SIYUANHEITI_LIGHT_28_1B CONFIG_SIYUANHEITI_LIGHT_28_1B
    #else
        #define SIYUANHEITI_LIGHT_28_1B 0
    #endif
#endif

#ifndef SIYUANHEITI_LIGHT_28_2B
    #ifdef CONFIG_SIYUANHEITI_LIGHT_28_2B
        #define SIYUANHEITI_LIGHT_28_2B CONFIG_SIYUANHEITI_LIGHT_28_2B
    #else
        #define SIYUANHEITI_LIGHT_28_2B 0
    #endif
#endif

#ifndef SIYUANHEITI_LIGHT_28_4B
    #ifdef CONFIG_SIYUANHEITI_LIGHT_28_4B
        #define SIYUANHEITI_LIGHT_28_4B CONFIG_SIYUANHEITI_LIGHT_28_4B
    #else
        #define SIYUANHEITI_LIGHT_28_4B 0
    #endif
#endif

#ifndef SIYUANHEITI_LIGHT_3000_28_1B
    #ifdef CONFIG_SIYUANHEITI_LIGHT_3000_28_1B
        #define SIYUANHEITI_LIGHT_3000_28_1B CONFIG_SIYUANHEITI_LIGHT_3000_28_1B
    #else
        #define SIYUANHEITI_LIGHT_3000_28_1B 0
    #endif
#endif

#ifndef LVGL_HC_IR
    #ifdef CONFIG_LVGL_HC_IR
        #define LVGL_HC_IR CONFIG_LVGL_HC_IR
    #else
        #define LVGL_HC_IR 0
    #endif
#endif

#ifndef LV_HC_KEYSTONE_AA_SW_FIX
#ifdef CONFIG_LV_HC_KEYSTONE_AA_SW_FIX
#define LV_HC_KEYSTONE_AA_SW_FIX  CONFIG_LV_HC_KEYSTONE_AA_SW_FIX
#else
#define LV_HC_KEYSTONE_AA_SW_FIX  0
#endif
#endif

#endif /* _LV_HICHIP_CONF_H */
