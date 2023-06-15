#ifndef __MEDIA_SPECTRUM_H_
#define __MEDIA_SPECTRUM_H_

#ifdef __cplusplus
extern "C" {
#endif

void * get_music_spect_data(void);

static void *i2so_spectrum_thread(void *args);
int music_spectrum_stop(void);
int music_spectrum_start(void);


#endif




