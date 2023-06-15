#ifndef _HCUAPI_AUDDEC_H_
#define _HCUAPI_AUDDEC_H_

#include <hcuapi/iocbase.h>

#define AUDDEC_INIT			_IOW (AUDIO_IOCBASE, 1, struct audio_config)
#define AUDDEC_RLS			_IO  (AUDIO_IOCBASE, 2)
#define AUDDEC_PAUSE			_IO  (AUDIO_IOCBASE, 3)
#define AUDDEC_START			_IO  (AUDIO_IOCBASE, 4)
#define AUDDEC_FLUSH			_IO  (AUDIO_IOCBASE, 5)
#define AUDDEC_DRAIN			_IO  (AUDIO_IOCBASE, 6)
#define AUDDEC_CHECK_EOS		_IOR (AUDIO_IOCBASE, 8, int)
#define AUDDEC_GET_CUR_TIME		_IOR (AUDIO_IOCBASE, 10, int64_t)
#define AUDDEC_GET_STATUS		_IOR (AUDIO_IOCBASE, 11, struct audio_decore_status)

#define AUDDEC_SET_BASE_TIME		_IOW (AUDIO_IOCBASE, 21, unsigned long)
#define AUDDEC_CHANGE_SYNC_TYPE		_IOW (AUDIO_IOCBASE, 22, enum AVSYNC_TYPE)
#define AUDDEC_GET_CAPABILITIES		_IOR (AUDIO_IOCBASE, 23, unsigned int)
#define AUDDEC_SET_FLUSH_TIME		_IO (AUDIO_IOCBASE, 24)

#define AUDIO_SET_MUTE			_IOW (AUDIO_IOCBASE, 200, unsigned int)
#define AUDIO_SET_VOLUME		_IOW (AUDIO_IOCBASE, 201, uint8_t)
#define AUDIO_GET_VOLUME		_IOR (AUDIO_IOCBASE, 202, uint8_t)
#define AUDIO_SET_BYPASS_MODE		_IOW (AUDIO_IOCBASE, 203, int)
#define AUDIO_CHANNEL_SELECT		_IOW (AUDIO_IOCBASE, 204, audio_channel_select_t)
#define AUDIO_GET_STATUS		_IOR (AUDIO_IOCBASE, 205, audio_status_t)
#define AUDIO_GET_UNDERRUN_TIMES	_IOR (AUDIO_IOCBASE, 206, unsigned int) //!< Get audio underrun times

typedef enum {
	AUDIO_STOPPED,		//!< Device is stopped
	AUDIO_PLAYING,		//!< Device is currently playing
	AUDIO_PAUSED		//!< Device is paused
} audio_play_state_t;

typedef enum {
	AUDIO_STEREO,
	AUDIO_MONO_LEFT,
	AUDIO_MONO_RIGHT,
	AUDIO_MONO,
	AUDIO_STEREO_SWAPPED
} audio_channel_select_t;

typedef struct audio_status {
	/* sync audio and video? */
	uint8_t AV_sync_state;
	/* audio is muted */
	uint8_t mute_state;
	/* audio_play_state_t */
	uint8_t play_state;
	/* audio_channel_select_t */
	uint8_t channel_select;
	/* pass on audio data to */
	uint8_t bypass_mode;
} audio_status_t;     /* separate decoder hardware */

struct audio_config
{
	uint8_t decode_mode;
	uint8_t sync_mode;
	uint8_t bits_per_coded_sample;	//!< bitdepth
	uint8_t channels;
	uint32_t codec_id;
	uint32_t codec_tag;
	uint32_t sample_rate;
	uint32_t bit_rate;
	uint32_t block_align;
	uint32_t snd_devs;
	int audio_flush_thres;

	unsigned char extra_data[512];//mode 0
	void *extradata;//mode 2;
	uint32_t codec_frame_size;
	uint32_t extradata_size;
	unsigned char extradata_mode;	//!< 1: pass extra-data via kshm; 0: pass extra-data via array extra_data[]
	unsigned char bypass;
	int kshm_size;

	/**
	 * Channel layout of the audio data.
	 */
	uint64_t channel_layout;
	int buffering_start;
	int buffering_end;
	int enable_audsink;
} __attribute__((aligned(8)));

/*! @struct audio_decore_status
@brief A structure defines current audio decode status.
*/
struct audio_decore_status
{
	uint32_t sample_rate;		//!< Audio sample rate.
	uint8_t channels;		//!< Audio channel numbers.
	uint8_t bits_per_sample;	//!< Bit numbers per one sample.
	uint8_t first_header_got;	//!< Not used any more.
	uint8_t first_header_parsed;	//!< Not used any more.
	uint32_t frames_decoded;	//!< The frame numbers have been decoded.
};

#endif /* _HCUAPI_AUDDEC_H_ */

