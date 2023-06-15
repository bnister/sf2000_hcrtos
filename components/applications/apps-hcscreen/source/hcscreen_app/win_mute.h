/**
 * win_mute.h
 */
#ifndef __WIN_MUTE_H__
#define __WIN_MUTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MUTE_STATE		0
#define UNMUTE_STATE	1

void win_mute_on_off(bool show_osd);
bool win_is_unmute(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
