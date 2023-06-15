/**
 * Proprietary software
 * @copyright Copyright Hichip Semiconductor (c) 2022
 *
 */
#ifndef AUDIO_DRC_H
#define AUDIO_DRC_H
#include <malloc.h>

struct audio_drc_t;

typedef struct audio_drc_op_t {
  /// \brief allocate and initialize object
	struct audio_drc_t *(*init)(int peak_dB, int transition_width_dB,
	                          int attack_msec, int release_msec,
	                          int sample_rate, int nchannel, int nbit);
	/// \brief destroy the object and free its memory
	void (*destroy)(struct audio_drc_t*);
	/// \brief query number of samples buffered
	int (*nbuffered)(struct audio_drc_t*);
	/// \brief process audio signal
	/// \return number of output blocks (a block is the combination of all channel samples in a tick)
	int (*process)(struct audio_drc_t *drc,
	             int nblock, const void *input, void *output);
	/// \brief flush buffered audio signal
	/// \return number of samples flushed
	int (*flush)(struct audio_drc_t *drc, int nblock, void *output);
} audio_drc_op_t;

struct audio_drc_handle_t {
	struct audio_drc_op_t *drc_op;
	struct audio_drc_t *drc;
	int drc_outbuf_size;
	void *drc_in;
	int bytes_per_frame;
	uint32_t totle_size;
};

struct audio_drc_op_t *get_audio_drc_op(void);
int audio_drc_handle(void *handle, void *data_in,
	int in_frames,void **data_out, uint32_t buf_size);
void audio_drc_deinit(void *drc);
void *audio_drc_init(int rate, int channels, int bitdepth);

#endif // AUDIO_DRC_H

