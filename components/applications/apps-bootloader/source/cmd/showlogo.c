#define LOG_TAG "boot show logo"
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
#include <kernel/hwspinlock.h>
#include <kernel/completion.h>
#include <kernel/lib/console.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/dis.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/sysdata.h>
#include <kernel/elog.h>
#include <hcuapi/snd.h>

typedef int (*hc_avread) (void *buf, int size, void *file);

#ifndef MKTAG
#define MKTAG(a, b, c, d)                                                      \
	((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#endif

typedef struct _logoshow {
	hc_avread read;
	void *file;
	int play;
	TaskHandle_t task_id;
} logoshow;

struct completion completion = { 0 };
struct completion feed_completion = { 0 };
static logoshow *logo = NULL;
static rotate_type_e video_rotate = ROTATE_TYPE_0;
static int video_rotate_mode = 0;
static int video_mirror_mode = 0;
typedef enum flip_mode_e_{
	FLIP_MODE_NORMAL_E,
    FLIP_MODE_ROTATE_180_E,
    FLIP_MODE_H_MIRROR_E,
    FLIP_MODE_V_MIRROR_E,
}flip_mode_e;

static uint8_t showlogo_get_sys_screen_direction(int rotate,int h_flip,int v_flip)
{
	uint8_t flip_flag;
	
	if(sys_get_sysdata_flip_mode(&flip_flag)!=0)
	{
		flip_flag=0;
	}

	log_d("flip_mode = %d\n",flip_flag);
	if(rotate==0||rotate==90)
		goto hv_flip;

	if(rotate==180||rotate==270)
	{
		switch (flip_flag)
		{
			case FLIP_MODE_NORMAL_E:
				flip_flag = FLIP_MODE_ROTATE_180_E;
				break;
			case FLIP_MODE_ROTATE_180_E:
				flip_flag = FLIP_MODE_NORMAL_E;
				break;
			case FLIP_MODE_H_MIRROR_E:
				flip_flag = FLIP_MODE_V_MIRROR_E;
				break;
			case FLIP_MODE_V_MIRROR_E:
				flip_flag = FLIP_MODE_H_MIRROR_E;
				break;
			default:
				flip_flag=FLIP_MODE_ROTATE_180_E;
			break;
		}
	}

hv_flip:
	if(h_flip)
	{
		switch (flip_flag)
		{
			case FLIP_MODE_NORMAL_E:
				flip_flag = FLIP_MODE_H_MIRROR_E;
				break;
			case FLIP_MODE_ROTATE_180_E:
				flip_flag = FLIP_MODE_V_MIRROR_E;
				break;
			case FLIP_MODE_H_MIRROR_E:
				flip_flag = FLIP_MODE_NORMAL_E;
				break;
			case FLIP_MODE_V_MIRROR_E:
				flip_flag = FLIP_MODE_ROTATE_180_E;
				break;
			default:
				flip_flag=FLIP_MODE_H_MIRROR_E;
			break;
		}
	}
	else{
		if(v_flip)
		{
			switch (flip_flag)
			{
				case FLIP_MODE_NORMAL_E:
					flip_flag = FLIP_MODE_V_MIRROR_E;
					break;
				case FLIP_MODE_ROTATE_180_E:
					flip_flag = FLIP_MODE_H_MIRROR_E;
					break;
				case FLIP_MODE_H_MIRROR_E:
					flip_flag = FLIP_MODE_ROTATE_180_E;
					break;
				case FLIP_MODE_V_MIRROR_E:
					flip_flag = FLIP_MODE_NORMAL_E;
					break;
				default:
					flip_flag=FLIP_MODE_V_MIRROR_E;
				break;
			}
		}
	}
	return flip_flag;
}


void showlogo_get_rotate_by_flip_mode(flip_mode_e mode ,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;

    switch(mode)
    {
        case FLIP_MODE_ROTATE_180_E:
        {
            printf("180\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 0;
            v_flip = 0;
            break;
        }

        case  FLIP_MODE_H_MIRROR_E:
        {
            printf("h_mirror\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 1;
            v_flip = 0;
            break;
        }

        case FLIP_MODE_V_MIRROR_E:
        {
            // printf("v_mirror\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 1;
            v_flip = 0;
            break;

        }
        case FLIP_MODE_NORMAL_E:
        {
            printf("normal\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 0;
            v_flip = 0;
            break;
        }
        default:
            break;
    }

    *p_rotate_mode = rotate;
    *p_h_flip = h_flip;
    *p_v_flip = v_flip;

}



void showlogo_transfer_rotate_mode_for_screen(
                                         int init_rotate,
                                         int init_h_flip,
                                         int init_v_flip,
                                         int *p_rotate_mode ,
                                         int *p_h_flip ,
                                         int *p_v_flip ,
                                         int *p_fbdev_rotate)
{
    int fbdev_rotate[4] = { 0,270,180,90 }; //setting is anticlockwise for fvdev
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    rotate = *p_rotate_mode;

    //if screen is V screen,h_flip and v_flip exchange
    if(init_rotate == 0 || init_rotate == 180)
    {
        h_flip = *p_h_flip;
        v_flip = *p_v_flip;
    }
    else
    {
        h_flip = *p_v_flip;
        v_flip = *p_h_flip;
    }

    /*setting in dts is anticlockwise */
    /*calc rotate mode*/
    if(init_rotate == 270)
    {
        rotate = (rotate + 1) & 3;
    }
    else if(init_rotate == 90)
    {
        rotate = (rotate + 3) & 3;
    }
    else if(init_rotate == 180)
    {
        rotate = (rotate + 2) & 3;
    }

    /*transfer v_flip to h_flip with rotate
    *rotate 0 + H
    *rotate 0 + V--> rotate 180 +H
    *rotate 180 + H
    *rotate 180 + V --> rotate 0  + H
    *rotate 90 + H
    *rotate 90 + V--> rotate 270 +H
    *rotate 270 +H
    *rotate 270 +V--> rotate 90 + H
    */
    if(v_flip == 1)
    {
        switch(rotate)
        {
            case ROTATE_TYPE_0:
                rotate = ROTATE_TYPE_180;
                break;
            case ROTATE_TYPE_90:
                rotate = ROTATE_TYPE_270;
                break;
            case ROTATE_TYPE_180:
                rotate = ROTATE_TYPE_0;
                break;
            case ROTATE_TYPE_270:
                rotate = ROTATE_TYPE_90;
                break;
            default:
                break;
        }
        v_flip = 0;
        h_flip = 1;
    }

    h_flip = h_flip ^ init_h_flip;

    if(p_rotate_mode != NULL)
    {
        *p_rotate_mode = rotate;
    }

    if(p_h_flip != NULL)
    {
        *p_h_flip = h_flip;
    }

    if(p_v_flip != NULL)
    {
        *p_v_flip = 0;
    }

    if(p_fbdev_rotate != NULL)
    {
        *p_fbdev_rotate = fbdev_rotate[rotate];
    }



}

int showlogo_get_flip_info(int init_rotate ,
                           int init_h_flip ,
                           int init_v_flip , 
                           int *rotate_type , 
                           int *flip_type)
{

    int rotate = 0 , h_flip = 0 , v_flip = 0;

    uint8_t flip_flag;

    if(sys_get_sysdata_flip_mode(&flip_flag) != 0)
    {
        flip_flag = 0;
    }

    showlogo_get_rotate_by_flip_mode(flip_flag ,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    showlogo_transfer_rotate_mode_for_screen(init_rotate ,
                                             init_h_flip ,
                                             init_v_flip , 
                                             &rotate ,
                                             &h_flip ,
                                             &v_flip ,
                                             NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}


static int get_video_rotate(void)
{
	static int np = -1;
	const char *status;
	int ret = 0;
	int init_rotate = 0,init_v_flip=0,init_h_flip=0;
	uint8_t flip_mode=0;

    np = fdt_get_node_offset_by_path("/hcrtos/rotate");
    if(np < 0){
        log_w("%s:%d: fdt_get_node_offset_by_path failed\n", __func__, __LINE__);
        return -1;
    }

    ret = fdt_get_property_string_index(np, "status", 0, &status);
    if(ret != 0){
        log_w("%s:%d: fdt_get_property_u_32_index ir_key failed\n", __func__, __LINE__);
    }

    if(strcmp(status, "okay")){
        log_w("%s:%d: rotate is disable\n", __func__, __LINE__);
        return -1;
    }

    ret = fdt_get_property_u_32_index(np, "rotate", 0, &init_rotate);
    if(ret != 0){
        log_w("%s:%d: fdt_get_property_u_32_index ir_key failed\n", __func__, __LINE__);
    }

	ret = fdt_get_property_u_32_index(np, "h_flip", 0, &init_h_flip);
	if(ret != 0){
        log_w("%s:%d: fdt_get_property_u_32_index init-h_flip failed\n", __func__, __LINE__);
    }

	ret = fdt_get_property_u_32_index(np, "v_flip", 0, &init_v_flip);
	if(ret != 0){
        log_w("%s:%d: fdt_get_property_u_32_index init-v_flip failed\n", __func__, __LINE__);
    }

    showlogo_get_flip_info(init_rotate, init_h_flip, init_v_flip,&video_rotate_mode ,&video_mirror_mode);
    video_rotate = video_rotate_mode;

	
    log_d("%s:%d: video_rotate=%d video_rotate_mode=%d video_mirror_mode %d init_rotate %d flip_mode %d\n", __func__, __LINE__,
            video_rotate, video_rotate_mode,video_mirror_mode,init_rotate,flip_mode);

    return 0;
}

#ifdef CONFIG_BOOT_HDMI_TX_CHECK_EDID
static int tvsys_to_tvtype(enum TVSYS tv_sys, struct dis_tvsys *tvsys)
{
    switch(tv_sys){
        case TVSYS_480I:
            tvsys->tvtype = TV_NTSC;
            tvsys->progressive = 0;
        break;

        case TVSYS_480P:
            tvsys->tvtype = TV_NTSC;
            tvsys->progressive = 1;
        break;

        case TVSYS_576I :
            tvsys->tvtype = TV_PAL;
            tvsys->progressive = 0;
        break;

        case TVSYS_576P:
            tvsys->tvtype = TV_PAL;
            tvsys->progressive = 1;
        break;

        case TVSYS_720P_50:
            tvsys->tvtype = TV_LINE_720_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_720P_60:
            tvsys->tvtype = TV_LINE_720_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080I_25:
            tvsys->tvtype = TV_LINE_1080_25;
            tvsys->progressive = 0;
        break;

        case TVSYS_1080I_30:
            tvsys->tvtype = TV_LINE_1080_30;
            tvsys->progressive = 0;
        break;

        case TVSYS_1080P_24:
            tvsys->tvtype = TV_LINE_1080_24;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_25:
            tvsys->tvtype = TV_LINE_1080_25;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_30:
            tvsys->tvtype = TV_LINE_1080_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_50:
            tvsys->tvtype = TV_LINE_1080_50;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_60:
            tvsys->tvtype = TV_LINE_1080_60;
            tvsys->progressive = 1;
        break;

        case TVSYS_3840X2160P_30:
            tvsys->tvtype = TV_LINE_3840X2160_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_4096X2160P_30:
            //tvsys->tvtype = TV_LINE_4096X2160_30;
            tvsys->tvtype = TV_LINE_3840X2160_30;
            tvsys->progressive = 1;
        break;

        default:
            tvsys->tvtype = TV_LINE_720_30;
            tvsys->progressive = 1;
    }

    //printf("%s:%d: tvtype=%d\n", __func__, __LINE__, tvsys->tvtype);
    //printf("%s:%d: progressive=%d\n", __func__, __LINE__, tvsys->progressive);

    return 0;
}

static int hdmi_tx_save_tvtype(tvtype_e tvtype)
{
	int fd;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	sysdata.tvtype = tvtype;
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = offsetof(struct sysdata, tvtype);
	node.size = sizeof(sysdata.tvtype);
	node.buf = &sysdata.tvtype;

	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		printf("put sysdata failed\n");
		close(fd);
		return -1;
	}

	close(fd);

    return 0;
}

/* only get edid and set tvtype */
static int hdmi_tx_edid_detect(void)
{
    int tx_fd = -1;
    int dis_fd = -1;
    int ret = 0;
    enum TVSYS tv_sys = 0;
    struct dis_tvsys out_tvsys = {0};
    enum TVTYPE cur_tvtype = 0;
    bool cur_progressive = 0;
    int timeout = 10;

    tx_fd = open("/dev/hdmi" , O_RDONLY);
    if(tx_fd < 0){
        printf("open /dev/hdmi_tx failed, tx_fd=%d\n", tx_fd);
        goto err;
    }

    dis_fd = open("/dev/dis" , O_RDWR);
    if(dis_fd < 0){
        printf("open /dev/dis failed, dis_fd=%d\n", dis_fd);
        goto err;
    }

    /* get default tvsys */
    out_tvsys.distype = DIS_TYPE_HD;
    out_tvsys.layer = DIS_LAYER_MAIN;

    ret = ioctl(dis_fd , DIS_GET_TVSYS , &out_tvsys);
    if(ret < 0){
        printf("DIS_SET_TVSYS failed, ret=%d\n", ret);
        goto err;
    }

    cur_tvtype = out_tvsys.tvtype;
    cur_progressive = out_tvsys.progressive;

#if 0
    printf("%s:%d: distype=%d\n", __func__, __LINE__, out_tvsys.distype);
    printf("%s:%d: layer=%d\n", __func__, __LINE__, out_tvsys.layer);
    printf("%s:%d: tvtype=%d\n", __func__, __LINE__, out_tvsys.tvtype);
    printf("%s:%d: progressive=%d\n", __func__, __LINE__, out_tvsys.progressive);
#endif

    /* get edid */
    while(timeout--){
        ret = ioctl(tx_fd, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
        if(ret == 0){
            printf("%s:%d: tv_sys=%d, timeout=%d\n", __func__, __LINE__, tv_sys, timeout);
            break;
        }

        usleep(10*1000);
    }

    if(timeout <= 0){
        printf("%s:%d: warning: HDMI_TX_GET_EDID_TVSYS failed\n", __func__, __LINE__);
        goto err;
    }

    tvsys_to_tvtype(tv_sys, &out_tvsys);
    if(cur_tvtype != out_tvsys.tvtype || cur_progressive != out_tvsys.progressive){
        /* set tvtype */
        out_tvsys.distype = DIS_TYPE_HD;
        out_tvsys.layer = DIS_LAYER_MAIN;

        printf("%s:%d: set tvtype to %d\n", __func__, __LINE__, out_tvsys.tvtype);
        ret = ioctl(dis_fd , DIS_SET_TVSYS , &out_tvsys);
        if(ret < 0){
            printf("DIS_SET_TVSYS failed, ret=%d\n", ret);
            goto err;
        }

        hdmi_tx_save_tvtype(out_tvsys.tvtype);
    }

    close(tx_fd);
    close(dis_fd);

    return 0;

err:
    if(tx_fd >= 0){
        close(tx_fd);
        tx_fd = -1;
    }

    if(dis_fd >= 0){
        close(dis_fd);
        dis_fd = -1;
    }

    return ret;
}
#endif

static int block_write(int fd, void *data, int size, int *play)
{
	if (fd < 0) {
		return -1;
	}

	do {
		if (write(fd, data, size) != size) {
			usleep(10 * 1000);
		} else {
			return 0;
		}
	} while (*play);

	return -1;
}

static void write_eos_packet(int fd, int *play)
{
	AvPktHd pkthd = { 0 };

	//write header only
	pkthd.pts = 0;
	pkthd.size = 0;
	pkthd.flag = AV_PACKET_EOS;
	block_write(fd, &pkthd, sizeof(AvPktHd), play);
}

static void api_set_i2so_gpio_mute(bool val)//1 mute 0 no mute
{
	int snd_fd = -1;
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return;
	}
	ioctl(snd_fd, SND_IOCTL_SET_MUTE, val);
	close(snd_fd);
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

	AvPktHd pkthd = { 0 };
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

    get_video_rotate();

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
				if (!vcfg) {
					goto err;
				}
				memset(vcfg, 0, sizeof(struct video_config));

				vcfg->codec_id = codec_id;
				vcfg->sync_mode = 1;
				vcfg->decode_mode = VDEC_WORK_MODE_KSHM;

                if(video_rotate != ROTATE_TYPE_0 ||video_mirror_mode != 0){
                    vcfg->rotate_enable = 1;
                    vcfg->rotate_type = video_rotate;
                }

				if(video_mirror_mode!=0)
				{
					vcfg->mirror_type=video_mirror_mode;
				}

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
					if (vcfg->extradata_size > 512) {
						write_vextra = 1;
						vcfg->extradata_mode = 1;
					} else {
						vcfg->extradata_mode = 0;
						memcpy(vcfg->extra_data, vext,
						       vcfg->extradata_size);
					}
				}
				video_idx = i;
			}
		} else if (codec_id < HC_AVCODEC_ID_FIRST_SUBTITLE &&
			   codec_id != 0) {
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
				if (!acfg) {
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
					if (acfg->extradata_size > 512) {
						write_aextra = 1;
						acfg->extradata_mode = 1;
					} else {
						acfg->extradata_mode = 0;
						memcpy(acfg->extra_data, aext,
						       acfg->extradata_size);
					}
				}
				audio_idx = i;
			}
		} else {
			continue;
		}
	}

	/* open & config the auddec */
	if (acfg) {
		audio_fd = open("/dev/auddec", O_RDWR);
		if (audio_fd >= 0) {
			api_set_i2so_gpio_mute(0);
			if (write_aextra) {
				AvPktHd pkthd = { 0 };
				/* first data, do not need block write, write ret must be ok*/
				pkthd.pts = 0;
				pkthd.size = acfg->extradata_size;
				pkthd.flag = AV_PACKET_EXTRA_DATA;
				ret = write(audio_fd, (uint8_t *)&pkthd,
					    sizeof(AvPktHd));
				if (ret != sizeof(AvPktHd)) {
					close(audio_fd);
					audio_fd = -1;
					audio_idx = -1;
				} else {
					ret = write(audio_fd, aext,
						    acfg->extradata_size);
					if (ret != (int)acfg->extradata_size) {
						close(audio_fd);
						audio_fd = -1;
						audio_idx = -1;
					}
				}
			}
			ret = ioctl(audio_fd, AUDDEC_INIT, acfg);
			if (ret < 0) {
				close(audio_fd);
				audio_fd = -1;
				audio_idx = -1;
			}
			ioctl(audio_fd, AUDDEC_START, 0);
			printf("auddec start success\n");
		}
	}

	/* open & config the viddec */
	if (vcfg) {
		video_fd = open("/dev/viddec", O_RDWR);
		if (video_fd >= 0) {

			if (audio_fd < 0) {
				/* freerun if only video */
				vcfg->sync_mode = 0;
			}

			if (write_vextra) {
				AvPktHd pkthd = { 0 };
				/* first data, do not need block write, write ret must be ok*/
				pkthd.pts = 0;
				pkthd.size = vcfg->extradata_size;
				pkthd.flag = AV_PACKET_EXTRA_DATA;
				ret = write(video_fd, (uint8_t *)&pkthd,
					    sizeof(AvPktHd));
				if (ret != sizeof(AvPktHd)) {
					close(video_fd);
					video_fd = -1;
					video_idx = -1;
				} else {
					ret = write(video_fd, vext,
						    vcfg->extradata_size);
					if (ret != (int)vcfg->extradata_size) {
						close(video_fd);
						video_fd = -1;
						video_idx = -1;
					}
				}
			}
			ret = ioctl(video_fd, VIDDEC_INIT, vcfg);
			if (ret < 0) {
				close(video_fd);
				video_fd = -1;
				video_idx = -1;
			}
			ioctl(video_fd, VIDDEC_START, 0);
			printf("viddec start success\n");
		}
	}

	if (video_fd < 0 && audio_fd < 0)
		goto err;

	/* read frames and write them to auddec/viddec */
	while (lgs->play) {
		int fd;

		if (4 != avread(&frm_hdr, 4, avfile)) {
			break;
		}
		if (4 != avread(&pts, 4, avfile)) {
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
		if ((int)size != avread(data, size, avfile)) {
			break;
		}
		if ((int)idx == video_idx) {
			fd = video_fd;
		} else if ((int)idx == audio_idx) {
			fd = audio_fd;
		} else {
			continue;
		}

		if (fd < 0)
			continue;

		pkthd.pts = pts;
		pkthd.dur = 0;
		pkthd.size = size;
		pkthd.flag = AV_PACKET_ES_DATA;

        if(video_rotate != ROTATE_TYPE_0){
            pkthd.video_rotate_mode = video_rotate_mode;
        }

		if(video_mirror_mode!=0)
		{
			pkthd.video_mirror_mode = video_mirror_mode;
		}
		if (0 != block_write(fd, (uint8_t *)&pkthd, sizeof(AvPktHd),
				     &lgs->play))
			break;

		if (0 != block_write(fd, data, size, &lgs->play))
			break;
	}

	complete(&feed_completion);

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
	while (!eos && lgs->play) {
		if (audio_fd >= 0) {
			usleep(20 * 1000);
			ioctl(audio_fd, AUDDEC_CHECK_EOS, &eos);
		} else if (video_fd >= 0) {
			usleep(10 * 1000);
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
		struct vdec_rls_param rls = { 0 };
		rls.closevp = 0;
		rls.fillblack = 0;
		ioctl(video_fd, VIDDEC_RLS, (unsigned long)&rls);
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

	complete(&completion);
	vTaskDelete(NULL);
}

void stop_show_logo(void)
{
	if (logo) {
		logo->play = 0;
		wait_for_completion(&completion);
		free(logo);
		logo = NULL;
	}
}

void wait_show_logo(void)
{
	if (logo) {
		wait_for_completion(&completion);
		free(logo);
		logo = NULL;
	}
}

void wait_show_logo_finish_feed(void)
{
	if (logo) {
		wait_for_completion(&feed_completion);
	}
}

void board_preboot_os(void)
{
	wait_show_logo();
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

	logo = malloc(sizeof(logoshow));
	if (!logo) {
		return -1;
	}

	memset(logo, 0, sizeof(logoshow));
	logo->file = file;
	logo->read = avread;
	logo->play = 1;
	init_completion(&completion);
	init_completion(&feed_completion);
	ret = xTaskCreate(showlogo_thread, "showlogo_thread", 0x2000, logo,
			  portPRI_TASK_NORMAL, &logo->task_id);
	if (ret != pdTRUE) {
		free(logo);
		logo = NULL;
	} else {
		ret = 0;
	}

	return ret;
}

static FILE *avfile = NULL;
static int hcread(void *buf, int size, void *file)
{
	fread(buf, 1, size, (FILE *)file);
}

int showlogo(int argc, char *argv[])
{
	int i = 0;

	if (argc < 2) {
		printf("ERROR arguments. Usage: <file>\n");
		return -1;
	}

	avfile = fopen(argv[1], "rb");

	if (!avfile) {
		return -1;
	}

#ifdef CONFIG_BOOT_HDMI_TX_CHECK_EDID
    hdmi_tx_edid_detect();
#endif

	return start_show_logo((void *)avfile, hcread);
}

CONSOLE_CMD(showlogo, NULL, showlogo, CONSOLE_CMD_MODE_SELF, "<file>")
