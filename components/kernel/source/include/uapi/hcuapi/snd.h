#ifndef _HCUAPI_SND_H_
#define _HCUAPI_SND_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/codec_id.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

#define SND_IOCTL_GETCAP			_IOR (SND_IOCBASE, 0, struct snd_capability)
#define SND_IOCTL_HW_PARAMS			_IOW (SND_IOCBASE, 1, struct snd_pcm_params)
#define SND_IOCTL_HW_FREE			_IO  (SND_IOCBASE, 2)
#define SND_IOCTL_START				_IO  (SND_IOCBASE, 3)
#define SND_IOCTL_DROP				_IO  (SND_IOCBASE, 4)
#define SND_IOCTL_DELAY				_IOR (SND_IOCBASE, 5, snd_pcm_uframes_t)
#define SND_IOCTL_DRAIN				_IO  (SND_IOCBASE, 6)
#define SND_IOCTL_PAUSE				_IO  (SND_IOCBASE, 7)
#define SND_IOCTL_RESUME			_IO  (SND_IOCBASE, 8)
#define SND_IOCTL_XFER				_IOWR(SND_IOCBASE, 9, struct snd_xfer)
#define SND_IOCTL_AVAIL_MIN			_IOW (SND_IOCBASE, 10, snd_pcm_uframes_t)
#define SND_IOCTL_SET_VOLUME			_IOW (SND_IOCBASE, 11, uint8_t)
#define SND_IOCTL_GET_VOLUME			_IOR (SND_IOCBASE, 12, uint8_t)
#define SND_IOCTL_SRC_SEL			_IOW (SND_IOCBASE, 13, snd_pcm_source_t)
#define SND_IOCTL_SRC_SEL_CLEAR			_IOW (SND_IOCBASE, 14, snd_pcm_source_t)
#define SND_IOCTL_SET_MUTE			_IO  (SND_IOCBASE, 15)
#define SND_IOCTL_GET_HW_INFO			_IOR (SND_IOCBASE, 16, struct snd_hw_info)
#define SND_IOCTL_SET_RECORD			_IO  (SND_IOCBASE, 17)
#define SND_IOCTL_SET_FREE_RECORD		_IO  (SND_IOCBASE, 18)
#define SND_IOCTL_SET_TWOTONE			_IOW (SND_IOCBASE, 19, struct snd_twotone)
#define SND_IOCTL_SET_LR_BALANCE		_IOW (SND_IOCBASE, 20, struct snd_lr_balance)
#define SND_IOCTL_SET_EQ6			_IOW (SND_IOCBASE, 21, struct snd_audio_eq6)
#define SND_IOCTL_SET_FLUSH_TIME		_IO (SND_IOCBASE, 22)

#define SND_EVENT_AUDIO_INFO			_IOW (SND_IOCBASE, 50, struct snd_pcm_params)
#define SND_EVENT_UNDERRUN			_IO  (SND_IOCBASE, 51)

typedef unsigned long snd_pcm_uframes_t;

typedef int snd_pcm_access_t;
#define SND_PCM_ACCESS_RW_INTERLEAVED		((snd_pcm_access_t) 0) /* readi/writei */
#define SND_PCM_ACCESS_RW_NONINTERLEAVED	((snd_pcm_access_t) 1) /* readn/writen */
#define SND_PCM_ACCESS_LAST			SND_PCM_ACCESS_RW_NONINTERLEAVED

typedef int snd_pcm_format_t;
#define	SND_PCM_FORMAT_S8			((snd_pcm_format_t) 0)
#define	SND_PCM_FORMAT_U8			((snd_pcm_format_t) 1)
#define	SND_PCM_FORMAT_S16_LE			((snd_pcm_format_t) 2)
#define	SND_PCM_FORMAT_S16_BE			((snd_pcm_format_t) 3)
#define	SND_PCM_FORMAT_U16_LE			((snd_pcm_format_t) 4)
#define	SND_PCM_FORMAT_U16_BE			((snd_pcm_format_t) 5)
#define	SND_PCM_FORMAT_S24_LE			((snd_pcm_format_t) 6) /* low three bytes */
#define	SND_PCM_FORMAT_S24_BE			((snd_pcm_format_t) 7) /* low three bytes */
#define	SND_PCM_FORMAT_U24_LE			((snd_pcm_format_t) 8) /* low three bytes */
#define	SND_PCM_FORMAT_U24_BE			((snd_pcm_format_t) 9) /* low three bytes */
#define	SND_PCM_FORMAT_S32_LE			((snd_pcm_format_t) 10)
#define	SND_PCM_FORMAT_S32_BE			((snd_pcm_format_t) 11)
#define	SND_PCM_FORMAT_U32_LE			((snd_pcm_format_t) 12)
#define	SND_PCM_FORMAT_U32_BE			((snd_pcm_format_t) 13)
#define	SND_PCM_FORMAT_FLOAT_LE			((snd_pcm_format_t) 14) /* 4-byte float, IEEE-754 32-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_FLOAT_BE			((snd_pcm_format_t) 15) /* 4-byte float, IEEE-754 32-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_FLOAT64_LE		((snd_pcm_format_t) 16) /* 8-byte float, IEEE-754 64-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_FLOAT64_BE		((snd_pcm_format_t) 17) /* 8-byte float, IEEE-754 64-bit, range -1.0 to 1.0 */
#define	SND_PCM_FORMAT_IEC958_SUBFRAME_LE	((snd_pcm_format_t) 18) /* IEC-958 subframe, Little Endian */
#define	SND_PCM_FORMAT_IEC958_SUBFRAME_BE	((snd_pcm_format_t) 19) /* IEC-958 subframe, Big Endian */
#define	SND_PCM_FORMAT_MU_LAW			((snd_pcm_format_t) 20)
#define	SND_PCM_FORMAT_A_LAW			((snd_pcm_format_t) 21)
#define	SND_PCM_FORMAT_IMA_ADPCM		((snd_pcm_format_t) 22)
#define	SND_PCM_FORMAT_MPEG			((snd_pcm_format_t) 23)
#define	SND_PCM_FORMAT_GSM			((snd_pcm_format_t) 24)
#define	SND_PCM_FORMAT_SPECIAL			((snd_pcm_format_t) 31)
#define	SND_PCM_FORMAT_S24_3LE			((snd_pcm_format_t) 32)	/* in three bytes */
#define	SND_PCM_FORMAT_S24_3BE			((snd_pcm_format_t) 33)	/* in three bytes */
#define	SND_PCM_FORMAT_U24_3LE			((snd_pcm_format_t) 34)	/* in three bytes */
#define	SND_PCM_FORMAT_U24_3BE			((snd_pcm_format_t) 35)	/* in three bytes */
#define	SND_PCM_FORMAT_S20_3LE			((snd_pcm_format_t) 36)	/* in three bytes */
#define	SND_PCM_FORMAT_S20_3BE			((snd_pcm_format_t) 37)	/* in three bytes */
#define	SND_PCM_FORMAT_U20_3LE			((snd_pcm_format_t) 38)	/* in three bytes */
#define	SND_PCM_FORMAT_U20_3BE			((snd_pcm_format_t) 39)	/* in three bytes */
#define	SND_PCM_FORMAT_S18_3LE			((snd_pcm_format_t) 40)	/* in three bytes */
#define	SND_PCM_FORMAT_S18_3BE			((snd_pcm_format_t) 41)	/* in three bytes */
#define	SND_PCM_FORMAT_U18_3LE			((snd_pcm_format_t) 42)	/* in three bytes */
#define	SND_PCM_FORMAT_U18_3BE			((snd_pcm_format_t) 43)	/* in three bytes */
#define	SND_PCM_FORMAT_G723_24			((snd_pcm_format_t) 44) /* 8 samples in 3 bytes */
#define	SND_PCM_FORMAT_G723_24_1B		((snd_pcm_format_t) 45) /* 1 sample in 1 byte */
#define	SND_PCM_FORMAT_G723_40			((snd_pcm_format_t) 46) /* 8 Samples in 5 bytes */
#define	SND_PCM_FORMAT_G723_40_1B		((snd_pcm_format_t) 47) /* 1 sample in 1 byte */
#define	SND_PCM_FORMAT_DSD_U8			((snd_pcm_format_t) 48) /* DSD, 1-byte samples DSD (x8) */
#define	SND_PCM_FORMAT_DSD_U16_LE		((snd_pcm_format_t) 49) /* DSD, 2-byte samples DSD (x16), little endian */
#define	SND_PCM_FORMAT_RAW			((snd_pcm_format_t) 50) /* bitstream raw data */
#define	SND_PCM_FORMAT_LAST			SND_PCM_FORMAT_RAW

#define	SND_PCM_FORMAT_S16			SND_PCM_FORMAT_S16_LE
#define	SND_PCM_FORMAT_U16			SND_PCM_FORMAT_U16_LE
#define	SND_PCM_FORMAT_S24			SND_PCM_FORMAT_S24_LE
#define	SND_PCM_FORMAT_U24			SND_PCM_FORMAT_U24_LE
#define	SND_PCM_FORMAT_S32			SND_PCM_FORMAT_S32_LE
#define	SND_PCM_FORMAT_U32			SND_PCM_FORMAT_U32_LE
#define	SND_PCM_FORMAT_FLOAT			SND_PCM_FORMAT_FLOAT_LE
#define	SND_PCM_FORMAT_FLOAT64			SND_PCM_FORMAT_FLOAT64_LE
#define	SND_PCM_FORMAT_IEC958_SUBFRAME		SND_PCM_FORMAT_IEC958_SUBFRAME_LE

#define SND_PCM_RATE_5512			(1<<0)		/* 5512Hz */
#define SND_PCM_RATE_8000			(1<<1)		/* 8000Hz */
#define SND_PCM_RATE_11025			(1<<2)		/* 11025Hz */
#define SND_PCM_RATE_12000			(1<<3)		/* 11025Hz */
#define SND_PCM_RATE_16000			(1<<4)		/* 16000Hz */
#define SND_PCM_RATE_22050			(1<<5)		/* 22050Hz */
#define SND_PCM_RATE_24000			(1<<6)		/* 22050Hz */
#define SND_PCM_RATE_32000			(1<<7)		/* 32000Hz */
#define SND_PCM_RATE_44100			(1<<8)		/* 44100Hz */
#define SND_PCM_RATE_48000			(1<<9)		/* 48000Hz */
#define SND_PCM_RATE_64000			(1<<10)		/* 64000Hz */
#define SND_PCM_RATE_88200			(1<<11)		/* 88200Hz */
#define SND_PCM_RATE_96000			(1<<12)		/* 96000Hz */
#define SND_PCM_RATE_128000			(1<<13)		/* 96000Hz */
#define SND_PCM_RATE_176400			(1<<14)		/* 176400Hz */
#define SND_PCM_RATE_192000			(1<<15)		/* 192000Hz */

#define SND_PCM_RATE_8000_44100                                                \
	(SND_PCM_RATE_8000 | SND_PCM_RATE_11025 | SND_PCM_RATE_12000 |         \
	 SND_PCM_RATE_16000 | SND_PCM_RATE_22050 | SND_PCM_RATE_24000 |        \
	 SND_PCM_RATE_32000 | SND_PCM_RATE_44100)
#define SND_PCM_RATE_8000_48000 (SND_PCM_RATE_8000_44100 | SND_PCM_RATE_48000)
#define SND_PCM_RATE_8000_96000                                                \
	(SND_PCM_RATE_8000_48000 | SND_PCM_RATE_64000 | SND_PCM_RATE_88200 |   \
	 SND_PCM_RATE_96000)
#define SND_PCM_RATE_8000_192000                                               \
	(SND_PCM_RATE_8000_96000 | SND_PCM_RATE_176400 | SND_PCM_RATE_192000)

#define _SND_PCM_FMTBIT(fmt)			(1ULL << (int)SND_PCM_FORMAT_##fmt)

#define SND_PCM_FMTBIT_S8			_SND_PCM_FMTBIT(S8)
#define SND_PCM_FMTBIT_U8			_SND_PCM_FMTBIT(U8)
#define SND_PCM_FMTBIT_S16_LE			_SND_PCM_FMTBIT(S16_LE)
#define SND_PCM_FMTBIT_S16_BE			_SND_PCM_FMTBIT(S16_BE)
#define SND_PCM_FMTBIT_U16_LE			_SND_PCM_FMTBIT(U16_LE)
#define SND_PCM_FMTBIT_U16_BE			_SND_PCM_FMTBIT(U16_BE)
#define SND_PCM_FMTBIT_S24_LE			_SND_PCM_FMTBIT(S24_LE)
#define SND_PCM_FMTBIT_S24_BE			_SND_PCM_FMTBIT(S24_BE)
#define SND_PCM_FMTBIT_U24_LE			_SND_PCM_FMTBIT(U24_LE)
#define SND_PCM_FMTBIT_U24_BE			_SND_PCM_FMTBIT(U24_BE)
#define SND_PCM_FMTBIT_S32_LE			_SND_PCM_FMTBIT(S32_LE)
#define SND_PCM_FMTBIT_S32_BE			_SND_PCM_FMTBIT(S32_BE)
#define SND_PCM_FMTBIT_U32_LE			_SND_PCM_FMTBIT(U32_LE)
#define SND_PCM_FMTBIT_U32_BE			_SND_PCM_FMTBIT(U32_BE)
#define SND_PCM_FMTBIT_FLOAT_LE			_SND_PCM_FMTBIT(FLOAT_LE)
#define SND_PCM_FMTBIT_FLOAT_BE			_SND_PCM_FMTBIT(FLOAT_BE)
#define SND_PCM_FMTBIT_FLOAT64_LE		_SND_PCM_FMTBIT(FLOAT64_LE)
#define SND_PCM_FMTBIT_FLOAT64_BE		_SND_PCM_FMTBIT(FLOAT64_BE)
#define SND_PCM_FMTBIT_IEC958_SUBFRAME_LE	_SND_PCM_FMTBIT(IEC958_SUBFRAME_LE)
#define SND_PCM_FMTBIT_IEC958_SUBFRAME_BE	_SND_PCM_FMTBIT(IEC958_SUBFRAME_BE)
#define SND_PCM_FMTBIT_MU_LAW			_SND_PCM_FMTBIT(MU_LAW)
#define SND_PCM_FMTBIT_A_LAW			_SND_PCM_FMTBIT(A_LAW)
#define SND_PCM_FMTBIT_IMA_ADPCM		_SND_PCM_FMTBIT(IMA_ADPCM)
#define SND_PCM_FMTBIT_MPEG			_SND_PCM_FMTBIT(MPEG)
#define SND_PCM_FMTBIT_GSM			_SND_PCM_FMTBIT(GSM)
#define SND_PCM_FMTBIT_SPECIAL			_SND_PCM_FMTBIT(SPECIAL)
#define SND_PCM_FMTBIT_S24_3LE			_SND_PCM_FMTBIT(S24_3LE)
#define SND_PCM_FMTBIT_U24_3LE			_SND_PCM_FMTBIT(U24_3LE)
#define SND_PCM_FMTBIT_S24_3BE			_SND_PCM_FMTBIT(S24_3BE)
#define SND_PCM_FMTBIT_U24_3BE			_SND_PCM_FMTBIT(U24_3BE)
#define SND_PCM_FMTBIT_S20_3LE			_SND_PCM_FMTBIT(S20_3LE)
#define SND_PCM_FMTBIT_U20_3LE			_SND_PCM_FMTBIT(U20_3LE)
#define SND_PCM_FMTBIT_S20_3BE			_SND_PCM_FMTBIT(S20_3BE)
#define SND_PCM_FMTBIT_U20_3BE			_SND_PCM_FMTBIT(U20_3BE)
#define SND_PCM_FMTBIT_S18_3LE			_SND_PCM_FMTBIT(S18_3LE)
#define SND_PCM_FMTBIT_U18_3LE			_SND_PCM_FMTBIT(U18_3LE)
#define SND_PCM_FMTBIT_S18_3BE			_SND_PCM_FMTBIT(S18_3BE)
#define SND_PCM_FMTBIT_U18_3BE			_SND_PCM_FMTBIT(U18_3BE)
#define SND_PCM_FMTBIT_G723_24			_SND_PCM_FMTBIT(G723_24)
#define SND_PCM_FMTBIT_G723_24_1B		_SND_PCM_FMTBIT(G723_24_1B)
#define SND_PCM_FMTBIT_G723_40			_SND_PCM_FMTBIT(G723_40)
#define SND_PCM_FMTBIT_G723_40_1B		_SND_PCM_FMTBIT(G723_40_1B)
#define SND_PCM_FMTBIT_DSD_U8			_SND_PCM_FMTBIT(DSD_U8)
#define SND_PCM_FMTBIT_DSD_U16_LE		_SND_PCM_FMTBIT(DSD_U16_LE)
#define SND_PCM_FMTBIT_RAW			_SND_PCM_FMTBIT(RAW)

#define SND_PCM_FMTBIT_S16			SND_PCM_FMTBIT_S16_LE
#define SND_PCM_FMTBIT_U16			SND_PCM_FMTBIT_U16_LE
#define SND_PCM_FMTBIT_S24			SND_PCM_FMTBIT_S24_LE
#define SND_PCM_FMTBIT_U24			SND_PCM_FMTBIT_U24_LE
#define SND_PCM_FMTBIT_S32			SND_PCM_FMTBIT_S32_LE
#define SND_PCM_FMTBIT_U32			SND_PCM_FMTBIT_U32_LE
#define SND_PCM_FMTBIT_FLOAT			SND_PCM_FMTBIT_FLOAT_LE
#define SND_PCM_FMTBIT_FLOAT64			SND_PCM_FMTBIT_FLOAT64_LE
#define SND_PCM_FMTBIT_IEC958_SUBFRAME		SND_PCM_FMTBIT_IEC958_SUBFRAME_LE

typedef uint32_t snd_pcm_source_t;
/* i2s sources */
#define SND_PCM_SOURCE_AUDPAD			0
#define SND_PCM_SOURCE_HDMIRX			1
/* spo sources */
#define SND_SPO_SOURCE_I2SODMA			0
#define SND_SPO_SOURCE_SPODMA			1
#define SND_SPO_SOURCE_HDMI_RX			2

typedef uint32_t snd_pcm_dest_t;
#define SND_PCM_DEST_DMA			0
#define SND_PCM_DEST_BYPASS			1

#define SND_PCM_SOURCEBIT_AUDPAD		(1 << SND_PCM_SOURCE_AUDPAD)
#define SND_PCM_SOURCEBIT_HDMIRX		(1 << SND_PCM_SOURCE_HDMIRX)

enum {
	SND_PCM_ALIGN_LEFT = 0,
	SND_PCM_ALIGN_RIGHT,
};

struct snd_capability {
	/* SND_PCM_RATE_* */
	uint32_t rates;

	/* SND_PCM_FMTBIT_* */
	uint64_t formats;
};

struct snd_pcm_params {
	/* SND_PCM_ACCESS_* */
	snd_pcm_access_t access;

	/* SND_PCM_FORMAT_* */
	snd_pcm_format_t format;

	/* HC_AVCODEC_ID_* */
	enum HCAVCodecID codec_id;

	/* rate in Hz */
	unsigned int rate;

	/* SND_PCM_ALIGN_* */
	uint8_t align;

	/* SND_SYNC_MODE_* */
	uint8_t sync_mode;

	/*
	 * dmabuf size(bytes) = channels * period_size * periods
	 */

	uint8_t channels;

	/*
	 * 8/16/32
	 * Do not support SND_PCM_FMTBIT_*_3LE
	 */
	uint8_t bitdepth;

	/*
	 * Number of periods to auto-trigger start
	 */
	uint16_t start_threshold;

	/*
	 * pcm data source switch
	 */
	snd_pcm_source_t pcm_source;
	snd_pcm_dest_t pcm_dest;

	/*
	 * byte size of one period, must be 32 bytes aligned
	 */
	uint32_t period_size;
	uint32_t periods;

	uint32_t audio_flush_thres;
};

struct snd_xfer {
	void *data;
	snd_pcm_uframes_t frames;
	uint32_t tstamp_ms;
};

struct snd_hw_info {
	uint32_t dma_addr;
	uint32_t dma_size;
	struct snd_pcm_params pcm_params;
	uint32_t i2si_wr_idx_register;
	uint32_t i2si_rd_idx_register;
};

typedef enum SND_TWOTONE_MODE {
	SND_TWOTONE_MODE_STANDARD = 0,
	SND_TWOTONE_MODE_MUSIC,
	SND_TWOTONE_MODE_MOVIE,
	SND_TWOTONE_MODE_SPORT,
	SND_TWOTONE_MODE_USER,
} snd_twotone_mode_e;

typedef enum SND_EQ6_MODE {
	SND_EQ6_MODE_OFF = 0,
	SND_EQ6_MODE_NORMAL,
	SND_EQ6_MODE_CLASSIC,
	SND_EQ6_MODE_JAZZ,
	SND_EQ6_MODE_ROCK,
	SND_EQ6_MODE_POP,
} snd_eq6_mode_e;

struct snd_twotone {
	int onoff;		/* 0: off; 1: on */

	snd_twotone_mode_e tt_mode;

	/*
	 * user specify param is valid only when tt_mode is SND_TWOTONE_MODE_USER
	 */
	int bass_index;		/* range: [ -10, 10 ] */
	int treble_index;	/* range: [ -10, 10 ] */
};

struct snd_lr_balance {
	int onoff;		/* 0: off; 1: on */
	int lr_balance_index;	/* range: [ -24, 24 ] */
};

struct snd_audio_eq6 {
	int onoff;		/* 0: off; 1: on */
	snd_eq6_mode_e mode;
};

#endif	/* _HCUAPI_SND_H_ */
