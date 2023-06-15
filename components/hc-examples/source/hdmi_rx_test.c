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

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#else
#include <kernel/lib/console.h>
#endif

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/hdmi_rx.h>

#include "wav.h"

static pthread_t rx_audio_read_thread_id = 0;
static pthread_t rx_video_read_thread_id = 0;
static int rx_fd = -1;
static bool stop_read = 0;
static int akshm_fd = -1;
static int vkshm_fd = -1;
static FILE *arecfile = NULL;
static FILE *vrecfile = NULL;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;

static void *rx_audio_read_thread(void *args)
{
    int data_size = 0;
    uint8_t *data = NULL;

    if(arecfile)
    {
        struct wave_header header = { 0 };
        generate_wave_header(&header , 44100 , 16 , 2);
        fwrite(&header , sizeof(struct wave_header) , 1 , arecfile);
    }

    //printf("rx_audio_read_thread run\n");
    while(!stop_read && akshm_fd >= 0)
    {
        AvPktHd hdr = { 0 };
        while(read(akshm_fd , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd))
        {
            //printf("read audio hdr from kshm err\n");
            usleep(20 * 1000);
            if(stop_read)
            {
                goto end;
            }
        }
        printf("apkt size %d\n" , (int)hdr.size);

        if(data_size < hdr.size)
        {
            data_size = hdr.size;
            if(data)
            {
                data = realloc(data , data_size);
            }
            else
            {
                data = malloc(data_size);
            }
            if(!data)
            {
                printf("no memory\n");
                goto end;
            }
        }

        while(read(akshm_fd , data , hdr.size) != hdr.size)
        {
            //printf("read audio data from kshm err\n");
            usleep(20 * 1000);
            if(stop_read)
            {
                goto end;
            }
        }

        //printf("adata: 0x%x, 0x%x, 0x%x, 0x%x\n", data[0], data[1], data[2], data[3]);
        if(arecfile)
        {
            fwrite(data , hdr.size , 1 , arecfile);
        }
        usleep(1000);
    }

end:
    if(data)
        free(data);

    (void)args;

    return NULL;
}

static void *rx_video_read_thread(void *args)
{
    int data_size = 0;
    uint8_t *data = NULL;

    //printf("rx_video_read_thread run\n");
    while(!stop_read && vkshm_fd >= 0)
    {
        AvPktHd hdr = { 0 };
        while(read(vkshm_fd , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd))
        {
            //printf("read audio hdr from kshm err\n");
            usleep(20 * 1000);
            if(stop_read)
            {
                goto end;
            }
        }
        printf("vpkt size %d\n" , (int)hdr.size);

        if(data_size < hdr.size)
        {
            data_size = hdr.size;
            if(data)
            {
                data = realloc(data , data_size);
            }
            else
            {
                data = malloc(data_size);
            }
            if(!data)
            {
                printf("no memory\n");
                goto end;
            }
        }

        while(read(vkshm_fd , data , hdr.size) != hdr.size && !stop_read)
        {
            //printf("read audio data from kshm err\n");
            usleep(20 * 1000);
            if(stop_read)
            {
                goto end;
            }
        }

        //printf("vdata: 0x%x, 0x%x, 0x%x, 0x%x\n", data[0], data[1], data[2], data[3]);
        if(vrecfile)
        {
            fwrite(data , hdr.size , 1 , vrecfile);
        }
        usleep(1000);
    }

end:
    if(data)
        free(data);

    (void)args;

    return NULL;
}

static int hdmi_rx_stop(int argc , char *argv[])
{
    stop_read = 1;
    if(rx_audio_read_thread_id)
        pthread_join(rx_audio_read_thread_id , NULL);
    if(rx_video_read_thread_id)
        pthread_join(rx_video_read_thread_id , NULL);
    rx_audio_read_thread_id = rx_video_read_thread_id = 0;

    if(arecfile)
        fclose(arecfile);
    if(vrecfile)
        fclose(vrecfile);
    arecfile = vrecfile = NULL;

    if(akshm_fd >= 0)
        close(akshm_fd);
    if(vkshm_fd >= 0)
        close(vkshm_fd);
    akshm_fd = vkshm_fd = -1;

    if(rx_fd >= 0)
    {
        ioctl(rx_fd , HDMI_RX_STOP);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , 0);
        close(rx_fd);
    }
    rx_fd = -1;

    (void)argc;
    (void)argv;
    return 0;
}

static int hdmi_rx_start(int argc , char *argv[])
{
    enum HDMI_RX_VIDEO_DATA_PATH vpath = HDMI_RX_VIDEO_TO_OSD;
    enum HDMI_RX_AUDIO_DATA_PATH apath = HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX;
    struct kshm_info rx_audio_read_hdl = { 0 };
    struct kshm_info rx_video_read_hdl = { 0 };

    pthread_attr_t attr;
    int opt;

    opterr = 0;
    optind = 0;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr , 0x1000);

    if(rx_fd >= 0)
    {
        hdmi_rx_stop(0, 0);
    }

    rx_fd = open("/dev/hdmi_rx" , O_WRONLY);
    if(rx_fd < 0)
    {
        goto err;
    }

    while((opt = getopt(argc , argv , "a:v:r:m:")) != EOF)
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
                rotate_mode = atoi(optarg) % 4;
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                break;
            default:
                break;
        }
    }

    printf("apath %d, vpath %d\n" , apath , vpath);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_DATA_PATH , vpath);
    ioctl(rx_fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);

    if(apath == HDMI_RX_AUDIO_TO_I2SI_AND_KSHM || apath == HDMI_RX_AUDIO_TO_I2SI_AND_KSHM_AND_BYPASS_TO_HDMI_TX)
    {
        arecfile = fopen("/media/hdd/astream" , "wb");
        if(!arecfile)
        {
            goto err;
        }

        akshm_fd = open("/dev/kshmdev" , O_RDONLY);
        if(akshm_fd < 0)
        {
            goto err;
        }

        ioctl(rx_fd , HDMI_RX_AUDIO_KSHM_ACCESS , &rx_audio_read_hdl);
        printf("get audio hdl, kshm desc 0x%x\n" , (int)rx_audio_read_hdl.desc);
        ioctl(akshm_fd , KSHM_HDL_SET , &rx_audio_read_hdl);

        stop_read = 0;
        if(pthread_create(&rx_audio_read_thread_id , &attr ,
                          rx_audio_read_thread , NULL))
        {
            printf("audio kshm recv thread create failed\n");
            goto err;
        }
    }

    if(vpath == HDMI_RX_VIDEO_TO_KSHM ||
       vpath == HDMI_RX_VIDEO_TO_DE_AND_KSHM ||
       vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM)
    {
        vrecfile = fopen("/media/hdd/vstream" , "wb");
        if(!vrecfile)
        {
            goto err;
        }

        vkshm_fd = open("/dev/kshmdev" , O_RDONLY);
        if(vkshm_fd < 0)
        {
            goto err;
        }

        ioctl(rx_fd , HDMI_RX_VIDEO_KSHM_ACCESS , &rx_video_read_hdl);
        printf("get video hdl, kshm desc 0x%x\n" , (int)rx_video_read_hdl.desc);
        ioctl(vkshm_fd , KSHM_HDL_SET , &rx_video_read_hdl);

        stop_read = 0;
        if(pthread_create(&rx_video_read_thread_id , &attr ,
                          rx_video_read_thread , NULL))
        {
            printf("video kshm recv thread create failed\n");
            goto err;
        }
    }

    if(vpath == HDMI_RX_VIDEO_TO_DE_ROTATE ||
       vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM)
    {
        printf("rotate_mode = 0x%x\n" , rotate_mode);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_mode);
		
        printf("mirror_mode = 0x%x\n" , mirror_mode);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_mode);
    }

    ioctl(rx_fd , HDMI_RX_START);
    printf("hdmi_rx start ok```\n");

    return 0;

err:
    hdmi_rx_stop(0 , 0);
    return -1;
}

static struct termios stored_settings;
static void exit_console(int signo)
{
    (void)signo;
    hdmi_rx_stop(0 , 0);
    tcsetattr(0 , TCSANOW , &stored_settings);
    exit(0);
}

int main(int argc , char *argv[])
{
    struct termios new_settings;

    tcgetattr(0 , &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0 , TCSANOW , &new_settings);

    signal(SIGTERM , exit_console);
    signal(SIGINT , exit_console);
    signal(SIGSEGV , exit_console);
    signal(SIGBUS , exit_console);
    console_init("hdmi_rx:");

    console_register_cmd(NULL , "start" , hdmi_rx_start , CONSOLE_CMD_MODE_SELF , "start -v vmode");
    console_register_cmd(NULL , "stop" , hdmi_rx_stop , CONSOLE_CMD_MODE_SELF , "stop");

    console_start();
    exit_console(0);
    (void)argc;
    (void)argv;
    return 0;
}
