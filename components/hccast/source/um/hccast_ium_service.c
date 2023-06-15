#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

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

extern ium_av_func_t ium_av_func;
extern um_ioctl ium_api_ioctl;
extern um_api ium_api_start;
extern um_api ium_api_stop;
static int m_ium_start = 0;

hccast_um_cb g_ium_evt_cb = NULL;

static void hccast_ium_event_process(int event, void *param1, void *param2)
{
    ium_upg_buf_obj_t *pbo;
    hccast_ium_upg_bo_t bo;
    hccast_um_param_t *um_param = hccast_um_param_get();
    
    if (!g_ium_evt_cb)
    {
        hccast_log(LL_ERROR, "No event process function register\n");
        return ;
    }

    switch (event)
    {
        case IUM_EVT_DEVICE_ADD:
            g_ium_evt_cb(HCCAST_IUM_EVT_DEVICE_ADD, NULL, NULL);
            break;
        case IUM_EVT_DEVICE_REMOVE:
            g_ium_evt_cb(HCCAST_IUM_EVT_DEVICE_REMOVE, NULL, NULL);
            break;
        case IUM_EVT_MIRROR_START:
            g_ium_evt_cb(HCCAST_IUM_EVT_MIRROR_START, NULL, NULL);
            break;
        case IUM_EVT_MIRROR_STOP:
            g_ium_evt_cb(HCCAST_IUM_EVT_MIRROR_STOP, NULL, NULL);
            break;
        case IUM_EVT_SAVE_PAIR_DATA:
            g_ium_evt_cb(HCCAST_IUM_EVT_SAVE_PAIR_DATA, param1, param2);
            break;
        case IUM_EVT_GET_PAIR_DATA:
            g_ium_evt_cb(HCCAST_IUM_EVT_GET_PAIR_DATA, param1, param2);
            break;
        case IUM_EVT_NEED_USR_TRUST:
            g_ium_evt_cb(HCCAST_IUM_EVT_NEED_USR_TRUST, NULL, NULL);
            break;
        case IUM_EVT_USR_TRUST_DEVICE:
            g_ium_evt_cb(HCCAST_IUM_EVT_USR_TRUST_DEVICE, NULL, NULL);
            break;
        case IUM_EVT_CREATE_CONN_FAILED:
            g_ium_evt_cb(HCCAST_IUM_EVT_CREATE_CONN_FAILED, NULL, NULL);
            break;
        case IUM_EVT_CANNOT_GET_AV_DATA:
            break;
        case IUM_EVT_UPG_DOWNLOAD_PROGRESS:
            g_ium_evt_cb(HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS, param1, param2);
            break;
        case IUM_EVT_GET_UPGRADE_DATA:
            pbo = (ium_upg_buf_obj_t *)param1;
            bo.buf = pbo->buf;
            bo.len = pbo->len;
            bo.crc = pbo->crc;
            bo.crc_chk_ok = pbo->crc_chk_ok;
            g_ium_evt_cb(HCCAST_IUM_EVT_GET_UPGRADE_DATA, &bo, NULL);
            break;
        case IUM_EVT_SAVE_UUID:
            g_ium_evt_cb(HCCAST_IUM_EVT_SAVE_UUID, param1, NULL);
            break;
        case IUM_EVT_CERT_INVALID:
            g_ium_evt_cb(HCCAST_IUM_EVT_CERT_INVALID, NULL, NULL);
            break;
        case IUM_EVT_SET_ROTATE:
            g_ium_evt_cb(HCCAST_IUM_EVT_SET_ROTATE, param1, NULL);
            um_param->screen_rotate_en = param1;
            break;
        case IUM_EVT_FAKE_LIB:
            g_ium_evt_cb(HCCAST_IUM_EVT_FAKE_LIB, NULL, NULL);
            break;
        case IUM_EVT_NO_DATA:
            g_ium_evt_cb(HCCAST_IUM_EVT_NO_DATA, NULL, NULL);
            break;    
        default:
            hccast_log(LL_WARNING, "Unknown ium event %d\n", event);
            break;
    }
}

int hccast_ium_get_flip_mode()
{
    int flip_mode = 0;
    g_ium_evt_cb(HCCAST_IUM_EVT_GET_FLIP_MODE, (void*)&flip_mode, NULL);
    return flip_mode;
}

int hccast_ium_start(char *uuid, hccast_um_cb event_cb)
{
    if (m_ium_start)
        return 0;

    if (!ium_api_ioctl || !ium_api_start)
    {
        return -1;
    }

    g_ium_evt_cb = event_cb;

    ium_api_ioctl(IUM_CMD_SET_UUID, uuid, NULL);
    ium_api_ioctl(IUM_CMD_SET_EVENT_CB, hccast_ium_event_process, NULL);
    ium_api_ioctl(IUM_CMD_SET_AV_FUNC, &ium_av_func, NULL);
    ium_api_start();
    m_ium_start = 1;
    return 0;
}

int hccast_ium_stop()
{
    if (!m_ium_start)
        return 0;

    if (!ium_api_stop)
    {
        return -1;
    }

    ium_api_stop();

    g_ium_evt_cb = NULL;
    m_ium_start = 0;

    return 0;
}

int hccast_ium_init(hccast_um_cb event_cb)
{
    if (!ium_api_ioctl || !ium_api_start)
    {
        return -1;
    }

    g_ium_evt_cb = event_cb;

    ium_api_ioctl(IUM_CMD_SET_EVENT_CB, hccast_ium_event_process, NULL);
    ium_api_ioctl(IUM_CMD_INIT, NULL, NULL);
}

int hccast_ium_stop_mirroring()
{
    ium_api_ioctl(IUM_CMD_STOP_MIRRORING, NULL, NULL);

    return 0;
}

int hccast_ium_ioctl(int cmd, void *param1, void *param2)
{
    if (cmd == HCCAST_UM_CMD_SET_IUM_RESOLUTION)
    {
        if ((param1 == NULL) || (param2 == NULL))
        {
            return -1;
        }

        ium_api_ioctl(IUM_CMD_SET_RESOLUTION, param1, param2);
    }

    return 0;
}

int hccast_ium_set_upg_buf(unsigned char *buf, unsigned int len)
{
    ium_api_ioctl(IUM_CMD_SET_UPGRADE_BUF, (void*)buf, (void*)len);
    return 0;
}

int hccast_ium_set_resolution(int width, int height)
{
    ium_api_ioctl(IUM_CMD_SET_RESOLUTION, (void*)width, (void*)height);
    return 0;
}


