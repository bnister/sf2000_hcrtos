#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/hwspinlock.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>

#include "showlogo.h"

#define SHOWLOGO_STOPPED (1 << 0)

#ifndef MKTAG
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#endif

typedef struct _logoshow {
	hc_avread read;
	void *file;
	int play;
	TaskHandle_t task_id;
}logoshow;

static EventGroupHandle_t showlogo_events = NULL;
static logoshow *logo = NULL;

static int block_write (int fd, void *data, int size, int *play)
{
	if (fd < 0) {
		return -1;
	}

	do {
		if (write(fd, data, size) != size) {
			usleep(10*1000);
		} else {
			return 0;
		}
	} while (*play);

	return -1;    
}

static void write_eos_packet(int fd, int *play)
{
	AvPktHd pkthd = {0};

	//write header only
	pkthd.pts = 0;
	pkthd.size = 0;
	pkthd.flag = AV_PACKET_EOS;
	block_write (fd, &pkthd, sizeof(AvPktHd), play);
}

static void showlogo_thread(void *args)
{
	logoshow *lgs = (logoshow *)args;
	void *avfile = lgs->file;
	hc_avread avread = lgs->read;

	int ret, i;
	uint32_t tag = 0;
	uint16_t version = 0;
	uint16_t nb_streams = 0;
	uint32_t codec_id = 0;

	struct audio_config *acfg = NULL;
	void *aext = NULL;
	int write_aextra = 0;

	struct video_config *vcfg = NULL;
	void *vext = NULL;
	int write_vextra = 0;

	int video_fd = -1;
	int video_idx = -1;
	int audio_fd = -1;
	int audio_idx = -1;

	AvPktHd pkthd = {0};
	uint32_t frm_hdr = 0;
	uint32_t pts = 0;
	void *data = NULL;
	int data_size = 0;
	uint32_t size = 0;
	uint32_t idx = 0;
	int eos = 0;

	/* check if it a logo av. */
	avread(&tag, sizeof(uint32_t), avfile);
	if (tag != MKTAG('h', 'c', 'a', 'v')) {
		goto err;
	}

	/* version check */
	avread(&version, sizeof(uint16_t), avfile);
	if (version != 0) {
		goto err;
	}

	/* how many stream tracks */
	avread(&nb_streams, sizeof(uint16_t), avfile);
	if (nb_streams == 0) {
		goto err;
	}

	/* read stream tracks infos */
	for (i = 0; i < nb_streams; i++) {
		avread(&codec_id, sizeof(uint32_t), avfile);
		if (codec_id < HC_AVCODEC_ID_FIRST_AUDIO && codec_id != 0) {
			/* it is a video */
			uint16_t width = 0;
			uint16_t height = 0;
			uint32_t frame_rate = 0;
			uint32_t extradata_size = 0;

			avread(&width, 2, avfile);
			avread(&height, 2, avfile);
			avread(&frame_rate, 4, avfile);
			avread(&extradata_size, 4, avfile);
			if (extradata_size > 0) {
				vext = realloc(vext, extradata_size);
				if (!vext) {
					goto err;
				}
				avread(vext, extradata_size, avfile);
			}

			/* keep the first video info */
			if (!vcfg) {
				vcfg = malloc(sizeof(struct video_config));
				if(!vcfg) {
					goto err;
				}
				memset(vcfg, 0, sizeof(struct video_config));

				vcfg->codec_id = codec_id;
				vcfg->sync_mode = 1;
				vcfg->decode_mode = VDEC_WORK_MODE_KSHM;

				if (width > 0 && height > 0) {
					vcfg->pic_width = width;
					vcfg->pic_height = height;
				} else {
					vcfg->pic_width = 1280;
					vcfg->pic_height = 720;
				}
				vcfg->frame_rate = frame_rate;

				vcfg->pixel_aspect_x = 1;
				vcfg->pixel_aspect_y = 1;
				vcfg->preview = 0;
				vcfg->extradata_size = extradata_size;
				if (vcfg->extradata_size > 0) {
					if(vcfg->extradata_size > 512) {
						write_vextra = 1;
						vcfg->extradata_mode = 1;
					} else {
						vcfg->extradata_mode = 0;
						memcpy(vcfg->extra_data, vext, vcfg->extradata_size);
					}
				}
				video_idx = i;
			}
		} else if (codec_id < HC_AVCODEC_ID_FIRST_SUBTITLE && codec_id != 0){
			/* it is an audio */
			uint8_t bitdepth = 0;
			uint8_t channels = 0;
			uint32_t bit_rate = 0;
			uint32_t sample_rate = 0;
			uint32_t block_align = 0;
			uint32_t extradata_size = 0;

			avread(&bitdepth, 1, avfile);
			avread(&channels, 1, avfile);
			avread(&bit_rate, 4, avfile);
			avread(&sample_rate, 4, avfile);
			avread(&block_align, 4, avfile);
			avread(&extradata_size, 4, avfile);
			if (extradata_size > 0) {
				aext = realloc(aext, extradata_size);
				if (!aext) {
					goto err;
				}
				avread(aext, extradata_size, avfile);
			}

			/* keep the first audio info */
			if (!acfg) {
				acfg = malloc(sizeof(struct audio_config));
				if(!acfg) {
					goto err;
				}
				memset(acfg, 0, sizeof(struct audio_config));

				acfg->codec_id = codec_id;
				acfg->sync_mode = 2;
				acfg->bits_per_coded_sample = bitdepth;
				acfg->channels = channels;
				acfg->bit_rate = bit_rate;
				acfg->sample_rate = sample_rate;
				acfg->block_align = block_align;
				acfg->extradata_size = extradata_size;
				if (acfg->extradata_size > 0) {
					if(acfg->extradata_size > 512) {
						write_aextra = 1;
						acfg->extradata_mode = 1;
					} else {
						acfg->extradata_mode = 0;
						memcpy(acfg->extra_data, aext, acfg->extradata_size);
					}
				}
				audio_idx = i;
			}
		} else {
			continue;
		}
	}

	do {
	/* open & config the auddec */
	if (acfg) {
		audio_fd = open("/dev/auddec", O_RDWR);
		if (audio_fd >= 0) {
			if (write_aextra) {
				AvPktHd pkthd = {0};
				/* first data, do not need block write, write ret must be ok*/
				pkthd.pts = 0;
				pkthd.size = acfg->extradata_size;
				pkthd.flag = AV_PACKET_EXTRA_DATA;
				ret = write(audio_fd, (uint8_t *)&pkthd, sizeof(AvPktHd));
				if (ret != sizeof(AvPktHd)) {
					close(audio_fd);
					audio_fd = -1;
					audio_idx = -1;
					break;
				} else {
					ret = write(audio_fd, aext, acfg->extradata_size);
					if (ret != (int)acfg->extradata_size) {
						close(audio_fd);
						audio_fd = -1;
						audio_idx = -1;
						break;
					}
				}

			}
			ret = ioctl(audio_fd, AUDDEC_INIT, acfg);
			if (ret < 0) {
				close(audio_fd);
				audio_fd = -1;
				audio_idx = -1;
				break;
			}
			ioctl(audio_fd, AUDDEC_START, 0);
			printf("auddec start success\n");
		}
	}

	/* open & config the viddec */
	if (vcfg) {
		video_fd = open("/dev/viddec", O_RDWR);
		if (video_fd >= 0) {
			if (write_vextra) {
				AvPktHd pkthd = {0};
				/* first data, do not need block write, write ret must be ok*/
				pkthd.pts = 0;
				pkthd.size = vcfg->extradata_size;
				pkthd.flag = AV_PACKET_EXTRA_DATA;
				ret = write(video_fd, (uint8_t *)&pkthd, sizeof(AvPktHd));
				if (ret != sizeof(AvPktHd)) {
					close(video_fd);
					video_fd = -1;
					video_idx = -1;
					break;
				} else {
					ret = write(video_fd, vext, vcfg->extradata_size);
					if (ret != (int)vcfg->extradata_size) {
						close(video_fd);
						video_fd = -1;
						video_idx = -1;
						break;
					}
				}
			}
			if (audio_fd < 0) {
				vcfg->sync_mode = 0;
			}
			ret = ioctl(video_fd, VIDDEC_INIT, vcfg);
			if (ret < 0) {
				close(video_fd);
				video_fd = -1;
				video_idx = -1;
				break;
			}
			ioctl(video_fd, VIDDEC_START, 0);
			printf("viddec start success\n");
		}
	}
	} while(0);

	if (audio_fd < 0 && video_fd < 0) {
		goto err;
	}

	/* read frames and write them to auddec/viddec */
	while (lgs->play) {
		int fd;

		if (4 != avread(&frm_hdr, 4, avfile)){
			break;
		}
		if (4 != avread(&pts, 4, avfile)){
			break;
		}

		idx = frm_hdr & 0x7F;
		size = frm_hdr >> 8;
		if ((int)size > data_size) {
			data = realloc(data, size);
			if (!data) {
				break;
			}
			data_size = size;
		}
		if ((int)size != avread(data, size, avfile)){
			break;
		}
		if ((int)idx == video_idx) {
			fd = video_fd;
		} else if ((int)idx == audio_idx) {
			fd = audio_fd;
		} else {
			continue;
		}

		if (fd < 0) {
			continue;
		}

		pkthd.pts = pts;
		pkthd.dur = 0;
		pkthd.size = size;
		pkthd.flag = AV_PACKET_ES_DATA;
		if(0 != block_write(fd, (uint8_t *)&pkthd, sizeof(AvPktHd), &lgs->play))
			break;

		if(0 != block_write (fd, data, size, &lgs->play))
			break;
	}

	/* write eos packet to auddec/viddec */
	if (lgs->play) {
		if (audio_fd >= 0) {
			//printf("audio write null packet\n");
			write_eos_packet(audio_fd, &lgs->play);
		}

		if (video_fd >= 0) {
			//printf("video write null packet\n");
			write_eos_packet(video_fd, &lgs->play);
		}
	}

	/* get real eos flag */
	while (!eos && lgs->play){
		if (audio_fd >= 0) {
			usleep(20 * 1000);
			ioctl(audio_fd, AUDDEC_CHECK_EOS, &eos);

		} else if (video_fd >= 0) {
			usleep(20 * 1000);
			ioctl(video_fd, VIDDEC_CHECK_EOS, &eos);
		} else {
			break;
		}
	}

err:

	if (audio_fd >= 0) {
		close(audio_fd);
	}

	if (video_fd >= 0) {
		close(video_fd);
	}

	if (vcfg) {
		free(vcfg);
		vcfg = NULL;
	}

	if (vext) {
		free(vext);
		vext = NULL;
	}

	if (acfg) {
		free(acfg);
		acfg = NULL;
	}

	if (aext) {
		free(aext);
		aext = NULL;
	}

	xEventGroupSetBits(showlogo_events, SHOWLOGO_STOPPED);
	vTaskDelete(NULL);
}

void stop_show_logo(void)
{
	if (logo) {
		logo->play = 0;
		xEventGroupWaitBits(showlogo_events, SHOWLOGO_STOPPED, 
				pdFALSE, pdFALSE, portMAX_DELAY);
		vEventGroupDelete(showlogo_events);
		showlogo_events = NULL;

		free(logo);
		logo = NULL;
	}
}

void wait_show_logo(void)
{
	if (logo) {
		xEventGroupWaitBits(showlogo_events, SHOWLOGO_STOPPED, 
				pdFALSE, pdFALSE, portMAX_DELAY);
		vEventGroupDelete(showlogo_events);
		showlogo_events = NULL;

		free(logo);
		logo = NULL;
	}
}

int start_show_logo(void *file, hc_avread avread)
{
	int ret = -1;

	if (!file || !avread) {
		return -1;
	}

	if (logo) {
		stop_show_logo();
	}

	if (!showlogo_events) {
		showlogo_events = xEventGroupCreate();
	}

	logo = malloc(sizeof(logoshow));
	if (!logo) {
		return -1;
	}

	memset(logo, 0, sizeof(logoshow));
	logo->file = file;
	logo->read = avread;
	logo->play = 1;
	xEventGroupClearBits(showlogo_events, SHOWLOGO_STOPPED);
	ret = xTaskCreate(showlogo_thread, "showlogo_thread", 
			0x2000, logo, portPRI_TASK_NORMAL, &logo->task_id);
	if (ret != pdTRUE) {
		free(logo);
		logo = NULL;
	} else {
		ret = 0;
	}

	return ret;
}

