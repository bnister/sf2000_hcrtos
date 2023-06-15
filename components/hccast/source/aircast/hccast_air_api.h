#ifndef __HCCAST_AIR_API_H__
#define __HCCAST_AIR_API_H__

typedef int (*air_api_service_init)(void);
typedef int (*air_api_service_start)(char *name, char* ifname);
typedef int (*air_api_service_stop)(void);
typedef void (*air_api_set_event_callback)(evt_cb event_cb);
typedef int (*air_api_set_resolution)(int width, int height, int refreshRate);
typedef void (*air_api_event_notify)(int event_type, void *param);
typedef int (*air_api_ioctl)(int req_cmd, void *param1, void *param2);

int hccast_air_api_service_init();
int hccast_air_api_service_start(char *name,char* ifname);
int hccast_air_api_service_stop(void);
void hccast_air_api_set_event_callback(evt_cb event_cb);
int hccast_air_api_set_resolution(int width,int height,int refreshRate);
void hccast_air_api_event_notify(int event_type,void *param);
int hccast_air_api_ioctl(int req_cmd, void *param1,void *param2);
int hccast_air_api_init();

#endif