#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <hcaec.h>

static void print_usage(const char *prog)
{
	printf("Usage: %s [-mrds]\n", prog);
	puts("  -m --micin         micin wav file(single channel)\n"
	     "  -r --reference     reference wav file(single channel)\n"
	     "                       -r ref1.wav -r ref2.wav ...(max 4 reference)\n"
	     "  -o --output        output wav file\n"
	     "  -d --denoise       denoiseEnable, default 0 (off)\n"
	     "                       0 - off\n"
	     "                       1 - on\n");
}

static int duration2bytes(int samplerate, int duration)
{
	return 2 * samplerate * duration / 1000;
}

static int duration2samples(int samplerate, int duration)
{
	return samplerate * duration / 1000;
}

int aec_main(int argc, char *argv[])
{
#ifdef CMD_BR2_PACKAGE_PREBUILTS_3A
	int ret = 0;
	int process_units = 0;
	int process_samples = 0;
	char *micin_file = NULL;
	char *ref_file[MAX_SPEAKER_NUM] = {NULL};
	char *dst_file = NULL;
	FILE *micin_fp = NULL;
	FILE *ref_fp[MAX_SPEAKER_NUM] = {NULL};
	FILE *dst_fp = NULL;
	uint8_t *micin = NULL, *dst = NULL, *ref[MAX_SPEAKER_NUM] = {NULL};
	int i, c;
	void *handle = NULL;
	char wavheader[44];
	int samplerate;
	struct hcaec_params params = { 0 };
	struct aec_data data_in = { 0 };
	int speaker = 0;

	/* Clear opt global variables
	 * opterr
	 * optind
	 */
	opterr = 0;
	optind = 0;

	static const struct option lopts[] = {
		{ "micin", 1, 0, 'm' },
		{ "reference", 1, 0, 'r' },
		{ "output", 1, 0, 'o' },
		{ "denoise", 1, 0, 'd' },
		{ NULL, 0, 0, 0 },
	};

	/* default params */
	params.filter_length = 2048;
	params.denoise = 0;
	params.ecval_mul = 0;

	while ((c = getopt_long(argc, argv, "m:r:o:d:", lopts, NULL)) != -1) {
		switch (c) {
		case 'm':
			/* File path */
			micin_file = optarg;
			break;
		case 'r':
			ref_file[speaker++] = optarg;
			break;
		case 'o':
			dst_file = optarg;
			break;
		case 'd':
			params.denoise = atoi(optarg);
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}

	if (speaker > MAX_SPEAKER_NUM) {
		print_usage(argv[0]);
		return -1;
	}
	speaker = 2;  //echo is stereo

	micin_file = "media/sda2/stream0";
	ref_file[0] = "media/sda2/stream1";
	ref_file[1] = "media/sda2/stream2";
	dst_file = "media/sda2/outaec";
	if ((micin_file == NULL) || (dst_file == NULL) || speaker == 0) {
		print_usage(argv[0]);
		return -1;
	}

	micin_fp = fopen(micin_file, "rb");
	if (micin_fp == NULL) {
		printf("open %s failed\n", micin_file);
		ret = -EIO;
		goto err;
	}
	printf("open source file %s\n", micin_file);

	for (i = 0; i < speaker; i++) {
		ref_fp[i] = fopen(ref_file[i], "rb");
		if (ref_fp[i] == NULL) {
			printf("open %s failed\n", ref_file[i]);
			ret = -EIO;
			goto err;
		}
	}

	dst_fp = fopen(dst_file, "wb+");
	if (dst_fp == NULL) {
		printf("open %s failed\n", dst_file);
		ret = -EIO;
		goto err;
	}
	printf("open destination file %s\n", dst_file);
	samplerate = 16000;
#if 0
	if (strstr(micin_file, ".wav")) {
		if (strstr(dst_file, ".wav")) {
			fseek(micin_fp, 0, SEEK_SET);
			ret = fread(wavheader, 1, 44, micin_fp);
			fwrite(wavheader, 1, 44, dst_fp);
		}

		samplerate = wavheader[24] | wavheader[25] << 8 | wavheader[26] << 16 | wavheader[27] << 24;

		fseek(micin_fp, 44, SEEK_SET);
	}
#endif
	if (samplerate == 8000 || samplerate == 16000) {
		process_units = duration2bytes(samplerate, 10);
		process_samples = duration2samples(samplerate, 10);
	} else {
		printf("Not support samplerate %d\n", samplerate);
		ret = -1;
		goto err;
	}

	micin = malloc(process_units);
	if (micin == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < speaker; i++) {
		ref[i] = malloc(process_units);
		if (ref[i] == NULL) {
			ret = -ENOMEM;
			goto err;
		}
	}

	dst = malloc(process_units);
	if (dst == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	params.sample_rate = samplerate;
	params.frame_size = process_samples;
	params.speaker_num = speaker;

	handle = hcaec_create(&params);
	if (handle == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	while (1) {
		ret = fread(micin, 1, process_units, micin_fp);
		if (ret != process_units) {
			ret = 0;
			break;
		}

		for (i = 0; i < speaker; i++) {
			ret = fread(ref[i], 1, process_units, ref_fp[i]);
			if (ret != process_units) {
				ret = 0;
				goto err;
			}
		}

		data_in.in = micin;
		for (i = 0; i < speaker; i++)
			data_in.ref[i] = ref[i];
		data_in.out = dst;
		data_in.nsamples = process_samples;
		memset(dst, 0, process_units);

		ret = hcaec_process(handle, &data_in);
		if (ret)
			printf("hcagc process failed %d\n", ret);

		ret = fwrite(dst, 1, process_units, dst_fp);
		if (ret != process_units) {
			printf("write data after denoise failed\n");
			ret = -EIO;
			break;
		}
	};
	printf("hcaec done!\n");
err:
	if (micin_fp)
		fclose(micin_fp);
	for (i = 0; i < speaker; i++) {
		if (ref_fp[i])
			fclose(ref_fp[i]);
	}
	if (dst_fp)
		fclose(dst_fp);

	if (micin)
		free(micin);
	for (i = 0; i < speaker; i++) {
		if (ref[i])
			free(ref[i]);
	}
	if (dst)
		free(dst);

	if (handle) {
		hcaec_destroy(handle);
	}
#endif
	return 0;
}
