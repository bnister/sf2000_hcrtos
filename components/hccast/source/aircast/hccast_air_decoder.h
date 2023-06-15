#ifndef _DECODER_H_H_
#define _DECODER_H_H_
#include <stdint.h>
#include <stdio.h>
#include <hcuapi/dis.h>

void *hccast_air_h264_decode_init(int width, int height, uint8_t *extradata, int size, int framenum);
int hccast_air_h264_decode(void *phandle, uint8_t *video_frame, size_t packet_size,unsigned int pts, int rotate_mode, int flip_mirror);
void hccast_air_h264_decoder_flush(void *phandle);
void hccast_air_h264_decoder_destroy(void *phandle,int close_vp);

void *hccast_air_audio_decoder_init(int bits, int channels, int samplerate, int audio_type);
int hccast_air_audio_decode(void *phandle, uint8_t *audio_frame, size_t packet_size,unsigned int pts);
void hccast_air_audio_decoder_flush(void *phandle);
void hccast_air_audio_decoder_destroy(void *phandle);

#endif
