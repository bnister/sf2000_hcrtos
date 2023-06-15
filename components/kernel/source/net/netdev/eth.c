#include <malloc.h>
#include <linux/compiler.h>
#include <linux/printk.h>
#include <linux/bug.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <uapi/linux/if_ether.h>

#include <linux/etherdevice.h>

static inline struct ethhdr *eth_hdr(const struct sk_buff *skb)
{
	return (struct ethhdr *)skb_mac_header(skb);
}

static u16 eth_htons(u16 n)
{
	return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

// __be16 eth_type_trans(struct sk_buff *skb, struct net_device *dev)
// {
// 	struct ethhdr *eth;

// 	skb->dev = dev;
// 	skb_reset_mac_header(skb);
// 	eth = eth_hdr(skb);

// 	if (eth->h_proto >= 1536)
// 		return eth->h_proto;

// 	/*
//      *      This is a magic hack to spot IPX packets. Older Novell breaks
//      *      the protocol design and runs IPX over 802.3 without an 802.2 LLC
//      *      layer. We look for FFFF which isn't a used 802.2 SSAP/DSAP. This
//      *      won't work for fault tolerant netware but does for the rest.
//      */
// 	if (skb->len >= 2 && *(unsigned short *)(skb->data) == 0xFFFF)
// 		return ETH_P_802_3;

// 	/*
//      *      Real 802.2 LLC
//      */
// 	return eth_htons(ETH_P_802_2);
// }


/**
 * eth_prepare_mac_addr_change - prepare for mac change
 * @dev: network device
 * @p: socket address
 */
int eth_prepare_mac_addr_change(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;

	// if (!(dev->priv_flags & IFF_LIVE_ADDR_CHANGE) && netif_running(dev))
	if (netif_running(dev))
		return -EBUSY;
	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;
	return 0;
}


/**
 * eth_commit_mac_addr_change - commit mac change
 * @dev: network device
 * @p: socket address
 */
void eth_commit_mac_addr_change(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;

	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
}

/**
 * eth_mac_addr - set new Ethernet hardware address
 * @dev: network device
 * @p: socket address
 *
 * Change hardware address of device.
 *
 * This doesn't change hardware matching, so needs to be overridden
 * for most real devices.
 */
int eth_mac_addr(struct net_device *dev, void *p)
{
	int ret;

	ret = eth_prepare_mac_addr_change(dev, p);
	if (ret < 0)
		return ret;
	eth_commit_mac_addr_change(dev, p);

	return 0;
}
