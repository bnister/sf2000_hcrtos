#ifndef _DECODER_H_H_
#define _DECODER_H_H_
#include <stdint.h>
#include <stdio.h>
#include <hcuapi/dis.h>

void *alac_decoder_init(int bits, int channels, int samplerate);
int alac_decode(void *phandle, uint8_t *audio_frame, size_t packet_size);
void alac_decoder_flush(void *phandle);
void alac_decoder_destroy(void *phandle);
int alac_test (int argc, char *argv[]);

#endif
