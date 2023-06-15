#ifndef _NET_DEVICE_MODULE_H
#define _NET_DEVICE_MODULE_H

#include <netinet/in.h>
#include <linux/skbuff.h>

int32_t NetIfDhcpsStart(char *if_name, char *ip, uint16_t ipNum);
int32_t NetIfDhcpsStop(char *if_name);
int32_t NetIfSetDefault(char *if_name);
int32_t NetIfGetGateway(char *if_name, uint32_t *addr);
int32_t NetIfDnsSetServer(int numdns, struct in_addr *addrs);
int32_t NetIfDnsGetServer(int numdns, struct in_addr *addrs);

//callback, if return 0, eat skb, otherwise clone skb
enum {
	MONITOR_SKB_EAT,
	MONITOR_SKB_CLONE,
	MONITOR_SKB_NOT_HANDLE,
};

typedef int (*NetifLLMonitorReceive)(struct sk_buff *skb, void *opaque);
void *NetIfLLMonitorConfig(char *if_name, NetifLLMonitorReceive monitor_receive_hook, void *opaque);
int32_t NetIfLLMonitorSend(void *config, struct sk_buff *skb);
void *NetifLLMonitorGetDev(void *config);

#endif
