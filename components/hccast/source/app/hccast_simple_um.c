#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>

#include <hccast_um.h>

static char ium_uuid[40] = {0};
static hccast_aum_param_t aum_param;

hccast_um_cb ium_event_process_cb(int event, void *param1, void *param2)
{
    //printf("ium event: %d\n", event);
}

hccast_um_cb aum_event_process_cb(int event, void *param1, void *param2)
{
    printf("aum event: %d\n", event);
}

int main(int argc, char *argv[])
{
    hccast_um_param_t um_param;
    
    printf("HCCast USB mirrorring demo\n");

    um_param.screen_rotate_en = 0;
    um_param.screen_rotate_auto = 1;
    um_param.full_screen_en = 1;
    hccast_um_param_set(&um_param);
    
    if (hccast_um_init() < 0)
    {
        return -1;
    }

    hccast_ium_init(ium_event_process_cb);
    hccast_ium_start(ium_uuid, ium_event_process_cb);

    memset(&aum_param, 0, sizeof(aum_param));
    strcat(aum_param.product_id, "HCT-AT01");
    strcat(aum_param.fw_url, "http://119.3.89.190:8080/apk/elfcast-HCT-AT01.json");
    strcat(aum_param.apk_url, "http://119.3.89.190:8080/apk/elfcast.apk");
    strcat(aum_param.aoa_desc, "ElfCast-Screen_Mirror");
    aum_param.fw_version = 0;
    hccast_aum_start(&aum_param, aum_event_process_cb);

    while (1)
    {
        sleep(10);
    }

    return 0;
}
