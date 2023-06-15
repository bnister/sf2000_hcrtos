#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <net/if.h>
#include <sys/unistd.h>
#include <net/net_device.h>

#include "../inc/hccast_dhcpd.h"
#include "../common/hccast_log.h"

#define STK_SIZE (1024 * 8)

int udhcpc_stop(udhcp_conf_t *conf);

int dhcpc_disable(const char *ifname)
{
    struct ifreq ifr;
    int skfd;

    if (!ifname)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;
    }

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        hccast_log(LL_ERROR, "%s: socket error!\n", __func__);
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
        close(skfd);
        return -1;
    }

    ifr.ifr_flags &= ~IFF_DYNAMIC;

    if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0) {
        hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
        close(skfd);
        return -1;
    }

    close(skfd);

    return 0;
}

int dhcpc_enable(const char *ifname)
{
    struct ifreq ifr;
    int skfd;

    if (!ifname)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;
    }

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        hccast_log(LL_ERROR, "%s: socket error!\n", __func__);
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
        close(skfd);
        return -1;
    }

    if (ifr.ifr_flags & IFF_DYNAMIC)
    {
        ifr.ifr_flags &= ~IFF_DYNAMIC;
        if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
            hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
            close(skfd);
            return -1;
        }

        usleep(200*1000);
    }

    ifr.ifr_flags |= IFF_DYNAMIC | IFF_UP | IFF_RUNNING;
    //ifr.ifr_flags |= IFF_DYNAMIC;

    if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0) {
        hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
        close(skfd);
        return -1;
    }

    close(skfd);
    return 0;
}

int dhcpc_get_status(const char *ifname)
{
    struct ifreq ifr;
    int skfd;
    int ret = 0;

    if (!ifname)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;
    }

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        hccast_log(LL_ERROR, "%s: socket error!\n", __func__);
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
        close(skfd);
        return -1;
    }

    if (ifr.ifr_flags & IFF_DYNAMIC)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    close(skfd);
    return ret;
}

int dhcpc_get_ip(const char *ifname)
{
    struct ifreq ifr;
    int skfd;

    if (!ifname)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;
    }

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        hccast_log(LL_ERROR, "%s: socket error!\n", __func__);
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
        hccast_log(LL_ERROR, "%s: ioctl SIOCGIFADDR\n", __func__);
        close(skfd);
        return -1;
    }

    struct sockaddr_in *our_ip = (struct sockaddr_in *) &ifr.ifr_addr;

    close(skfd);

    return our_ip->sin_addr.s_addr;
}

int dhcpc_main(void* arg)
{
    udhcp_conf_t* conf = (udhcp_conf_t*) arg;
    char ifname[64] = {0};
    struct in_addr addr_pre = {0};
    struct in_addr addr = {0};
    int count = 0;
    hccast_udhcp_result_t res = {0};

    if (!conf)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;
    }

    conf->run = 1;

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

    if (dhcpc_get_status(ifname))
    {
        dhcpc_disable(ifname);
    }

    addr_pre.s_addr = dhcpc_get_ip(ifname);

    dhcpc_enable(ifname);

    while(conf->run)
    {
        if (count++ >= 10)
        {
            hccast_log(LL_ERROR, "%s: timeout!\n", __func__);
            break;
        }

        addr.s_addr = dhcpc_get_ip(ifname);
        if (addr.s_addr != 0 && addr.s_addr != addr_pre.s_addr)
        {
            if (conf->func)
            {
                res.stat = 1;
                strncpy(res.ip, inet_ntoa(addr), sizeof(res.ip));
                NetIfGetGateway(ifname, &addr.s_addr);
                strncpy(res.gw, inet_ntoa(addr), sizeof(res.gw));
                //strncpy(res.gw, inet_ntoa("255.255.255.0"), sizeof(res.gw));
                strncpy(res.ifname, ifname, sizeof(res.ifname));
            }

            goto RET;
        }

        usleep(1000 * 1000);
    }

    dhcpc_disable(ifname);

RET:
    if (conf->func)
    {
        conf->func((unsigned int)&res);
    }

    conf->run = 0;
    conf->pid = 0;

    return 0;
}

// rtos
int udhcpc_start(udhcp_conf_t *conf)
{
    pthread_t pid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, STK_SIZE);

    if (!conf)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;
    }

    if (pthread_create((pthread_t*)&conf->pid, NULL, (void*)dhcpc_main, conf) != 0)
    {
        hccast_log(LL_ERROR, "udhcpd_main create failed!\n");
        return -1;
    }

    return 0;
}

int udhcpc_stop(udhcp_conf_t *conf)
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
    
    conf->run = 0;
    conf->pid = 0;

    return dhcpc_disable(ifname);
}
