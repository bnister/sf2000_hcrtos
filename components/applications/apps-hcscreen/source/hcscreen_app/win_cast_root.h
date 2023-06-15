/*
* win_cast_root.h
 */

#ifndef __WIN_CAST_ROOT_H__
#define __WIN_CAST_ROOT_H__

#ifdef __cplusplus
extern "C" {
#endif

//Exit from dlna/miracast/aircast to cast ui by remote key.
//This cast, do not need to wait cast UI open before stop dlna/miracast/aircast.
void win_exit_to_cast_root_by_key_set(bool exit_by_key);
bool win_exit_to_cast_root_by_key_get(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
