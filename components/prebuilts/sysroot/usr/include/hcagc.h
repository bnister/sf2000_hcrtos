#ifndef MODULES_AUDIO_PROCESSING_AGC_LEGACY_HCAGC_H_
#define MODULES_AUDIO_PROCESSING_AGC_LEGACY_HCAGC_H_

#include <stdint.h>
#include <stdbool.h>

enum {
	hcAgcModeUnchanged,
	hcAgcModeAdaptiveAnalog,
	hcAgcModeAdaptiveDigital,
	hcAgcModeFixedDigital
};

struct hcagc_params {
	int16_t targetLevelDbfs; // default 3 (-3 dBOv)
	int16_t compressionGaindB; // default 9 dB
	uint8_t limiterEnable;
	uint8_t mode; // default hcAgcModeAdaptiveDigital
	int sample_rate_hz;
};

void *hcagc_create(struct hcagc_params *param);
void hcagc_destroy(void *handle);
/*
 * The data_in and data_out can point to the same buffer or different buffer
 */
int hcagc_process(void *handle, int16_t *data_in, int16_t *data_out, int nsamples, bool stream_has_echo);

#endif
