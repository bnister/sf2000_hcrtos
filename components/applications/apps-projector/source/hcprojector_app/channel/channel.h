// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: ProjectorSetting

#ifndef _PROJECTORCHANNEL_UI_H
#define _PROJECTORCHANNEL_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "screen.h"



typedef void (*Func)(void);

typedef enum{
#ifdef CVBSIN_SUPPORT
    CVBS_ID,
#endif
    HDMI_ID,
#if PROJECTER_C2_VERSION
    HDMI2_ID,
#endif
    MEDIA_ID,
#ifdef MAIN_PAGE_SUPPORT
    MAIN_PAGE_ID
#endif
} Channel;


typedef struct {
    Channel channel;
    Func func;
} Channel_map;



void lv_example_setting(void);
int hdmi_rx_leave(void);
int hdmi_rx_enter(void);
int cvbs_rx_enter(void);
int local_mp_enter(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
