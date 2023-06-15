#include <linux/netdevice.h>
#include <pthread.h>
#include <unistd.h>
/*#include <lwip/netif.h>*/
#include "raw_proto.h"
#include <net/net_device.h>

/*static uint8_t llc_header[] = {*/

#define FREE_SKB 1
extern struct net_device *g_eth_dev;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int data_len = 0;
static struct raw_eth_header net_header = {
	{
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		ETH_P_HC
	},
	0
};


//not return util sending success
static void start_xmit(struct sk_buff *skb)
{
	if(NetIfLLMonitorSend(netdev_config, skb) != 0)
		dev_kfree_skb_any(skb);	
}

int raw_send_func(data_header_t *info, uint8_t *data, uint32_t length, int stream_id, int stream_type)
{
	raw_header_t *header;
	int len = 1500;
	int res = length;
	int ret = 0;
	struct sk_buff *send_skb = NULL;
	uint8_t *buf;
	int i = 0;

	pthread_mutex_lock(&mutex);
	if(!netdev_config){
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	// send head
	send_skb = netdev_alloc_skb(NetifLLMonitorGetDev(netdev_config), RAW_HEADER_LENGTH + RAW_LLC_HEADER);
	buf = skb_put(send_skb, RAW_HEADER_LENGTH + RAW_LLC_HEADER);
	memcpy(buf, &net_header, RAW_LLC_HEADER);
	header = (raw_header_t *)(buf + RAW_LLC_HEADER);

	header->magic = RAW_MAGIC;
	header->stream_type = stream_type;
	header->stream_id = stream_id;
	header->length = length;
	header->segments = length/RAW_SEGEMENT_UNIT;
	memcpy(&header->info, info, sizeof(*info));
	if(length % RAW_SEGEMENT_UNIT)
		header->segments++;
	start_xmit(send_skb);
	data_len = 0;
	uint8_t segments = 1;

	//send data
	while(res > 0){ 
		len = (res > (int)RAW_SEGEMENT_UNIT)? (int)RAW_SEGEMENT_UNIT: res;

		send_skb = netdev_alloc_skb(NetifLLMonitorGetDev(netdev_config), len + RAW_LLC_HEADER);
		buf = skb_put(send_skb, len + RAW_LLC_HEADER);
		net_header.id = segments;
		memcpy(buf, &net_header, RAW_LLC_HEADER);
		memcpy(buf + RAW_LLC_HEADER, data, len);
		start_xmit(send_skb);
		segments++;
		res -= len;
		data += len;
	}
	
	//maybe send end
	//TODO:
	
	pthread_mutex_unlock(&mutex);

	return 0;
}
