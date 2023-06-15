#include <linux/netdevice.h>
#include <pthread.h>
#include <unistd.h>
/*#include <lwip/netif.h>*/
#include "raw_proto.h"
#include <net/net_device.h>
#include <errno.h>

void *netdev_config;
int recv_calback(struct sk_buff *skb, void *opaque)
{
	dev_kfree_skb_any(skb);	
	return MONITOR_SKB_EAT;
}

int raw_init(char *if_name, raw_recv_func cb, void *opaque)
{
	if(netdev_config){
		return 0;
	}

	if(!if_name)
		return -EINVAL;

	netdev_config = NetIfLLMonitorConfig("eth0", cb, opaque);
	if(!netdev_config){
		return -1;
	}
	return 0;
}
