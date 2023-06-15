#ifndef _HCUAPI_AUDSINK_H_
#define _HCUAPI_AUDSINK_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/snd.h>
#include <hcuapi/avsync.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

#define AUDSINK_IOCTL_INIT		_IOW(AUDSINK_IOCBASE, 0, struct audsink_init_params)
#define AUDSINK_IOCTL_RESETBUF		_IO (AUDSINK_IOCBASE, 1)
#define AUDSINK_IOCTL_SETDUPLICATE	_IO (AUDSINK_IOCBASE, 2)
#define AUDSINK_IOCTL_GETDUPLICATE	_IOR(AUDSINK_IOCBASE, 3, int)
#define AUDSINK_IOCTL_XFER		_IOW(AUDSINK_IOCBASE, 4, struct audsink_xfer)
#define AUDSINK_IOCTL_DELAY		_IOR(AUDSINK_IOCBASE, 5, snd_pcm_uframes_t)
#define AUDSINK_IOCTL_FLUSH		_IO (AUDSINK_IOCBASE, 6)
#define AUDSINK_IOCTL_PAUSE		_IO (AUDSINK_IOCBASE, 7)
#define AUDSINK_IOCTL_RESUME		_IO (AUDSINK_IOCBASE, 8)
#define AUDSINK_IOCTL_DROP		_IO (AUDSINK_IOCBASE, 9)
#define AUDSINK_IOCTL_START		_IO (AUDSINK_IOCBASE, 10)
#define AUDSINK_IOCTL_SET_VOLUME	_IOW(AUDSINK_IOCBASE, 11, uint8_t)
#define AUDSINK_IOCTL_GET_VOLUME	_IOR(AUDSINK_IOCBASE, 12, uint8_t)
#define AUDSINK_IOCTL_DRAIN		_IO (AUDSINK_IOCBASE, 13)
#define AUDSINK_IOCTL_SET_FLUSH_TIME	_IO (AUDSINK_IOCBASE, 14)

/* L=L, R=R */
#define AUDSINK_PCM_DUPLICATE_STEREO	((int) 0)
/* L=L, R=L */
#define AUDSINK_PCM_DUPLICATE_LEFT	((int) 1)
/* L=R, R=R */
#define AUDSINK_PCM_DUPLICATE_RIGHT	((int) 2)
/* L=(L+R)/2, R=(L+R)/2 */
#define AUDSINK_PCM_DUPLICATE_MONO	((int) 3)

struct audsink_xfer {
	void *data;
	/*
	 * Data size in bytes for bitstream raw data
	 * Frames for PCM data
	 */
	snd_pcm_uframes_t frames;
	/*
	 * Pitch is for non-interleave access mode
	 */
	snd_pcm_uframes_t pitch;
	uint32_t tstamp_ms;
};

#define AUDSINK_SND_DEVBIT_I2SO		BIT(0)
#define AUDSINK_SND_DEVBIT_PCMO		BIT(1)
#define AUDSINK_SND_DEVBIT_SPO		BIT(2)
#define AUDSINK_SND_DEVBIT_DDP_SPO	BIT(3)

struct audsink_init_params {
	/* byte size of buffer */
	size_t buf_size;

	/* AUDSINK_SND_DEVBIT_* */
	uint32_t snd_devs;

	/* AVSYNC_TYPE_* */
	uint8_t sync_type;

	struct snd_pcm_params pcm;

	int audio_flush_thres;
};

/* Invalid parameters */
#define AUDSINK_ERR_INVAL (-1)
/* No output sound device */
#define AUDSINK_ERR_NO_SNDDEV (-2)
/* Conflict output sound device */
#define AUDSINK_ERR_CONFLICT_SNDDEV (-3)
/* Conflict sync_type and pcm.sync_mode */
#define AUDSINK_ERR_CONFLICT_SYNCTYPE (-4)
/* Conflict input with AUDSINK_SYNC_TYPE_UPDATESTC or AUDSINK_SYNC_TYPE_UPDATESYNCSTC */
#define AUDSINK_ERR_CONFLICT_SYNCTYPE2 (-5)
/* Different output sound device for multiple PCM input */
#define AUDSINK_ERR_DIFFERENT_SNDDEV  (-6)

#endif	/* _HCUAPI_AUDSINK_H_ */
