/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-11-29 16:11:56
 */
#ifndef _MP_ZOOM_H
#define _MP_ZOOM_H

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "screen.h"

#define DIS_ZOOM_FULL_X  0
#define DIS_ZOOM_FULL_Y  0
#define DIS_ZOOM_FULL_H  1080
#define DIS_ZOOM_FULL_W  1920
// #define DIS_ZOOM_SRC_X
// #define DIS_ZOOM_SRC_X

#define ZOOM_MOVE_STEP  20

// for music cover pic 
#define MUSIC_COVER_ZOOM_W 600
#define MUSIC_COVER_ZOOM_H 600
#define MUSIC_COVER_ZOOM_X 1170
#define MUSIC_COVER_ZOOM_Y 150


typedef enum Zoom_mode
{
    MPZOOM_IN=0,
    MPZOOM_OUT=1,
}Zoom_mode_e;
typedef enum Zoom_size
{
    ZOOM_OUTSIZE_3=-3,
    ZOOM_OUTSIZE_2=-2,
    ZOOM_OUTSIZE_1=-1,
    ZOOM_NORMAL=0,
    ZOOM_INSIZE_1=1,
    ZOOM_INSIZE_2=2,
    ZOOM_INSIZE_3=3,

}Zoom_size_e;
typedef struct Zoom_Param
{
    // Zoom_mode_e zoom_mode;
    Zoom_size_e zoom_size;
    int32_t zoom_state;
}Zoom_Param_t ;
/*for zoom move func*/
typedef struct area_range{
   int32_t up_range;
   int32_t down_range;
   int32_t left_range;
   int32_t right_range;
}area_range_t; 
typedef struct zoom_move
{
    uint32_t move_step;
    area_range_t zoom_area;
}zoom_move_t;



int app_set_diszoom(Zoom_mode_e Zoom_mode);
void app_reset_diszoom_param(void);
int create_zoommoove_win(lv_obj_t* p,lv_obj_t * subbtn);
void* app_get_zoom_param(void);
dis_zoom_t app_reset_mainlayer_param(int rotate , int h_flip);
int app_reset_mainlayer_pos(int rotate , int h_flip);


#endif
