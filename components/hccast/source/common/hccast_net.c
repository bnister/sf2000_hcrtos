#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#ifdef __linux__
    #include <linux/if_vlan.h>
    #include <linux/sockios.h>
#else
    #include <net/net_device.h>
    #include <arpa/inet.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <hccast_log.h>

#include "hccast_net.h"

int hccast_net_ifconfig(char *if_name, char *ip, char *mask, char *gw)
{
    struct sockaddr_in sin;
    struct ifreq ifr;
    struct rtentry rt;
    int sock;

    if (!if_name || !ip)
    {
        return -1;
    }

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", if_name);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return -1;
    }

    // set mask
    if (mask)
    {
        inet_pton(AF_INET, mask, &sin.sin_addr.s_addr);
        sin.sin_family = AF_INET;
        memcpy((char *)&ifr.ifr_netmask, (char *)&sin, sizeof(struct sockaddr));
        if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0)
        {
            perror("SIOCSIFNETMASK:");
        }
    }

    // set ip
    inet_pton(AF_INET, ip, &sin.sin_addr.s_addr);
    sin.sin_family = AF_INET;
    memcpy((char *)&ifr.ifr_addr, (char *)&sin, sizeof(struct sockaddr));
    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0)
    {
        perror("SIOCSIFADDR:");
        close(sock);
        return -1;
    }

    if (gw)
    {
        memset(&rt, 0, sizeof(struct rtentry));

        inet_pton(AF_INET, gw, &sin.sin_addr.s_addr);
        sin.sin_family = AF_INET;
        sin.sin_port = 0;

        memcpy(&rt.rt_gateway, &sin, sizeof(struct sockaddr_in));
        ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
        rt.rt_flags = RTF_GATEWAY;
        rt.rt_dev = if_name;
        if (ioctl(sock, SIOCADDRT, &rt) < 0)
        {
            perror("SIOCADDRT:");
        }
    }

    close(sock);

    return 0;
}

int hccast_net_route_delete(char *if_name, char *gw)
{
    struct sockaddr_in sin;
    struct rtentry rt;
    int sock;

    if (!if_name || !gw)
    {
        return -1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return -1;
    }

    memset(&rt, 0, sizeof(struct rtentry));

    inet_pton(AF_INET, gw, &sin.sin_addr.s_addr);
    sin.sin_family = AF_INET;
    sin.sin_port = 0;

    memcpy(&rt.rt_gateway, &sin, sizeof(struct sockaddr_in));
    ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
    rt.rt_flags = RTF_GATEWAY;
    rt.rt_dev = if_name;
    if (ioctl(sock, SIOCDELRT, &rt) < 0)
    {
        perror("SIOCADDRT:");
    }

    close(sock);
}

int hccast_net_get_mac(unsigned int wifi_model, char *mac)
{
    int fd;
    char mac_s[32] = {0};
    char path[128] = {0};
    char subs[4] = {0};
    int i;

    if (!mac)
    {
        return -1;
    }

    switch (wifi_model)
    {
    case HCCAST_NET_WIFI_8188FTV:
        strcat(path, "/var/lib/misc/RTL8188FTV.probe");
        break;
    case HCCAST_NET_WIFI_8811FTV:
        strcat(path, "/var/lib/misc/RTL8811FTV.probe");
        break;
    default:
        return -1;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("Get MAC");
        return -1;
    }
    if (read(fd, mac_s, 20) <= 0)
    {
        perror("Read MAC");
        close(fd);
        return -1;
    }
    close(fd);

    subs[0] = mac_s[0];
    subs[1] = mac_s[1];
    mac[0] = (char)strtol(subs, NULL, 16);
    subs[0] = mac_s[3];
    subs[1] = mac_s[4];
    mac[1] = (char)strtol(subs, NULL, 16);
    subs[0] = mac_s[6];
    subs[1] = mac_s[7];
    mac[2] = (char)strtol(subs, NULL, 16);
    subs[0] = mac_s[9];
    subs[1] = mac_s[10];
    mac[3] = (char)strtol(subs, NULL, 16);
    subs[0] = mac_s[12];
    subs[1] = mac_s[13];
    mac[4] = (char)strtol(subs, NULL, 16);
    subs[0] = mac_s[15];
    subs[1] = mac_s[16];
    mac[5] = (char)strtol(subs, NULL, 16);

    return 0;
}

#ifdef __linux__
/*
* Set DNS server address for ifname.
* @param ifname: The name of the interface (hcrtos not support).
* @param dns: The DNS server address.
* @return: 0 on success, -1 on failure.
* @note: the impl only use for hclinux.
*/
int hccast_net_set_dns(char *ifname, char *dns)
{
    int fd;
    int ret;

    if (!dns)
    {
        return -1;
    }

    if ((fd = open("/etc/resolv.conf", O_RDWR | O_TRUNC | O_CREAT, 0775)) < 0)
    {
        perror("open error!");
        return -1;
    }
    else
    {
        char dns1[16] = {0};
        char dns2[16] = {0};
        char buf[64]  = {0};

        ret = sscanf(dns, "%s %s", dns1, dns2);
        if (0 == ret)
        {
            hccast_log(LL_ERROR, "dns format error!\n");
            close(fd);
            return -1;
        }
        else if (1 == ret)
        {
            sprintf(buf, "nameserver %s # %s \n", dns1, ifname);
            write(fd, buf, strlen(buf));
        }
        else if (2 == ret)
        {
            sprintf(buf, "nameserver %s # %s\n", dns1, ifname);
            write(fd, buf, strlen(buf));
            sprintf(buf, "nameserver %s # %s\n", dns2, ifname);
            write(fd, buf, strlen(buf));
        }
    }

    fsync(fd);
    close(fd);
    return 0;
}
#else
/*
* Set DNS server address for ifname.
* @param ifname: The name of the interface (hcrtos not support).
* @param dns: The DNS server address.
* @return: 0 on success, -1 on failure.
* @note: the impl only for hcrtos.
*/
int hccast_net_set_dns(char *ifname, char *dns)
{
    int ret = -1;

    if (!dns)
    {
        return ret;
    }

    char dns1[16] = {0};
    char dns2[16] = {0};
    char buf[64]  = {0};

    ret = sscanf(dns, "%s %s", dns1, dns2);
    if (0 == ret)
    {
        hccast_log(LL_ERROR, "dns format error!\n");
        return -1;
    }
    else if (1 == ret)
    {
        struct in_addr addr;
        inet_aton(dns1, &addr);
        ret = NetIfDnsSetServer(1, &addr);
    }
    else if (2 == ret)
    {
        struct in_addr addr[2] = {0};
        inet_aton(dns1, &addr[0]);
        inet_aton(dns2, &addr[1]);
        ret = NetIfDnsSetServer(2, &addr[0]);
    }

    return ret;
}
#endif

int hccast_net_set_if_updown(const char *ifname, hccast_net_if_flag_e flag)
{
    struct ifreq ifr;
    int skfd;

    if (!ifname)
    {
        hccast_log(LL_ERROR, "%s: param error!\n", __func__);
        return -1;
    }

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        hccast_log(LL_ERROR, "%s: socket error!\n", __func__);
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
    {
        hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
        close(skfd);
        return -1;
    }

    if (flag)
    {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    }
    else if (!flag)
    {
        ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
    }

    if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0)
    {
        hccast_log(LL_ERROR, "%s: ioctl SIOCSIFFLAGS\n", __func__);
        close(skfd);
        return -1;
    }

    close(skfd);
    return 0;
}