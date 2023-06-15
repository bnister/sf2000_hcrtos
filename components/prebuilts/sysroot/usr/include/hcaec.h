#ifndef __HCAEC_H__
#define __HCAEC_H__

#include <stdint.h>

#define MAX_SPEAKER_NUM 4

struct hcaec_params {
	/* sample rate of the pcm data */
	int sample_rate;

	/* the number of processing samples for each time */
	int frame_size;

	int filter_length;

	uint8_t speaker_num;
	int denoise;
	int ecval_mul;
};

struct aec_data {
	void *in;
	void *ref[MAX_SPEAKER_NUM];
	void *out;
	int nsamples;
};
//create an instance according the parameters
void *hcaec_create(struct hcaec_params *params);

//do aec process
int hcaec_process(void *handle, struct aec_data *raw);

//release all resource and destroy the instance
int hcaec_destroy(void *handle);

#endif
