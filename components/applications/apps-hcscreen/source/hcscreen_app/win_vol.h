/**/

#ifndef __WIN_VOL_H__
#define __WIN_VOL_H__

#ifdef __cplusplus
extern "C" {
#endif



int win_volume_set(uint32_t param, uint8_t set_vol);
int win_volume_close(void *arg);



#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

