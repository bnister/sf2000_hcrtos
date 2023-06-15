#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef HC_RTOS
#include <aircast/aircast_api.h>
#include <aircast/aircast_mdns.h>
#include <aircast/aircast_urlplay.h>
#else
#include <hccast/aircast_api.h>
#include <hccast/aircast_mdns.h>
#include <hccast/aircast_urlplay.h>
#include <dlfcn.h>
#endif
#include <hccast_log.h>
#include "hccast_air_api.h"

#define AIRCAST_LIBRARY   "/usr/lib/libaircast.so"
air_api_service_init g_air_api_service_init = NULL;
air_api_service_start g_air_api_service_start = NULL;
air_api_service_stop g_air_api_service_stop = NULL;
air_api_set_event_callback g_air_api_set_event_callback = NULL;
air_api_set_resolution g_air_api_set_resolution = NULL;
air_api_event_notify g_air_api_event_notify = NULL;
air_api_ioctl g_air_api_ioctl = NULL;
static int g_air_api_inited = 0;
static pthread_mutex_t g_air_api_mutex = PTHREAD_MUTEX_INITIALIZER;


int hccast_air_api_service_init()
{
    if(g_air_api_service_init)
    {
        return g_air_api_service_init();
    }
    else
    {
         hccast_log(LL_WARNING, "error: air_api_service_init is NULL\n");
         return -1;
    }
}

int hccast_air_api_service_start(char *name, char* ifname)
{
    if(g_air_api_service_start)
    {
        return g_air_api_service_start(name, ifname);
    }
    else
    {
         hccast_log(LL_WARNING, "error: air_api_service_start is NULL\n");
         return -1;
    }
}

int hccast_air_api_service_stop(void)
{
    if(g_air_api_service_stop)
    {
        return g_air_api_service_stop();
    }
    else
    {
         hccast_log(LL_WARNING, "error: air_api_service_stop is NULL\n");
         return -1;
    }
}

void hccast_air_api_set_event_callback(evt_cb event_cb)
{
    if(g_air_api_set_event_callback)
    {
        g_air_api_set_event_callback(event_cb);
    }
    else
    {
         hccast_log(LL_WARNING, "error: air_api_set_event_callback is NULL\n");
    }
}

int hccast_air_api_set_resolution(int width, int height, int refreshRate)
{
    if(g_air_api_set_resolution)
    {
        return g_air_api_set_resolution(width, height, refreshRate);
    }
    else
    {
         hccast_log(LL_WARNING, "error: air_api_set_resolution is NULL\n");
         return -1;
    }
}

void hccast_air_api_event_notify(int event_type, void *param)
{
    if(g_air_api_event_notify)
    {
        g_air_api_event_notify(event_type, param);
    }
    else
    {
         hccast_log(LL_WARNING, "error: air_api_event_notify is NULL\n");
    }
}

int hccast_air_api_ioctl(int req_cmd, void *param1, void *param2)
{
    if(g_air_api_ioctl)
    {
        return g_air_api_ioctl(req_cmd, param1, param2);
    }
    else
    {
         hccast_log(LL_WARNING, "error: air_api_ioctl is NULL\n");
         return -1;
    }
}


int hccast_air_api_init()
{
    void *air_dl_handle = NULL;
    
    pthread_mutex_lock(&g_air_api_mutex);
    if(g_air_api_inited)
    {
        pthread_mutex_unlock(&g_air_api_mutex);
        return 0;
    }

#ifdef HC_RTOS

    g_air_api_service_init = aircast_service_init;
    g_air_api_service_start = aircast_service_start;
    g_air_api_service_stop = aircast_service_stop;
    g_air_api_set_event_callback = aircast_set_event_callback;
    g_air_api_set_resolution = aircast_set_resolution;
    g_air_api_event_notify = aircast_event_notify;
    g_air_api_ioctl = aircast_ioctl;
    
#else

    air_dl_handle = dlopen(AIRCAST_LIBRARY, RTLD_LAZY);
    if (!air_dl_handle)
    {
        hccast_log(LL_ERROR, "%s:dlopen %s\n", __func__, dlerror());
        pthread_mutex_unlock(&g_air_api_mutex);
        return -1;

    }

    g_air_api_service_init = dlsym(air_dl_handle, "aircast_service_init");
    if (!g_air_api_service_init)
    {
        hccast_log(LL_ERROR, "dlsym aircast_service_init %s\n", dlerror());
    }

    g_air_api_service_start = dlsym(air_dl_handle, "aircast_service_start");
    if (!g_air_api_service_start)
    {
        hccast_log(LL_ERROR, "dlsym aircast_service_start %s\n", dlerror());
    }

    g_air_api_service_stop = dlsym(air_dl_handle, "aircast_service_stop");
    if (!g_air_api_service_stop)
    {
        hccast_log(LL_ERROR, "dlsym aircast_service_stop %s\n", dlerror());
    }

    g_air_api_set_event_callback = dlsym(air_dl_handle, "aircast_set_event_callback");
    if (!g_air_api_set_event_callback)
    {
        hccast_log(LL_ERROR, "dlsym aircast_set_event_callback %s\n", dlerror());
    }

    g_air_api_set_resolution = dlsym(air_dl_handle, "aircast_set_resolution");
    if (!g_air_api_set_resolution)
    {
        hccast_log(LL_ERROR, "dlsym aircast_set_resolution %s\n", dlerror());
    }

    g_air_api_event_notify = dlsym(air_dl_handle, "aircast_event_notify");
    if (!g_air_api_event_notify)
    {
        hccast_log(LL_ERROR, "dlsym aircast_event_notify %s\n", dlerror());
    }

    g_air_api_ioctl = dlsym(air_dl_handle, "aircast_ioctl");
    if (!g_air_api_ioctl)
    {
        hccast_log(LL_ERROR, "dlsym aircast_ioctl %s\n", dlerror());
    }

#endif
    g_air_api_inited = 1;    
    pthread_mutex_unlock(&g_air_api_mutex);
    return 0;
}

