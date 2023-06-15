#ifndef __HCUAPI_VINDVP_H_
#define __HCUAPI_VINDVP_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/kshm.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/dis.h>

typedef enum VINDVP_VIDEO_DATA_PATH {
	VINDVP_VIDEO_TO_DE,
	VINDVP_VIDEO_TO_DE_ROTATE,
	VINDVP_VIDEO_TO_KSHM,
	VINDVP_VIDEO_TO_DE_AND_KSHM,
	VINDVP_VIDEO_TO_DE_ROTATE_AND_KSHM,
} vindvp_video_data_path_e;

typedef enum VINDVP_COMBINED_MODE {
	VINDVP_COMBINED_MODE_DISABLE,
	VINDVP_COMBINED_MODE_4_REGION,
} vindvp_combined_mode_e;

typedef enum VINDVP_COMBINED_CAPTURE_MODE {
	VINDVP_COMBINED_CAPTRUE_ORIGINAL,
	VINDVP_COMBINED_CAPTRUE_COMBINED,
} vindvp_combined_capture_mode_e;

typedef enum VINDVP_BG_COLOR {
	VINDVP_BG_COLOR_BLACK,
	VINDVP_BG_COLOR_BLUE,
} vindvp_bg_color_e;

typedef enum VINDVP_STOP_MODE {
	VINDVP_BLACK_SRCREEN_ANYWAY, //black screen anyway
	VINDVP_KEEP_LASTFRAME_ANYWAY, //keep last frame anyway
	VINDVP_KEEP_LASTFRAME_IF_NO_SIGNAL, //keep last frame screen when no signal stop
	VINDVP_KEEP_LASTFRAME_IF_HAS_SIGNAL, //keep last frame screen when have signal stop
} vindvp_stop_mode_e;

#define VINDVP_START				_IO (VINDVP_IOCBASE,  0)
#define VINDVP_STOP				_IO (VINDVP_IOCBASE,  1)
#define VINDVP_ENABLE				_IO (VINDVP_IOCBASE,  2)
#define VINDVP_DISABLE				_IO (VINDVP_IOCBASE,  3)
#define VINDVP_SET_VIDEO_DATA_PATH		_IO (VINDVP_IOCBASE,  4)
#define VINDVP_VIDEO_KSHM_ACCESS		_IOR(VINDVP_IOCBASE,  5, struct kshm_info)
#define VINDVP_SET_VIDEO_ROTATE_MODE		_IO (VINDVP_IOCBASE,  6)
#define VINDVP_SET_COMBINED_MODE		_IO (VINDVP_IOCBASE,  7)
#define VINDVP_SET_COMBINED_REGION_FREEZE	_IOW(VINDVP_IOCBASE,  8, struct combine_region_freeze_cfg)
#define VINDVP_CAPTURE_ONE_PICTURE		_IO (VINDVP_IOCBASE,  9)	//!< param: vindvp_combined_capture_mode_e
#define VINDVP_SET_EXT_DEV_INPUT_PORT_NUM	_IO (VINDVP_IOCBASE, 10)
#define VINDVP_SET_DISPLAY_LAYRER		_IO (VINDVP_IOCBASE, 11)	//!< param: dis_layer_e in dis.h
#define VINDVP_NOTIFY_CONNECT			_IO (VINDVP_IOCBASE, 12)
#define VINDVP_NOTIFY_DISCONNECT		_IO (VINDVP_IOCBASE, 13)
#define VINDVP_SET_BG_COLOR			_IO (VINDVP_IOCBASE, 14) // !< param:enum VINDVP_BG_COLOR
#define VINDVP_SET_VIDEO_STOP_MODE		_IO (VINDVP_IOCBASE, 15) //<! param: enum VINDVP_STOP_MODE

#endif /* __HCUAPI_TVDEC_H_ */
