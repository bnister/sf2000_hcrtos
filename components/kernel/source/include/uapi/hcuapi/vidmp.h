#ifndef _VID_MP_H_
#define _VID_MP_H_

#ifndef __KERNEL__
#include <stdint.h>
#endif
#include <stdbool.h>

typedef struct av_area {
	uint16_t x;		//!< Horizontal start point.
	uint16_t y;		//!< Vertical start point.
	uint16_t w;		//!< Horizontal size.
	uint16_t h;		//!< Vertical size.
} av_area_t;

typedef enum IMG_SHOW_MODE {
	IMG_SHOW_NULL,
	IMG_SHOW_NORMAL,	//!< No display style
	IMG_SHOW_SHUTTERS,	//!< Use Shutter style
	IMG_SHOW_BRUSH,		//!< Use Brush style
	IMG_SHOW_SLIDE,		//!< Use Slide style: new picture slide in and cover on old picture
	IMG_SHOW_RANDOM,	//!< Use Random style
	IMG_SHOW_FADE,		//!< Use Fade style
	IMG_SHOW_SLIDE2,	//!< Use Slide style: new picture slide in with old picture slide out
	IMG_SHOW_MODE_MAX
} img_show_mode_e;

typedef struct img_show_shutters {
	uint8_t			direction;	//!< 0 : horizontal 1: vertical
	uint8_t			type;
	uint16_t		time;		//!< Interval time, unit:ms
} img_show_shutters_t,*pimg_show_shutters;

typedef struct img_show_brush {
	uint8_t			direction; 	//!< 0 : from left to right 1: from top to bottom
	uint8_t			type;
	uint16_t		time;		//!< Interval time, unit:ms
} img_show_brush_t,*pimg_show_brush;

typedef struct img_show_slide {
	uint8_t			direction;	//!< 0 : from left to right 1: from top to bottom
	uint8_t			type;
	uint16_t		time;		//!< Interval time, unit:ms
} img_show_slide_t,*pimg_show_slide;

typedef struct img_show_random
{
	uint8_t			type;		//!< 0: random block operation
	uint16_t		time;		//!< Interval time, unit:ms
} img_show_random_t,*pimg_show_random;

typedef struct img_show_fade
{
	uint8_t			type;	//!< Only support the condition when Type is set to 0
	uint16_t		time;	//!< Interval time, unit:ms
} img_show_fade_t,*pimg_show_fade;

typedef union {
	img_show_shutters_t	shuttles_param;
	img_show_brush_t	brush_param;
	img_show_slide_t	slide_param;
	img_show_random_t	random_param;
	img_show_fade_t		fade_param;
} mode_param_t;

typedef struct image_effect {
	img_show_mode_e		mode;
	mode_param_t		mode_param;
} image_effect_t;

typedef enum IMG_DIS_MODE {
	IMG_DIS_FULLSCREEN = 1,		//!< Fullscreen mode, adapt to screen's width/height
	IMG_DIS_REALSIZE,		//!< Realize mode
	IMG_DIS_THUMBNAIL,		//!< Thumbnail mode
	IMG_DIS_AUTO,			//!< Auto mode, not supported yet
	IMG_DIS_CUSTON_SIZE,		//!< Customized size mode, not supported yet
	IMG_DIS_MULTI_PIC,		//!< Multipicture mode
	IMG_DIS_SCALE,			//!< Scale img with un-changed ratio of img-width:img-height
	IMG_DIS_CROP,			//!<Crop out part of the picture
} img_dis_mode_e;

typedef enum ROTATE_TYPE {
	/* Clockwise rotate 顺时针旋转*/
	ROTATE_TYPE_0,
	ROTATE_TYPE_90,
	ROTATE_TYPE_180,
	ROTATE_TYPE_270,
} rotate_type_e;

typedef enum MIRROR_TYPE {
	MIRROR_TYPE_NONE,
	MIRROR_TYPE_LEFTRIGHT,
	MIRROR_TYPE_UPDOWN
} mirror_type_e;

typedef enum JPG_SCAN_TYPE {
	YUV444_YH1V1,
	YUV422_YH2V1,
	YUV420_YH1V2,
	YUV420_YH2V2,
	YUV411_YH4V1,
} jpg_scan_type_e;

typedef enum VIDDEC_SCALE_FACTOR {
	VIDDEC_SCACLE_OFF, //
	VIDDEC_SCACLE_1_2, //1/2
	VIDDEC_SCACLE_1_4, //1/4
	VIDDEC_SCACLE_1_8, //1/8
} viddec_scale_factor_e;

struct video_transcode_config {
	bool b_enable;
	bool b_show;
	bool b_scale;
	bool b_capture_one;
	viddec_scale_factor_e scale_factor;
};

typedef enum COMBINE_REGION {
	E_COMBINE_REGION_1 = 0,
	E_COMBINE_REGION_2,
	E_COMBINE_REGION_3,
	E_COMBINE_REGION_4,
	E_COMBINE_REGION_ALL,
} combine_region_e;

typedef enum COMBINE_REGION_STATUS {
	REGION_STATUS_UNFREEZE,
	REGION_STATUS_FREEZE
} combine_region_status_e;

typedef struct combine_region_freeze_cfg {
	combine_region_e region;
	combine_region_status_e status;
} combine_region_freeze_cfg_t;

struct video_config {
	uint32_t        codec_id;
	uint32_t        codec_tag;       //!< Additional information about the codec (corresponds to the FOURCC).

	int8_t          independent_url;    //!< audio and video use independent url.
	int8_t          combine_enable;
	int8_t          sync_mode;       //!< enum avsync_sync_mode
	int8_t          decode_mode;     //!< enum vdec_work_mode
	int8_t          decoder_flag;    //!< Decode flag
	int8_t          rotate_by_cfg;   //!< bool, if true, use rotate & mirror args from init cfgs, else  from pkt.
	int8_t          rotate_enable;   //!< bool, enable rotate and mirror setting
	int8_t          preview;         //!< bool, Whether in preview
	int8_t          b_aux_layer;
	int8_t          extradata_mode;
	uint32_t        frame_rate;      //!< Frame rate
	int32_t         pic_width;       //!< Picture width
	int32_t         pic_height;      //!< Picture height
	int32_t         pixel_aspect_x;  //!< Pixel aspect raio width
	int32_t         pixel_aspect_y;  //!< Pixel aspect raio height
	unsigned char   extra_data[512];  //!< used when extradata_mode=0
	void *          extradata;       //!< used when extradata_mode=1 or 2, 2:from main cpu, need flush cache.
	int             extradata_size;
	int             codec_frame_size;
	struct av_area  src_area;
	struct av_area  dst_area;
	image_effect_t  img_effect;
	int             quick_mode;
	img_dis_mode_e  img_dis_mode;
	mirror_type_e   mirror_type;
	rotate_type_e   rotate_type;
	int             bit_rate;
	int             kshm_size;
	struct video_transcode_config transcode_config;
	int             buffering_start;
	int             buffering_end;
	jpg_scan_type_e scan_type;
}__attribute__((aligned(4)));

struct vdec_decore_status {
	uint32_t		decode_status;
	uint32_t		pic_width;
	uint32_t		pic_height;
	uint32_t		sar_width;
	uint32_t		sar_height;
	uint32_t		frame_rate;
	int32_t			interlaced_frame;
	int32_t			top_field_first;
	int32_t			first_header_got;
	int32_t			first_pic_showed;
	uint32_t		frames_decoded;
	uint32_t		frames_displayed;
	int64_t			frame_last_pts;
	uint32_t		buffer_size;
	uint32_t		buffer_used;
	uint32_t		decode_error;
	uint32_t		decoder_feature;
	uint32_t		under_run_cnt;
	uint32_t		output_mode;
	int32_t			first_pic_decoded;	//!< Whether the first picture is decoded
	uint32_t		frame_angle;		//!< Decoder's output frame angle
	int64_t			first_i_frame_pts;	//!< First I frame's pts
	uint8_t			layer;
	uint8_t			get_pkt_eos;
};

#endif
