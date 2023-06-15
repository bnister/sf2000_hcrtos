#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <hcuapi/common.h>
#include <hcuapi/snd.h>
#include <hcuapi/dis.h>
#include <hccast_com.h>
#include <hccast_av.h>
#include <hcuapi/viddec.h>
#include <hccast_log.h>
#include <unistd.h>
#include <sys/poll.h>
#include <hcuapi/snd.h>
#include <hcuapi/avsync.h>
#include <hcuapi/audsink.h>
#include <hcuapi/dis.h>


#ifndef BIT
#ifndef __ASSEMBLER__
#define BIT(nr)				(1UL << (nr))
#else
#define BIT(nr)				(1 << (nr))
#endif
#endif

int hccast_audio_set_i2so_mute(int mute)
{
    int snd_fd = -1;

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0)
    {
        hccast_log(LL_ERROR,"open snd_fd %d failed\n", snd_fd);
        return -1;
    }

    hccast_log(LL_WARNING,"[%s] %d\n", __func__, mute);
    ioctl(snd_fd, SND_IOCTL_SET_MUTE, mute);
    close(snd_fd);

    return 0;
}

void hccast_set_aspect_mode(dis_tv_mode_e ratio, 
                            dis_mode_e dis_mode, 
                            dis_scale_avtive_mode_e active_mode)
{
    int fd = open("/dev/dis", O_RDWR);
    if ( fd < 0)
        return;
    dis_aspect_mode_t aspect = {0};
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    aspect.active_mode = active_mode;
    ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}


void hccast_get_miracast_picture_area(av_area_t *src_rect)
{
    int fd = open("/dev/dis" , O_RDWR);
    if(fd < 0)
        return;

    dis_screen_info_t picture_info = { 0 };

    picture_info.distype = DIS_TYPE_HD;
    ioctl(fd , DIS_GET_MIRACAST_PICTURE_ARER , &picture_info);
    src_rect->x = picture_info.area.x;
    src_rect->y = picture_info.area.y;
    src_rect->w = picture_info.area.w;
    src_rect->h = picture_info.area.h;

    printf("%s %d %d %d %d\n",__FUNCTION__, 
           src_rect->x , 
           src_rect->y, 
           src_rect->w, 
           src_rect->h);
    close(fd);
}


void hccast_set_volume(int vol)
{
    int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0)
    {
        hccast_log(LL_ERROR,"Open /dev/sndC0i2so fail. %s\n", strerror(errno));
        return;
    }
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
    close(snd_fd);
}

int hccast_get_volume(void)
{
    int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    uint8_t vol = 0;
    if (snd_fd < 0)
    {
        return -1;
    }
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol);
    //printf("get vol: %d\n", vol);
    close(snd_fd);

    return vol;
}

void hccast_set_dis_zoom(av_area_t *src_rect,
                         av_area_t *dst_rect,
                         dis_scale_avtive_mode_e active_mode)
{
    int dis_fd = open("/dev/dis" , O_WRONLY);

    if (dis_fd >= 0) 
    {
        struct dis_zoom dz;
        dz.distype = DIS_TYPE_HD;
        dz.layer = DIS_LAYER_MAIN;
        dz.active_mode = active_mode;
        memcpy(&dz.src_area, src_rect, sizeof(struct av_area));
        memcpy(&dz.dst_area, dst_rect, sizeof(struct av_area));
        ioctl(dis_fd, DIS_SET_ZOOM, &dz);
        close(dis_fd);
    }
}


static int m_snd_fd_i2so = -1;
static int m_snd_fd_spo = -1; //SPDIF output
static inline int IS_SND_OUT_I2SO(void) 
{ 
    int dev_path = hccast_com_media_control(HCCAST_CMD_SND_DEVS_GET, 0);
    if (0 == dev_path)
        return 1;   
    else if (dev_path & AUDSINK_SND_DEVBIT_I2SO) 
        return 1;
    else
        return 0;
} 
#define IS_SND_OUT_SPDIF() (hccast_com_media_control(HCCAST_CMD_SND_DEVS_GET, 0) & AUDSINK_SND_DEVBIT_SPO)

static int _hccast_snd_dev_open(const char* dev_name, int channels, int bitdepth,int rate, int format, uint32_t pcm_source)
{
    int snd_fd = -1;
    struct snd_pcm_params params = {0};
    snd_pcm_uframes_t poll_size = 24000/(channels * bitdepth /8);


    snd_fd = open(dev_name, O_RDWR);
    if(snd_fd < 0)
    {
        hccast_log(LL_ERROR,"%s %d\n",__func__,__LINE__);
        return -1;
    }
    //printf("%s(), line:%d, dev_name:%s, fd=%d\n", __func__, __LINE__, dev_name, snd_fd);

    params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
    params.format = format;//SND_PCM_FORMAT_S16_LE;SND_PCM_FORMAT_S16_BE

    params.sync_mode =AVSYNC_TYPE_SYNCSTC;
    params.align = 0;
    params.rate = rate;

    params.channels = channels;
    params.period_size = 3072;
    params.periods = 40;
    params.bitdepth = bitdepth;
    params.start_threshold = 2;
    params.pcm_source = pcm_source;


    if(ioctl(snd_fd, SND_IOCTL_HW_PARAMS, &params) != 0)
    {
        hccast_log(LL_ERROR,"%s %d\n",__func__,__LINE__);
        return -1;
    }

    ioctl(snd_fd, SND_IOCTL_AVAIL_MIN, &poll_size);

    if(ioctl(snd_fd, SND_IOCTL_START, 0) != 0)
    {
        hccast_log(LL_ERROR,"%s %d\n",__func__,__LINE__);
        return -1;
    }
    return snd_fd;
}

static void _hccast_snd_dev_close(int snd_fd)
{
    if(snd_fd > 0)
    {
        ioctl(snd_fd, SND_IOCTL_DROP, 0);
        ioctl(snd_fd, SND_IOCTL_HW_FREE, 0);
        close(snd_fd);
    }
}


static int _hccast_snd_dev_feed(int devfd,unsigned char*buf,int size,unsigned int pts)
{
    int ret = -1;
    struct pollfd pfd = {0};

    pfd.fd = devfd;
    pfd.events = POLLOUT | POLLWRNORM;
    while (ret < 0)
    {
        struct snd_xfer xfer = {0};
        xfer.data = buf;
        xfer.frames = size/4;
        xfer.tstamp_ms = pts;
        ret = ioctl(devfd, SND_IOCTL_XFER, &xfer);
        if (ret < 0) 
        {
            poll(&pfd, 1, 500);//wait dma has free buf.
        }
    }
    return 0;
}

int hccast_snd_dev_open(int channels, int bitdepth,int rate, int format)
{
    int open_ok = 0;
    if (IS_SND_OUT_I2SO()){
        if (m_snd_fd_i2so > 0){
            close(m_snd_fd_i2so);
            m_snd_fd_i2so = -1;
        }
        m_snd_fd_i2so = _hccast_snd_dev_open("/dev/sndC0i2so", channels, bitdepth, \
            rate, format, SND_SPO_SOURCE_I2SODMA);
        if (m_snd_fd_i2so > 0)
            open_ok ++;
    }

    if (IS_SND_OUT_SPDIF()){
        if (m_snd_fd_spo > 0){
            close(m_snd_fd_spo);
            m_snd_fd_spo = -1;
        }
        m_snd_fd_spo = _hccast_snd_dev_open("/dev/sndC0spo", channels, bitdepth, \
            rate, format, SND_SPO_SOURCE_SPODMA);
        if (m_snd_fd_spo > 0)
            open_ok ++;
    }

    if (open_ok > 0)
        return 0;
    else
        return -1;
}

void hccast_snd_dev_close(void)
{
    if (IS_SND_OUT_I2SO()){
        _hccast_snd_dev_close(m_snd_fd_i2so);
        m_snd_fd_i2so = -1;
    }

    if (IS_SND_OUT_SPDIF()){
        _hccast_snd_dev_close(m_snd_fd_spo);
        m_snd_fd_spo = -1;
    }
}

int hccast_snd_dev_feed(unsigned char*buf,int size,unsigned int pts)
{
    int ret = 0;
    if (IS_SND_OUT_I2SO() && m_snd_fd_i2so > 0 ){
        ret != _hccast_snd_dev_feed(m_snd_fd_i2so, buf, size, pts);
    }

    if (IS_SND_OUT_SPDIF() && m_snd_fd_spo > 0 ){
        ret != _hccast_snd_dev_feed(m_snd_fd_spo, buf, size, pts);
    }

    return ret;
}

//drop the audio data of audio buffer
void hccast_snd_dev_flush(void)
{
    if (IS_SND_OUT_I2SO() && m_snd_fd_i2so > 0 ){
        ioctl(m_snd_fd_i2so, SND_IOCTL_DROP, 0);
    }

    if (IS_SND_OUT_SPDIF() && m_snd_fd_spo > 0 ){
        ioctl(m_snd_fd_spo, SND_IOCTL_DROP, 0);
    }
}

int hccast_set_audio_sync_thresh(int ms)
{
    int fd = 0;
    int param = ms;
    fd = open("/dev/avsync0", O_RDWR);
    if(fd < 0)
    {
        hccast_log(LL_ERROR,"%s %d\n",__func__,__LINE__);
        return -1;
    }
    else
    {
        if (ioctl(fd, AVSYNC_SET_AUD_SYNC_THRESH, param) != 0)
        {
            hccast_log(LL_ERROR,"%s %d\n",__func__,__LINE__);
        }
        hccast_log(LL_NOTICE,"%s %d, param:%d\n",__func__,__LINE__,param);
    }

    close(fd);
    return 0;
}

int hccast_get_audio_sync_thresh()
{
    int fd = 0;
    int param = 0;
    fd = open("/dev/avsync0", O_RDWR);
    if(fd < 0)
    {
        hccast_log(LL_ERROR,"%s %d\n",__func__,__LINE__);
        return -1;
    }
    else
    {
        if (ioctl(fd, AVSYNC_GET_AUD_SYNC_THRESH, &param) != 0)
        {
            hccast_log(LL_ERROR,"%s %d\n",__func__,__LINE__);
        }
        hccast_log(LL_NOTICE,"%s %d, param:%d\n",__func__,__LINE__,param);
    }

    close(fd);
    return param;
}

static uint32_t m_snd_devs = 0;
uint32_t hccast_com_media_control(int cmd, uint32_t param)
{
    switch (cmd)
    {
    case HCCAST_CMD_SND_DEVS_SET:
        m_snd_devs = param;
        break;
    case HCCAST_CMD_SND_DEVS_GET:
        return m_snd_devs;
        break;
    }

    return 0;
}

int hccast_get_current_pic_info(struct dis_display_info *mpinfo)
{
    int fd = -1;

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }

    mpinfo->distype = DIS_TYPE_HD;
    mpinfo->info.layer = DIS_PIC_LAYER_MAIN;
    ioctl(fd , DIS_GET_DISPLAY_INFO , (uint32_t)mpinfo);
    close(fd);
    return 0;
}
