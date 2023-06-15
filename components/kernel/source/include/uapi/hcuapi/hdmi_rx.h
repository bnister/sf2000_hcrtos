#ifndef __HCUAPI_HDMI_RX_H_
#define __HCUAPI_HDMI_RX_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/kshm.h>
#include <hcuapi/jpeg_enc.h>

#define EDID_DATA_LEN 256
#define HDCP_KEY_LEN 286

typedef enum HDMI_RX_VIDEO_DATA_PATH {
	HDMI_RX_VIDEO_TO_HDMI_TX,
	HDMI_RX_VIDEO_TO_DE,
	HDMI_RX_VIDEO_TO_OSD,
	HDMI_RX_VIDEO_TO_KSHM,
	HDMI_RX_VIDEO_TO_DE_ROTATE,
	HDMI_RX_VIDEO_TO_DE_AND_KSHM,
	HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM,
} hdmi_rx_video_data_path_e;

#if 0
/*
 * Deprecated
 */
typedef enum HDMI_RX_AUDIO_DATA_PATH {
	HDMI_RX_AUDIO_TO_HDMI_TX,		//<! bypass to hdmi tx
	HDMI_RX_AUDIO_TO_I2SO,			//<! bypass to i2so
	HDMI_RX_AUDIO_TO_SPDIF_OUT,		//<! bypass to spdif out
	HDMI_RX_AUDIO_TO_I2SI,
	HDMI_RX_AUDIO_TO_I2SI_AND_BYPASS_TO_HDMI_TX,
	HDMI_RX_AUDIO_TO_I2SI_AND_I2SO,
	HDMI_RX_AUDIO_TO_SPDIF_IN,
	HDMI_RX_AUDIO_TO_SPDIF_IN_AND_BYPASS_TO_HDMI_TX,
	HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO,
} hdmi_rx_audio_data_path_e;

/*
 * Just for backward compatible
 */
#define HDMI_RX_AUDIO_TO_KSHM				HDMI_RX_AUDIO_TO_I2SI
#define HDMI_RX_AUDIO_TO_KSHM_AND_HDMI_TX		HDMI_RX_AUDIO_TO_I2SI_AND_BYPASS_TO_HDMI_TX
#endif

/*
 * NOTE: All the pathes with 'bypass' are not able to set/adjust the output volume
 */
typedef enum HDMI_RX_AUDIO_DATA_PATH {
	/* bypass output group */
	HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX = 0,		//<! bypass to hdmi tx
	HDMI_RX_AUDIO_BYPASS_TO_I2SO,			//<! bypass to i2so
	HDMI_RX_AUDIO_BYPASS_TO_SPDIF_OUT,		//<! bypass to spdif out

	/* output via i2si group */
	HDMI_RX_AUDIO_TO_I2SI_AND_KSHM = 100,
	HDMI_RX_AUDIO_TO_I2SI_AND_KSHM_AND_BYPASS_TO_HDMI_TX,
	HDMI_RX_AUDIO_TO_I2SI_AND_KSHM_AND_I2SO,
	HDMI_RX_AUDIO_TO_I2SI_AND_I2SO,

	/* output via spdif-in group */
	HDMI_RX_AUDIO_TO_SPDIF_IN_AND_KSHM = 200,
	HDMI_RX_AUDIO_TO_SPDIF_IN_AND_KSHM_AND_BYPASS_TO_HDMI_TX,
	HDMI_RX_AUDIO_TO_SPDIF_IN_AND_KSHM_AND_I2SO,
	HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO,
} hdmi_rx_audio_data_path_e;

typedef enum HDMI_RX_VIDEO_COLOR_FORMAT {
	HDMI_RX_VIDEO_FORMAT_RGB,
	HDMI_RX_VIDEO_FORMAT_422,
	HDMI_RX_VIDEO_FORMAT_444,
	HDMI_RX_VIDEO_FORMAT_NUM,
} hdmi_rx_video_color_format_e;

typedef enum HDMI_RX_DATA_RANGE {
	HDMI_RX_DATA_RANGE_FULL,		//!< Full range is  [0, 255]
	HDMI_RX_DATA_RANGE_LIMIT,		//!< Limit range is [16,235]
} hdmi_rx_data_range_e;

typedef struct hdmi_rx_video_info {
	enum HDMI_RX_VIDEO_COLOR_FORMAT color_format;
	int width;
	int height;
	hdmi_rx_data_range_e range;
	int frame_rate;
	int b_progressive;
} hdmi_rx_video_info_t;

enum {
	/* screen: unblanked */
	HDMI_RX_VIDEO_BLANK_UNBLANK,

	/* screen: blanked */
	HDMI_RX_VIDEO_BLANK_NORMAL,
};

typedef struct hdmi_rx_hdcp_key {
	uint8_t hdcp_key[HDCP_KEY_LEN];
	uint8_t b_encrypted;
	uint32_t seed;
} hdmi_rx_hdcp_key_t;

typedef struct hdmi_rx_edid_data {
	uint8_t edid_data[EDID_DATA_LEN];
} hdmi_rx_edid_data_t;

#define HDMI_RX_START				_IO (HDMI_RX_IOCBASE, 0)
#define HDMI_RX_STOP				_IO (HDMI_RX_IOCBASE, 1)
#define HDMI_RX_SET_VIDEO_DATA_PATH		_IO(HDMI_RX_IOCBASE, 2)
#define HDMI_RX_SET_AUDIO_DATA_PATH		_IO(HDMI_RX_IOCBASE, 3)
#define HDMI_RX_VIDEO_KSHM_ACCESS		_IOR(HDMI_RX_IOCBASE, 4, struct kshm_info)
#define HDMI_RX_AUDIO_KSHM_ACCESS		_IOR(HDMI_RX_IOCBASE, 5, struct kshm_info)
#define HDMI_RX_SET_VIDEO_ROTATE_MODE		_IO (HDMI_RX_IOCBASE, 6)
#define HDMI_RX_SET_VIDEO_STOP_MODE		_IO (HDMI_RX_IOCBASE, 7) //<! param: Keep last frame: 1, Black screen: 0

#define HDMI_RX_NOTIFY_CONNECT			_IO (HDMI_RX_IOCBASE, 8)
#define HDMI_RX_NOTIFY_DISCONNECT		_IO (HDMI_RX_IOCBASE, 9)

#define HDMI_RX_GET_VIDEO_INFO			_IOR(HDMI_RX_IOCBASE, 10, struct hdmi_rx_video_info)
#define HDMI_RX_VIDEO_BLANK			_IO (HDMI_RX_IOCBASE, 11) //<! param: HDMI_RX_VIDEO_BLANK_*
#define HDMI_RX_NOTIFY_ERR_INPUT		_IO (HDMI_RX_IOCBASE, 12)
#define HDMI_RX_SET_HDCP_KEY			_IOW(HDMI_RX_IOCBASE, 13, struct hdmi_rx_hdcp_key)
/*
 * HDMI_RX_SET_VIDEO_MIRROR_MODE is only valid when
 * VPATH       = HDMI_RX_VIDEO_TO_DE_ROTATE or HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM
 * MIRROR_TYPE = MIRROR_TYPE_LEFTRIGHT
 *
 * In the other cases, HDMI_RX_SET_VIDEO_MIRROR_MODE is invalid and will won't take effect
 */
#define HDMI_RX_SET_VIDEO_MIRROR_MODE		_IO (HDMI_RX_IOCBASE, 14)
#define HDMI_RX_SET_EDID			_IOW(HDMI_RX_IOCBASE, 15, struct hdmi_rx_edid_data)
#define HDMI_RX_SET_BUF_YUV2RGB_ONOFF		_IO (HDMI_RX_IOCBASE, 16) //<! param: On: 1, Off: 0
#define HDMI_RX_SET_VIDEO_ENC_QUALITY		_IO (HDMI_RX_IOCBASE, 17) //<! param: jpeg_enc_quality_type_e
#define HDMI_RX_PAUSE				_IO (HDMI_RX_IOCBASE, 18) //<! Keep the HDMI RX connection, but don't receive data
#define HDMI_RX_RESUME				_IO (HDMI_RX_IOCBASE, 19) //<! Resume receiving data from HDMI RX if the interface is paused. It is faster

/*
 * if HDMI_RX_SET_VIDEO_ENC_QUALITY is JPEG_ENC_QUALITY_TYPE_USER_DEFINE,
 * need use this api to set enc quant table
*/
#define HDMI_RX_SET_ENC_QUANT			_IOW(HDMI_RX_IOCBASE, 20, struct jpeg_enc_quant)

#endif /* __HCUAPI_HDMI_RX_H_ */
