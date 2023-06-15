#ifndef _CVBS_RX_H_
#define _CVBS_RX_H_

#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/fb.h>

#ifdef __HCRTOS__
#include <kernel/lib/console.h>
#include <nuttx/wqueue.h>
#else
//#include <console.h>
#endif

#define CVBS_SWITCH_HDMI_STATUS_PLUGIN 1
#define CVBS_SWITCH_HDMI_STATUS_PLUGOUT 0

struct cvbs_info{
    int fd;

#ifdef __HCRTOS__
    struct work_notifier_s notify_plugin;
    struct work_notifier_s notify_plugout;
    struct work_notifier_s notify_edid;
	int in_ntkey;
	int out_ntkey;
#endif    

    char plug_status;
    char enable;
};


#endif