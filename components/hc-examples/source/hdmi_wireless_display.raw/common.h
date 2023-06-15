#ifndef _HCAIP_H_H_
#define _HCAIP_H_H_ 1

#include <stdint.h>

#define AUDIO_CTRL_PORT 7001
#define AUDIO_DATA_PORT 7002
#define VIDEO_CTRL_PORT 7003
#define VIDEO_DATA_PORT 7004


typedef  struct data_header_t
{
	char magic[8];
	int32_t size;
	int32_t pts;
	uint32_t sample_rate;
}__attribute__((packed)) data_header_t;

#define DATA_HEADER_MAGIC "hdmi_rx"
#endif
