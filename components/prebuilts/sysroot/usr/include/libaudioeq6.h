#pragma once
#ifndef LIB_AUDIO_EQUALIZER_6_H
#define LIB_AUDIO_EQUALIZER_6_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <hcuapi/snd.h>

/* 6-band audio equalizer
 */

struct audio_eq6_t;

struct audio_eq6_op_t {
  /// \brief allocate and initialize object
  struct audio_eq6_t* (*init)(int sample_rate, int nchannel, int bits_per_sample);
  /// \brief destroy the object and free its memory
  void (*destroy)(struct audio_eq6_t*);
  /// \brief process audio signal
  /// \return number of output blocks (a block is the combination of all channel samples in a tick)
  int (*filter)(struct audio_eq6_t *tt,
                int nblock, const void *input, void *output);
  /// \brief set equalizer mode
  /// \return 1 for success, or 0 for failure
  int (*set_mode)(struct audio_eq6_t *ctx, snd_eq6_mode_e mode);
};

struct audio_eq6_hdl_t {
	const struct audio_eq6_op_t *eq6_op;
	struct audio_eq6_t *eq6;
	void *eq6_in;
	int bytes_per_frame;
	uint32_t totle_size;
	int eq6_mode;
};
const struct audio_eq6_op_t *get_audio_eq6_op(void);

#ifdef __cplusplus
}
#endif

#endif // LIB_AUDIO_EQUALIZER_6_H