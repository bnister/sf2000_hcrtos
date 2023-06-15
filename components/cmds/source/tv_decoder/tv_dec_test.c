#include <fcntl.h>
#include <unistd.h>
#include <kernel/vfs.h>
#include <stdio.h>
#include <kernel/io.h>
#include <getopt.h>
#include <malloc.h>

#include <kernel/lib/console.h>
#include <kernel/completion.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/tvdec.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/vidmp.h>
#include <stdlib.h>

static int tv_dec_fd = -1;
static struct completion video_read_task_completion;
static bool video_stop_read = 0;
static struct kshm_info video_read_hdl = { 0 };
static enum TVDEC_VIDEO_DATA_PATH vpath = TVDEC_VIDEO_TO_DE;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;
static enum TVTYPE tv_sys = TV_NTSC;
static bool tv_dec_started = false;
static unsigned int stop_mode = 0;

static void tv_dec_video_read_thread(void *args)
{
    struct kshm_info *hdl = (struct kshm_info *)args;
    AvPktHd hdr = { 0 };
    uint8_t *data = malloc(1024 * 1024);

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
    }
    usleep(1000);

    if(data)
        free(data);

    complete(&video_read_task_completion);
    vTaskSuspend(NULL);
}

static int tv_dec_start(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int value = 0;

    if(tv_dec_started == true)
    {
        return 0;
    }

    vpath = TVDEC_VIDEO_TO_DE;
    rotate_mode = ROTATE_TYPE_0;
    mirror_mode = MIRROR_TYPE_NONE;


    tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
    if(tv_dec_fd < 0)
    {
        return -1;
    }

    while((opt = getopt(argc , argv , "v:r:t:m:s:")) != EOF)
    {
        switch(opt)
        {
            case 'v':
                vpath = atoi(optarg);
                break;
            case 'r':
                rotate_mode = atoi(optarg);
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                break;
            case 't':
                value = atoi(optarg);
                if(value == 0)
                {
                    tv_sys = TV_PAL;
                }
                else
                {
                    tv_sys = TV_NTSC;
                }
                break;
            case 's':
                stop_mode = atoi(optarg);
                break;
            default:
                break;
        }
    }

    printf("vpath %d\n" , vpath);
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , vpath);

    if(vpath == TVDEC_VIDEO_TO_KSHM || 
       vpath == TVDEC_VIDEO_TO_DE_AND_KSHM||
       vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM)
    {
        int ret;
        struct kshm_info kshm_hdl;

        ioctl(tv_dec_fd , TVDEC_VIDEO_KSHM_ACCESS , &video_read_hdl);

        init_completion(&video_read_task_completion);
        video_stop_read = 0;
        ret = xTaskCreate(tv_dec_video_read_thread , "video_read_thread" ,
                          0x1000 , &video_read_hdl , portPRI_TASK_HIGH , NULL);
        if(ret != pdTRUE)
        {
            printf("kshm recv thread create failed\n");
        }
    }

    if(vpath == TVDEC_VIDEO_TO_DE_ROTATE ||
       vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM)
    {
        printf("rotate_mode = 0x%x\n" , rotate_mode);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_ROTATE_MODE , rotate_mode);

        printf("mirror_mode = 0x%x\n" , mirror_mode);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_MIRROR_MODE , mirror_mode);

    }
    printf("stop_mode = 0x%x\n" , stop_mode);
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_STOP_MODE , stop_mode);
    ioctl(tv_dec_fd , TVDEC_START , tv_sys);
    printf("tv_dec start ok\n");
    tv_dec_started = true;
    return 0;
}

static int tv_dec_stop(int argc , char *argv[])
{
    int opt;
    
    
    if(tv_dec_fd >= 0)
    {
        video_stop_read = 1;
        tv_dec_started = false;

        if(vpath == TVDEC_VIDEO_TO_KSHM ||
           vpath == TVDEC_VIDEO_TO_DE_AND_KSHM ||
           vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM)
        {
            wait_for_completion_timeout(&video_read_task_completion , 3000);
        }

        ioctl(tv_dec_fd , TVDEC_STOP);
        close(tv_dec_fd);
        tv_dec_fd = -1;
        return 0;
    }
    else
    {
        return -1;
    }
   
}

static int tv_dec_get_video_info(int argc , char *argv[])
{
    struct tvdec_video_info video_info = { 0 };
    if(tv_dec_fd >= 0)
    {
        ioctl(tv_dec_fd , TVDEC_GET_VIDEO_INFO , &video_info);
        printf("video_info.tv_sys =%d  width =%d height =%d framerate = %d progressive = %d\n" ,
               video_info.tv_sys ,
               video_info.width ,
               video_info.height ,
               video_info.frame_rate ,
               video_info.b_progressive
        );
        return 0;
    }
    else
    {
        return -1;
    }
}
static int tv_dec_enter(int argc , char *argv[])
{
    return 0;
}
CONSOLE_CMD(tv_dec , NULL , tv_dec_enter , CONSOLE_CMD_MODE_SELF , "enter tv dec")
CONSOLE_CMD(start , "tv_dec" , tv_dec_start , CONSOLE_CMD_MODE_SELF , "tv_dec_test")
CONSOLE_CMD(stop , "tv_dec" , tv_dec_stop , CONSOLE_CMD_MODE_SELF , "stop tv_dec")
CONSOLE_CMD(get_video_info , "tv_dec" , tv_dec_get_video_info , CONSOLE_CMD_MODE_SELF , "get_video_info")
