#ifndef _HCUAPI_VIDSINK_H_
#define _HCUAPI_VIDSINK_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/pixfmt.h>
#include <hcuapi/vidmp.h>

struct vframe_info {
	/* pixel format defined in enum AVPixelFormat */
	enum FFPixelFormat pixfmt;

	/* picture width & height */
	int16_t width, height;

	/* original width & height of video source */
	int16_t src_width, src_height;

	/**
	 * this is filled in with a pointer to the locked pixels, appropriately offset by the locked area;
	 * sizeof(pixels[i]) = pitch[i] * height;
	 */
	uint8_t *pixels[4];

	/* this is filled in with the pitch of the locked pixels; the pitch is the length of one row in bytes */
	int pitch[4];

	enum IMG_DIS_MODE mode;

	enum ROTATE_TYPE angle;

	enum MIRROR_TYPE mirror;

	image_effect_t img_effect;

	/*
	 * enable preview
	 */
	struct av_area src_area;
	struct av_area dst_area;
	bool preview_enable;

	/*
	 * back ground effection, default: enable
	 */
	bool bg_disable;

	struct video_transcode_config transcode_config;
};

#define VIDSINK_DISPLAY_FRAME		_IOW(VIDSINK_IOCBASE, 0, struct vframe_info)
#define VIDSINK_ENABLE_IMG_EFFECT	_IO(VIDSINK_IOCBASE, 1)
#define VIDSINK_DISABLE_IMG_EFFECT	_IO(VIDSINK_IOCBASE, 2)
#define VIDSINK_CHECK_EOS		_IOR(VIDSINK_IOCBASE, 3, int)

#endif /* _HCUAPI_VIDSINK_H_ */
