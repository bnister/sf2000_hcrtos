#ifndef _HCUAPI_VIDDEC_H_
#define _HCUAPI_VIDDEC_H_

#include <hcuapi/vidmp.h>
#include <hcuapi/iocbase.h>

#define VIDDEC_DEVICE_NAME            "viddec"

enum vdec_work_mode {
	VDEC_WORK_MODE_NORMAL = 0,           /**< Normal decode */
	VDEC_WORK_MODE_VOL,                  /**< Parse all headers above (including) VOP level without VOP reconstuction (Internal use) */
	VDEC_WORK_MODE_HEADER,               /**< Parse header to get current frame's prediction type (Internal use) */
	VDEC_WORK_MODE_SKIP_B_FRAME,         /**< Skip b frame (Internal use) */
	VDEC_WORK_MODE_SKIP_B_P_FRAME,       /**< Only decode i frame (Internal use) */
	VDEC_WORK_MODE_KSHM,                 /**< Decode from kshm */
	VDEC_WORK_MODE_LAST
};

enum vdec_decoding_mode {
	VIDDEC_DECODE_NORMAL,
	VIDDEC_DECODE_I_FRAME,
	VIDDEC_CAPTURE_THUMBNAIL,
};

typedef struct vdec_rls_param {
	bool closevp;
	bool fillblack;
} vdec_rls_param_t;

typedef struct vdec_dis_rect {
	struct av_area src_rect;
	struct av_area dst_rect;
} vdec_dis_rect_t;

#define VIDDEC_INIT                         _IOW(VIDEO_IOCBASE, 0, struct video_config)
#define VIDDEC_RLS                          _IOW(VIDEO_IOCBASE, 1, struct vdec_rls_param)
#define VIDDEC_PAUSE                        _IO (VIDEO_IOCBASE, 2)
#define VIDDEC_START                        _IO (VIDEO_IOCBASE, 3)
#define VIDDEC_FLUSH                        _IOW(VIDEO_IOCBASE, 4, float)
#define VIDDEC_CHECK_EOS                    _IOR(VIDEO_IOCBASE, 5, int)
#define VIDDEC_SET_STC                      _IO (VIDEO_IOCBASE, 6)  //!< _IOC_WRITE, type is uint32_t
#define VIDDEC_SET_ROTATE_MODE              _IO (VIDEO_IOCBASE, 7)  //!< _IOC_WRITE, type is rotate_type_e
#define VIDDEC_SET_MIRROR_MODE              _IO (VIDEO_IOCBASE, 8)  //!< _IOC_WRITE, type is mirror_type_e

#define VIDDEC_GET_CUR_TIME                 _IOR(VIDEO_IOCBASE, 50, int64_t)
#define VIDDEC_GET_STATUS                   _IOR(VIDEO_IOCBASE, 51, struct vdec_decore_status)

#define VIDDEC_SET_DISPLAY_RECT             _IOW(VIDEO_IOCBASE, 100, struct vdec_dis_rect)
#define VIDDEC_START_CAPTURE_ONE_FRAME      _IO (VIDEO_IOCBASE, 101)
#define VIDDEC_STOP_CAPTURE_ONE_FRAME       _IO (VIDEO_IOCBASE, 102)
#define VIDDEC_SET_COMBINED_REGION_FREEZE   _IOW(VIDEO_IOCBASE, 103, struct combine_region_freeze_cfg)

#define VIDDEC_NOTIFY_UNDERRUN              _IO (VIDEO_IOCBASE, 104)
#define VIDDEC_SET_FRAMERATE                _IO (VIDEO_IOCBASE, 105)
#define VIDDEC_SET_SHOW_MASAIC_ON_ERR       _IO (VIDEO_IOCBASE, 106)

#endif /* _HCUAPI_VIDDEC_H_ */
