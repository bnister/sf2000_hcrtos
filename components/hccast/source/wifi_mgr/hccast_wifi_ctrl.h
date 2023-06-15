#ifndef _HCCAST_WIFI_CTRL_H_
#define _HCCAST_WIFI_CTRL_H_

#include <stdbool.h>
#include "hccast_wifi_mgr.h"

#define WIFI_CTRL_PATH_HOSTAP   "/var/run/hostapd"
#define WIFI_CTRL_PATH_STA      "/var/run/wpa_supplicant"
#define WIFI_CTRL_IFACE_NAME    "wlan0"
#define HS_CTRL_IFACE_NAME      "wlan0"
#define P2P_CTRL_IFACE_NAME     "p2p0"


//// * ############################################## * ////

/**
 * It runs a command on the wpa_supplicant or hostapd process
 *
 * @param ifname the interface name of the wireless network card, such as wlan0, p2p0, etc.
 * @param mode HCCAST_WIFI_MODE_HOSTAP or HCCAST_WIFI_MODE_STA
 * @param cmd the command to be executed
 * @param result the result of the command execution
 * @param len the length of the result buffer
 *
 * @return The return value  0 - success, other - failed.
 *
 * common CMD:
 *  "STATUS"
 *  "STATUS-DRIVER"
 *  "STATUS-VERBOSE"
 *  "SIGNAL_POLL"
 *  ""
 */
int wifi_ctrl_run_cmd(const char *ifname, hccast_wifi_mode_e mode, char *cmd, char *result, unsigned int *len);

/**
 * It runs a command on the wpa_supplicant process, wpas handle can next used. 
 * 
 * @param ifname the name of the interface, such as wlan0
 * @param mode the mode of the wifi, which can be HCCAST_WIFI_MODE_HOSTAP or HCCAST_WIFI_MODE_STA etc.
 * @param cmd the command to be executed
 * @param result the result of the command execution
 * @param len the length of the result
 * 
 * @return The return value is the result of the wpa_ctrl_request function.
 *
 * NOTE: When not keeping status, will be called wifi_ctrl_run_cmd()
 * HCCAST_WIFI_MODE_XXX_KEEP enable keeping, HCCAST_WIFI_MODE_XXX disable keeping.
 */
int wifi_ctrl_run_cmd_keep(char* ifname, hccast_wifi_mode_e mode, char *cmd, char *result, unsigned int *len);

/**
 * It gets the current status of the WiFi interface
 *
 * @param ifname the interface name, such as wlan0, p2p0, etc.
 * @param mode the mode of the interface, STA or AP
 * @param result the result of the query
 */
int wifi_ctrl_get_status(const char* ifname, hccast_wifi_mode_e mode, hccast_wifi_status_result_t *result);

/**
 * It gets the signal strength of the current connection
 *
 * @param result the result of the signal poll
 */
int wifi_ctrl_get_signal_poll(hccast_wifi_signal_poll_result_t *result);

/**
 * This function is used to get the list of APs that the device connected (save)
 *
 * @param ifname network interface name
 *
 * @param info_res The structure of the networks information returned by the function
 *
 * @return
 */
int wifi_ctrl_get_list_net(char* ifname, hccast_wifi_list_net_result_t *res);

/**
 * It removes a network from the wpa_supplicant.conf file.
 *
 * @param ifname network interface name
 *
 * @param net_id the network id of the network to be removed.
 *
 * @return The return value is the network id of the newly created network.
 */
int wifi_ctrl_do_remove_net_by_id(char* ifname, char* net_id);

/**
 * It removes all the networks from the interface.
 *
 * @param ifname the interface name of the wifi device, such as wlan0
 *
 * @return The return value is the number of networks that were removed.
 */
int wifi_ctrl_do_remove_net_all(char* ifname);

int wifi_ctrl_is_connecting();
int wifi_ctrl_is_disconnecting();
int wifi_ctrl_is_scanning();

/**
 * Set the global variable g_wifi_is_connecting or  g_wifi_is_disconnecting to the value of the flag parameter.
 *
 * @param flag true or false
 *
 * @return The value of g_wifi_is_connecting or g_wifi_is_disconnecting.
 */
int wifi_ctrl_set_connecting_by_user(bool flag);
int wifi_ctrl_set_disconnecting_by_user(bool flag);

/**
 * It connects to a WiFi AP
 *
 * @param ap_info the structure of the AP information, including the SSID and password of the AP.
 *
 * @return The return value is the number of bytes that were written to the file.
 */
int wifi_ctrl_do_ap_connect(const hccast_wifi_ap_info_t *ap_info);

/**
 * This function is used to disconnect APs
 */
int wifi_ctrl_do_ap_disconnect();

/**
 * This function is used to scan the available APs
 */
int wifi_ctrl_do_scan();

/**
 * Used to abort the current wifi block operation (include scanning or connecting).
 */
void wifi_ctrl_do_op_abort();

/**
 * Used to abort the current wifi scanning block operation.
 */
void wifi_ctrl_do_op_scanning_abort();

/**
 * Used to abort the current wifi connecting block operation.
 */
void wifi_ctrl_do_op_connecting_abort();

/**
 * Used to reset the current wifi operation stat for HCCAST_WIFI_STAT_IDLE.
 */
void wifi_ctrl_do_op_reset();

int wifi_ctrl_p2p_lock();
int wifi_ctrl_p2p_unlock();
int wifi_ctrl_signal();

/**
 * This function is wait scan end.
 * @param sec: timeout time (s).
 */
int wifi_ctrl_wait_scan_timeout(unsigned int sec);

/**
 * This function is wait connect end.
 * @param sec: timeout time (s).
 */
int wifi_ctrl_wait_connect_timeout(unsigned int sec);

/**
 * Get the scan result of the AP list
 *
 * @param scan_res the pointer to the structure of the scan result
 */
int wifi_ctrl_get_aplist(hccast_wifi_scan_result_t *scan_res);

/**
 * It gets the capability of the wifi device
 *
 * @param type the type of capability to get.
 * @param res the result of the command
 * @param res_len the length of the buffer pointed to by res.
 *
 * @return The reply is a string of the form:
 */
int wifi_ctrl_get_capability(char* type, char* res, size_t res_len);

/**
 * It enables or disables the hostap interface
 *
 * @param enable 1 for enable, 0 for disable
 *
 * @return 0: success, < 0 failed.
 */
int wifi_ctrl_hostap_interface_enable(bool enable);

/**
 * It gets the status of the hostapd
 *
 * @param result the result of the command execution
 *
 * @return the status of the hostapd.
 */
int wifi_ctrl_hostap_get_status(hccast_wifi_hostap_status_result_t *result);

/**
 * It sets the hostapd configuration
 *
 * @param conf the configuration of the AP
 *
 * @return The return value is the result of the command.
 */
int wifi_ctrl_hostap_set_conf(hccast_wifi_hostap_conf_t *conf);

/**
 * It changes the wifi mode to the specified mode
 *
 * @param mode the frequency mode of the wifi, which can be 5G, 60G, or 24G.
 *
 * @return The return value is the return value of the function wifi_ctrl_run_cmd.
 */
int wifi_ctrl_hostap_switch_mode(hccast_wifi_freq_mode_e mode);

/**
 * It changes the wifi mode to the specified mode
 *
 * @param mode the frequency mode of the wifi, which can be 5G, 60G, or 24G.
 *
 * @param channel the frequency for channel (Must correspond to mode)
 *
 * @flag hostap option (res)
 *
 * @return The return value is the return value of the function wifi_ctrl_run_cmd.
 */
int wifi_ctrl_hostap_switch_mode_ex(hccast_wifi_freq_mode_e mode, int channel, int flag);

/**
 * It changes the hostap wifi channel to the specified mode
 *
 * @param channel the channel of the wifi, which can be 5G, 60G, or 24G channel list.
 *
 * @return The return value is the return value of the function wifi_ctrl_run_cmd.
 */
int wifi_ctrl_hostap_switch_channel(int channel);

/**
* It converts a string like "\xE4\xB8\xAD\xE6\x96\x87" to "中文"
*
* @param srcStr The string to be converted.
*
* @return the address of the first character in the string.
*/
char *wifi_ctrl_chinese_conversion(char *srcStr);

bool wifi_ctrl_is_initted();

int wifi_ctrl_init(hccast_wifi_mgr_event_callback func);


// ! The following functions are only for hcrtos

#ifdef HC_RTOS
int wifi_ctrl_hostap_store_config(hccast_wifi_hostap_conf_t* conf);
int wifi_ctrl_hostap_thread_start(void);
int wifi_ctrl_hostap_thread_stop(void);
int wifi_ctrl_sta_thread_start(void);
int wifi_ctrl_sta_thread_stop(void);
int wifi_ctrl_p2p_wps_pbc();
int wifi_ctrl_hostap_set_best_channel(int ch24G, int ch5G);
#endif // HC_RTOS

#endif