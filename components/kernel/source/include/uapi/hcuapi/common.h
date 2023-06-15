#ifndef _HC_COMMON_H_
#define _HC_COMMON_H_

#include <hcuapi/iocbase.h>

#define MAX_KUMSG_SIZE				1024

#define SET_AV_BUFFERING_RANGE			_IOW (AVCOMMON_IOCBASE, 1, struct BufferingRange)
#define GET_AV_BUFFERING_PERCENT		_IOR (AVCOMMON_IOCBASE, 2, int)
#define GET_AV_BUFFER_SIZE			_IOR (AVCOMMON_IOCBASE, 3, int)

typedef struct BufferingRange {
	int start;
	int end;
} BufRng;

typedef struct MsgDataHeader {
	int32_t type;
	void *user_args;
	void *params;
} KuMsgDH;

typedef enum AvPacketType {
	AV_PACKET_ES_DATA,
	AV_PACKET_EXTRA_DATA,
	AV_PACKET_EOS,
	AV_PACKET_ES_DATA_POINTER,
} AvPktTp;

typedef struct AvPacketHeader AvPktHd;
struct AvPacketHeader {
	int32_t pts;
	int32_t dur;

	/* data size, do not contain header */
	uint32_t size : 30;

	/* AvPktTp */
	uint32_t flag : 2;

	uint16_t video_rotate_mode;
	uint16_t video_mirror_mode;
} __attribute__((packed));

#endif /*_HC_COMMON_H_*/
