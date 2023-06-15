#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HC_RTOS
#include <um/iumirror_api.h>
#include <um/aumirror_api.h>
#else
#include <hccast/iumirror_api.h>
#include <hccast/aumirror_api.h>
#endif
#include <hccast_um.h>
#include <hccast_log.h>
#include "hccast_um_avplayer.h"
#include "hccast_um_api.h"

extern aum_av_func_t aum_av_func;
extern um_ioctl aum_api_ioctl;
extern um_api aum_api_start;
extern um_api aum_api_stop;

static int m_aum_start = 0;
static aum_setting_info_t g_aum_setting;
hccast_um_cb g_aum_evt_cb = NULL;
static unsigned int g_aum_res = 0;  //HCCAST_AUM_RES_AUTO

static void hccast_aum_get_setting(aum_setting_info_t *paum_setting, hccast_aum_param_t *param)
{
    hccast_um_param_t *um_param = hccast_um_param_get();

    if (paum_setting)
    {
        memset(paum_setting, 0, sizeof(aum_setting_info_t));
        strcpy(paum_setting->fw_url, param->fw_url);
        strcpy(paum_setting->product_id, param->product_id);

        paum_setting->tvsys = 1;
        paum_setting->resolution = g_aum_res;
        paum_setting->sw_version = param->fw_version;
        paum_setting->support_audio = 0;
        paum_setting->screen_rotate = um_param->screen_rotate_en;
        paum_setting->auto_rotate = um_param->screen_rotate_auto;
        paum_setting->full_screen = um_param->full_screen_en;
    }
}

static void hccast_aum_event_process(int event, void *param1, void *param2)
{
    aum_screen_mode_t *screen_mode = NULL;
    aum_upg_buf_obj_t *pbo;
    hccast_aum_upg_bo_t bo;
    hccast_um_param_t *um_param = hccast_um_param_get();

    if (!g_aum_evt_cb)
    {
        hccast_log(LL_WARNING, "No AUM event process function register\n");
        return ;
    }

    switch (event)
    {
        case AUM_EVT_DEVICE_ADD:
            g_aum_evt_cb(HCCAST_AUM_EVT_DEVICE_ADD, NULL, NULL);
            break;
        case AUM_EVT_DEVICE_REMOVE:
            g_aum_evt_cb(HCCAST_AUM_EVT_DEVICE_REMOVE, NULL, NULL);
            break;
        case AUM_EVT_MIRROR_START:
            g_aum_evt_cb(HCCAST_AUM_EVT_MIRROR_START, NULL, NULL);
            break;
        case AUM_EVT_MIRROR_STOP:
            g_aum_evt_cb(HCCAST_AUM_EVT_MIRROR_STOP, NULL, NULL);
            break;
        case AUM_EVT_IGNORE_NEW_DEVICE:
            g_aum_evt_cb(HCCAST_AUM_EVT_IGNORE_NEW_DEVICE, NULL, NULL);
            break;
        case AUM_EVT_UPG_DOWNLOAD_PROGRESS:
            g_aum_evt_cb(HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS, param1, param2);
            break;
        case AUM_EVT_SET_TVSYS:
            break;
        case AUM_EVT_GET_UPGRADE_DATA:
            pbo = (aum_upg_buf_obj_t *)param1;
            bo.buf = pbo->buf;
            bo.len = pbo->len;
            g_aum_evt_cb(HCCAST_AUM_EVT_GET_UPGRADE_DATA, &bo, NULL);
            break;
        case AUM_EVT_SET_SCREEN_ROTATE:
            hccast_log(LL_DEBUG, "[Screen rotate] %d\n", (unsigned int)param1);
            g_aum_setting.screen_rotate = param1;
            aum_api_ioctl(AUM_CMD_SET_SETTING, &g_aum_setting, NULL);
            g_aum_evt_cb(HCCAST_AUM_EVT_SET_SCREEN_ROTATE, param1, NULL);
            um_param->screen_rotate_en = param1;
            aum_av_func._video_rotate(param1);
            break;
        case AUM_EVT_SET_AUTO_ROTATE:
            hccast_log(LL_DEBUG, "[Auto rotate] %d\n", (unsigned int)param1);
            g_aum_setting.auto_rotate = param1;
            aum_api_ioctl(AUM_CMD_SET_SETTING, &g_aum_setting, NULL);
            g_aum_evt_cb(HCCAST_AUM_EVT_SET_AUTO_ROTATE, param1, NULL);
            um_param->screen_rotate_auto = param1;
            break;
        case AUM_EVT_SET_FULL_SCREEN:
            hccast_log(LL_DEBUG, "[Full screen] %d\n", (unsigned int)param1);
            g_aum_setting.full_screen = param1;
            aum_api_ioctl(AUM_CMD_SET_SETTING, &g_aum_setting, NULL);
            g_aum_evt_cb(HCCAST_AUM_EVT_SET_FULL_SCREEN, param1, NULL);
            um_param->full_screen_en = param1;
            aum_av_func._video_mode(param1);
            break;
        case AUM_EVT_SET_SCREEN_MODE:
            screen_mode = (aum_screen_mode_t *)param1;
            hccast_log(LL_NOTICE, "[Screen mode] %d %dX%d -> %dX%d\n", screen_mode->mode, screen_mode->screen_width,
                       screen_mode->screen_height, screen_mode->video_width, screen_mode->video_height);
            aum_av_func._screen_mode(screen_mode);
            break;
        default:
            hccast_log(LL_DEBUG, "Unknown aum event %d\n", event);
            break;
    }
}

int hccast_aum_get_flip_mode()
{
    int flip_mode = 0;
    g_aum_evt_cb(HCCAST_AUM_EVT_GET_FLIP_MODE, (void*)&flip_mode, NULL);
    return flip_mode;
}

int hccast_aum_start(hccast_aum_param_t *param, hccast_um_cb event_cb)
{
    if (m_aum_start)
        return 0;

    if (!aum_api_ioctl || !aum_api_start)
    {
        return -1;
    }

    g_aum_evt_cb = event_cb;

    memset(&g_aum_setting, 0, sizeof(g_aum_setting));
    hccast_aum_get_setting(&g_aum_setting, param);

    aum_api_ioctl(AUM_CMD_SET_SETTING, &g_aum_setting, NULL);
    aum_api_ioctl(AUM_CMD_SET_AV_FUNC, &aum_av_func, NULL);
    aum_api_ioctl(AUM_CMD_SET_EVENT_CB, hccast_aum_event_process, NULL);
    aum_api_ioctl(AUM_CMD_SET_APP_URL, param->apk_url, NULL);
    aum_api_ioctl(AUM_CMD_SET_AOA_DESCRIPTION, param->aoa_desc, NULL);
    aum_api_start();
    m_aum_start = 1;

    return 0;
}

int hccast_aum_stop()
{
    if (!m_aum_start)
        return 0;

    if (!aum_api_stop)
    {
        return -1;
    }

    aum_api_stop();
    g_aum_evt_cb = NULL;
    m_aum_start = 0;

    return 0;
}

int hccast_aum_stop_mirroring()
{
    aum_api_ioctl(AUM_CMD_STOP_MIRRORING, NULL, NULL);

    return 0;
}

int hccast_aum_ioctl(int cmd, void *param1, void *param2)
{
    hccast_aum_res_e res;

    switch (cmd)
    {
        case HCCAST_UM_CMD_SET_AUM_RESOLUTION:
            res = (hccast_aum_res_e)param1;
            if ((res >= HCCAST_AUM_RES_AUTO) && (res <= HCCAST_AUM_RES_480P60))
            {
                g_aum_res = res;
            }
            break;
        default:
            break;
    }

    return 0;
}

int hccast_aum_set_upg_buf(unsigned char *buf, unsigned int len)
{
    aum_api_ioctl(AUM_CMD_SET_UPGRADE_BUF, (void*)buf, (void*)len);
    return 0;
}

int hccast_aum_set_resolution(hccast_aum_res_e res)
{
    if ((res >= HCCAST_AUM_RES_AUTO) && (res <= HCCAST_AUM_RES_480P60))
    {
        g_aum_res = res;
    }
    
    return 0;
}

