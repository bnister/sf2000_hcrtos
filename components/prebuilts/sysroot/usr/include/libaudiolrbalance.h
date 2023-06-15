#ifndef LIB_AUDIO_LR_BALANCE
#define LIB_AUDIO_LR_BALANCE

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct audio_lr_balance_t;

struct audio_lr_balance_op_t {
  /// \brief initialize L/R balance object
  struct audio_lr_balance_t* (*init)(void);

  /// \brief destroy L/R balance object
  void (*destroy)(struct audio_lr_balance_t*);

  /// \brief set LR balance
  /// \param lr_balance_index L/R balance index, negative leans to Left,
  ///                   0 untouch, while positive leans to Right.
  ///                   Leaning to a channel means that channel has a higher
  ///                   volume than the other.
  ///                   Approximately 0.5dB stepping.
  /// \returns 1 for success, 0 for failure (out of range)
  int (*set_balance)(struct audio_lr_balance_t *ctx, int lr_balance_index);

  void (*filter_16b)(struct audio_lr_balance_t *ctx,
                     int nblock, const int16_t *in, int16_t *out);

  void (*filter_24b)(struct audio_lr_balance_t *ctx,
                     int nblock, const int32_t *in, int32_t *out);
};


struct audio_lr_balance_hdl_t {
	const struct audio_lr_balance_op_t *lr_op;
	struct audio_lr_balance_t *lr_balance;
	void *lr_balance_in;
	int bytes_per_frame;
	uint32_t totle_size;
	int lr_balance_index;
};
const struct audio_lr_balance_op_t* get_audio_lr_balance_op(void);
void audio_lrbalance_deinit(void *lr);
void *audio_lrbalance_init(int rate, int channels,int bitdepth);
int audio_lrbalance_handle(int lrbalance_index,void *handle,
	void *data_in,int in_frames,void **data_out, uint32_t buf_size);

#ifdef __cplusplus
}
#endif

#endif
