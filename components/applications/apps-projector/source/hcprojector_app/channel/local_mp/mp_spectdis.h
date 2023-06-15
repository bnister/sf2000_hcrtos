#ifndef __MP_SPECTDIS_H_
#define __MP_SPECTDIS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"

#define REFRESH_CODE 10

int music_spect_refresh_dis(void);
int  create_music_spectrum(lv_obj_t* p);
int spectrum_uimsg_handle(uint32_t msg_type);


#endif
