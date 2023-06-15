#include <string.h>

struct wave_header_tag {
	/* "RIFF" */
	char chunk_id[4];
	/* Saved file size, not include chunk id and chunk size these 8 bytes*/
	unsigned long chunk_size;
	/* "WAVE" */
	char format[4];
};

struct wave_format {
	/* "fmt " */
	char sub_chunk1_id[4];
	/* This sub chunk size, not include sub chunk1 id and size 8 bytes */
	unsigned long sub_chunk1_size;
	/* PCM = 1 */
	unsigned short audio_format;
	/* Mono = 1, stereo = 2 */
	unsigned short channels_num;
	/* Samples per second */
	unsigned long sample_rate;
	/* Average bytes per second : 
	 * (sample_rate *channels_num * bits_per_sample) / 8 */
	unsigned long byte_rate;
	/* (channels_num * bits_per_sample) / 8 */
	unsigned short block_align;
	unsigned short bits_per_sample;
};

struct wave_data {
	/* "data" */
	char sub_chunk2_id[4];
	/* data size */
	unsigned long sub_chunk2_size;
};

struct wave_header {
    struct wave_header_tag wav_header_tag;
    struct wave_format wav_fmt;
    struct wave_data wav_dat;
};

int generate_wave_header(struct wave_header *header,
				int sample_rate, int bits_per_sample, int channels_num);
