#include "wav.h"

int generate_wave_header(struct wave_header *header,
				int sample_rate, int bits_per_sample, int channels_num)
{
	unsigned long header_siz = sizeof(struct wave_header);

	memset(header, 0, sizeof(struct wave_header));
	memcpy(header->wav_header_tag.chunk_id, "RIFF", 4);
	memcpy(header->wav_header_tag.format, "WAVE", 4);
	header->wav_header_tag.chunk_size = header_siz - 8;

	memcpy(header->wav_fmt.sub_chunk1_id, "fmt ", 4);
	header->wav_fmt.sub_chunk1_size = sizeof(struct wave_format) - 8;
	/* PCM = 1 */
	header->wav_fmt.audio_format = 1;
	header->wav_fmt.channels_num = channels_num;
	header->wav_fmt.sample_rate = sample_rate;
	header->wav_fmt.bits_per_sample = bits_per_sample;
	header->wav_fmt.byte_rate =
		(sample_rate * header->wav_fmt.channels_num * bits_per_sample) / 8;
	header->wav_fmt.block_align = (header->wav_fmt.channels_num * bits_per_sample) / 8;

	memcpy(header->wav_dat.sub_chunk2_id, "data", 4);
	header->wav_dat.sub_chunk2_size = 0;//file_siz - header_siz;

	return 0;
}
