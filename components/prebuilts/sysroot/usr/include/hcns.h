#ifndef __DENOISE_H__
#define __DENOISE_H__

#include <stdint.h>

struct hcns_params {
	/* bit depth of the pcm data*/
	int bit_depth;
	
	/* sample rate of the pcm data */
	int sample_rate;

	/* the number of processing samples for each time */
	int process_units;

	/* denoise level (0 ~ 3) */
	int level;
};

void *hcns_create(struct hcns_params *params);

/*
 * Set denoise level, level rang: 0 ~ 3
 * Support change level dynamically during running
 */
int hcns_set_level(void *handle, int level);

/*
 * The in and out can be point to the same buffer
 */
int hcns_process(void *handle, int16_t *in, int16_t *out, int nsamples);

int hcns_destroy(void *handle);

#endif
