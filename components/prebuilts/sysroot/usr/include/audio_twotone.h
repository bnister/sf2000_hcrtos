#ifndef LIB_AUDIO_TWO_TONE_H
#define LIB_AUDIO_TWO_TONE_H
#include <malloc.h>
#include <stdint.h>

struct audio_two_tone_t;

struct audio_two_tone_op_t {
  /// \brief allocate and initialize object
  struct audio_two_tone_t* (*init)(int sample_rate,
                                   int nchannel,
                                   int bits_per_sample);
  /// \brief destroy the object and free its memory
  void (*destroy)(struct audio_two_tone_t*);
  /// \brief process audio signal
  /// \return number of output blocks (a block is the combination of all channel samples in a tick)
  int (*filter)(struct audio_two_tone_t *tt,
                int nblock, const void *input, void *output);
  /// \brief set bass strength. index range in [-10, 10]
  /// \return 1 for success, or 0 for failure
  int (*set_bass)(struct audio_two_tone_t *ctx, int index);
  /// \brief set treble strength. index range in [-10, 10]
  /// \return 1 for success, or 0 for failure
  int (*set_treble)(struct audio_two_tone_t *ctx, int index);
};

struct audio_tt_hdl_t {
	const struct audio_two_tone_op_t *tt_op;
	struct audio_two_tone_t *tt;
	void *two_tone_in;
	int bytes_per_frame;
	uint32_t totle_size;
	int bass_index;
	int treble_index;
};

const struct audio_two_tone_op_t *get_audio_two_tone_op(void);
void audio_twotone_deinit(void *tt);
void *audio_twotone_init(int rate, int channels,
	int bitdepth, int bass_idx, int treble_idx);
int audio_twotone_handle(int bass_idx, int treble_idx, void *handle, void *data_in,
	int in_frames,void **data_out, uint32_t buf_size);

#endif // LIB_AUDIO_TWO_TONE_H

