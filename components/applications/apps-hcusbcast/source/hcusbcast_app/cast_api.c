/**
 * @file cast_api.c
 * @author your name (you@domain.com)
 * @brief hichip cast api
 * @version 0.1
 * @date 2022-01-21
 *
 * @copyright Copyright (c) 2022
 *
 */


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <time.h>
#include <pthread.h>
#include <hccast/hccast_scene.h>
#include <hcuapi/dis.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "com_api.h"
#include "osd_com.h"
#include "cast_api.h"
#include "data_mgr.h"

#define CAST_SERVICE_NAME               "HCcast"
#define CAST_AIRCAST_SERVICE_NAME       "HCcast"
#define CAST_DLNA_FIRENDLY_NAME         "HCcast"
#define CAST_MIRACAST_NAME              "HCcast"

#define UUID_HEADER "HCcast"

static bool m_is_demo = false;

#ifdef USBMIRROR_SUPPORT
static char m_ium_uuid[40] = {0};
static char g_um_upgrade_buf[0x201000];
static hccast_ium_upg_bo_t ium_upg_buf_info;
static hccast_aum_upg_bo_t aum_upg_buf_info;
static int um_start_upgrade = 0;//0--idel, 1--download, 2--burning.

static void ium_event_process_cb(int event, void *param1, void *param2);


static void *ium_burning_process(void *arg)
{
    app_data_t* app_data = data_mgr_app_get();
    hccast_ium_stop();
    sys_upg_flash_burn(ium_upg_buf_info.buf, ium_upg_buf_info.len);
    hccast_ium_start(app_data->ium_uuid, ium_event_process_cb);
    um_start_upgrade = 0;
}

static void *aum_burning_process(void *arg)
{
    sys_upg_flash_burn(aum_upg_buf_info.buf, aum_upg_buf_info.len);
    um_start_upgrade = 0;
}

void cast_api_set_dis_zoom(av_area_t *src_rect,
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

void cast_um_set_dis_zoom(hccast_um_zoom_info_t *um_zoom_info)
{
    av_area_t src_rect;
    av_area_t dst_rect;

    memcpy(&src_rect, &um_zoom_info->src_rect, sizeof(av_area_t));
    memcpy(&dst_rect, &um_zoom_info->dst_rect, sizeof(av_area_t));

    cast_api_set_dis_zoom(&src_rect, &dst_rect, DIS_SCALE_ACTIVE_IMMEDIATELY);
}

static void ium_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};
    int got_data_len;
    int total_data_len ;
    pthread_t pid;
    app_data_t* app_data = data_mgr_app_get();
    
    if ((event != HCCAST_IUM_EVT_GET_FLIP_MODE) && (event != HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS))
        printf("ium event: %d\n", event);

    switch (event)
    {
        case HCCAST_IUM_EVT_DEVICE_ADD:
            um_start_upgrade = 0;
            ctl_msg.msg_type = MSG_TYPE_IUM_DEV_ADD;
            break;
        case HCCAST_IUM_EVT_DEVICE_REMOVE:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_DEVICE_REMOVE;
            break;
        case HCCAST_IUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_IUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_START;
            api_osd_off_time(1000);
            break;
        case HCCAST_IUM_EVT_MIRROR_STOP:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_STOP;
            break;
        case HCCAST_IUM_EVT_SAVE_PAIR_DATA: //param1: buf; param2: length
        {    
            char *pair_buf = (char *)param1;
            int len = (int)param2;    
            if (len > 20 * 1024)
            {
                printf("%s %d: error \n", __func__, __LINE__);
                if (pair_buf)
                {
                    free(pair_buf);
                }
                
               return ;
            }

            data_mgr_ium_pdata_len_set(len);
            data_mgr_ium_pair_data_set(pair_buf);
            free(pair_buf);
            break;
        }    
        case HCCAST_IUM_EVT_GET_PAIR_DATA: //param1: buf; param2: length
        {
            if (app_data->ium_pdata_len > 0)                
            {
                char **pair_buf = (char **)param1;
                int *len = (int*)param2;
                if (pair_buf)
                {
                    char *buf = malloc(app_data->ium_pdata_len);
                    memcpy(buf, app_data->ium_pair_data, app_data->ium_pdata_len);
                    *pair_buf = buf;
                    if (len) *len = app_data->ium_pdata_len;
                }
            }

            break;
        }
        case HCCAST_IUM_EVT_NEED_USR_TRUST:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_NEED_TRUST;
            break;
        case HCCAST_IUM_EVT_USR_TRUST_DEVICE:
            break;
        case HCCAST_IUM_EVT_CREATE_CONN_FAILED:
            break;
        case HCCAST_IUM_EVT_CANNOT_GET_AV_DATA:
            break;
        case HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS: //param1: data len; param2: file len

            if(um_start_upgrade == 0)
            {
                ctl_msg.msg_type = MSG_TYPE_IUM_START_UPGRADE;
                um_start_upgrade = 1;
            }
            else if(um_start_upgrade == 1)
            {
                got_data_len = (int)param1;
                total_data_len = (int)param2;
                ctl_msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
                ctl_msg.msg_code = got_data_len*100/total_data_len;

                printf("HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS progress: %d\n",ctl_msg.msg_code);
            }  
            
            break;
        case HCCAST_IUM_EVT_GET_UPGRADE_DATA: //param1: hccast_ium_upg_bo_t
        
            if(um_start_upgrade == 1)
            {
                memcpy(&ium_upg_buf_info, param1, sizeof(ium_upg_buf_info));
                um_start_upgrade = 2;
                pthread_create(&pid, NULL, ium_burning_process, NULL);
            } 
            
            break;
        case HCCAST_IUM_EVT_SAVE_UUID:
            memcpy(app_data->ium_uuid, param1, 37);
            data_mgr_ium_uuid_set(app_data->ium_uuid);
            break;
        case HCCAST_IUM_EVT_CERT_INVALID:
            printf("[%s],line:%d. HCCAST_IUM_EVT_CERT_INVALID\n",__func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_AIR_INVALID_CERT;
            m_is_demo = true;
            break;
        case HCCAST_IUM_EVT_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param1 = flip_mode;
            break;
        }
        case HCCAST_IUM_EVT_SET_ROTATE:
        {
            app_data->mirror_rotation = (int)param1;
            break;
        }
        case HCCAST_IUM_EVT_NO_DATA:
        {
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_NO_DATA;
            break;
        }
        case HCCAST_IUM_EVT_SET_DIS_ZOOM_INFO:
        {
            cast_um_set_dis_zoom(param1);
            break;
        }
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }
}

static void aum_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};
    int got_data_len;
    int total_data_len ;
    pthread_t pid;
    app_data_t* app_data = data_mgr_app_get();   
    
    if ((HCCAST_AUM_EVT_GET_FLIP_MODE != event) && (HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS != event))
        printf("aum event: %d\n", event);
    
    switch (event)
    {
        case HCCAST_AUM_EVT_DEVICE_ADD:
            printf("%s(), line:%d. HCCAST_AUM_EVT_DEVICE_ADD\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_AUM_DEV_ADD;
            um_start_upgrade = 0;
            
        #ifdef WIFI_SUPPORT
            hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
        #endif
            break;
        case HCCAST_AUM_EVT_DEVICE_REMOVE:
            break;
        case HCCAST_AUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_START;
            api_osd_off_time(1000);
            break;
        case HCCAST_AUM_EVT_MIRROR_STOP:
        #ifdef WIFI_SUPPORT
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            if(hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR)
            {
                hccast_scene_reset(HCCAST_SCENE_AUMIRROR,HCCAST_SCENE_NONE);
            }
        #endif
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_STOP;
            break;
        case HCCAST_AUM_EVT_IGNORE_NEW_DEVICE:
            break;
        case HCCAST_AUM_EVT_SERVER_MSG:
            break;
        case HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS:
        
            if(um_start_upgrade == 0)
            {
                ctl_msg.msg_type = MSG_TYPE_IUM_START_UPGRADE;
                um_start_upgrade = 1;
            }
            else if(um_start_upgrade == 1)
            {
                got_data_len = (int)param1;
                total_data_len = (int)param2;
                ctl_msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
                ctl_msg.msg_code = got_data_len*100/total_data_len;

                printf("HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS progress: %d\n",ctl_msg.msg_code);
            }  
        
            break;
        case HCCAST_AUM_EVT_GET_UPGRADE_DATA:
        
            if(um_start_upgrade == 1)
            {
                memcpy(&aum_upg_buf_info, param1, sizeof(aum_upg_buf_info));
                um_start_upgrade = 2;
                pthread_create(&pid, NULL, aum_burning_process, NULL);
            } 
            
            break;
        case HCCAST_AUM_EVT_SET_SCREEN_ROTATE:
            app_data->mirror_rotation = (int)param1;
            break;
        case HCCAST_AUM_EVT_SET_AUTO_ROTATE:
            break;
        case HCCAST_AUM_EVT_SET_FULL_SCREEN:
            break;
        case HCCAST_AUM_EVT_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param1 = flip_mode;
            break;
        }
        case HCCAST_AUM_EVT_SET_DIS_ZOOM_INFO:
        {
            cast_um_set_dis_zoom(param1);
            break;
        }
        default:
            break;
    }
    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }
}

int cast_usb_mirror_init(void)
{
    hccast_um_param_t um_param = {0};

    hccast_um_init();
    if (hccast_um_init() < 0)
    {
        printf("%s(), line:%d. hccast_um_init() fail!\n", __func__, __LINE__);
        return API_FAILURE;
    }

    if (data_mgr_cast_rotation_get())
        um_param.screen_rotate_en = 1;
    else
        um_param.screen_rotate_en = 0;

    um_param.screen_rotate_auto = 1;
    um_param.full_screen_en = 1;
    hccast_um_param_set(&um_param);

    hccast_ium_init(ium_event_process_cb);

    return API_SUCCESS;
}

int cast_usb_mirror_deinit(void)
{
    if (hccast_um_deinit() < 0)
        return API_FAILURE;
    else
        return API_SUCCESS;
}

int cast_usb_mirror_start(void)
{
    int ret;
    hccast_aum_param_t aum_param = {0};
    sys_data_t* sys_data = data_mgr_sys_get();
    app_data_t* app_data = data_mgr_app_get();

    ret = hccast_ium_start(app_data->ium_uuid, ium_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

    //um need a buf to store upgrade file data.
    hccast_ium_set_upg_buf(g_um_upgrade_buf, sizeof(g_um_upgrade_buf));

    strcat(aum_param.product_id, (char*)sys_data->product_id);//HCT-AT01
    sprintf(aum_param.fw_url, "http://119.3.89.190:8080/apk/elfcast-%s.json",(char*)sys_data->product_id);
    strcat(aum_param.apk_url, "http://119.3.89.190:8080/apk/elfcast.apk");
    strcat(aum_param.aoa_desc, "ElfCast-Screen_Mirror");
    aum_param.fw_version = (unsigned int)sys_data->firmware_version;

    hccast_aum_set_upg_buf(g_um_upgrade_buf, sizeof(g_um_upgrade_buf));
    ret = hccast_aum_start(&aum_param, aum_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

    return API_SUCCESS;
}

int cast_usb_mirror_stop(void)
{
    hccast_aum_stop();
    hccast_ium_stop();
    return API_SUCCESS;
}
#endif

int cast_init(void)
{
    hccast_scene_init();
    api_set_aspect_mode(1, 3,DIS_SCALE_ACTIVE_IMMEDIATELY);//16:9 as default.

    return API_SUCCESS;
}

int cast_deinit(void)
{

#ifdef USBMIRROR_SUPPORT
    cast_usb_mirror_stop();
    cast_usb_mirror_deinit();
#endif

    return API_SUCCESS;
}



bool cast_is_demo(void)
{
    return m_is_demo;
}

