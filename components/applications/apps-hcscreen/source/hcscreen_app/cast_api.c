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

#include <hccast/hccast_scene.h>
#include <hcuapi/dis.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "com_api.h"
#include "osd_com.h"
#include "cast_api.h"

#define CAST_SERVICE_NAME               "HCcast"
#define CAST_AIRCAST_SERVICE_NAME       "HCcast"
#define CAST_DLNA_FIRENDLY_NAME         "HCcast"
#define CAST_MIRACAST_NAME              "HCcast"

#define UUID_HEADER "HCcast"

static bool m_is_demo = false;

#ifndef DLNA_SUPPORT
//implement fake functions
int hccast_dlna_service_uninit(void)
{
    return 0;
}

int hccast_dlna_service_start(void)
{
    return 0;
}

int hccast_dlna_service_stop(void)
{
    return 0;
}

#endif


#ifndef AIRCAST_SUPPORT
//implement fake functions
int hccast_air_ap_mirror_stat(void)
{
    return 0;
}
int hccast_air_ap_audio_stat(void)
{
    return 0;
}
int hccast_air_service_start(void)
{
    return 0;
}
int hccast_air_service_stop(void)
{
    return 0;
}
void hccast_air_mdnssd_start(void)
{}
void hccast_air_mdnssd_stop(void)
{}
void hccast_air_mediaplayer_2_aircast_event(int type, void *param)
{}
int hccast_air_ap_get_mirror_frame_num(void)
{
    return 0;
}
int hccast_air_service_is_start(void)
{
    return 0;
}

#endif

#ifndef MIRACAST_SUPPORT
//implement fake functions


int hccast_mira_service_start(void)
{
    return 0;
}

int hccast_mira_service_stop(void)
{
    return 0;
}
int hccast_mira_player_init(void)
{
    return 0;
}

int hccast_mira_get_stat(void)
{
    return 0;
}

int hccast_mira_service_uninit(void)
{
    return 0;
}

#endif


//#ifndef USBMIRROR_SUPPORT
#if 0
int hccast_um_init(void)
{
    return 0;
}

int hccast_um_deinit(void)
{
    return 0;
}

int hccast_um_param_set(hccast_um_param_t *param)
{
    (void)param;
    return 0;
}

int hccast_ium_start(char *uuid, hccast_um_cb event_cb)
{
    (void)uuid;
    (void)event_cb;
    return 0;
}

int hccast_ium_stop(void)
{
    return 0;
}

int hccast_aum_start(hccast_aum_param_t *param, hccast_um_cb event_cb)
{
    (void)param;
    (void)event_cb;
    return 0;
}

int hccast_aum_stop(void)
{
    return 0;
}

#endif

int cast_get_service_name(cast_type_t cast_type, char *service_name, int length)
{
    unsigned char mac_addr[6] = {0};
    char service_prefix[32] = CAST_SERVICE_NAME;

    snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_SERVICE_NAME);
    if (0 != api_get_mac_addr((char*)mac_addr))
        memset(mac_addr, 0xff, sizeof(mac_addr));

    if (CAST_TYPE_AIRCAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_AIRCAST_SERVICE_NAME);
    else if (CAST_TYPE_DLNA == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_DLNA_FIRENDLY_NAME);
    else if (CAST_TYPE_MIRACAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_MIRACAST_NAME);

    snprintf(service_name, length, "%s-%02x%02x%02x",
             service_prefix, mac_addr[3]&0xFF, mac_addr[4]&0xFF, mac_addr[5]&0xFF);

    return 0;
}

bool cast_is_demo(void)
{
    return m_is_demo;
}

