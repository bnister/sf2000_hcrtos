#include "ffplayer.h"

void *hcplayer_multi_create (HCPlayerInitArgs *audio_initargs, HCPlayerInitArgs *video_initargs);
void hcplayer_multi_destroy(void *hdl);
int hcplayer_multi_seek(void *hdl, int64_t time_ms);
int hcplayer_multi_play(void *hdl);
int hcplayer_multi_pause(void *hdl);
int hcplayer_multi_duration(void *hdl);
int hcplayer_multi_position(void *hdl);
