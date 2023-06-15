#ifndef __RAW_PROTO_H
#define __RAW_PROTO_H
#include "common.h"
//#include <netinet/if_ether.h>
#include <uapi/linux/if_ether.h>
#include <linux/skbuff.h>

enum {
	STREAM_VIDEO = 1,
	STREAM_AUDIO,
};

enum {
	STREAM_VIDEO_HEADER = 1,
	STREAM_VIDEO_DATA,
	STREAM_AUDIO_HEADER,
	STREAM_AUDIO_DATA,
};

struct raw_eth_header
{
	struct ethhdr eth;
	uint32_t id;
};

#define ETH_P_HC  0x0880

#define RAW_MAGIC 0xdeadbeaf
#define RAW_HEADER_LENGTH 64

// 32 multiple
//#define RAW_SEGEMENT_UNIT ((1516/32)*32)
#define RAW_LLC_HEADER (sizeof(struct raw_eth_header))
#define RAW_SEGEMENT_UNIT (1500 - RAW_LLC_HEADER)

//start signal
typedef struct raw_header{
	uint32_t magic;
	uint16_t stream_type;
	uint16_t stream_id;
	uint32_t length;
	uint16_t segments;
	data_header_t info;
}__packed raw_header_t;

int raw_send_func(data_header_t *header, uint8_t *data, uint32_t length, int stream_id, int stream_type);
int raw_recv_init(void);
int raw_get_data(data_header_t *pinfo, uint8_t **pdata, int stream_id);
void raw_free_data(uint8_t *data);

typedef int (*raw_recv_func)(struct sk_buff *skb, void *opaque);

int raw_init(char *if_name, raw_recv_func cb, void *opaque);

int recv_calback(struct sk_buff *skb, void *opaque);
int raw_recv_callback(struct sk_buff *skb, void *opaque);


extern void *netdev_config;

#endif
