#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/net_device.h>

#include "../inc/hccast_dhcpd.h"
#include "../common/hccast_log.h"

#ifdef STK_SIZE
#undef STK_SIZE
#define STK_SIZE (1024 * 8)
#endif

#define DHCPD_ADDR_START ("192.168.68.10")
#define DHCPD_ADDR_NUM   (20)

int udhcpd_start(udhcp_conf_t *conf)
{
    char ifname[64] = {0};

    if (!conf)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;        
    }

    if (conf->ifname == UDHCP_IF_NONE)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "eth0");
    }
    else if (conf->ifname == UDHCP_IF_WLAN0)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "wlan0");
    }
    else if (conf->ifname == UDHCP_IF_WLAN1)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "wlan1");
    }
    else if (conf->ifname == UDHCP_IF_P2P0)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "p2p0");
    }

    NetIfDhcpsStart(ifname, DHCPD_ADDR_START, DHCPD_ADDR_NUM);

    return 0;
}

int udhcpd_stop(udhcp_conf_t *conf)
{
    char ifname[64] = {0};

    if (!conf)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;        
    }

    if (conf->ifname == UDHCP_IF_NONE)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "eth0");
    }
    else if (conf->ifname == UDHCP_IF_WLAN0)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "waln0");
    }
    else if (conf->ifname == UDHCP_IF_WLAN1)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "wlan1");
    }
    else if (conf->ifname == UDHCP_IF_P2P0)
    {
        snprintf(ifname, sizeof(ifname) - 1, "%s", "p2p0");
    }

    NetIfDhcpsStop(ifname);

    return 0;
}
