#ifndef __HCCAST_UM_API_H__
#define __HCCAST_UM_API_H__

typedef void (*um_ioctl)(int, void *, void *);
typedef void (*um_api)(void);

hccast_um_param_t *hccast_um_param_get();
int hccast_ium_ioctl(int cmd, void *param1, void *param2);
int hccast_aum_ioctl(int cmd, void *param1, void *param2);

#endif