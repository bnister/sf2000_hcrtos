#ifndef __CAST_CONF_H__
#define __CAST_CONF_H__

/**
 * Cast delta UI, config WIFI manual, please choose only one
 */
//#define SOLUTION_CAST_DELTA	 
/**
 * Cast fly UI, config WIFI automatically, please choose only one
 */
//#define SOLUTION_CAST_FLY

/**
 * Cast unify UI, include cast, media features, please choose only one
 */
#define SOLUTION_CAST_UNIFY

#if defined(SOLUTION_CAST_DELTA) && (defined(SOLUTION_CAST_FLY) || defined(SOLUTION_CAST_UNIFY))
#error "Choose SOLUTION_CAST_DELTA, can not choose other solution again!"
#endif
#if defined(SOLUTION_CAST_FLY) && (defined(SOLUTION_CAST_DELTA) || defined(SOLUTION_CAST_UNIFY))
#error "Choose SOLUTION_CAST_FLY, can not choose other solution again!"
#endif
#if defined(SOLUTION_CAST_UNIFY) && (defined(SOLUTION_CAST_DELTA) || defined(SOLUTION_CAST_FLY))
#error "Choose SOLUTION_CAST_UNIFY, can not choose other solution again!"
#endif


//#define NETWORK_SUPPORT

//#define CAST_DLNA_ENABLE
//#define CAST_AIRPLAY_ENABLE
//#define CAST_MIRACAST_ENABLE

//Change/delete LVGL objects and frush/draw LVGL objects in same main task, 
//to avoid access LVGL objects conflict.
#define DRAW_UI_SYNC




#endif