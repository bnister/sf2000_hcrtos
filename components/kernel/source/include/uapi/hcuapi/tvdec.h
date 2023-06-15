#ifndef __HCUAPI_TVDEC_H_
#define __HCUAPI_TVDEC_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/kshm.h>
#include <hcuapi/tvtype.h>

typedef enum TVDEC_VIDEO_DATA_PATH {
	TVDEC_VIDEO_TO_DE,
	TVDEC_VIDEO_TO_DE_ROTATE,
	TVDEC_VIDEO_TO_KSHM,
	TVDEC_VIDEO_TO_DE_AND_KSHM,
	TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM,
} tvdec_video_data_path_e;

typedef struct tvdec_video_info {
	tvtype_e tv_sys;
	int width;
	int height;
	int frame_rate;
	int b_progressive;
} tvdec_video_info_t;

#define TVDEC_START			_IO (TVDEC_IOCBASE, 0)
#define TVDEC_STOP			_IO (TVDEC_IOCBASE, 1)
#define TVDEC_SET_VIDEO_DATA_PATH	_IO (TVDEC_IOCBASE, 2)
#define TVDEC_VIDEO_KSHM_ACCESS		_IOR(TVDEC_IOCBASE, 3, struct kshm_info)
#define TVDEC_SET_VIDEO_ROTATE_MODE	_IO (TVDEC_IOCBASE, 4)
#define TVDEC_SET_VIDEO_MIRROR_MODE	_IO (TVDEC_IOCBASE, 5)
#define TVDEC_NOTIFY_CONNECT		_IO (TVDEC_IOCBASE, 6)
#define TVDEC_NOTIFY_DISCONNECT		_IO (TVDEC_IOCBASE, 7)
#define TVDEC_GET_VIDEO_INFO		_IOR(TVDEC_IOCBASE, 8, struct tvdec_video_info)
#define TVDEC_SET_VIDEO_STOP_MODE	_IO (TVDEC_IOCBASE, 9) //<! param: Keep last frame: 1, Black screen: 0

#endif /* __HCUAPI_TVDEC_H_ */
