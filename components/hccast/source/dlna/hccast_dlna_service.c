
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <hccast_dlna.h>
#ifdef HC_RTOS
#include <dlna/dlna_api.h>
#else
#include <hccast/dlna_api.h>
#endif
#include <hccast_log.h>
#include <pthread.h>

extern struct output_module output_ffplayer;
hccast_dlna_event_callback dlna_callback = NULL;

static pthread_mutex_t g_dlna_svr_mutex = PTHREAD_MUTEX_INITIALIZER;

static int g_dlna_started = 0;

int hccast_dlna_service_is_start()
{
    return g_dlna_started;
}

int hccast_dlna_service_start()
{
    char service_name[DLNA_SERVICE_NAME_LEN] = "hccast_dlna";

    pthread_mutex_lock(&g_dlna_svr_mutex);
    if (g_dlna_started)
    {
        hccast_log(LL_WARNING,"[%s]: dlna service has been start\n", __func__);
        pthread_mutex_unlock(&g_dlna_svr_mutex);
        return 0;
    }

#if 1
    struct dlna_svr_param params =
    {
        .ifname = DLNA_BIND_IFNAME,
        .svrname = service_name,
        .svrport = DLNA_UPNP_PORT,
        .output = &output_ffplayer
    };
#endif

    if (dlna_callback)
    {
        dlna_callback (HCCAST_DLNA_GET_DEVICE_NAME, &service_name, NULL);
    }

    dlna_service_start_ex(&params);
    //dlna_service_start(&service_name, &output_ffplayer);
    g_dlna_started = 1;
    pthread_mutex_unlock(&g_dlna_svr_mutex);

    hccast_log(LL_NOTICE,"dlna service start\n");
    return 0;
}

int hccast_dlna_service_stop()
{
    pthread_mutex_lock(&g_dlna_svr_mutex);
    if (g_dlna_started == 0)
    {
        hccast_log(LL_WARNING,"[%s]: dlna service has been stop\n", __func__);
        pthread_mutex_unlock(&g_dlna_svr_mutex);
        return 0;
    }

    dlna_sevice_stop();
    g_dlna_started = 0;
    pthread_mutex_unlock(&g_dlna_svr_mutex);

    hccast_log(LL_NOTICE, "dlna service stop\n");
    return 0;
}

int hccast_dlna_service_init(hccast_dlna_event_callback func)
{
    hccast_log(LL_NOTICE, "%s\n", dlna_get_version());

    dlna_callback = func;
    return 0;
}

int hccast_dlna_service_uninit()
{
	dlna_callback = NULL;
	return 0;
}
