#ifndef _P2P_CTRL_H_
#define _P2P_CTRL_H_

#include <stdbool.h>

#define MIRA_RTSP_PORT (7236)

typedef int (*p2p_ctrl_state_callback)(int state);
typedef int (*p2p_ctrl_set_res_callback)(int state);

typedef struct _p2p_in_param_st
{
    char wifi_ifname[64];                            // wifi interface name
    char p2p_ifname[64];                             // p2p interface name
    char device_name[64];                            // p2p device name
    int  listen_channel;                             // p2p listen channel
    int  oper_channel;                               // p2p oper channel
    p2p_ctrl_state_callback state_update_func;       // p2p stat update func
    p2p_ctrl_set_res_callback res_set_func;          // p2p set resolution
} p2p_param_st;

typedef struct _p2p_peer_result_st_
{
    char device_name[64];
    char wfd_subelems[64];
    int config_methods;
    int dev_capab;
    int group_capab;
    int listen_freq;
} p2p_peer_result_st;


int p2p_ctrl_init(p2p_param_st *params);
int p2p_ctrl_uninit(void);

void p2p_ctrl_device_init(void);
int p2p_ctrl_device_is_go(void);
void p2p_ctrl_device_enable(void);
void p2p_ctrl_device_disable(void);

bool p2p_ctrl_get_enable(void);
bool p2p_ctrl_set_enable(bool enable);
unsigned int p2p_ctrl_get_device_ip(void);
int p2p_ctrl_get_device_rtsp_port(void);

int p2p_ctrl_get_connect_stat(void);
int p2p_ctrl_set_connect_stat(bool stat);

#ifdef HC_RTOS
/**
 * It sends a command to the driver to abort the p2p scan
 * note: unofficial cmd
 */
void p2p_ctrl_device_abort_scan(void);

int p2p_ctrl_iwlist_scan_cmd(char *inf);
#endif

#endif