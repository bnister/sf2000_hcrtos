#ifndef _DECODER_H_H_
#define _DECODER_H_H_
#include <stdint.h>
#include <stdio.h>
#include <hcuapi/dis.h>

void *h264_decode_init(int width , int height ,
                       uint8_t *extradata , int size ,
                       int8_t rotate_enable , int8_t quick_mode_enable);
int h264_decode(void *phandle, uint8_t *video_frame, size_t packet_size , uint32_t rotate_mode);
void h264_decoder_flush(void *phandle);
void h264_decoder_destroy(void *phandle);

void *aac_decoder_init(int bits, int channels, int samplerate);
int aac_decode(void *phandle, uint8_t *audio_frame, size_t packet_size);
void aac_decoder_flush(void *phandle);
void aac_decoder_destroy(void *phandle);

void set_volume(uint8_t vol);
void set_aspect_mode(dis_tv_mode_e ratio, dis_mode_e dis_mode);
#endif
