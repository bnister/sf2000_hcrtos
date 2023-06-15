#ifndef __HCCAST_WIFI_SERVICE_H__
#define __HCCAST_WIFI_SERVICE_H__
#include <stddef.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "hccast_dhcpd.h"

#define APLIST_NUM 64
#define WIFI_MAX_SSID_LEN   64
#define WIFI_MAX_PWD_LEN    32

#define HCCAST_HOSTAP_INF   "wlan0"
#define HCCAST_WIFI_INF     HCCAST_HOSTAP_INF
#define HCCAST_P2P_INF      "p2p0"

extern char HCCAST_HOSTAP_IP[32];
extern char HCCAST_HOSTAP_MASK[32];
extern char HCCAST_HOSTAP_GW[32];
extern char HCCAST_P2P_IP[32];
extern char HCCAST_P2P_MASK[32];
extern char HCCAST_P2P_GW[32];

#define HCCAST_P2P_LISTEN_CH_DEFAULT (6) // value: [1,6,11], Used when hostapd channel isn't [1,6,11].
#define HCCAST_P2P_OPER_CH_DEFAULT   (136) // value: [1,6,11], Used when hostapd channel isn't [1,6,11].

typedef enum _hccast_wifi_mode_e_
{
    HCCAST_WIFI_MODE_NONE = 0,
    HCCAST_WIFI_MODE_STA,
    HCCAST_WIFI_MODE_HOSTAP,
    HCCAST_WIFI_MODE_KEEP = 0x10,
    HCCAST_WIFI_MODE_STA_KEEP,
    HCCAST_WIFI_MODE_HOSTAP_KEEP,
} hccast_wifi_mode_e;

typedef enum _hccast_wifi_FREQ_mode_e_
{
    HCCAST_WIFI_FREQ_MODE_NONE = 0,
    HCCAST_WIFI_FREQ_MODE_24G,
    HCCAST_WIFI_FREQ_MODE_5G,
    HCCAST_WIFI_FREQ_MODE_60G,
} hccast_wifi_freq_mode_e;

typedef enum _hccast_wifi_stat_e_
{
    HCCAST_WIFI_STAT_NONE,
    HCCAST_WIFI_STAT_IDLE               = 0x1,

    HCCAST_WIFI_STAT_SCANNING           = 0x10,
    HCCAST_WIFI_STAT_CONNECTING         = 0x20,
    HCCAST_WIFI_STAT_DISCONNECTING      = 0x40,
    HCCAST_WIFI_STAT_MAX,
} hccast_wifi_stat_e;

enum
{
    HCCAST_WIFI_ERR_NO_ERROR                = -0x0,
    HCCAST_WIFI_ERR_UNKNOW_ERROR            = -0x1,
    HCCAST_WIFI_ERR_INITTED                 = -0x2,
    HCCAST_WIFI_ERR_MEM                     = -0x3,
    HCCAST_WIFI_ERR_SOCKET_FAILED           = -0x4,
    HCCAST_WIFI_ERR_IOCTL_FAILED            = -0x5,

    HCCAST_WIFI_ERR_CMD_WPAS_NO_RUN         = -0x100,
    HCCAST_WIFI_ERR_CMD_WPAS_OPEN_FAILED    = -0x101,
    HCCAST_WIFI_ERR_CMD_WPAS_ATTACH_FAILED  = -0x102,
    HCCAST_WIFI_ERR_CMD_PARAMS_ERROR        = -0x110,
    HCCAST_WIFI_ERR_CMD_RUN_FAILED          = -0x111,
    HCCAST_WIFI_ERR_CMD_RUN_UNDEFINE        = -0x112,

    HCCAST_WIFI_ERR_CONN_SSID_NOT_FOUND     = -0x200,       // unused
    HCCAST_WIFI_ERR_CONN_PWD_ERROR          = -0x201,       // unused
};

/*
 * RSSI=-77
 * LINKSPEED=48
 * NOISE=9999
 * FREQUENCY=2437
 */
typedef struct _hccast_wifi_signal_poll_result_t_
{
   int rssi;
   int linkspeed;
   int noise;
   int frequency;
} hccast_wifi_signal_poll_result_t;

/*
 * bssid=64:3e:8c:37:9e:40
 * freq=2437
 * ssid=HD-WLAN
 * id=0
 * mode=station
 * pairwise_cipher=TKIP
 * group_cipher=TKIP
 * key_mgmt=WPA-PSK
 * wpa_state=COMPLETED/DISCONNECTED/SCANNING
 * ip_address=172.16.2.196
 * address=08:e9:f6:10:39:5e
 * uuid=0889e912-d3e3-52c6-8ad3-4d8a24294f7b
 */
typedef struct _hccast_wifi_status_result_t_
{
    char bssid[18];
    int freq;
    char ssid[WIFI_MAX_SSID_LEN];
    int id;
    char mode[16];
    char pairwise_cipher[16];
    char group_cipher[16];
    char key_mgmt[16];
    char wpa_state[32];
    char ip_address[16];
    char address[18];
    char uuid[64];
    char p2p_device_address[18];
} hccast_wifi_status_result_t;

typedef enum _hccast_wifi_encrypt_mode_e_
{
    HCCAST_WIFI_ENCRYPT_MODE_NONE = 0,
    HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP,
    HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP,
    HCCAST_WIFI_ENCRYPT_MODE_WPAPSK_TKIP,
    HCCAST_WIFI_ENCRYPT_MODE_WPAPSK_AES,
    HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_TKIP,
    HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_AES,
    HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE,
} hccast_wifi_encrypt_mode_e;

typedef struct _hccast_wifi_ap_info_t_
{
    char    ssid[WIFI_MAX_SSID_LEN];               // AP's SSID
    hccast_wifi_encrypt_mode_e   encryptMode;       // Encrypt mode
    char    keyIdx;                                 // for WEP key index
    char    pwd[WIFI_MAX_PWD_LEN];                  // Password
    char    numCharInPwd;                           // Number of characters in pwd
    int     quality;                                // rssi strength
    int     special_ap;                             // 1: hidden ap, 0: normal
    int     freq;
    char    bssid[18];                              // AP's BSSID
} hccast_wifi_ap_info_t;

typedef struct _hccast_wifi_scan_result_t_
{
    hccast_wifi_ap_info_t apinfo[APLIST_NUM];
    int ap_num;
} hccast_wifi_scan_result_t;

typedef struct _hccast_wifi_list_net_info_t_
{
    char    net_id[64];                     // network id
    char    ssid[WIFI_MAX_SSID_LEN];        // AP's SSID
    char    bssid[WIFI_MAX_SSID_LEN];       // AP's BSSID
    char    flags[64];                      // network status
} hccast_wifi_list_net_info_t;

typedef struct _hccast_wifi_list_net_result_t_
{
    hccast_wifi_list_net_info_t netinfo[APLIST_NUM];
    int net_num;
} hccast_wifi_list_net_result_t;

typedef struct _hccast_wifi_hostap_status_result_t_
{
    char    status[64];
    int     channel;
    char    ssid[WIFI_MAX_SSID_LEN];
    int     num_sta;
} hccast_wifi_hostap_status_result_t;

typedef enum
{
    HCCAST_WIFI_NONE = 0,
    HCCAST_WIFI_SCAN,
    HCCAST_WIFI_SCAN_RESULT,
    HCCAST_WIFI_CONNECT,
    HCCAST_WIFI_CONNECT_SSID,
    HCCAST_WIFI_CONNECT_RESULT,
    HCCAST_WIFI_DISCONNECT,
    HCCAST_WIFI_DISCONNECT_RESULT,
    HCCAST_WIFI_HOSTAP_OFFER,
    HCCAST_WIFI_MAX,
} hccast_wifi_event_e;

typedef enum _hccast_p2p_state_e_
{
    HCCAST_P2P_STATE_NONE = 0,
    HCCAST_P2P_STATE_LISTEN,
    HCCAST_P2P_STATE_CONNECTING,
    HCCAST_P2P_STATE_CONNECTED,
    HCCAST_P2P_STATE_CONNECT_FAILED,
    HCCAST_P2P_STATE_DISCONNECTED,
    HCCAST_P2P_STATE_MAX,
} hccast_p2p_state_e;

typedef int (*hccast_wifi_mgr_event_callback)(hccast_wifi_event_e event, void* in, void* out);
typedef int (*hccast_wifi_mgr_p2p_state_callback)(hccast_p2p_state_e state);
typedef int (*hccast_wifi_mgr_p2p_set_resolution)(int res);
typedef struct _hccast_wifi_hostap_conf_t_
{
    int     channel;
    char    ssid[WIFI_MAX_SSID_LEN];
    char    pwd[WIFI_MAX_PWD_LEN];
    unsigned char    mode;      // 0: default(conf), 1: g (2.4 GHz), 2: a (5 GHz), 3: ad (60 GHz)
    char    country_code[3];    // ISO/IEC 3166-1, two char
    unsigned char    band;      // Reserved
    char    res[3];             // Reserved
} hccast_wifi_hostap_conf_t;

typedef struct _hccast_wifi_p2p_param_t_
{
    char* device_name;
    hccast_wifi_mgr_p2p_state_callback func_update_state;
    hccast_wifi_mgr_p2p_set_resolution func_set_resolution;
} hccast_wifi_p2p_param_t;


//// * ############################################## * ////

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Get the result of scanning AP
 *
 * @param res the result of the scan
 *
 * @return The current state of the wifi scan: 1:scanning other: other .
 */
int hccast_wifi_mgr_scan(hccast_wifi_scan_result_t *scan_res);

/**
 * Get the current status of scanning AP
 *
 * @param NULL
 *
 * @return The return value is the current connection status of the device.
 */
int hccast_wifi_mgr_get_scan_status(void);

/**
 * Get the current status of connecting AP
 *
 * @param NULL
 *
 * @return The return value is the current connection status of the device.
 */
int hccast_wifi_mgr_get_connect_status(void);

/**
 * Get the SSID of the connected AP
 *
 * @param ssid: the name of the current connected AP
 * @param len: the length of the ssid buffer
 *
 * @return the status of the wifi connection.
 */
int hccast_wifi_mgr_get_connect_ssid(char* ssid, size_t len);

/**
 * It gets the list of networks (save).
 *
 * @param ifname network interface name
 *
 * @param res: The result of the operation.
 *
 * @return < 0: failed, 0: success
 */
int hccast_wifi_mgr_get_list_net(hccast_wifi_list_net_result_t *res);

/**
 * It removes a network from the list of networks.
 *
 * @param net_id the network ID of the network to be deleted.
 *
 * @return < 0: failed, 0: success
 */
int hccast_wifi_mgr_rm_list_net(char* net_id);

/**
 * Used to connect to an AP.
 * @param ap_info: conn AP info
 * @return < 0: err, 0:failed, 1: success
 */
int hccast_wifi_mgr_connect(const hccast_wifi_ap_info_t *ap_info);

/**
 * This function is used to disconnect AP, with cb message
 *
 * @return <0: failed; 0: success
 */
int hccast_wifi_mgr_disconnect(void);

/**
 *  This function is used to disconnect AP, without cb message
 * 
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_disconnect_no_message(void);

/**
 * It returns the value of the global variable g_wifi_model.
 *
 * @return the global variable wifi_model.
 */
int hccast_wifi_mgr_get_wifi_model(void);

/**
 *  Set according to wifi model
 *
 * @param wifi_model 0 for AP mode, 1 for STA mode
 *
 * @return the global variable wifi_model
 */
int hccast_wifi_mgr_set_wifi_model(int wifi_model);

/**
 * It get the frequency of the wifi model capacity.
 * @return hccast_wifi_freq_mode_e
 *
 * @note rtos dont support.
 */
hccast_wifi_freq_mode_e hccast_wifi_mgr_freq_support_mode(void);

/*
 *
*/
hccast_wifi_freq_mode_e hccast_wifi_mgr_get_current_freq_mode();

/*
 *
*/
int hccast_wifi_mgr_get_current_freq();

/**
 * It gets the signal poll result include rssi/linkspeed/noise/frequency.
 * 
 * @param result the result of the signal polling
 */
int hccast_wifi_mgr_get_signal_poll(hccast_wifi_signal_poll_result_t *result);

/**
 * It sets the hostap configuration.
 *
 * @param conf a pointer to a structure of type hccast_wifi_hostap_conf_t.
 */
int hccast_wifi_mgr_hostap_set_conf(hccast_wifi_hostap_conf_t *conf);

/**
 * It switches the wifi mode to 24G or 5G mode.
 *
 * @param mode The frequency mode to switch to.
 */
int hccast_wifi_mgr_hostap_switch_mode(hccast_wifi_freq_mode_e mode);

/**
 * It switches the wifi mode to 24G or 5G mode.
 *
 * @param mode The frequency mode to switch to.
 * @param channel The frequency for channel.
 * @param flag The mode option (res)
 */
int hccast_wifi_mgr_hostap_switch_mode_ex(hccast_wifi_freq_mode_e mode, int channel, int flag);

/**
 * It switches the wifi channel to 24G or 5G channel list.
 *
 * @param channel The hostap channel to switch to.
 */
int hccast_wifi_mgr_hostap_switch_channel(int channel);

/**
* It gets the number of stations connected to the AP.
*
* @param result the result of the hostap status
*
* @return The number of stations connected to the hostapd.
*/
int hccast_wifi_mgr_hostap_get_sta_num(hccast_wifi_hostap_status_result_t *result);

/**
 * It enables the hostap interface.
 */
int hccast_wifi_mgr_hostap_enable(void);

/**
 * It enables the hostap interface.
 */
int hccast_wifi_mgr_hostap_disenable(void);

/**
 * It starts the hostapd daemon, configures the IP address of the interface, and starts the udhcpd
 * daemon
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_hostap_start(void);

/**
 * It starts the hostapd daemon, configures the interface, and starts the udhcpd daemon
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_hostap_restart(void);

/**
 * It stops the hostapd service.
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_hostap_stop(void);

/**
 * It starts the udhcpd server (ifname: wlan0)
 * 
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpd_start(void);

/**
 * It stops the udhcpd server (ifname: wlan0)
 * 
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpd_stop(void);

/**
 * It starts the udhcpc client (ifname: wlan0)
 * 
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpc_start(void);

/**
 * It stops the udhcpc client (ifname: wlan0)
 * 
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpc_stop(void);

/**
 * It initializes the P2P interface
 *
 * @param p2p_param the parameter structure of the p2p module, which is defined as follows:
 *
 * @return The return value is the return value of the function p2p_ctrl_init.
 */
int hccast_wifi_mgr_p2p_init(hccast_wifi_p2p_param_t* p2p_param);

/**
 * It un-initializes the p2p_ctrl module.
 *
 * @return The return value is the return value of the function p2p_ctrl_uninit().
 */
int hccast_wifi_mgr_p2p_uninit(void);

/**
 * > Initialize the P2P device
 *
 * @return The return value is the return value of p2p_ctrl_device_init().
 */
void hccast_wifi_mgr_p2p_device_init(void);

/**
 * when p2p connected, get p2p role (go).
 */
int hccast_wifi_mgr_p2p_is_go(void);

/**
 * It enables the p2p device.
 *
 * @param enable true to enable P2P, false to disable P2P.
 *
 * @return enables value.
 */
bool hccast_wifi_mgr_p2p_set_enable(bool enable);

/**
 *  get p2p device enable status
 */
bool hccast_wifi_mgr_p2p_get_enable(void);

/**
 * It returns the IP address of the device.
 * 
 * @return The IP address of the device.
 */
unsigned int hccast_wifi_mgr_p2p_get_ip(void);

/**
 * > This function returns the status of the hostapd service
 *
 * @return The hostap_en variable is being returned.
 */
int hccast_wifi_mgr_get_hostap_status(void);

/**
 * It returns the RTSP port of the device.
 * 
 * @return The RTSP port of the device.
 */
int hccast_wifi_mgr_p2p_get_rtsp_port(void);

/**
 * It returns the connection status of the P2P connection.
 */
int hccast_wifi_mgr_p2p_get_connect_stat(void);

/**
 * It sets the connection status of the P2P connection.
 * 
 * @param stat true if connected, false if not connected
 */
int hccast_wifi_mgr_p2p_set_connect_stat(bool stat);

/**
 * get the wifi is currently connecting flag
 */
int hccast_wifi_mgr_get_wifi_is_connecting(void);

/**
 * > Initialize the WiFi manager
 * 
 * @param func The callback function that will be called when the WiFi connection status changes.
 * 
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_init(hccast_wifi_mgr_event_callback func);

/**
 * > This function is used to unregister the callback function of the WiFi module
 * 
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_uninit(void);

/**
 * This function is called by the HCCast application to abort the current WiFi operation
 *
 * @return always 0.
 * @note: need to be used with hccast_wifi_mgr_op_reset
 */
int hccast_wifi_mgr_op_abort();

/**
 * It resets the wifi op stat for HCCAST_WIFI_STAT_IDLE.
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 *
 * @note: need to be used with hccast_wifi_mgr_op_abort
 */
int hccast_wifi_mgr_op_reset();

#define hccast_wifi_is_connecting hccast_wifi_mgr_is_connecting
#define hccast_wifi_is_scaning hccast_wifi_mgr_is_scanning
#define hccast_wifi_mgr_is_scaning hccast_wifi_mgr_is_scanning

/**
 * It sets the udhcpd configuration
 * 
 * @param conf udhcpd configuration
 * 
 * @return 0: success.
 * 
 * @note: conf.func == NULL will use hccast default callback func, 
 * otherwise, will use conf.func when udhcpd offer ip success.
 */
int hccast_wifi_mgr_set_udhcpd_conf(udhcp_conf_t conf);

/**
 * It sets the udhcpd configuration
 * 
 * @param conf udhcpd configuration
 * 
 * @return 0: success.
 * 
 * @note: conf.func == NULL will use hccast default callback func, 
 * otherwise, will use conf.func when udhcpc get ip success or failed.
 */
int hccast_wifi_mgr_set_udhcpc_conf(udhcp_conf_t conf);

/**
 * It sets the IP address, netmask and gateway for the host network.
 * 
 * @param inf the interface name, such as wlan0, p2p0, etc.
 * @param ip the IP address of the interface
 * @param netmask such as 255.255.255.0
 * @param gateway the gateway of the network
 * 
 * @return 0: success, < 0: failed
 */
int hccast_wifi_mgr_set_host_network(char *inf, char *ip, char *netmask, char *gateway);


// ! The following functions are only for hcrtos

int hccast_wifi_mgr_hostap_store_conf(hccast_wifi_hostap_conf_t *conf);
int hccast_wifi_mgr_enter_sta_mode(void);
int hccast_wifi_mgr_exit_sta_mode(void);
int hccast_wifi_mgr_get_station_status(void);
int hccast_wifi_mgr_is_connecting(void);
int hccast_wifi_mgr_is_scanning(void);
int hccast_wifi_mgr_trigger_scan(char *inf);
int hccast_wifi_mgr_get_best_channel(int argc, int **argv);
int hccast_wifi_mgr_set_best_channel(int *ch24G, int *ch5G);
int hccast_wifi_mgr_p2p_wps_pbc(void);
int hccast_wifi_mgr_get_support_miracast();


#ifdef __cplusplus
}
#endif

#endif
