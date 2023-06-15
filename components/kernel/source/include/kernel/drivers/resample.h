#ifndef _HCUAPI_RESAMPLE_H_
#define _HCUAPI_RESAMPLE_H_

#include <kernel/drivers/snd.h>
#include <errno.h>

#define MAX_RESCALE_CHANNELS 8

struct rescale {
	unsigned int enabled;

	unsigned int delta;
	unsigned int phase;
	int32_t  history[MAX_RESCALE_CHANNELS][3];

	unsigned int frequency;
	unsigned int frequency_out;
};

void init_scale(struct rescale *r, int in_freq, int out_freq);
void resample_flush(struct rescale *r);
int resample_process(struct rescale *r, int *in, int *out, snd_pcm_uframes_t frames,
	     snd_pcm_uframes_t ipitch, snd_pcm_access_t access,
	     snd_pcm_format_t format, uint8_t channels_in);

#endif	/* _HCUAPI_RESAMPLE_H_ */
