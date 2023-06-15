#include <linux/netdevice.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "raw_proto.h"
#include "queue.h"
#include <linux/skbuff.h>
#include <freertos/message_buffer.h>
#include <kernel/lib/console.h>
#include <net/net_device.h>

static raw_header_t g_header;
static uint32_t g_data_len = 0;
static MessageBufferHandle_t video_skb_queue;
static MessageBufferHandle_t audio_skb_queue;
static int g_stream_id = -1;
static int g_init = 0;

#define MESSSAGE_BUFFER_SIZE (5000*sizeof(void *))

int raw_recv_init(void)
{
	video_skb_queue = xMessageBufferCreate( MESSSAGE_BUFFER_SIZE );
	if(!video_skb_queue){
		printf("%s:video_skb_queue error.\n", __func__);
		goto err;
	}
	audio_skb_queue = xMessageBufferCreate( MESSSAGE_BUFFER_SIZE );
	if(!audio_skb_queue){
		printf("%s:audio_skb_queue error.\n", __func__);
		goto err;
	}

	g_init = 1;

	return 0;
err:
	if(video_skb_queue){
		vMessageBufferDelete(video_skb_queue);
		video_skb_queue = NULL;
	}
	if(audio_skb_queue){
		vMessageBufferDelete(audio_skb_queue);
		audio_skb_queue = NULL;
	}

	return -1;
}

#define RAW_COPY_DATA() do{ \
	if(valid){ \
		if((data_len + skb->len - RAW_LLC_HEADER)  <= (header.length + RAW_SEGEMENT_UNIT)){ \
			memcpy(data + data_len, skb->data + RAW_LLC_HEADER, skb->len - RAW_LLC_HEADER); \
			data_len += skb->len - RAW_LLC_HEADER; \
			segments++; \
		}else { \
			valid = false; \
			printf("receive frame error. throw away all\n"); \
			printf("(%ld, %ld), (%d,%d), stream_id:%d\n", header.length, data_len, header.segments, segments, header.stream_id); \
		} \
	} \
}while(0)

int raw_get_data(data_header_t *pinfo, uint8_t **pdata, int stream_id)
{
	MessageBufferHandle_t q = NULL;

	if(!pinfo || !pdata)
		return -1;

	switch(stream_id){
		case STREAM_VIDEO:
			q = video_skb_queue;
			break;
		case STREAM_AUDIO:
			q = audio_skb_queue;
			break;
		default:
			printf("unknown\n");
			return -1;
	}

	struct sk_buff *skb = NULL;
	uint32_t data_len = 0;
	int segments = 0;
	raw_header_t header;
	raw_header_t *h;
	uint8_t *data = NULL;
	size_t xReceivedBytes = 0;
	struct raw_eth_header *net_header;
	bool valid = 0;

	while(true){
		xReceivedBytes = xMessageBufferReceive(q, &skb, sizeof(skb), 0);//pdMS_TO_TICKS( 20 )); 
		if(xReceivedBytes != sizeof(skb)){
			usleep(1000);
			continue;
		}
		net_header = (struct raw_eth_header *)skb->data;

		/*printf("%ld\n", net_header->id);*/
		if(skb->len == (RAW_HEADER_LENGTH + RAW_LLC_HEADER)){
			h = (raw_header_t *)(skb->data + RAW_LLC_HEADER);
			if(h->magic == RAW_MAGIC){
				if(data){
					free(data);
					data = NULL;
					printf("throw:(%ld, %ld), (%d,%d), stream_id:%d\n", header.length, 
							data_len, header.segments, segments, header.stream_id); 
				}
				memcpy(&header, skb->data + RAW_LLC_HEADER, sizeof(raw_header_t));
				data = malloc(header.length + RAW_SEGEMENT_UNIT);
				if(!data){
					printf("%s:%d, not enough memroy.\n", __func__, __LINE__);
					dev_kfree_skb_any(skb);	
					goto err;
				}
				memcpy(pinfo, &header.info, sizeof(data_header_t));	
				data_len = 0;
				segments = 0;
				valid = true;


			}else{
				RAW_COPY_DATA();
			}
		}else{
			RAW_COPY_DATA();
		}

		if(!valid){
			printf("receive invalid data package. skb->len: %d\n", skb->len);
		}

		dev_kfree_skb_any(skb);	

		if(valid && ((data_len == g_header.length) || (segments == header.segments))){
			valid = false;
			break;
		}
	}
	*pdata = data;

	return 0;
err:
	if(data)
		free(data);
	return -1;
}

void raw_free_data(uint8_t *data)
{
	if(data)
		free(data);
}


static int enqueue_skb(MessageBufferHandle_t xMessageBuffer,  void *skb)
{
	size_t xBytesSent;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (!uxInterruptNesting){
		xBytesSent = xMessageBufferSend( xMessageBuffer, ( void * ) &skb, sizeof(skb), 0 );
	} else{
		xBytesSent = xMessageBufferSendFromISR( xMessageBuffer, ( void * ) &skb, sizeof(skb), &xHigherPriorityTaskWoken );
	}
	if(xBytesSent != sizeof(skb)){
		printf("enqueu skb error\n");
		return -1;
	}
	return 0;
}

int raw_recv_callback(struct sk_buff *skb, void *opaque)
{


	if(!g_init){
		dev_kfree_skb_any(skb);	
		return MONITOR_SKB_EAT;
	}


	struct raw_eth_header *h = (struct raw_eth_header *)skb->data;
	if(h->eth.h_proto != ETH_P_HC){
		return MONITOR_SKB_NOT_HANDLE;
	}

	raw_header_t *header;
	if(skb->len == (RAW_HEADER_LENGTH + RAW_LLC_HEADER)){
		header = (raw_header_t *)(skb->data + RAW_LLC_HEADER);
		if(header->magic == RAW_MAGIC){
			memcpy(&g_header, header, sizeof(raw_header_t));	
			g_stream_id = g_header.stream_id;
		}
	}

	if(g_stream_id == STREAM_VIDEO){
		if(enqueue_skb(video_skb_queue, skb) != 0)
			dev_kfree_skb_any(skb);	
	}else if(g_stream_id == STREAM_AUDIO){
		if(enqueue_skb(audio_skb_queue, skb) != 0)
			dev_kfree_skb_any(skb);	
	}else{
		printf("BUGS\n");
		dev_kfree_skb_any(skb);	
	}

	return  MONITOR_SKB_EAT;
}
