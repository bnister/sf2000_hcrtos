#ifndef _HCUAPI_AVSYNC_H_
#define _HCUAPI_AVSYNC_H_

#include <hcuapi/iocbase.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

enum AVSYNC_TYPE {
	AVSYNC_TYPE_FREERUN,

	/*
	 * Sync with STC, drop data if lagging behind and hold data if leading ahead.
	 */
	AVSYNC_TYPE_SYNCSTC,

	/*
	 * Auto update PTS to STC. When PTS doesn't match STC,
	 * just update the new PTS to STC, don't drop data.
	 */
	AVSYNC_TYPE_UPDATESTC,

	/*
	 * Sync with STC, hold data if leading ahead.
	 */
	AVSYNC_TYPE_SYNCSTC_NODROP,
};

enum AVSYNC_STCID {
	AVSYNC_STCID_0,
	AVSYNC_STCID_1,
};

enum AVSYNC_STATUS {
	AVSYNC_NORMAL,
	AVSYNC_DROP,
	AVSYNC_HOLD,
	AVSYNC_INVALID,
};

uint32_t avsync_get_stc_tick(enum AVSYNC_STCID id);
uint32_t avsync_get_stc_ms(enum AVSYNC_STCID id);
void avsync_set_stc_tick(enum AVSYNC_STCID id, uint32_t tick);
void avsync_set_stc_ms(enum AVSYNC_STCID id, uint32_t ms);
void avsync_set_stc_divisor(enum AVSYNC_STCID id, uint16_t div);
uint32_t avsync_ms_to_stc_tick(enum AVSYNC_STCID id, uint32_t ms);
uint32_t avsync_stc_tick_to_ms(enum AVSYNC_STCID id, uint32_t tick);

void avsync_audio_update_stc(enum AVSYNC_STCID id, uint32_t audio_tick);
void avsync_video_update_stc(enum AVSYNC_STCID id, uint32_t tick);

enum AVSYNC_STATUS avsync_audio_sync_check(enum AVSYNC_STCID id, uint32_t tick);

/**
 * pause or resume the hardware STC
 *
 * @param id the id of hardware STC
 * @param pause 0: pause 1: resume
 */
void avsync_pause_stc(enum AVSYNC_STCID id, int pause);

#define AVSYNC_GET_STC					_IOR (AVSYNC_IOCBASE, 0, uint32_t)
#define AVSYNC_GET_STC_MS				_IOR (AVSYNC_IOCBASE, 1, uint32_t)
#define AVSYNC_SET_STC					_IO (AVSYNC_IOCBASE, 2)
#define AVSYNC_SET_STC_MS				_IO (AVSYNC_IOCBASE, 3)

#define AVSYNC_SET_AUD_UPDATE_THRESH			_IO (AVSYNC_IOCBASE, 4)//unit: ms, default 2ms
#define AVSYNC_SET_AUD_UPDATE_DELAY_THRESH		_IO (AVSYNC_IOCBASE, 5) //unit: ms, default 0ms
#define AVSYNC_SET_AUD_SYNC_THRESH			_IO (AVSYNC_IOCBASE, 6) //unit: ms, default 180ms
#define AVSYNC_SET_AUD_SYNC_STC_VALID_THRESH		_IO (AVSYNC_IOCBASE, 7) //unit: ms, default 10000ms
#define AVSYNC_SET_AUD_SYNC_DELAY_THRESH		_IO (AVSYNC_IOCBASE, 8) //unit: ms, default 0ms
#define AVSYNC_ENABLE_AUD_HOLD				_IO (AVSYNC_IOCBASE, 9) //1: enable audio hold, default 0

#define AVSYNC_SET_VID_UPDATE_THRESH			_IO (AVSYNC_IOCBASE, 10) //unit: ms, default 100ms

#define AVSYNC_GET_AUD_SYNC_THRESH			_IOR (AVSYNC_IOCBASE, 11, uint32_t) //unit: ms

#define AVSYNC_PAUSE_STC				_IO (AVSYNC_IOCBASE, 12)//1 pause; 0 start.

#endif /* _HCUAPI_AVSYNC_H_ */
