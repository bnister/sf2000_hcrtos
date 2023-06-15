#ifndef _HCAIP_H_H_
#define _HCAIP_H_H_ 1

#include <stdint.h>

#define AUDIO_CTRL_PORT 7001
#define AUDIO_DATA_PORT 7002
#define VIDEO_CTRL_PORT 7003
#define VIDEO_DATA_PORT 7004

#ifdef BR2_PACKAGE_HDMI_WIRELSS_DISPLAY_UDP
#define USE_UDP_PROTOCOL 1
#endif

typedef struct udp_header{
	uint32_t magic;
	uint32_t seg_id;
	uint32_t seg_len;
	uint32_t seg_nums;
} udp_header;

#define UDP_SEG_LENGTH 1400
#define UDP_MAX_PAYLOAD_LEN (UDP_SEG_LENGTH - sizeof(udp_header))


typedef  struct data_header_t
{
	char magic[8];
	int32_t size;
	int32_t pts;
	uint32_t sample_rate;
}__attribute__((packed)) data_header_t;

#define DATA_HEADER_MAGIC "hdmi_rx"
#endif
