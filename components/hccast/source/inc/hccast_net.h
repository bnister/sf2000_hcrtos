#ifndef __HCCAST_NET_H__
#define __HCCAST_NET_H__

#include <stdbool.h>

typedef enum {
	HCCAST_NET_WIFI_NONE = 0,

	HCCAST_NET_WIFI_24G = 0x10000,
	HCCAST_NET_WIFI_8188FTV,
	// The 24G fill here

	HCCAST_NET_WIFI_24G_BT = 0x11000,
	// The 24G with Bluetooth fill here

	HCCAST_NET_WIFI_5G = 0x20000,
	HCCAST_NET_WIFI_8811FTV,
	HCCAST_NET_WIFI_8733BU,
	// The 5G fill here

	HCCAST_NET_WIFI_5G_BT = 0x21000,
	// The 5G with Bluetooth  fill here

	HCCAST_NET_WIFI_60G = 0x30000,
	// The 60G fill here

	HCCAST_NET_WIFI_60G_BT = 0x31000,
	// The 60G with Bluetooth  fill here

	HCCAST_NET_WIFI_MAX,
} hccast_net_wifi_model_e;

typedef enum
{
    HCCAST_NET_IF_DOWN = 0,
    HCCAST_NET_IF_UP,
} hccast_net_if_flag_e;

#ifdef __cplusplus
extern "C" {
#endif

int hccast_net_ifconfig(char *if_name, char *ip, char *mask, char *gw);
int hccast_net_route_delete(char *if_name, char *gw);
int hccast_net_get_mac(unsigned int wifi_model, char *mac);
int hccast_net_set_dns(char *ifname, char *dns);
int hccast_net_set_if_updown(const char* ifname, hccast_net_if_flag_e flag);

#ifdef __cplusplus
}
#endif

#endif
