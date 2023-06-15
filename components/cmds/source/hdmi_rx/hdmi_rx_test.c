#include <generated/br2_autoconf.h>
#include <fcntl.h>
#include <unistd.h>
#include <kernel/vfs.h>
#include <stdio.h>
#include <kernel/io.h>
#include <getopt.h>
#include <malloc.h>
#include <string.h>
#include <kernel/lib/console.h>
#include <kernel/completion.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/dis.h>
#include <stdlib.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/snd.h>
#include <hcuapi/audsink.h>
#include <stdlib.h>
#include <kernel/lib/crc32.h>
#include <hcuapi/gpio.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <kernel/module.h>
#include <sys/mount.h>

//#define HDMI_RX_TEST_DUMP_I2SO_DATA

static struct completion audio_read_task_completion;
static struct completion video_read_task_completion;

static bool audio_stop_read = 0;
static bool video_stop_read = 0;

static struct kshm_info audio_read_hdl = { 0 };
static struct kshm_info video_read_hdl = { 0 };
static enum HDMI_RX_VIDEO_DATA_PATH vpath = HDMI_RX_VIDEO_TO_OSD;
static enum HDMI_RX_AUDIO_DATA_PATH apath = HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX;
static int rx_fd = -1;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;
static unsigned int stop_mode = 0;
static unsigned int set_edid = false;
static unsigned char *buf_yuv444 = NULL;
hdmi_rx_edid_data_t hdmi_rx_edid;
//hdmi_rx_hdcp_key_t hdmi_rx_hdcp;
jpeg_enc_quality_type_e hdmi_rx_quality_type = JPEG_ENC_QUALITY_TYPE_NORMAL;
static int hdmi_rx_set_enc_table(void);

static uint8_t edid_data[]=
{
#if 1 //nativ 1080P
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0d, 0xc3, 0x20, 0x03, 0x0a, 0x1a, 0x00, 0x00,
    0x18, 0x1a, 0x01, 0x04, 0xa2, 0x46, 0x27, 0x78, 0xfa, 0xd6, 0xa5, 0xa2, 0x59, 0x4a, 0x95, 0x24,
    0x14, 0x50, 0x54, 0xaf, 0xcf, 0x00, 0x81, 0x00, 0x81, 0x40, 0x71, 0x40, 0xa9, 0xc0, 0x81, 0x80,
    0x81, 0xc0, 0x95, 0x00, 0xb3, 0x00, 0xab, 0x22, 0xa0, 0xa0, 0x50, 0x84, 0x1a, 0x30, 0x30, 0x20,
    0x36, 0x00, 0xb0, 0x0e, 0x11, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x50, 0xb0, 0x51, 0x00, 0x1a, 0x30,
    0x40, 0x70, 0x36, 0x00, 0xb0, 0xf3, 0x10, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x56, 0xaa, 0x51, 0x00,
    0x1e, 0x30, 0x46, 0x8f, 0x33, 0x00, 0xaa, 0xef, 0x10, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc,
    0x00, 0x48, 0x44, 0x4d, 0x49, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x0d,
    0x02, 0x03, 0x23, 0x70, 0x4d, 0x05, 0x03, 0x04, 0x07, 0x90, 0x01, 0x20, 0x12, 0x13, 0x14, 0x22,
    0x16, 0x20, 0x23, 0x09, 0x07, 0x07, 0x83, 0x4f, 0x00, 0x00, 0x68, 0x03, 0x0c, 0x00, 0x10, 0x00,
    0x80, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95,
#else //nativ 720p
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0d, 0xc3, 0x20, 0x03, 0x0a, 0x1a, 0x00, 0x00,
    0x18, 0x1a, 0x01, 0x04, 0xa2, 0x46, 0x27, 0x78, 0xfa, 0xd6, 0xa5, 0xa2, 0x59, 0x4a, 0x95, 0x24,
    0x14, 0x50, 0x54, 0xaf, 0xcf, 0x00, 0x81, 0x00, 0x81, 0x40, 0x71, 0x40, 0xa9, 0xc0, 0x81, 0x80,
    0x81, 0xc0, 0x95, 0x00, 0xb3, 0x00, 0xab, 0x22, 0xa0, 0xa0, 0x50, 0x84, 0x1a, 0x30, 0x30, 0x20,
    0x36, 0x00, 0xb0, 0x0e, 0x11, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x50, 0xb0, 0x51, 0x00, 0x1a, 0x30,
    0x40, 0x70, 0x36, 0x00, 0xb0, 0xf3, 0x10, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x56, 0xaa, 0x51, 0x00,
    0x1e, 0x30, 0x46, 0x8f, 0x33, 0x00, 0xaa, 0xef, 0x10, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc,
    0x00, 0x48, 0x44, 0x4d, 0x49, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x0d,
    0x02, 0x03, 0x23, 0x70, 0x4d, 0x05, 0x03, 0x84, 0x07, 0x10, 0x01, 0x20, 0x12, 0x13, 0x14, 0x22,
    0x16, 0x20, 0x23, 0x09, 0x07, 0x07, 0x83, 0x4f, 0x00, 0x00, 0x68, 0x03, 0x0c, 0x00, 0x10, 0x00,
    0x80, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95,
#endif
                                                                                     
};

uint8_t decoder_quant_table_y[64] =
{
    6		,3		,3		,6		,9		,15	,18	,21,
    3		,3		,6		,6		,9		,21	,21	,21,
    6		,6		,6		,9		,15		,21	,24	,21,
    6		,6		,9		,9		,18		,30	,30	,21,
    6		,9		,12		,21		,24		,39	,36	,27,
    9		,12		,21		,24		,30		,36	,42	,33,
    18		,24		,27		,30		,36		,45	,42	,36,
    27		,33		,33		,36		,39		,36	,36	,36,
};

uint8_t decoder_quant_table_c[64] =
{
6		,6		,9		,18	,36	,36	,36	,36,
6		,9		,9		,24	,36	,36	,36	,36,
9		,9		,21	    ,36	,36	,36	,36	,36,
18	    ,24	    ,36	    ,36	,36	,36	,36	,36,
36	    ,36	    ,36	    ,36	,36	,36	,36	,36,
36	    ,36	    ,36	    ,36	,36	,36	,36	,36,
36	    ,36	    ,36	    ,36	,36	,36	,36	,36,
36	    ,36	    ,36	    ,36	,36	,36	,36	,36,
};


uint16_t encoder_q_table_y[64] =
{
    0x1555, 0x0d55, 0x0d55, 0x1555, 0x0d55, 0x0d55, 0x1555, 0x1555,
    0x1555, 0x1555, 0x1555, 0x1555, 0x1555, 0x1555, 0x1f1c, 0x1c44,
    0x1f1c, 0x1f1c, 0x1f1c, 0x1f1c, 0x1f1c, 0x271c, 0x1d55, 0x1d55,
    0x1f1c, 0x1c44, 0x2618, 0x271c, 0x2618, 0x2618, 0x2618, 0x271c,
    0x2618, 0x2618, 0x2555, 0x24be, 0x2fc2, 0x24be, 0x2555, 0x2555,
    0x2444, 0x2555, 0x2618, 0x2618, 0x2444, 0x2e90, 0x2444, 0x2444,
    0x2fc2, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2618, 0x24be, 0x2e18,
    0x2db0, 0x2e90, 0x2f1c, 0x2e18, 0x2fc2, 0x2f1c, 0x2f1c, 0x2f1c,
};
uint16_t encoder_q_table_c[64] =
{
    0x1555, 0x1555, 0x1555, 0x1f1c, 0x1f1c, 0x1f1c, 0x271c, 0x1f1c,
    0x1f1c, 0x271c, 0x2f1c, 0x2555, 0x2618, 0x2555, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
};


static void audio_read_thread(void *args)
{
    struct kshm_info *hdl = (struct kshm_info *)args;
    AvPktHd hdr = { 0 };
    uint8_t *data = malloc(1024 * 1024);
    int collumn_num = 12;
    int *spectrum = NULL;

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    FILE *recfile = fopen("/media/sda1/rec_file", "wb+");
    printf("recfile %p\n", recfile);
#endif

    printf("audio_read_thread run```\n");
    while(!audio_stop_read)
    {
        while(kshm_read(hdl , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd) && !audio_stop_read)
        {
            usleep(20 * 1000);
        }

        data = realloc(data , hdr.size);
        while(kshm_read(hdl , data , hdr.size) != hdr.size && !audio_stop_read)
        {
            usleep(20 * 1000);
        }

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
        if (recfile)
            fwrite(data, hdr.size, 1, recfile);
#endif
    }
    usleep(1000);

    if(data)
        free(data);

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    if (recfile)
        fclose(recfile);
#endif

    complete(&audio_read_task_completion);
    vTaskSuspend(NULL);
}

static void video_read_thread(void *args)
{
    struct kshm_info *hdl = (struct kshm_info *)args;
    AvPktHd hdr = { 0 };
    uint8_t *data = malloc(1024 * 1024);
    uint32_t crc = 0;
    uint8_t flag0 = 0, flag1 = 0;

    printf("video_read_thread run```\n");
    while(!video_stop_read)
    {
        while(kshm_read(hdl , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd) && !video_stop_read)
        {
            //printf("read audio hdr from kshm err\n");
            usleep(20 * 1000);
        }
        printf("pkt size 0x%x, flag %d\n" , (int)hdr.size , (int)hdr.flag);

        data = realloc(data , hdr.size);
        while(kshm_read(hdl , data , hdr.size) != hdr.size && !video_stop_read)
        {
            //printf("read audio data from kshm err\n");
            usleep(20 * 1000);
        }
        printf("data: 0x%x\n" , (unsigned int)data);

#ifdef CONFIG_CMDS_HDMI_RX_SORTING
	crc = crc32(0, data, 0x9a21);
	printf("crc = 0x%lx\n", crc);

	if (crc == 0x2b0f0f40) {
		flag0++;
		flag1 = 0;
	} else {
		flag1++;
		flag0 = 0;
	}

	if (flag0 >= 3) {
		gpio_set_output(PINPAD_L01, 1);
		gpio_set_output(PINPAD_L02, 1);
		printf("test pass\n");
		while (1);
	}

	if (flag1 >= 3) {
		gpio_set_output(PINPAD_L01, 0);
		gpio_set_output(PINPAD_L02, 1);
		printf("test fail\n");
		while (1);
	}
#endif
    }
    usleep(1000);

    if(data)
        free(data);

    complete(&video_read_task_completion);
    vTaskSuspend(NULL);
}

static void hdmi_rx_set_aspect_mode(dis_tv_mode_e ratio , dis_mode_e dis_mode)
{
    int fd = open("/dev/dis" , O_RDWR);
    printf("ratio: %d, dis_mode: %d, fd: %d" , ratio , dis_mode , fd);
    if(fd < 0)
        return;
    dis_aspect_mode_t aspect = { 0 };
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    ioctl(fd , DIS_SET_ASPECT_MODE , &aspect);
    close(fd);
}

static void hdmi_rx_start_audio_thread(void)
{
    if(apath >= HDMI_RX_AUDIO_TO_I2SI_AND_KSHM)
    {
        int ret = -1;
        if (apath == HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO ||
            apath == HDMI_RX_AUDIO_TO_I2SI_AND_I2SO) {
            #ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
            int i2so_fd = open("/dev/sndC0i2so" , O_WRONLY);
            if (i2so_fd >= 0) {
                ret = ioctl(i2so_fd, SND_IOCTL_SET_RECORD, 300 * 1024);
                printf("SND_IOCTL_SET_RECORD ret %d\n", ret);
                ret |= ioctl(i2so_fd, KSHM_HDL_ACCESS, &audio_read_hdl);
                printf("KSHM_HDL_ACCESS ret %d\n", ret);
                close(i2so_fd);
            }
            #endif
        } else {
            ret = ioctl(rx_fd , HDMI_RX_AUDIO_KSHM_ACCESS , &audio_read_hdl);
        }
        if (ret < 0) {
            printf("get audio kshm handle failed\n");
            return;
        }
        init_completion(&audio_read_task_completion);
        audio_stop_read = 0;
        ret = xTaskCreate(audio_read_thread , "audio_read_thread" ,
                          0x1000 , &audio_read_hdl , portPRI_TASK_HIGH , NULL);
        if(ret != pdTRUE)
        {
            printf("kshm recv thread create failed\n");
        }
    }
}

static void hdmi_rx_start_video_thread(void)
{
    if(vpath == HDMI_RX_VIDEO_TO_KSHM ||
      vpath == HDMI_RX_VIDEO_TO_DE_AND_KSHM ||
      vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM)
    {
        int ret;
        struct kshm_info kshm_hdl;

        ioctl(rx_fd , HDMI_RX_VIDEO_KSHM_ACCESS , &video_read_hdl);

        init_completion(&video_read_task_completion);
        video_stop_read = 0;
        ret = xTaskCreate(video_read_thread , "video_read_thread" ,
                          0x1000 , &video_read_hdl , portPRI_TASK_HIGH , NULL);
        if(ret != pdTRUE)
        {
            printf("kshm recv thread create failed\n");
        }
    }

}

int sorting_rx_test(int argc, char **argv)
{
	if(rx_fd >= 0)
	{
		return 0;
	}

	rx_fd = open("/dev/hdmi_rx" , O_RDWR);
	if(rx_fd < 0)
	{
		return -1;
	}
	apath = 0;
	vpath = 3;
	stop_mode = 1;
	hdmi_rx_quality_type = 0;
	printf("apath %d, vpath %d stop_mode %d hdmi_rx_quality_type %d\n" ,
			apath , vpath , stop_mode, hdmi_rx_quality_type);
	printf("set_edid = %d\n", set_edid);
	ioctl(rx_fd , HDMI_RX_SET_VIDEO_DATA_PATH , vpath);
	ioctl(rx_fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);
	ioctl(rx_fd , HDMI_RX_SET_VIDEO_STOP_MODE , stop_mode);
	ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , hdmi_rx_quality_type);
	hdmi_rx_set_enc_table();
#if 0
	memcpy(&(hdmi_rx_hdcp.hdcp_key[0]), g_hdmi_rx_hdcp_cstm_key, HDCP_KEY_LEN);
	hdmi_rx_hdcp.b_encrypted = TRUE;
	ioctl(rx_fd , HDMI_RX_SET_HDCP_KEY , &hdmi_rx_hdcp);
#endif
	if(set_edid == true)
	{
		memcpy(&(hdmi_rx_edid.edid_data[0]), edid_data, EDID_DATA_LEN);
		ioctl(rx_fd , HDMI_RX_SET_EDID , &hdmi_rx_edid);
	}

	hdmi_rx_start_audio_thread();
	hdmi_rx_start_video_thread();


	if(vpath == HDMI_RX_VIDEO_TO_DE_ROTATE ||
			vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM)
	{
		printf("rotate_mode = 0x%x\n" , rotate_mode);
		ioctl(rx_fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_mode);
		printf("mirror_mode = 0x%x\n" , mirror_mode);
		ioctl(rx_fd , HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_mode);
	}

	//hdmi_rx_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX);
	ioctl(rx_fd , HDMI_RX_START);
	printf("hdmi_rx start ok```\n");
	return 0;
}

static int hdmi_rx_start_test(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;

    if(rx_fd >= 0)
    {
        return 0;
    }

    rx_fd = open("/dev/hdmi_rx" , O_RDWR);
    if(rx_fd < 0)
    {
        return -1;
    }

    while((opt = getopt(argc , argv , "a:v:r:s:m:e:q:")) != EOF)
    {
        switch(opt)
        {
            case 'a':
                apath = atoi(optarg);
                break;
            case 'v':
                vpath = atoi(optarg);
                break;
            case 'r':
                rotate_mode = atoi(optarg);
                break;
            case 's':
                stop_mode = atoi(optarg);
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                break;
            case 'e':
                set_edid = atoi(optarg);
                break;
            case 'q':
                hdmi_rx_quality_type = atoi(optarg);
                break;
            default:
                break;
        }
    }

    printf("apath %d, vpath %d stop_mode %d hdmi_rx_quality_type %d\n" , 
           apath , vpath , stop_mode, hdmi_rx_quality_type);
    printf("set_edid = %d\n", set_edid);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_DATA_PATH , vpath);
    ioctl(rx_fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_STOP_MODE , stop_mode);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , hdmi_rx_quality_type);
    hdmi_rx_set_enc_table();

#if 0
	memcpy(&(hdmi_rx_hdcp.hdcp_key[0]), g_hdmi_rx_hdcp_cstm_key, HDCP_KEY_LEN);
	hdmi_rx_hdcp.b_encrypted = TRUE;
	ioctl(rx_fd , HDMI_RX_SET_HDCP_KEY , &hdmi_rx_hdcp);
#endif
    if(set_edid == true)
    {
        memcpy(&(hdmi_rx_edid.edid_data[0]), edid_data, EDID_DATA_LEN);
        ioctl(rx_fd , HDMI_RX_SET_EDID , &hdmi_rx_edid);
    }

    hdmi_rx_start_audio_thread();
    hdmi_rx_start_video_thread();
    

    if(vpath == HDMI_RX_VIDEO_TO_DE_ROTATE ||
       vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM)
    {
        printf("rotate_mode = 0x%x\n" , rotate_mode);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_mode);
        printf("mirror_mode = 0x%x\n" , mirror_mode);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_mode);
    }

    //hdmi_rx_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX);
    ioctl(rx_fd , HDMI_RX_START);
    printf("hdmi_rx start ok```\n");
    return 0;
}

static void hdmi_rx_stop_audio_thread(void)
{
    audio_stop_read = 1;
    if(apath >= HDMI_RX_AUDIO_TO_I2SI_AND_KSHM  && audio_read_hdl.desc)
    {
        wait_for_completion_timeout(&audio_read_task_completion , 3000);
    }
    audio_read_hdl.desc = NULL;
}

static void hdmi_rx_stop_video_thread(void)
{
    video_stop_read = 1;

    if(vpath == HDMI_RX_VIDEO_TO_KSHM ||
       vpath == HDMI_RX_VIDEO_TO_DE_AND_KSHM ||
       vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM)
    {
        wait_for_completion_timeout(&video_read_task_completion , 3000);
    }

}

static int hdmi_rx_stop_test(int argc , char *argv[])
{
    if(rx_fd >= 0)
    {
        hdmi_rx_stop_audio_thread();
        hdmi_rx_stop_video_thread();
        
        ioctl(rx_fd , HDMI_RX_STOP);
        close(rx_fd);
        rx_fd = -1;
        if(buf_yuv444 != NULL)
        {
            free(buf_yuv444);
            buf_yuv444 = NULL;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

static int hdmi_rx_get_video_info(int argc , char *argv[])
{
    if(rx_fd >= 0)
    {
        hdmi_rx_video_info_t hdmi_rx_video_info = { 0 };
        ioctl(rx_fd , HDMI_RX_GET_VIDEO_INFO , &hdmi_rx_video_info);
        printf("w:%d h:%d format:%d range:%d: frame_rate:%d b_progressive:%d\n" ,
               hdmi_rx_video_info.width ,
               hdmi_rx_video_info.height ,
               hdmi_rx_video_info.color_format ,
               hdmi_rx_video_info.range,
               hdmi_rx_video_info.frame_rate,
               hdmi_rx_video_info.b_progressive);

        return 0;
    }
    else
    {
        return -1;
    }
}

#include <sys/mman.h>
#define HDMI_RX_INTER_FRAME_SIZE_Y           (1920 * 1088)

enum VideoColorSpace
{
    ITU_601 ,
    ITU_709 ,
    SMPTE_240M ,
};
static int Trunc8(int v)
{
    return v >> 8;
}

static int Clamp8(int v)
{
    if(v < 0)
    {
        return 0;
    }
    else if(v > 255)
    {
        return 255;
    }
    else
    {
        return v;
    }
}

static void pixel_ycbcrL_to_argbF(unsigned char y , unsigned char cb , unsigned char cr ,
                                  unsigned char *r , unsigned char *g , unsigned char *b ,
                                  int c[3][3])
{
    int  y_2 = 0 , cr_r = 0 , cr_g = 0 , cb_g = 0 , cb_b = 0;
    int  red = 0 , green = 0 , blue = 0;

    const int y1 = (y - 16) * c[0][0];
    const int pr = cr - 128;
    const int pb = cb - 128;
    red = Clamp8(Trunc8(y1 + (pr * c[0][2])));
    green = Clamp8(Trunc8(y1 + (pb * c[1][1] + pr * c[1][2])));
    blue = Clamp8(Trunc8(y1 + (pb * c[2][1])));

    *r = red;
    *g = green;
    *b = blue;
}

static void pixel_ycbcrF_to_argbF(unsigned char y , unsigned char cb , unsigned char cr ,
                                  unsigned char *r , unsigned char *g , unsigned char *b ,
                                  int c[3][3])
{
    int  y_2 = 0 , cr_r = 0 , cr_g = 0 , cb_g = 0 , cb_b = 0;
    int  red = 0 , green = 0 , blue = 0;

    const int y1 = (y)*c[0][0];
    const int pr = cr - 128;
    const int pb = cb - 128;
    red = Clamp8(Trunc8(y1 + (pr * c[0][2])));
    green = Clamp8(Trunc8(y1 + (pb * c[1][1] + pr * c[1][2])));
    blue = Clamp8(Trunc8(y1 + (pb * c[2][1])));

    *r = red;
    *g = green;
    *b = blue;
}

static void YUV422_YUV444ORRGB(char *p_buf_y ,
                               char *p_buf_c ,
                               char *p_buf_yuv ,
                               int ori_width ,
                               int ori_height ,
                               enum VideoColorSpace color_space ,
                               bool b_yuv2rgb
)
{
    int i = 0 , j = 0 , k = 0;
    int width = (ori_width + 31) & 0xFFFFFFE0;
    int height = (ori_height + 15) & 0xFFFFFFF0;
    int h_block = (width & 0xFFFFFFE0) >> 5;
    int v_block = (height & 0xFFFFFFF0) >> 4;
    char *p_y_block = NULL;
    char *p_c_block = NULL;
    char *p_y_line = NULL;
    char *p_c_line = NULL;
    char *p_y = NULL;
    char *p_c = NULL;
    char *p_out = NULL;
    unsigned char y = 0 , u = 0 , v = 0;
    unsigned char r , g , b;
    unsigned char cur_y , cur_u , cur_v;
    int block_size = 32 * 16;
    int h_block_size = h_block * block_size;

    int c_bt601_yuvL2rgbF[3][3] =
    { {298,0 ,  409},
      {298,-100 ,-208},
      {298,516,0} };

    int c_bt709_yuvL2rgbF[3][3] =
    { {298,0 ,  459},
      {298,-55 ,-136},
      {298,541,0} };

    p_y = p_buf_y;
    p_c = p_buf_c;
    p_out = p_buf_yuv;
    p_y_block = p_buf_y;
    p_c_block = p_buf_c;
    p_y_line = p_y_block;
    p_c_line = p_c_block;
    for(i = 0; i < v_block; i++)
    {
        for(j = 0; j < 16; j++)
        {
            p_y = p_y_line;
            p_c = p_c_line;

            for(k = 0; k < width; k++)
            {
                y = *p_y++;
                if((k & 0x1) == 0)
                {
                    u = *p_c++;
                    v = *p_c++;
                }
                cur_v = v;
                cur_u = u;
                cur_y = y;

                if(b_yuv2rgb == true)
                {
                    if(k < ori_width)
                    {
                        if(color_space == ITU_601)
                        {
                            pixel_ycbcrL_to_argbF(y , u , v , &r , &g , &b , c_bt601_yuvL2rgbF);
                        }
                        else
                        {
                            pixel_ycbcrL_to_argbF(y , u , v , &r , &g , &b , c_bt709_yuvL2rgbF);
                        }

                        *p_out++ = b;
                        *p_out++ = g;
                        *p_out++ = r;
                        *p_out++ = 0xFF;
                    }
                }
                else
                {
                    if(k < ori_width)
                    {
                        *p_out++ = cur_v;
                        *p_out++ = cur_u;
                        *p_out++ = cur_y;
                        *p_out++ = 0xFF;
                    }
                }

                if(k >= 31 && (((k + 1) & 0x1F) == 0))
                {
                    p_y += block_size - 32;
                    p_c += block_size - 32;
                }
            }
            p_y_line += 32;
            p_c_line += 32;
        }
        p_y_block += h_block_size;
        p_c_block += h_block_size;
        p_y_line = p_y_block;
        p_c_line = p_c_block;
    }
}

static void get_yuv(unsigned char *p_buf_y ,
                    unsigned char *p_buf_c ,
                    int ori_width ,
                    int ori_height ,
                    int x ,
                    int y ,
                    unsigned char *y_value ,
                    unsigned char *u ,
                    unsigned char *v)
{
    int mb_x = 0;
    int mb_y = 0;
    int x1 , y1;
    int pos;
    int width1 = (ori_width + 31) & 0xFFFFFFE0;
    int mb_width = width1 >> 5;
    int pos_mb;

    mb_x = x >> 5;
    mb_y = y >> 4;
    x1 = x & 0x1F;
    y1 = y & 0xF;

    pos_mb = ((mb_width * mb_y + mb_x) << 9) + (y1 << 5);
    pos = pos_mb + x1;
    *y_value = p_buf_y[pos];

    pos = pos_mb + (x1 >> 1 << 1);
    *u = p_buf_c[pos];
    *v = p_buf_c[pos + 1];
}

static void YUV422_YUV444(unsigned char *p_buf_y ,
                          unsigned char *p_buf_c ,
                          unsigned char *p_buf_yuv ,
                          int ori_width ,
                          int ori_height)
{
    int i = 0 , j = 0 , k = 0;
    int width = (ori_width + 31) & 0xFFFFFFE0;
    int height = (ori_height + 15) & 0xFFFFFFF0;
    int h_block = (width & 0xFFFFFFE0) >> 5;
    int v_block = (height & 0xFFFFFFF0) >> 4;
    char *p_y_block = NULL;
    char *p_c_block = NULL;
    char *p_y_line = NULL;
    char *p_c_line = NULL;
    char *p_y = NULL;
    char *p_c = NULL;
    char *p_out = NULL;
    unsigned char y , u , v = 0;
    unsigned char r , g , b;
    unsigned char cur_y , cur_u , cur_v;
    unsigned char yuv[4][3];
    int index = 0;
    unsigned int value[3] = { 0 };

    int block_size = 32 * 16;
    int h_block_size = h_block * block_size;

    p_y = p_buf_y;
    p_c = p_buf_c;
    p_out = p_buf_yuv;
    p_y_block = p_buf_y;
    p_c_block = p_buf_c;
    p_y_line = p_y_block;
    p_c_line = p_c_block;

    for(i = 0; i < ori_height; i++)
    {
        for(j = 0;j < ori_width;j++)
        {
            get_yuv((unsigned char *)p_buf_y ,
                    (unsigned char *)p_buf_c ,
                    ori_width ,
                    ori_height ,
                    j ,
                    i ,
                    &cur_y , &cur_u , &cur_v);
            *p_out++ = cur_v;
            *p_out++ = cur_u;
            *p_out++ = cur_y;
            *p_out++ = 0xFF;
        }
    }
}

static void hdmi_rx_YUV422_YUV44(unsigned char *p_buf ,
                                 unsigned char *p_buf_yuv ,
                                 int ori_width ,
                                 int ori_height)
{
    unsigned char *p_y = p_buf;
    unsigned char *p_c = p_buf + HDMI_RX_INTER_FRAME_SIZE_Y;

    YUV422_YUV444(p_y ,
                  p_c ,
                  p_buf_yuv ,
                  ori_width ,
                  ori_height);
}
static int hdmi_rx_get_video_buf(int argc , char *argv[])
{
    int buf_size = 0;
    void *buf = NULL;

    if(rx_fd >= 0)
    {
        hdmi_rx_video_info_t hdmi_rx_video_info = { 0 };
        ioctl(rx_fd , HDMI_RX_GET_VIDEO_INFO , &hdmi_rx_video_info);
        printf("w:%d h:%d format:%d range:%d\n" , 
               hdmi_rx_video_info.width , 
               hdmi_rx_video_info.height ,
               hdmi_rx_video_info.color_format,
               hdmi_rx_video_info.range);
        buf_size = hdmi_rx_video_info.width * hdmi_rx_video_info.height * 4;
        buf = mmap(0 , buf_size , PROT_READ | PROT_WRITE , MAP_SHARED , rx_fd , 0);
        printf("buf = 0x%x\n" , (int)buf);
        if(vpath == HDMI_RX_VIDEO_TO_DE)
        {
            buf_yuv444 = realloc(buf_yuv444 , hdmi_rx_video_info.width * hdmi_rx_video_info.height * 4);
            printf("buf_yuv444 = 0x%x\n" , (int)buf_yuv444);
            hdmi_rx_YUV422_YUV44((unsigned char *)buf ,
                                 buf_yuv444 ,
                                 hdmi_rx_video_info.width ,
                                 hdmi_rx_video_info.height);
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

static int hdmi_rx_set_yuv2rgb_en(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int yuv2rgb_en = 0;
    
    while((opt = getopt(argc , argv , "e:")) != EOF)
    {
        switch(opt)
        {
            case 'e':
                yuv2rgb_en = atoi(optarg);
                break;
            default:
                break;
        }
    }
    if(rx_fd >= 0)
    {
        ioctl(rx_fd , HDMI_RX_SET_BUF_YUV2RGB_ONOFF , yuv2rgb_en);
        return 0;
    }
    else
    {
        return -1;
    }
}

static int hdmi_rx_dump(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    long buf_addr = 0;
    int len = 0;
    FILE *osd_fd = 0;
    long long tmp = 0;

    char *m_video_file_name = "/media/sda1/OSD.bin";
    osd_fd = fopen(m_video_file_name , "w");
    
    while((opt = getopt(argc , argv , "b:l:")) != EOF)
    {
        switch(opt)
        {
            case 'b':
                tmp = strtoll(optarg, NULL,16);
                buf_addr = (unsigned long)tmp;
                break;
            case 'l':
                len = atoi(optarg);
                break;
            default:
                break;
        }
    }
    
   // buf = (char *)0x876bc000;
    //len = 8355840;
    printf("buf_addr = 0x%lx len = %d\n" , buf_addr , len);
    fwrite((char*)buf_addr , 1 , len , osd_fd);
    fsync((int)osd_fd);
    fclose(osd_fd);
    
    return 0;
}

static int hdmi_rx_set_enc_table(void)
{
    int opt;
    opterr = 0;
    optind = 0;

    struct jpeg_enc_quant enc_table;

    if(rx_fd >= 0)
    {
        memcpy(&enc_table.dec_quant_y , decoder_quant_table_y,64);
        memcpy(&enc_table.dec_quant_c , decoder_quant_table_c , 64);
        memcpy(&enc_table.enc_quant_y , encoder_q_table_y , 128);
        memcpy(&enc_table.enc_quant_c , encoder_q_table_c , 128);

        ioctl(rx_fd , HDMI_RX_SET_ENC_QUANT , &enc_table);
        return 0;
    }
    else
    {
        return -1;
    }
}


static int hdmi_rx_set_quality(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;

    
    while((opt = getopt(argc , argv , "q:")) != EOF)
    {
        switch(opt)
        {
            case 'q':
                hdmi_rx_quality_type = atoi(optarg);
                break;
            default:
                break;
        }
    }


	if(rx_fd >= 0)
    {
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , hdmi_rx_quality_type);
        return 0;
    }
    else
    {
        return -1;
    }
 
}

static int hdmi_rx_pause(int argc , char *argv[])
{
    if(rx_fd >= 0)
    {
        hdmi_rx_stop_audio_thread();
        hdmi_rx_stop_video_thread();
        ioctl(rx_fd , HDMI_RX_PAUSE);
    }
    return 0;
}
static int hdmi_rx_resume(int argc , char *argv[])
{
    if(rx_fd >= 0)
    {
        ioctl(rx_fd , HDMI_RX_RESUME);

        hdmi_rx_start_audio_thread();
        hdmi_rx_start_video_thread();

    }
    return 0;
}


static int hdmi_rx_enter(int argc , char *argv[])
{
    return 0;
}


CONSOLE_CMD(hdmi_rx , NULL , hdmi_rx_enter , CONSOLE_CMD_MODE_SELF , "enter hdmi rx")
CONSOLE_CMD(start , "hdmi_rx" , hdmi_rx_start_test , CONSOLE_CMD_MODE_SELF , "hdmi_rx -a apath -v vpath, default video to osd, audio to tx")
CONSOLE_CMD(stop , "hdmi_rx" , hdmi_rx_stop_test , CONSOLE_CMD_MODE_SELF , "stop hdmi_rx")
CONSOLE_CMD(get_video_info , "hdmi_rx" , hdmi_rx_get_video_info , CONSOLE_CMD_MODE_SELF , "get hdmi_rx video info")
CONSOLE_CMD(get_video_buf , "hdmi_rx" , hdmi_rx_get_video_buf , CONSOLE_CMD_MODE_SELF , "get hdmi_rx video buf")
CONSOLE_CMD(set_yuv2rgb_en , "hdmi_rx" , hdmi_rx_set_yuv2rgb_en , CONSOLE_CMD_MODE_SELF , "set_yuv2rgb_en")
CONSOLE_CMD(dump , "hdmi_rx" , hdmi_rx_dump , CONSOLE_CMD_MODE_SELF , "dump data")
CONSOLE_CMD(set_quality , "hdmi_rx" , hdmi_rx_set_quality , CONSOLE_CMD_MODE_SELF , "set_quality")
CONSOLE_CMD(pause , "hdmi_rx" , hdmi_rx_pause , CONSOLE_CMD_MODE_SELF , "pause")
CONSOLE_CMD(resume , "hdmi_rx" , hdmi_rx_resume , CONSOLE_CMD_MODE_SELF , "resume")
CONSOLE_CMD(sorting_hdmi_rx_test, NULL, sorting_rx_test,CONSOLE_CMD_MODE_SELF, "soring test lvds to hdmrx")

#ifdef CONFIG_CMDS_HDMI_RX_SORTING
static int get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
	static int np = -1;
	static u32 part_num = 0;
	u32 i = 1;
	const char *label;
	char property[32];

	if (np < 0) {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
	}

	if (np < 0)
		return -1;

	if (part_num == 0)
		fdt_get_property_u_32_index(np, "part-num", 0, &part_num);

	for (i = 1; i <= part_num; i++) {
		snprintf(property, sizeof(property), "part%d-label", i);
		if (!fdt_get_property_string_index(np, property, 0, &label) &&
		    !strcmp(label, partname)) {
			memset(devpath, 0, len);
			snprintf(devpath, len, "/dev/mtdblock%d", i);
			return i;
		}
	}

	return -1;
}

static int auto_mount_eromfs(void)
{
	char devpath[64];
	int ret;
	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "eromfs");
	if (ret >= 0)
		ret = mount(devpath, "/etc", "romfs", MS_RDONLY, NULL);
	return 0;
}
__initcall(auto_mount_eromfs)
#endif
