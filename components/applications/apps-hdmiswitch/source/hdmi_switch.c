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
#include <malloc.h>

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
#include <kernel/lib/libfdt/libfdt.h>

#include "hdmi_switch.h"

#define HDMI_SWITCH_HDMI_STATUS_PLUGOUT		0
#define HDMI_SWITCH_HDMI_STATUS_PLUGIN		1
#define HDMI_SWITCH_BOOTLOGO_LAYER			DIS_LAYER_MAIN
#define HDMI_SWITCH_HDMI_RX_LAYER           DIS_LAYER_AUXP

#define HDMI_SWITCH_DEFAULT_DISP_RESOLUTION TVSYS_1080P_60
#define HDMI_SWITCH_GET_EDID_TRY_TIME		500

struct hdmi_info{
    int fd;

    struct work_notifier_s notify_plugin;
    struct work_notifier_s notify_plugout;
    struct work_notifier_s notify_edid;

    char plug_status;
    char enable;
};

struct hdmi_switch{
    struct hdmi_info in;
    struct hdmi_info out;

    int dis_fd;
    struct dis_display_info bootlogo;
    struct dis_display_info hdmi_rx;
    struct dis_tvsys hdmi_out_tvsys;
    enum TVSYS last_tv_sys;
};

struct hdmi_switch g_switch;

#ifdef HDMI_SWITCH_HDMI_TX
static struct completion get_edid_task_completion;
static struct completion get_edid_task_stop_completion;
static int get_edid_task_run = 0;
static int try_get_edid = 0;
static int hdmi_out_force_4k = 0;
static int hdmi_out_force_1080p = 0;
static int hdmi_out_force_720p = 0;
static enum TVSYS tv_sys_4k = TVSYS_4096X2160P_30;
static enum TVSYS tv_sys_1080p = TVSYS_1080P_60;
static enum TVSYS tv_sys_720p = TVSYS_720P_60;
#endif
static int video_rotate_mode = 0;


#ifdef HDMI_SWITCH_BACK_BOOTLOGO
#if 1
static void print_display_info(struct dis_display_info *display_info, char *lable)
{
    if(display_info){
        printf("\n%s info: \n", lable);
        printf("distype         = %d\n", display_info->distype);
        printf("de_index        = %d\n", display_info->info.de_index);
        printf("layer           = %d\n", display_info->info.layer);
        printf("mf_buf          = 0x%x\n", (unsigned int)(long)display_info->info.mf_buf);
        printf("mf_buf_size     = %ld\n", display_info->info.mf_buf_size);
        printf("y_buf           = 0x%x\n", (unsigned int)(long)display_info->info.y_buf);
        printf("y_buf_size      = %ld\n", display_info->info.y_buf_size);
        printf("c_buf           = 0x%x\n", (unsigned int)(long)display_info->info.c_buf);
        printf("c_buf_size      = %ld\n", display_info->info.c_buf_size);
        printf("pic_height      = %ld\n", display_info->info.pic_height);
        printf("pic_width       = %ld\n", display_info->info.pic_width);
        printf("de_map_mode     = %d\n", display_info->info.de_map_mode);
        printf("status          = %d\n", display_info->info.status);
    }

    return;
}
#else
static void print_display_info(struct dis_display_info *display_info, char *lable)
{
    return;
}
#endif

/****************************************************************************
 * Name: backup_bootlogo
 *
 * Description:
 *   backup hdmi out picture.
 *
 * Input Parameters:
 *   distype - display type.
 *   layer   - picture layer.
 *   backup  - 0: only print current picture info; 1: backup picture.
 *
 * Returned Value:
 *   0: success
 *   1: failed
 *
 ****************************************************************************/
static int show_and_backup_hdmi_out_picture(struct dis_display_info *info, int backup)
{
#if 0
    int ret = 0;

    if( g_switch.dis_fd < 0){
        printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
        return -1;
    }

    g_switch.bootlogo.distype = DIS_TYPE_HD;
    g_switch.bootlogo.info.layer = HDMI_SWITCH_BOOTLOGO_LAYER;

    if (backup) {
        ret = ioctl(g_switch.dis_fd, DIS_BACKUP_MP, info->distype);
        if (ret != 0) {
            printf("%s:%d: DIS_BACKUP_MP failed, ret=%d\n", __func__, __LINE__, ret);
            return -1;
        }
    }

    return 0;

#else

    struct dis_display_info display_info = {0};
    int ret = 0;

    if( g_switch.dis_fd < 0){
        printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
        return -1;
    }

    info->distype = DIS_TYPE_HD;
    info->info.layer = HDMI_SWITCH_BOOTLOGO_LAYER;

    if(backup){
        /* get logo info */
        display_info.distype = info->distype;
        display_info.info.layer = info->info.layer;
        ret = ioctl(g_switch.dis_fd, DIS_GET_DISPLAY_INFO, &display_info);
        if(ret != 0){
            printf("%s:%d: DIS_GET_DISPLAY_INFO failed, ret=%d\n", __func__, __LINE__, ret);
            return -1;
        }
        print_display_info(&display_info, "old");

        /* backup logo data to system memory */
        memcpy(info, &display_info, sizeof(struct dis_display_info));
        if(display_info.info.mf_buf_size){
            info->info.mf_buf = (uint32_t)memalign(256, display_info.info.mf_buf_size);
            if(info->info.mf_buf == 0){
                printf("%s:%d: mf_buf memalign failed\n", __func__, __LINE__);
                return -1;
            }
            memcpy((void *)info->info.mf_buf, (void *)display_info.info.mf_buf, display_info.info.mf_buf_size);
        }

        if(display_info.info.y_buf_size){
            info->info.y_buf = (uint32_t)memalign(256, display_info.info.y_buf_size);
            if(info->info.y_buf == 0){
                printf("%s:%d: y_buf memalign failed\n", __func__, __LINE__);
                goto err;
            }
            memcpy((void *)info->info.y_buf, (void *)display_info.info.y_buf, display_info.info.y_buf_size);
        }

        if(display_info.info.c_buf_size){
            info->info.c_buf = (uint32_t)memalign(256, display_info.info.c_buf_size);
            if(info->info.c_buf == 0){
                printf("%s:%d: c_buf memalign failed\n", __func__, __LINE__);
                goto err;
            }
            memcpy((void *)info->info.c_buf, (void *)display_info.info.c_buf, display_info.info.c_buf_size);
        }
    }

    print_display_info(info, "new");

    /* show logo */
    ret = ioctl(g_switch.dis_fd, DIS_SET_DISPLAY_INFO, info);
    if(ret != 0){
        printf("%s:%d: DIS_GET_DISPLAY_INFO failed, ret=%d\n", __func__, __LINE__, ret);
        goto err;
    }

    return 0;

err:
    if(info->info.mf_buf){
        free((void *)info->info.mf_buf);
        info->info.mf_buf = 0;
    }

    if(info->info.y_buf){
        free((void *)info->info.y_buf);
        info->info.y_buf = 0;
    }

    if(info->info.c_buf){
        free((void *)info->info.c_buf);
        info->info.c_buf = 0;
    }

    return -1;
#endif
}

static int onoff_show_picture(enum DIS_TYPE distype, enum DIS_PIC_LAYER layer, int on)
{
    int ret = 0;
    struct dis_win_onoff winon = {0};

    printf("%s:%d: distype=%d, layer=%d, on=%d\n", __func__, __LINE__, distype, layer, on);

    if( g_switch.dis_fd < 0){
        printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
        return -1;
    }

    winon.distype = distype;
    winon.layer = layer;
    winon.on = on;

    if(winon.distype != DIS_TYPE_SD && winon.distype != DIS_TYPE_HD &&
        winon.distype != DIS_TYPE_UHD) {
        printf("error display type %d\n" , winon.distype);
        return -1;
    }

    if(winon.layer != DIS_LAYER_MAIN && winon.layer != DIS_LAYER_AUXP) {
        printf("error display layer %d\n" , winon.layer);
        return -1;
    }

    if(winon.on != 0 && winon.layer != 1) {
        printf("error display on %d\n" , winon.on);
        return -1;
    }

    if(on){
        show_and_backup_hdmi_out_picture(&g_switch.bootlogo, 0);
    }

    ret = ioctl(g_switch.dis_fd , DIS_SET_WIN_ONOFF , &winon);
    if(ret != 0){
        printf("DIS_SET_WIN_ONOFF failed, ret=%d\n", ret);
        return -1;
    }

    return 0;
}

static int onoff_show_bootlogo(int on)
{
    return onoff_show_picture(g_switch.bootlogo.distype, g_switch.bootlogo.info.layer, on);
}

#endif

#ifdef HDMI_SWITCH_HDMI_TX
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
            tvsys->tvtype = TV_LINE_4096X2160_30;
            tvsys->progressive = 1;
        break;

        default:
            tvsys->tvtype = TV_LINE_720_30;
            tvsys->progressive = 1;
    }

    printf("%s:%d: tvtype=%d\n", __func__, __LINE__, tvsys->tvtype);
    printf("%s:%d: progressive=%d\n", __func__, __LINE__, tvsys->progressive);

    return 0;
}

static int __set_hdmi_out_tvsys(struct dis_tvsys *tvsys)
{
    int ret = 0;

    if( g_switch.dis_fd < 0){
        printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
        return -1;
    }

    ret = ioctl(g_switch.dis_fd , DIS_SET_TVSYS , tvsys);
    if(ret < 0){
        printf("DIS_SET_TVSYS failed, ret=%d\n", ret);
        return -1;
    }

    return 0;
}

static int set_hdmi_out_tvsys(enum TVSYS tv_sys)
{
    struct dis_tvsys out_tvsys = {0};

    printf("%s:%d: tv_sys= %d\n", __func__, __LINE__, tv_sys);

    if(hdmi_out_force_4k){
        tv_sys = tv_sys_4k;
        }else if(hdmi_out_force_1080p){
            tv_sys = tv_sys_1080p;
        }else if(hdmi_out_force_720p){
            tv_sys = tv_sys_720p;
    }

    tvsys_to_tvtype(tv_sys, &out_tvsys);
    if(g_switch.hdmi_out_tvsys.tvtype !=out_tvsys.tvtype
        ||g_switch.hdmi_out_tvsys.progressive !=out_tvsys.progressive){

        printf("%s:%d: set new display resolution, old(%d, %d), new(%d, %d)\n",
                __func__, __LINE__, g_switch.hdmi_out_tvsys.tvtype, g_switch.hdmi_out_tvsys.progressive,
                out_tvsys.tvtype, out_tvsys.progressive);

        g_switch.hdmi_out_tvsys.tvtype =out_tvsys.tvtype;
        g_switch.hdmi_out_tvsys.progressive =out_tvsys.progressive;
        g_switch.last_tv_sys = tv_sys;
        __set_hdmi_out_tvsys(&g_switch.hdmi_out_tvsys);
    }

    return 0;
}

static int get_hdmi_out_tvsys(struct dis_tvsys *tvsys)
{
    int ret = 0;

    if(tvsys->distype != DIS_TYPE_HD){
        printf("warning: tvsys->distype is DIS_TYPE_HD\n");
        tvsys->distype = DIS_TYPE_HD;
    }

    if( g_switch.dis_fd < 0){
        printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
        return -1;
    }

    ret = ioctl(g_switch.dis_fd , DIS_GET_TVSYS , tvsys);
    if(ret < 0){
        printf("DIS_SET_TVSYS failed, ret=%d\n", ret);
        return -1;
    }

    printf("%s:%d: distype=%d\n", __func__, __LINE__, tvsys->distype);
    printf("%s:%d: layer=%d\n", __func__, __LINE__, tvsys->layer);
    printf("%s:%d: tvtype=%d\n", __func__, __LINE__, tvsys->tvtype);
    printf("%s:%d: progressive=%d\n", __func__, __LINE__, tvsys->progressive);

    return 0;
}

/* nothing todo */
static void notifier_hdmi_out_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.out.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGIN;

    /* create thread to get edid info, if not recieve edid notify */
    try_get_edid = 1;
    complete(&get_edid_task_completion);

    return ;
}

/* nothing todo */
static void notifier_hdmi_out_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.out.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGOUT;
    try_get_edid = 0;

    return ;
}

/* detect the HDMI format and adapt it */
static void notifier_hdmi_out_edidready(void *arg, unsigned long param)
{
    struct hdmi_switch *h_switch = (struct hdmi_switch *)arg;
    struct hdmi_edidinfo *edidinfo = (struct hdmi_edidinfo *)param;
    struct dis_tvsys out_tvsys = {0};

    printf("%s:%d: \n", __func__, __LINE__);

    if(h_switch == NULL){
        printf("%s:%d: switch is null\n", __func__, __LINE__);
        return ;
    }

    try_get_edid = 0;

    if(hdmi_out_force_4k){
        int i = 0;
        int tvsys_4096 = 0;
        int tvsys_3840 = 0;

        for(i = 0; i < HDMI_TX_MAX_EDID_TVSYS_NUM; i++){
            printf("%s:%d: %d, %d\n", __func__, __LINE__, i, edidinfo->tvsys[i]);

            if(edidinfo->tvsys[i] == TVSYS_3840X2160P_30){
                tvsys_3840 = 1;
            }

            if(edidinfo->tvsys[i] == TVSYS_4096X2160P_30){
                tvsys_4096 = 1;
            }
        }

        if (tvsys_4096) {
            tv_sys_4k = TVSYS_4096X2160P_30;
        } else if(tvsys_3840){
            tv_sys_4k = TVSYS_3840X2160P_30;
        }else{
                tv_sys_4k = TVSYS_4096X2160P_30;
        }
    }

    /* update hdmi out info */
    if(g_switch.last_tv_sys != edidinfo->best_tvsys){
        set_hdmi_out_tvsys(edidinfo->best_tvsys);
    }else{
        printf("%s:%d: last_tv_sys(%d) == tv_sys(%d), not set resolution\n",
                __func__, __LINE__, g_switch.last_tv_sys, edidinfo->best_tvsys);
    }

    /* switch hdmi format and show picture */
    if(g_switch.in.plug_status == HDMI_SWITCH_HDMI_STATUS_PLUGIN){
        /* show hdmi in picture */
        /* nothing to do */
    }else{
        /* show bootlogo */
        onoff_show_bootlogo(1);
    }

    return ;
}

static int hdmi_hotplug_tx_enable(void)
{
    g_switch.out.notify_plugin.evtype   = HDMI_TX_NOTIFY_CONNECT;
    g_switch.out.notify_plugin.qid          = LPWORK;
    g_switch.out.notify_plugin.remote   = false;
    g_switch.out.notify_plugin.oneshot  = false;
    g_switch.out.notify_plugin.qualifier  = NULL;
    g_switch.out.notify_plugin.arg          = (void *)&g_switch;
    g_switch.out.notify_plugin.worker2  = notifier_hdmi_out_plugin;
    work_notifier_setup(& g_switch.out.notify_plugin);

    g_switch.out.notify_plugout.evtype   = HDMI_TX_NOTIFY_DISCONNECT;
    g_switch.out.notify_plugout.qid          = LPWORK;
    g_switch.out.notify_plugout.remote   = false;
    g_switch.out.notify_plugout.oneshot  = false;
    g_switch.out.notify_plugout.qualifier  = NULL;
    g_switch.out.notify_plugout.arg          = (void *)&g_switch;
    g_switch.out.notify_plugout.worker2  = notifier_hdmi_out_plugout;
    work_notifier_setup(& g_switch.out.notify_plugout);

    g_switch.out.notify_edid.evtype   = HDMI_TX_NOTIFY_EDIDREADY;
    g_switch.out.notify_edid.qid          = LPWORK;
    g_switch.out.notify_edid.remote   = false;
    g_switch.out.notify_edid.oneshot  = false;
    g_switch.out.notify_edid.qualifier  = NULL;
    g_switch.out.notify_edid.arg          = (void *)&g_switch;
    g_switch.out.notify_edid.worker2  = notifier_hdmi_out_edidready;
    work_notifier_setup(& g_switch.out.notify_edid);

    return 0;
}

static int hdmi_hotplug_tx_disable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    return 0;
}

int get_edid(int argc , char *argv[])
{
    int ret = 0;
    enum TVSYS tv_sys = 0;

    ret = ioctl(g_switch.out.fd, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
    if(ret == 0){
        printf("%s:%d: tv_sys=%d\n", __func__, __LINE__, tv_sys);
    }else{
        printf("%s:%d: warning: HDMI_TX_GET_EDID_TVSYS failed\n", __func__, __LINE__);
    }

    return 0;
}

static void get_edid_thread(void *args)
{
    struct dis_tvsys out_tvsys = {0};
    enum TVSYS tv_sys = 0;
    int ret = 0;
    int try_time = HDMI_SWITCH_GET_EDID_TRY_TIME;
    int i = 0;

    //printf("%s:%d: \n", __func__, __LINE__);
    //printf("%s:%d: \n", __func__, __LINE__);
    while(get_edid_task_run){
        wait_for_completion(&get_edid_task_completion);

        //printf("%s:%d: \n", __func__, __LINE__);

        /* check display resolution */
        i = 0;
        while(i < try_time && try_get_edid){
            ret = ioctl(g_switch.out.fd, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
            if(ret == 0){
                if(g_switch.last_tv_sys != tv_sys){
                    set_hdmi_out_tvsys(tv_sys);
                }else{
                    printf("%s:%d: last_tv_sys(%d) == tv_sys(%d), not set resolution\n",
                        __func__, __LINE__, g_switch.last_tv_sys, tv_sys);
                }
                break;
            }

            printf("%s:%d: try_get_edid time=%d\n", __func__, __LINE__, i);

            i++;
            usleep(10000);
        }

        //printf("%s:%d: time=%d, try_get_edid=%d\n", __func__, __LINE__, i, try_get_edid);
        //printf("%s:%d: time=%d, try_get_edid=%d\n", __func__, __LINE__, i, try_get_edid);

        try_get_edid = 0;
        if(i == try_time){
            printf("%s:%d: try_get_edid timeout, set display resolution to default \n", __func__, __LINE__);
            if(g_switch.last_tv_sys == TVSYS_INVALID){
                /* set display resolution to default value */
                set_hdmi_out_tvsys(HDMI_SWITCH_DEFAULT_DISP_RESOLUTION);
            }else{
                /* set display resolution to last value */
                set_hdmi_out_tvsys(g_switch.last_tv_sys);
            }
        }
    }

    printf("%s:%d: \n", __func__, __LINE__);

    complete(&get_edid_task_stop_completion);
    vTaskSuspend(NULL);

    return;
}

static int hdmi_tx_exit(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    hdmi_hotplug_tx_disable();

    get_edid_task_run = 0;
    try_get_edid = 0;
    complete(&get_edid_task_completion);
    wait_for_completion_timeout(&get_edid_task_stop_completion, 3000);

    if( g_switch.out.fd >= 0){
        close( g_switch.out.fd);
        g_switch.out.fd = -1;
    }

    return 0;
}

static int hdmi_tx_init(void)
{
    int ret = 0;
    enum TVSYS tv_sys = 0;
    struct dis_tvsys out_tvsys = {0};

    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.out.fd = open("/dev/hdmi" , O_RDONLY);
    if( g_switch.out.fd < 0){
        printf("open /dev/hdmi_tx failed, ret=%d\n", g_switch.out.fd);
        goto err;
    }

    /* get default tvsys */
    g_switch.hdmi_out_tvsys.distype = DIS_TYPE_HD;
    ret = get_hdmi_out_tvsys(&g_switch.hdmi_out_tvsys);
    if(ret != 0){
        printf("get_hdmi_out_tvsys failed, ret=%d\n", ret);
        goto err;
    }

    init_completion(&get_edid_task_completion);
    init_completion(&get_edid_task_stop_completion);
    get_edid_task_run = 1;
    ret = xTaskCreate(get_edid_thread , "get_edid_thread" ,
                          0x1000 , NULL , portPRI_TASK_HIGH , NULL);
    if(ret != pdTRUE){
        printf("get_edid_thread create failed\n");
    }

    /* get edid, try to check display resolution */
    ret = ioctl(g_switch.out.fd, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
    if(ret != 0){
        printf("HDMI_TX_GET_EDID_TVSYS failed, ret=%d\n", ret);
        goto err;
    }

    printf("%s:%d: tv_sys= %d\n", __func__, __LINE__, tv_sys);
    if(g_switch.last_tv_sys != tv_sys){
        set_hdmi_out_tvsys(tv_sys);
    }else{
        printf("%s:%d: last_tv_sys(%d) == tv_sys(%d), not set resolution\n",
            __func__, __LINE__, g_switch.last_tv_sys, tv_sys);
    }

    hdmi_hotplug_tx_enable();

    return 0;

err:
    hdmi_tx_exit();
    return -1;
}

int set_out_tvsys(int argc , char *argv[])
{
    struct dis_tvsys hdmi_out_tvsys = {0};

    int opt = 0;
    int on = 0;

    opterr = 0;
    optind = 0;

    while((opt = getopt(argc , argv , "l:d:t:p:")) != EOF) {
        switch(opt) {
            case 'l':
                hdmi_out_tvsys.layer = atoi(optarg);
            break;

            case 'd':
                hdmi_out_tvsys.distype = atoi(optarg);
            break;

            case 't':
                hdmi_out_tvsys.tvtype = atoi(optarg);
            break;

            case 'p':
                hdmi_out_tvsys.progressive = atoi(optarg);
            break;

            default:
            break;
        }
    }

    __set_hdmi_out_tvsys(&hdmi_out_tvsys);

    return 0;
}
#endif

#ifdef HDMI_SWITCH_HDMI_RX

static int get_video_rotate(void)
{
	static int np = -1;
	const char *status;
	int ret = 0;
	int ui_rotate = 0;

    np = fdt_get_node_offset_by_path("/hcrtos/rotate");
    if(np < 0){
        printf("%s:%d: fdt_get_node_offset_by_path failed\n", __func__, __LINE__);
        return -1;
    }

    ret = fdt_get_property_string_index(np, "status", 0, &status);
    if(ret != 0){
        printf("%s:%d: fdt_get_property_u_32_index ir_key failed\n", __func__, __LINE__);
    }

    if(strcmp(status, "okay")){
        video_rotate_mode = 0;
        printf("%s:%d: rotate is disable\n", __func__, __LINE__);
        return -1;
    }

    ret = fdt_get_property_u_32_index(np, "rotate", 0, &ui_rotate);
    if(ret != 0){
        printf("%s:%d: fdt_get_property_u_32_index ir_key failed\n", __func__, __LINE__);
    }

    printf("%s:%d: ui_rotate=%d\n", __func__, __LINE__, ui_rotate);

    /* Convert ui rotate to video rotate */
    switch(ui_rotate){
        case 0:
            video_rotate_mode = 0;
            break;

        case 90:
            video_rotate_mode = 3;
            break;

        case 180:
            video_rotate_mode = 2;
            break;

        case 270:
            video_rotate_mode = 1;
            break;

        default:
            video_rotate_mode = 0;
    }

    printf("%s:%d: video_rotate_mode=%d\n", __func__, __LINE__, video_rotate_mode);

    return 0;
}


#if 0
static int set_fb(uint16_t h_div, uint16_t v_div, uint16_t h_mul, uint16_t v_mul)
{
    int ret = 0;
    int fd = -1;
    struct hcfb_scale scale = {0};

    fd = open("/dev/fb0" , O_RDWR);
    if( fd < 0){
        printf("open /dev/fb0 failed, ret=%d\n", fd);
        return -1;
    }

    scale.h_div = h_div;
    scale.v_div = v_div;
    scale.h_mul = h_mul;
    scale.v_mul = v_mul;

    printf("%s:%d: h_div=%d, v_div=%d, h_mul=%d, v_mul=%d\n", __func__, __LINE__,
            scale.h_div, scale.v_div, scale.h_mul, scale.v_mul);

    ret = ioctl(fd, HCFBIOSET_SCALE, &scale);
    if( ret != 0 ){
        printf("%s:%d: warning: HCFBIOSET_SCALE failed\n", __func__, __LINE__);
        close(fd);
        fd = -1;
        return -1;
    }

    close(fd);

    return 0;
}
#endif

#ifdef HDMI_SWITCH_HDMI_RX_SET_ZOOM_480x640
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

#if defined(HDMI_SWITCH_HDMI_RX_SET_ZOOM_480x640)
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

#ifdef HDMI_SWITCH_HDMI_RX_DIS_PILLBOX
static void hdmi_rx_set_aspect_mode(dis_tv_mode_e ratio , dis_mode_e dis_mode)
{
    int ret = 0;
    dis_aspect_mode_t aspect = { 0 };

    printf("ratio: %d, dis_mode: %d\n" , ratio , dis_mode);

    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    ret = ioctl(g_switch.dis_fd, DIS_SET_ASPECT_MODE , &aspect);
    if( ret != 0 ){
        printf("%s:%d: err: DIS_SET_ASPECT_MODE failed\n", __func__, __LINE__);
        return;
    }

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
    enum HDMI_RX_VIDEO_DATA_PATH data_path = HDMI_RX_VIDEO_TO_OSD;

    printf("%s:%d: \n", __func__, __LINE__);

    get_video_rotate();

    if(video_rotate_mode){
        data_path = HDMI_RX_VIDEO_TO_DE_ROTATE;
    }else{
        data_path = HDMI_RX_VIDEO_TO_DE;
    }

    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , data_path);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_DATA_PATH failed\n");
        return -1;
    }

#ifdef HDMI_SWITCH_HDMI_RX_TO_HDMI_TX
    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_AUDIO_DATA_PATH , HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX);
    if(ret != 0){
        printf("HDMI_RX_SET_AUDIO_DATA_PATH failed\n");
        return -1;
    }
#else
    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_AUDIO_DATA_PATH , HDMI_RX_AUDIO_BYPASS_TO_I2SO);
    if(ret != 0){
        printf("HDMI_RX_SET_AUDIO_DATA_PATH failed\n");
        return -1;
    }
#endif

    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_STOP_MODE , 1);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }

    ret = ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , video_rotate_mode);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }

    hdmi_rx_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX);

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
    if(g_switch.in.fd >= 0){
        ioctl(g_switch.in.fd , HDMI_RX_STOP);
        close(g_switch.in.fd);
        g_switch.in.fd = -1;
    }

    return 0;
}

/* stop show bootlogo, end show hdmi in picture */
static void notifier_hdmi_in_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.in.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGIN;

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(0);
#endif

    /* scale hdmi rx picture */
#ifdef HDMI_SWITCH_HDMI_RX_SET_ZOOM_480x640
    usleep(300*1000);
    set_zoom();
#endif

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

    return 0;
}

static int hdmi_hotplug_rx_disable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    return 0;
}

static int hdmi_rx_exit(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    hdmi_rx_stop();
    hdmi_hotplug_rx_disable();

    if( g_switch.in.fd >= 0){
        close( g_switch.in.fd);
        g_switch.in.fd = -1;
    }

    return 0;
}

static int hdmi_rx_init(void)
{
    int ret = 0;

    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.in.fd = open("/dev/hdmi_rx" , O_RDWR);
    if( g_switch.in.fd < 0){
        printf("open /dev/hdmi_rx failed, ret=%d\n", g_switch.in.fd);
        return -1;
    }

    g_switch.hdmi_rx.distype = DIS_TYPE_HD;
    g_switch.hdmi_rx.info.layer = HDMI_SWITCH_HDMI_RX_LAYER;

    hdmi_hotplug_rx_enable();

    hdmi_rx_start();

    printf("%s:%d: \n", __func__, __LINE__);

    return 0;
}

#endif

static int hdmi_init(void)
{
    int ret = 0;

#ifdef HDMI_SWITCH_HDMI_TX
    ret = hdmi_tx_init();
    if(ret != 0){
        printf("hdmi_tx_init failed\n");
        return -1;
    }
#endif

#ifdef HDMI_SWITCH_HDMI_RX
    ret = hdmi_rx_init();
    if(ret != 0){
        printf("hdmi_rx_init failed\n");
        goto err;
    }
#endif

    return 0;
err:
#ifdef HDMI_SWITCH_HDMI_TX
    hdmi_tx_exit();
#endif
    return -1;
}

static int hdmi_exit(void)
{
#ifdef HDMI_SWITCH_HDMI_TX
    hdmi_tx_exit();
#endif

#ifdef HDMI_SWITCH_HDMI_RX
    hdmi_rx_exit();
#endif

    return 0;
}

int hdmi_switch (void)
{
    int ret = 0;

    memset(&g_switch, 0, sizeof(struct hdmi_switch));
    g_switch.in.fd = -1;
    g_switch.out.fd = -1;
    g_switch.dis_fd = -1;

    g_switch.dis_fd = open("/dev/dis" , O_RDWR);
    if( g_switch.dis_fd < 0){
        printf("open /dev/dis failed, ret=%d\n", g_switch.dis_fd);
        return -1;
    }

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
    /* backup bootlogo */
    show_and_backup_hdmi_out_picture(&g_switch.bootlogo, 1);
#endif

    ret = hdmi_init();
    if(ret != 0){
        printf("err: hdmi_init failed\n");
        goto err;
    }

    return 0;
err:
    close(g_switch.dis_fd);
    g_switch.dis_fd = -1;
    return -1;
}

#ifdef HDMI_SWITCH_BACK_BOOTLOGO

int backup_picture(int argc , char *argv[])
{
    struct dis_display_info display_info = {0};
    int backup = 0;
    int opt;

    opterr = 0;
    optind = 0;

    while((opt = getopt(argc , argv, "d:l:b:")) != EOF) {
        switch(opt) {
            case 'd':
                display_info.distype = atoi(optarg);
            break;

            case 'l':
                display_info.info.layer = atoi(optarg);
            break;

            case 'b':
                backup = atoi(optarg);
            break;

            default:
            break;
        }
    }

    show_and_backup_hdmi_out_picture(&display_info, backup);

    return 0;
}

int print_layer(int argc , char *argv[])
{
    struct dis_display_info display_info = {0};
    int opt;

    opterr = 0;
    optind = 0;

    while((opt = getopt(argc , argv, "d:l:")) != EOF) {
        switch(opt) {
            case 'd':
                display_info.distype = atoi(optarg);
            break;

            case 'l':
                display_info.info.layer = atoi(optarg);
            break;

            default:
            break;
        }
    }

    show_and_backup_hdmi_out_picture(&display_info, 0);

    return 0;
}

int show_picture(int argc , char *argv[])
{
    enum DIS_TYPE distype = 0;
    enum DIS_PIC_LAYER layer = 0;
    int opt = 0;
    int on = 0;

    opterr = 0;
    optind = 0;

    while((opt = getopt(argc , argv , "l:d:o:")) != EOF) {
    switch(opt) {
        case 'l':
            layer = atoi(optarg);
        break;

        case 'd':
            distype = atoi(optarg);
        break;

        case 'o':
            on = atoi(optarg);
        break;

        default:
        break;
        }
    }

    onoff_show_picture(distype, layer, on);

    return 0;
}

#endif

#ifdef HDMI_SWITCH_DE_EFFECT_ADJUST
int get_enhance(int argc , char *argv[])
{
    struct hcfb_enhance eh = {0};
    int fd = -1;
    int ret = 0;

    int opt = 0;
    int on = 0;

    opterr = 0;
    optind = 0;

    printf("%s:%d: \n", __func__, __LINE__);

    fd = open("/dev/fb0" , O_RDWR);
    if( fd < 0){
        printf("open /dev/fb0 failed, ret=%d\n", fd);
        return -1;
    }

    printf("%s:%d: fd=%d\n", __func__, __LINE__, fd);

    ret = ioctl(fd, HCFBIOGET_ENHANCE, &eh);
    if( ret != 0 ){
        printf("%s:%d: warning: HCFBIOGET_ENHANCE failed\n", __func__, __LINE__);
        close(fd);
        fd = -1;
        return -1;
    }

    printf("%s:%d: fd=%d\n", __func__, __LINE__, fd);

    printf("brightness  = %d\n", eh.brightness);
    printf("contrast    = %d\n", eh.contrast);
    printf("saturation  = %d\n", eh.saturation);
    printf("hue         = %d\n", eh.hue);
    printf("sharpness   = %d\n", eh.sharpness);

    printf("%s:%d: fd=%d\n", __func__, __LINE__, fd);

    close(fd);
    fd = -1;

    return 0;
}

int set_enhance(int argc , char *argv[])
{
    struct hcfb_enhance eh = {0};
    int fd = -1;
    int ret = 0;

    int opt = 0;

    opterr = 0;
    optind = 0;

    printf("%s:%d: \n", __func__, __LINE__);

    while((opt = getopt(argc , argv , "b:c:s:h:a:")) != EOF) {
        switch(opt) {
            case 'b':
                eh.brightness = atoi(optarg);
            break;

            case 'c':
                eh.contrast = atoi(optarg);
            break;

            case 's':
                eh.saturation = atoi(optarg);
            break;

            case 'h':
                eh.hue = atoi(optarg);
            break;

            case 'p':
                eh.sharpness = atoi(optarg);
            break;

            default:
            break;
        }
    }

    printf("brightness  = %d\n", eh.brightness);
    printf("contrast    = %d\n", eh.contrast);
    printf("saturation  = %d\n", eh.saturation);
    printf("hue         = %d\n", eh.hue);
    printf("sharpness   = %d\n", eh.sharpness);

    fd = open("/dev/fb0" , O_RDWR);
    if( fd < 0){
        printf("open /dev/fb0 failed, ret=%d\n", fd);
        return -1;
    }

    ret = ioctl(fd, HCFBIOSET_ENHANCE, &eh);
    if( ret != 0 ){
        printf("%s:%d: warning: HCFBIOSET_ENHANCE failed\n", __func__, __LINE__);
        close(fd);
        fd = -1;
        return -1;
    }

    close(fd);
    fd = -1;

    return 0;
}

int set_pix_fmt(int argc , char *argv[])
{
    int opt = 0;
    struct dis_hdmi_output_pix_fmt pix_fmt = {0};
    int ret = 0;

    opterr = 0;
    optind = 0;

    pix_fmt.distype = DIS_TYPE_HD;
    pix_fmt.pixfmt = HC_PIX_FMT_RGB_MODE1;

    while((opt = getopt(argc , argv , "f:")) != EOF) {
        switch(opt) {
            case 'f':
                pix_fmt.pixfmt = atoi(optarg);
            break;

            default:
                printf("%s:%d: unkown para\n", __func__, __LINE__);
            break;
        }
    }

    printf("%s:%d: pixfmt= %d\n", __func__, __LINE__, pix_fmt.pixfmt);

    ret = ioctl(g_switch.dis_fd, DIS_SET_HDMI_OUTPUT_PIXFMT, &pix_fmt);
    if(ret != 0){
        printf("DIS_SET_HDMI_OUTPUT_PIXFMT failed, ret=%d\n", ret);
        return -1;
    }

    return 0;
}

int set_zoom_test(int argc , char *argv[])
{
	struct dis_zoom dz = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	//set_zoom();
	//return 0;

	while((opt = getopt(argc , argv , "l:d:x:y:w:h:o:k:j:g:")) != EOF) {
		switch(opt) {
			case 'l':
				dz.layer = atoi(optarg);
				break;
			case 'd':
				dz.distype = atoi(optarg);
				break;
			case 'x':
				dz.src_area.x = atoi(optarg);
				break;
			case 'y':
				dz.src_area.y = atoi(optarg);
				break;
			case 'w':
				dz.src_area.w = atoi(optarg);
				break;
			case 'h':
				dz.src_area.h = atoi(optarg);
				break;
			case 'o':
				dz.dst_area.x = atoi(optarg);
				break;
			case 'k':
				dz.dst_area.y = atoi(optarg);
				break;
			case 'j':
				dz.dst_area.w = atoi(optarg);
				break;
			case 'g':
				dz.dst_area.h = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(dz.layer != DIS_LAYER_MAIN && dz.layer != DIS_LAYER_AUXP) {
		printf("error layer %d\n" , dz.layer);
		return -1;
	}

	if(dz.distype != DIS_TYPE_SD && dz.distype != DIS_TYPE_HD &&
			dz.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , dz.distype);
		return -1;
	}

	ioctl(g_switch.dis_fd, DIS_SET_ZOOM , &dz);

	return 0;
}

#endif

int hdmi_switch_main (int argc, char *argv[])
{
    printf("%s:%d: \n", __func__, __LINE__);

    return 0;
}

CONSOLE_CMD(hdmi_switch, NULL, hdmi_switch_main, CONSOLE_CMD_MODE_SELF, "hdmi switch")

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
CONSOLE_CMD(backup, "hdmi_switch", backup_picture, CONSOLE_CMD_MODE_SELF,
                        "backup picture, eg: backup_logo -d display type -l layer -b 1:backup/0: not backup")
CONSOLE_CMD(print, "hdmi_switch", print_layer, CONSOLE_CMD_MODE_SELF,
                                    "print layer, eg: print -d display type -l layer")
CONSOLE_CMD(show, "hdmi_switch", show_picture, CONSOLE_CMD_MODE_SELF,
                                    "show picture, eg: show_logo -d display type -l layer -o 1:on/0:off")
#endif

#ifdef HDMI_SWITCH_HDMI_TX
CONSOLE_CMD(set_tvsys, "hdmi_switch", set_out_tvsys, CONSOLE_CMD_MODE_SELF,
                                    "get hdmi out tvsys, eg: get_tvsys -d display type -l layer -t tv_type -p progressive")
CONSOLE_CMD(get_edid, "hdmi_switch", get_edid, CONSOLE_CMD_MODE_SELF, "get edid")
#endif

#ifdef HDMI_SWITCH_DE_EFFECT_ADJUST
CONSOLE_CMD(get_enhance, "hdmi_switch", get_enhance, CONSOLE_CMD_MODE_SELF, "get enhance")
CONSOLE_CMD(set_enhance, "hdmi_switch", set_enhance, CONSOLE_CMD_MODE_SELF,
                "set enhance, -b brightness -c contrast -s saturation -h hue -p sharpness")
CONSOLE_CMD(set_pix_fmt, "hdmi_switch", set_pix_fmt, CONSOLE_CMD_MODE_SELF,
                "set_pix_fmt -f HC_PIXELFORMAT")
CONSOLE_CMD(set_zoom, "hdmi_switch", set_zoom_test, CONSOLE_CMD_MODE_SELF,
                "tvsys -l DIS_LAYER -d DIS_TYPE source area:(-x -y -w -h ) \
                dst area:(-o x-offset of the area -k y-offset of the area -j \
                Width of the area -g Height of the area")
#endif

