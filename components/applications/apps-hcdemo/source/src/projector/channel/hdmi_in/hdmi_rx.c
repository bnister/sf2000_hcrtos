#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <malloc.h>
#include <kernel/io.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/fb.h>
#include <kernel/lib/console.h>
#include <nuttx/wqueue.h>
#include <kernel/lib/fdt_api.h>
#include <cpu_func.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/snd.h>
#include <hcuapi/audsink.h>

#include "hdmi_rx.h"
#include "../local_mp/com_api.h"
#include "factory_setting.h"
#include "screen.h"
#include "../../setup/setup.h"
#include <bluetooth.h>

static struct hdmi_switch g_switch;
static int hdcpkey_set = 0;

#define QT_TEST_EN  // just for qt hrx test

#ifndef HDMI_SWITCH_HDMI_RX
#define HDMI_SWITCH_HDMI_RX

extern int *snd_spectrum_run(int *data, int ch, int bitdepth, int rate, int size, int *collumn_num);
extern void snd_spectrum_stop(void);

int hdmi_rx_leave(void);
int hdmi_rx_enter(void);
static int projector_hdcpkey_load(uint8_t *key);
static int load_hdcpkey_from_udisk(uint8_t *key);

#if 0
static int set_zoom(void)
{
    struct dis_zoom zoom = {0};
    int ret = 0;

    zoom.distype = DIS_TYPE_HD;
    zoom.layer = DIS_LAYER_MAIN;

    zoom.src_area.x = 0;
    zoom.src_area.y = 0;
    zoom.src_area.w = 1920;
    zoom.src_area.h = 1080;

#if 0
    zoom.dst_area.x = (1920-(360*1920/480))/2;
    zoom.dst_area.y = 0;
    zoom.dst_area.w = (360*1920)/480;
    zoom.dst_area.h = 1080;
#else
    zoom.dst_area.x = (1920-1280)/2;
    zoom.dst_area.y = (1080-480)/2;
    zoom.dst_area.w = 1280;
    zoom.dst_area.h = 480;
#endif

    printf("%s:%d: distype=%d, layer=%d, src(%d,%d,%d,%d), dst(%d,%d,%d,%d)\n", __func__, __LINE__,
        zoom.distype, zoom.layer,
        zoom.src_area.x, zoom.src_area.y, zoom.src_area.w, zoom.src_area.h,
        zoom.dst_area.x, zoom.dst_area.y, zoom.dst_area.w, zoom.dst_area.h);

    ret = ioctl(g_switch.dis_fd, DIS_SET_ZOOM, &zoom);
    if( ret != 0 ){
        printf("%s:%d: warning: DIS_SET_ZOOM failed\n", __func__, __LINE__);
        return -1;
    }

    printf("%s:%d: \n", __func__, __LINE__);

    return 0;
}
#endif

#if 1// HDMI rx audio
#ifndef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#endif
#define MAX_TRANSFER_SIZE    (1024)

struct hrx_snd {
    struct audsink_init_params cfg;
    int fd;
};
static struct completion audio_read_task_completion;
static bool audio_stop_read = 0;
static struct kshm_info audio_read_hdl = { 0 };
static int apath = HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO;
static struct hrx_snd *g_sndhdl=NULL;

#if 0
static void *hrx_snd_init(int bits, int channels, int samplerate, uint32_t size)
{
    uint32_t times;
    struct hrx_snd *p =
        (struct hrx_snd *)malloc(sizeof(struct hrx_snd));
    memset(&p->cfg , 0 , sizeof(struct audsink_init_params));
    struct snd_pcm_params *params = &p->cfg.pcm;

    times = size / 960;
    if (times > 5)
        times = 5;
    if (times < 1)
        times = 1;

    p->cfg.buf_size = times * channels * samplerate * bits / 16;
    p->cfg.sync_type = 0;
    p->cfg.audio_flush_thres = 0;
    p->cfg.snd_devs = AUDSINK_SND_DEVBIT_I2SO;

    params->sync_mode = AVSYNC_TYPE_UPDATESTC;
    params->access = SND_PCM_ACCESS_RW_INTERLEAVED;
    params->format = SND_PCM_FORMAT_S16;
    params->align = SND_PCM_ALIGN_RIGHT;
    params->rate = samplerate;
    params->channels = channels;
    params->bitdepth = bits;
    params->period_size = 1536;
    params->periods = times * 16;
    params->start_threshold = MAX(1, params->periods / 5);

    p->fd = open("/dev/audsink" , O_RDWR);
    if( p->fd < 0 ) {
        printf("Open /dev/audsink error.\n");
        free(p);
        return NULL;
    }

    if( ioctl(p->fd , AUDSINK_IOCTL_INIT , &p->cfg) != 0 ) {
        printf("Init audsink error.");
        close(p->fd);
        free(p);
        return NULL;
    }

    return p;
}
#endif

static int hrx_snd_mix(void *phandle, uint8_t *audio_frame, size_t packet_size)
{
    uint32_t pts;
    int ret = 0;
    uint8_t *data = NULL;
    int xfer_frames = 0;
    struct audsink_xfer xfer = {0};
    struct pollfd pfd = {0};
    struct hrx_snd *p = (struct hrx_snd *)phandle;

    if(phandle==NULL)
        return 0;

    data = (void *)audio_frame;
    xfer_frames = packet_size / (p->cfg.pcm.channels * p->cfg.pcm.bitdepth / 8);
    xfer.pitch = xfer_frames;
    pts = -1;
    pfd.fd = p->fd;
    pfd.events = POLLOUT | POLLWRNORM;

    do {
        if (xfer_frames > MAX_TRANSFER_SIZE) {
            xfer.frames = MAX_TRANSFER_SIZE;
            xfer.tstamp_ms = pts;
            xfer.data = data;
            do {
                ret = ioctl(p->fd, AUDSINK_IOCTL_XFER, &xfer);
                if (ret < 0 && !audio_stop_read) {
                    poll(&pfd, 1, 100);
                }
            } while(ret < 0 && !audio_stop_read);

            xfer_frames -= xfer.frames;
            pts += xfer.frames * 1000 / p->cfg.pcm.rate;
            if (p->cfg.pcm.access == SND_PCM_ACCESS_RW_INTERLEAVED) {
                data += xfer.frames * p->cfg.pcm.channels * p->cfg.pcm.bitdepth / 8;
            } else {
                data += xfer.frames * p->cfg.pcm.bitdepth / 8;
            }
        } else {
            xfer.frames = xfer_frames;
            xfer.tstamp_ms = pts;
            xfer.data = data;
            do {
                ret = ioctl(p->fd, AUDSINK_IOCTL_XFER, &xfer);
                if (ret < 0 && !audio_stop_read) {
                    poll(&pfd, 1, 100);
                }
            } while(ret < 0 && !audio_stop_read);
            xfer_frames = 0;
        }
    } while (xfer_frames > 0 && !audio_stop_read);

    return 0;
}

static void audio_read_thread(void *args)
{
    struct kshm_info *hdl = (struct kshm_info *)args;
    AvPktHd hdr = { 0 };
    int snd_fd = -1;
    uint8_t *data = NULL;//malloc(1024 );
    static uint32_t hdr_size = 0;
    //int collumn_num = 12;
    //int *spectrum = NULL;
    //void *g_sndhdl = NULL;
    struct snd_hw_info hw_info = {0};
    printf("audio_read_thread run```\n");
    while(!audio_stop_read)
    {
        while(kshm_read(hdl , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd) && !audio_stop_read)
        {
            usleep(20 * 1000);
        }
        if(hdr.size == 0)
            continue;

		if(hdr_size < hdr.size){
			 //printf(" >>> hdr_size: %d -- hdr.size:%d - 0x%x<<<\n", hdr_size, hdr.size, hdr.size);
             data = realloc(data, hdr.size);
             hdr_size = hdr.size;
		}

        while(kshm_read(hdl , data , hdr.size) != hdr.size && !audio_stop_read)
        {
            usleep(20 * 1000);
        }

        //this code can be processs in driver
	#if 0
        if (apath == HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO) {
            if (g_sndhdl == NULL) {
                snd_fd = open("/dev/sndC1spin" , O_RDWR);
                ioctl(snd_fd, SND_IOCTL_GET_HW_INFO, &hw_info);
                g_sndhdl = hrx_snd_init(hw_info.pcm_params.bitdepth,
                    hw_info.pcm_params.channels, hw_info.pcm_params.rate, hdr.size);
                close(snd_fd);
            }

            hrx_snd_mix(g_sndhdl, data, hdr.size);
        }

        if (apath == HDMI_RX_AUDIO_TO_I2SI_AND_I2SO) {
            if (g_sndhdl == NULL) {
                snd_fd = open("/dev/sndC0i2si" , O_RDWR);
                ioctl(snd_fd, SND_IOCTL_GET_HW_INFO, &hw_info);
                g_sndhdl = hrx_snd_init(hw_info.pcm_params.bitdepth,
                    hw_info.pcm_params.channels, hw_info.pcm_params.rate, hdr.size);
                close(snd_fd);
            }

            hrx_snd_mix(g_sndhdl, data, hdr.size);
        }
	#endif

#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
        spectrum = snd_spectrum_run((int *)data, 2, 16, 48000, hdr.size, &collumn_num);
        if (spectrum && collumn_num > 0) {
            printf("spectrum:");
            for (int i = 0; i < collumn_num; i++) {
                printf(" %d", spectrum[i]);
            }
            printf("\n");
        }
#endif
        usleep(8 * 1000);
    }
    usleep(1000);

#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
    snd_spectrum_stop();
#endif

    if(data)
        free(data);

    hdr_size = 0;
    complete(&audio_read_task_completion);
    vTaskSuspend(NULL);
}


#endif
#if 1//def HDMI_SWITCH_HDMI_RX_DIS_PILLBOX
static void hdmi_rx_set_aspect_mode(dis_tv_mode_e ratio , dis_mode_e dis_mode)
{
    int ret = 0;
    dis_aspect_mode_t aspect = { 0 };

    printf("ratio: %d, dis_mode: %d\n" , ratio , dis_mode);
    int fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
    	return;
    }

    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    ret = ioctl(fd, DIS_SET_ASPECT_MODE , &aspect);
    if( ret != 0 ){
        printf("%s:%d: err: DIS_SET_ASPECT_MODE failed\n", __func__, __LINE__);
        close(fd);
        return;
    }
    close(fd);
    return;
}
#else
static void hdmi_rx_set_aspect_mode(dis_tv_mode_e ratio , dis_mode_e dis_mode)
{
    return;
}
#endif

static int hdmi_rx_start(void)
{
    int ret = 0;
    sys_param_t * psys_param;
    psys_param = projector_get_sys_param();
    rotate_type_e rotate_type=ROTATE_TYPE_0;
    mirror_type_e mirror_type=MIRROR_TYPE_NONE;
    int rotate = projector_get_some_sys_param(P_INIT_ROTATE);
    int h_flip = projector_get_some_sys_param(P_INIT_H_FLIP);
    int v_flip = projector_get_some_sys_param(P_INIT_V_FLIP);
    printf("%s:%d: \n", __func__, __LINE__);

#ifdef HDMI_SWITCH_HDMI_RX_TO_HDMI_TX
    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_AUDIO_DATA_PATH , HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX);
    if(ret != 0){
        printf("HDMI_RX_SET_AUDIO_DATA_PATH failed\n");
        return -1;
    }
#else
    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);
    if(ret != 0){
        printf("HDMI_RX_SET_AUDIO_DATA_PATH failed\n");
        return -1;
    }

    if ((apath >= HDMI_RX_AUDIO_TO_I2SI_AND_KSHM) &&
        (apath != HDMI_RX_AUDIO_TO_I2SI_AND_I2SO) &&
        (apath != HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO))
    {
        //int ret;
        //struct kshm_info kshm_hdl;

        ioctl(g_switch.in.fd , HDMI_RX_AUDIO_KSHM_ACCESS , &audio_read_hdl);

        init_completion(&audio_read_task_completion);
        audio_stop_read = 0;
        ret = xTaskCreate(audio_read_thread , "audio_read_thread" ,
                          0x1000 , &audio_read_hdl , portPRI_TASK_HIGH , NULL);
        if(ret != pdTRUE)
        {
            printf("kshm recv thread create failed\n");
        }
    }
#endif

    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_STOP_MODE , 0);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }

    if(psys_param->sysdata.flip_mode == FLIP_MODE_NORMAL){
        if(rotate==0)
            rotate_type=ROTATE_TYPE_0;
        else if(rotate==90)
            rotate_type=ROTATE_TYPE_270;
        else if(rotate==180)
            rotate_type=ROTATE_TYPE_180;
        else if(rotate==270)
            rotate_type=ROTATE_TYPE_90;
        else
            rotate_type=ROTATE_TYPE_180;
        if(h_flip)
            mirror_type = MIRROR_TYPE_LEFTRIGHT;
        if(v_flip)
        {
            if(rotate_type<ROTATE_TYPE_180)
                rotate_type+=ROTATE_TYPE_180;
            else
                rotate_type-=ROTATE_TYPE_180;
            mirror_type = MIRROR_TYPE_LEFTRIGHT;
        }
    }
    else  { // rotate or mirror
        if(psys_param->sysdata.flip_mode == FLIP_MODE_ROTATE_180){
            if(rotate==0)
                rotate_type=ROTATE_TYPE_180;
            else if(rotate==90)
                rotate_type=ROTATE_TYPE_90;
            else if(rotate==180)
                rotate_type=ROTATE_TYPE_0;
            else if(rotate==270)
                rotate_type=ROTATE_TYPE_270;
            else
                rotate_type=ROTATE_TYPE_180;

            if(h_flip)
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            if(v_flip)
            {
                if(rotate_type<ROTATE_TYPE_180)
                    rotate_type+=ROTATE_TYPE_180;
                else
                    rotate_type-=ROTATE_TYPE_180;
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            }
        }
        else if(psys_param->sysdata.flip_mode == FLIP_MODE_H_MIRROR){
            if(rotate==0)
                rotate_type=ROTATE_TYPE_0;
            else if(rotate==90)
                rotate_type=ROTATE_TYPE_270;
            else if(rotate==180)
                rotate_type=ROTATE_TYPE_180;
            else if(rotate==270)
                rotate_type=ROTATE_TYPE_90;
            else
                rotate_type=ROTATE_TYPE_0;
            if(h_flip==0)
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            if(v_flip)
            {
                if(rotate_type<ROTATE_TYPE_180)
                    rotate_type+=ROTATE_TYPE_180;
                else
                    rotate_type-=ROTATE_TYPE_180;
                mirror_type = MIRROR_TYPE_NONE;
            }
        }
        else if(psys_param->sysdata.flip_mode == FLIP_MODE_V_MIRROR){
            if(rotate==0)
                rotate_type=ROTATE_TYPE_180;
            else if(rotate==90)
                rotate_type=ROTATE_TYPE_90;
            else if(rotate==180)
                rotate_type=ROTATE_TYPE_0;
            else if(rotate==270)
                rotate_type=ROTATE_TYPE_270;
            else
                rotate_type=ROTATE_TYPE_180;
            if(h_flip==0)
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            if(v_flip)
            {
                if(rotate_type<ROTATE_TYPE_180)
                    rotate_type+=ROTATE_TYPE_180;
                else
                    rotate_type-=ROTATE_TYPE_180;
                mirror_type = MIRROR_TYPE_NONE;
            }
        }
    }


    if(rotate_type>ROTATE_TYPE_0)
        ret = ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE_ROTATE);
    else
        ret = ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }
    ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_type);
    ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_type);
    ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , JPEG_ENC_QUALITY_TYPE_HIGH_QUALITY);

    if(hdcpkey_set == 0)
    {
        struct hdmi_rx_hdcp_key hdcpk;
        if(0==projector_hdcpkey_load(hdcpk.hdcp_key)){
            if((hdcpk.hdcp_key[0]==0xff)&& (hdcpk.hdcp_key[1]==0xff)){
           #ifdef  QT_TEST_EN
                ret = load_hdcpkey_from_udisk(hdcpk.hdcp_key);
                if(ret <0)
                    printf("No hdcp rx key\n");
                else
                    ret = ioctl(g_switch.in.fd, HDMI_RX_SET_HDCP_KEY , &hdcpk);
           #endif
            }
            else
                ret = ioctl(g_switch.in.fd, HDMI_RX_SET_HDCP_KEY , &hdcpk);
            if(ret != 0){
                printf("Error: HDMI_RX_SET_HDCP_KEY failed\n");
                //return -1;
            }
            hdcpkey_set = 1;
        }
        else{
            printf("Error: load hdcpkey fail\n");
        }
    }
    hdmi_rx_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE);

    if(bt_get_connet_state()>=BT_CONNECT_STATUS_CONNECTED)
	{
		bluetooth_set_gpio_mutu(1);
		app_set_i2so_gpio_mute(1);
	}
	else
	{
		app_set_i2so_gpio_mute(0);
        bluetooth_set_gpio_mutu(0);
	}

    ret = ioctl(g_switch.in.fd, HDMI_RX_START);
    if(ret != 0){
        printf("HDMI_RX_START failed\n");
        return -1;
    }

    g_switch.in.enable = 1;

    return 0;
}

static int hdmi_rx_stop(void)
{
    if(g_switch.in.fd > 0){
        audio_stop_read = 1;
        if ((apath >= HDMI_RX_AUDIO_TO_I2SI_AND_KSHM) &&
            (apath != HDMI_RX_AUDIO_TO_I2SI_AND_I2SO) &&
            (apath != HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO))
        {
            wait_for_completion_timeout(&audio_read_task_completion , 3000);
        }
        if (g_sndhdl)
        {
            close(g_sndhdl->fd);
            free(g_sndhdl);
            g_sndhdl = NULL;
         }
        ioctl(g_switch.in.fd , HDMI_RX_STOP);
        close(g_switch.in.fd);
        g_switch.in.fd = -1;
    }

    return 0;
}

#if  HDMI_RX_FLIP_SMOTHLY
void hdmi_rx_set_flip_mode(flip_mode_e flip_mode)
{
    //printf(">>> %s,  %d\n", __func__,flip_mode);
    if(g_switch.in.fd>0){
        if(flip_mode == FLIP_MODE_NORMAL){
            ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE);
        }
        else { // rotate or mirror
            ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE_ROTATE);
            if(flip_mode == FLIP_MODE_ROTATE_180){
                ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , ROTATE_TYPE_180);
            }
            else if(flip_mode == FLIP_MODE_H_MIRROR){
                ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , ROTATE_TYPE_0);
                ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_MIRROR_MODE , MIRROR_TYPE_LEFTRIGHT);
            }
            else if(flip_mode == FLIP_MODE_V_MIRROR){
                ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , ROTATE_TYPE_180);
                ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_MIRROR_MODE , MIRROR_TYPE_LEFTRIGHT);
            }
        }
    }
}
#endif
/* stop show bootlogo, end show hdmi in picture */
static void notifier_hdmi_in_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.in.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGIN;

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(0);
#endif

    /* scale hdmi rx picture */
    //usleep(300*1000);
    //set_zoom();

    return ;
}

/* start show bootlogo */
static void notifier_hdmi_in_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.in.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGOUT;

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(1);
#endif
    return ;
}


static void notifier_hdmi_in_err(void *arg, unsigned long param){
    printf("%s:%d: \n", __func__, __LINE__);
    g_switch.in.plug_status = HDMI_SWITCH_HDMI_STATUS_ERR_INPUT;

    #ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(1);
#endif
    return ;

}

static int hdmi_hotplug_rx_enable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.in.notify_plugin.evtype   = HDMI_RX_NOTIFY_CONNECT;
    g_switch.in.notify_plugin.qid          = LPWORK;
    g_switch.in.notify_plugin.remote   = false;
    g_switch.in.notify_plugin.oneshot  = false;
    g_switch.in.notify_plugin.qualifier  = NULL;
    g_switch.in.notify_plugin.arg          = (void *)&g_switch;
    g_switch.in.notify_plugin.worker2  = notifier_hdmi_in_plugin;
    work_notifier_setup(& g_switch.in.notify_plugin);

    g_switch.in.notify_plugout.evtype   = HDMI_RX_NOTIFY_DISCONNECT;
    g_switch.in.notify_plugout.qid          = LPWORK;
    g_switch.in.notify_plugout.remote   = false;
    g_switch.in.notify_plugout.oneshot  = false;
    g_switch.in.notify_plugout.qualifier  = NULL;
    g_switch.in.notify_plugout.arg          = (void *)&g_switch;
    g_switch.in.notify_plugout.worker2  = notifier_hdmi_in_plugout;
    work_notifier_setup(& g_switch.in.notify_plugout);

    g_switch.in.notify_err_input.evtype = HDMI_RX_NOTIFY_ERR_INPUT;
    g_switch.in.notify_err_input.qid = LPWORK;
    g_switch.in.notify_err_input.remote = false;
    g_switch.in.notify_err_input.oneshot = false;
    g_switch.in.notify_err_input.qualifier = NULL;
    g_switch.in.notify_err_input.arg = (void *)&g_switch;
    g_switch.in.notify_err_input.worker2 = notifier_hdmi_in_err;
    work_notifier_setup(& g_switch.in.notify_err_input);

    return 0;
}

static int hdmi_hotplug_rx_disable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    return 0;
}
int hdmirx_get_plug_status(void)
{
    return g_switch.in.plug_status;
}

int hdmi_rx_leave(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    if( g_switch.in.fd > 0){
        hdmi_rx_stop();
        hdmi_hotplug_rx_disable();
        close( g_switch.in.fd);
        g_switch.in.fd = -1;
    }

    return 0;
}

int hdmi_rx_enter(void)
{
   //int ret = 0;

    printf("%s:%d: \n", __func__, __LINE__);

    if(g_switch.in.fd  <=0)
    {
        g_switch.in.fd = open("/dev/hdmi_rx" , O_RDWR);
        if( g_switch.in.fd < 0){
            printf("open /dev/hdmi_rx failed, ret=%d\n", g_switch.in.fd);
            return -1;
        }

        g_switch.hdmi_rx.distype = DIS_TYPE_HD;
        g_switch.hdmi_rx.info.layer = HDMI_SWITCH_HDMI_RX_LAYER;

        hdmi_hotplug_rx_enable();

        hdmi_rx_start();
        printf("%s:%d:  really\n", __func__, __LINE__);
    }

    return 0;
}

/* Load HDCP key*/
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

static void dump_buf(char *tag, uint8_t *buf, int len)
{
     int i;

     printf("******%s*******\n", tag);
     for(i = 0; i < len; i++)
    {
         if(i %16==0)
             printf("\n");
           printf("%02x ", buf[i]);
    }
    printf("\n\n");
}

int projector_hdcpkey_load(uint8_t *key)
{
        int fd = -1;
        int ret = -1;
        char devpath[64];

        ret = get_mtdblock_devpath(devpath, sizeof(devpath), "individual");
        if (ret < 0) {
            return -1;
        }
        printf("%s devpath:%s\n", __func__, devpath);
        fd = open(devpath, O_RDONLY);
        if (fd < 0) {
            printf("Error:  Open %s failed\n", devpath);
            return -1;
        }

        memset(key, 0, HDCP_KEY_LEN);
        read(fd, key,HDCP_KEY_LEN);
        //dump_buf("read flash/individual", key, HDCP_KEY_LEN);

        close(fd);

	return 0;
}

/* store projector system parameters to flash */
int load_hdcpkey_from_udisk(uint8_t *key)
{
        int fd, ret=-1;
        uint8_t key_buf[HDCP_KEY_LEN];
        char devpath[64];
        if(mmp_get_usb_stat()==USB_STAT_MOUNT)
        {
            //read from udisk first
            fd = open("/media/sda/hdmirxkey.bin",O_RDWR);
            if(fd> 0) {
                if(HDCP_KEY_LEN !=read(fd, key_buf, HDCP_KEY_LEN))
                {
                    printf("read *.bin fail\n");
                    close(fd);
                    return -1;
                }
                close(fd);
                fd = -1;
            }
            else{
                return -1;
            }
            dump_buf("read udisk",  key_buf, HDCP_KEY_LEN);
            //write it to flash/individual
            ret = get_mtdblock_devpath(devpath, sizeof(devpath), "individual");
            if (ret < 0) {
                return -1;
            }
            fd = open(devpath, O_RDWR);
            if (fd <0){
            	printf("Error: open %s failed \n", devpath);
            	return -1;
            }

             if(HDCP_KEY_LEN !=write(fd, key_buf, HDCP_KEY_LEN)){
                 printf("Error: write %s failed \n", devpath);
                 ret = -1;
             }
             else{
                printf("Write key to flash/individual success!\n");
#ifdef  QT_TEST_EN
                fsync(fd);
                close(fd);
                reset();
#endif
                memcpy(key, key_buf, HDCP_KEY_LEN);
                ret = 0;
            }
            close(fd);
            return ret;
        }
        return -1;
}

#endif


