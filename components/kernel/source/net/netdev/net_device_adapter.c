#include <lwip/opt.h>
#include <lwip/netif.h>
#include <lwip/etharp.h>
#include <lwip/netifapi.h>
#include <lwip/pbuf.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/printk.h>
#include <sys/ioctl.h>
#include <net/net_device.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <lwip/netifapi.h>

#define MAX_CONVERSION_LEN	65535

#ifndef LWIP_NETIF_IFINDEX_MAX_EX
#define LWIP_NETIF_IFINDEX_MAX_EX 255
#endif

struct hc_custom_pbuf {
	struct pbuf_custom pc;
	struct sk_buff *skb;
};

static void hc_pbuf_free_custom(struct pbuf *p)
{
	struct hc_custom_pbuf *ppc;
	SYS_ARCH_DECL_PROTECT(old_level);

	ppc = (struct hc_custom_pbuf *)p;

	SYS_ARCH_PROTECT(old_level);
	dev_kfree_skb_any(ppc->skb);
	mem_free(p);
	SYS_ARCH_UNPROTECT(old_level);
}

static char *strnchr(const char *s, size_t count, int c)
{
	for (; count-- && *s != '\0'; ++s)
		if (*s == (char)c)
			return (char *)s;
	return NULL;
}

static int lwip_port_send_check(struct netif *netif, struct pbuf *p)
{
	if (netif == NULL || p == NULL)
		return -1;

	struct net_device *ndev = (struct net_device *)netif->state;
	if (ndev == NULL || ndev->netdev_ops == NULL || ndev->netdev_ops->ndo_start_xmit == NULL)
		return -1;

	return 0;
}

static struct sk_buff *convert_pbuf_to_skb(struct net_device *ndev, struct pbuf *p)
{
	struct sk_buff *skb;
	struct pbuf *q;

	if (ndev == NULL || p == NULL)
		return NULL;

	skb = netdev_alloc_skb(ndev, p->tot_len);
	for (q = p; q != NULL; q = q->next) {
		memcpy(skb_put(skb, q->len), q->payload, q->len);
	}     

	return skb;
}

static struct pbuf *convert_skb_to_pbuf_custom(struct sk_buff *skb)
{
	struct hc_custom_pbuf *ppc = NULL;
	struct pbuf *p  = NULL;
	unsigned int len;

	if (skb == NULL)
		return NULL;

	len = skb->len;

#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

	if (len > MAX_CONVERSION_LEN) {
		printk("%s skb len exceeds the maximum length of the pbuf!", __func__);
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return NULL;
	}

	ppc = (struct hc_custom_pbuf *)mem_malloc(sizeof(struct hc_custom_pbuf));
	if (ppc == NULL) {
		printk("%s pbuf_alloc failed! len = %d", __func__, len);
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return NULL;
	}

	ppc->skb = skb;
	ppc->pc.custom_free_function = hc_pbuf_free_custom;

#if ETH_PAD_SIZE
	p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, &ppc->pc, skb->data - ETH_PAD_SIZE, skb->truesize + ETH_PAD_SIZE);
#else
	p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, &ppc->pc, skb->data, skb->truesize);
#endif
	if (p == NULL) {
		mem_free(ppc);
		printk("%s pbuf_alloc failed! len = %d", __func__, len);
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return NULL;
	}

	LINK_STATS_INC(link.recv);

	return p;
}

static struct pbuf *convert_skb_to_pbuf(struct sk_buff *skb)
{
	struct pbuf *p  = NULL, *q = NULL;
	unsigned int len;
	u32_t start;

	if (skb == NULL)
		return NULL;

	len = skb->len;

#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

	if (len > MAX_CONVERSION_LEN) {
		printk("%s skb len exceeds the maximum length of the pbuf!", __func__);
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return NULL;
	}

	p = pbuf_alloc(PBUF_RAW, (uint16_t)len, PBUF_RAM);
	if (p == NULL) {
		printk("%s pbuf_alloc failed! len = %d", __func__, len);
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return NULL;
	}

#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

	start = 0;
	for(q = p; q != NULL; q = q->next) {
		memcpy(q->payload, &skb->data[start], q->len);
		start += q->len;
	}

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

	LINK_STATS_INC(link.recv);

	return p;
}

void lwip_port_send(struct netif *netif, struct pbuf *p)
{
	struct net_device *ndev;
	struct sk_buff *skb;

	if (lwip_port_send_check(netif, p) != 0) {
		return;
	}

	ndev = (struct net_device *)netif->state;
	skb = convert_pbuf_to_skb(ndev, p);
	if (skb == NULL)
		return;

	while(ndev->netdev_ops->ndo_start_xmit(skb, ndev) == NETDEV_TX_BUSY){
		usleep(1000);
	}

	return;
}

uint8_t lwip_port_set_hwaddr(struct netif *netif, uint8_t *addr, uint8_t len)
{
	if (netif == NULL || addr == NULL) {
		return ERR_VAL;
	}

	if (len != ETH_HWADDR_LEN) {
		return ERR_VAL;
	}

	struct net_device *ndev = (struct net_device *)netif->state;
	if (ndev == NULL || ndev->netdev_ops == NULL ||
	    ndev->netdev_ops->ndo_set_mac_address == NULL) {
		return ERR_VAL;
	}

	return ndev->netdev_ops->ndo_set_mac_address(ndev, addr);
}

void lwip_port_drv_config(struct netif *netif, u32_t config_flags, u8_t setBit)
{
}

int netif_receive_skb(struct sk_buff *skb)
{
	struct pbuf *p;
	struct net_device *ndev;
	
#if LWIP_MONITOR 
	extern int netif_monitor_receive_hook(struct sk_buff *skb);
	if(netif_monitor_receive_hook(skb) == MONITOR_SKB_EAT)
		return NET_RX_SUCCESS;
#endif

//	p = convert_skb_to_pbuf(skb);
	p = convert_skb_to_pbuf_custom(skb);
	if (p == NULL) {
		dev_kfree_skb_any(skb);
		return NET_RX_DROP;
	}

	ndev = skb->dev;

	driverif_input(ndev->netif, p);

//	dev_kfree_skb_any(skb);

	return NET_RX_SUCCESS;
}

static struct netif *netif_get_first_available(void)
{
	struct netif *netif = NULL;

	NETIF_FOREACH(netif) {
		if (netif->link_layer_type != LOOPBACK_IF && netif_is_up(netif))
			return netif;
		else if (netif->link_layer_type != LOOPBACK_IF && netif_is_link_up(netif))
			return netif;
		else if (netif_is_up(netif))
			return netif;
		else if (netif_is_link_up(netif))
			return netif;
	}

	return NULL;
}

static void netif_auto_change_default(struct netif *netif)
{
	struct netif *new_netif;

	if (netif_default == netif) {
		new_netif = netif_get_first_available();
		if (new_netif) {
			netif_set_default(new_netif);
		}
	}
}

static void netif_status_change_callback(struct netif *netif)
{
	struct net_device *dev = (struct net_device *)netif->state;
	const struct net_device_ops *ops = dev->netdev_ops;
	int ret = 0;

	if (netif_is_up(netif) && !(dev->flags & IFF_UP)) {
		set_bit(__LINK_STATE_START, &dev->state);
		if (ops->ndo_open)
			ret = ops->ndo_open(dev);
		if (ret)
			clear_bit(__LINK_STATE_START, &dev->state);
		else {
			dev->flags |= IFF_UP;
			if (ops->ndo_set_rx_mode)
				ops->ndo_set_rx_mode(dev);
		}
	} else if (!netif_is_up(netif) && (dev->flags & IFF_UP)) {
		clear_bit(__LINK_STATE_START, &dev->state);
		if (ops->ndo_stop)
			ops->ndo_stop(dev);
		dev->flags &= ~IFF_UP;

		netif_auto_change_default(netif);
	}
}

static void netif_link_change_callback(struct netif *netif)
{
	if (!netif_is_link_up(netif)) {
		netif_auto_change_default(netif);
	}else if (netif_is_link_up(netif)) {
		if(!netif_default || netif_default->link_layer_type == LOOPBACK_IF){
			netif_set_default(netif);
		}
	}
}

static int driverif_init_ifname(struct netif *netif)
{
	struct netif *tmpnetif = NULL;
	struct net_device *ndev = (struct net_device *)netif->state;
	const char *p;
	const char *prefix;
	int is_fixed_name = 0;

	if (ndev == NULL)
		return -1;

	if (ndev->name[0] == '\0') {
		prefix = "eth%d";
	} else {
		prefix = ndev->name;
		p = strnchr(prefix, IFNAMSIZ-1, '%');
		if (p) {
			/*
			 * Verify the string as this thing may have come from
			 * the user.  There must be either one "%d" and no other "%"
			 * characters.
			 */
			if (p[1] != 'd' || strchr(p + 2, '%'))
				return -1;
		} else {
			is_fixed_name = 1;
		}
	}

	netif->name[0] = prefix[0];
	netif->name[1] = prefix[1];

	for (int i = 0; i < LWIP_NETIF_IFINDEX_MAX_EX; ++i) {
		if (is_fixed_name)
			strncpy(netif->full_name, prefix, sizeof(netif->full_name));
		else
			snprintf(netif->full_name, sizeof(netif->full_name), prefix, i);

		NETIF_FOREACH(tmpnetif) {
			if (strcmp(tmpnetif->full_name, netif->full_name) == 0) {
				break;
			}
		}

		if (tmpnetif == NULL) {
			/* set full_name success */
			return 0;
		}
	}

	netif->full_name[0] = '\0';

	return -1;
}

int dev_ifsioc(struct netif *netif, struct ifreq *ifr, unsigned int cmd)
{
	int err;
	struct net_device *dev = (struct net_device *)netif->state;
	const struct net_device_ops *ops;

	if (!dev)
		return -ENODEV;

	ops = dev->netdev_ops;

	switch (cmd) {
	/*
	 *	Unknown or private ioctl
	 */
	default:
		if ((cmd >= SIOCDEVPRIVATE &&
		    cmd <= SIOCDEVPRIVATE + 15)
		    ) {
			err = -EOPNOTSUPP;
			if (ops->ndo_do_ioctl) {
				err = ops->ndo_do_ioctl(dev, ifr, cmd);
			}
		} else
			err = -EINVAL;

	}
	return err;
}

static void __netif_set_status_callback(void *netif)
{
	netif_set_status_callback((struct netif *)netif, netif_status_change_callback);
}

static void __netif_set_link_callback(void *netif)
{
	netif_set_link_callback((struct netif *)netif, netif_link_change_callback);
}

int register_netdev(struct net_device *dev)
{
	struct netif *netif = NULL;
	int ret = 0;
	ip4_addr_t gw, ipaddr, netmask;
	IP4_ADDR(&gw, 0, 0, 0, 0);
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	IP4_ADDR(&netmask, 0, 0, 0, 0);

	netif = (struct netif *)malloc(sizeof(struct netif));
	if (netif == NULL) {
		return -ENOMEM;
	}

	netif->state = (void *)dev;
	netif->drv_send = lwip_port_send;
	netif->drv_set_hwaddr = lwip_port_set_hwaddr;
	netif->drv_config = lwip_port_drv_config;
	netif->link_layer_type = ETHERNET_DRIVER_IF;
	netif->hwaddr_len = ETH_HWADDR_LEN;
	dev->netif = netif;

	ether_addr_copy(netif->hwaddr, dev->dev_addr);

	/* init the netif's full name */
	if (driverif_init_ifname(netif)) {
		free(netif);
		return -EFAULT;
	}

	memcpy(dev->name, netif->full_name, sizeof(dev->name));
	if (dev->netdev_ops->ndo_init) {
		ret = dev->netdev_ops->ndo_init(dev);
		if (ret) {
			if (ret > 0)
				ret = -EIO;
			free(netif);
			return ret;
		}
	}


	if ((ret = netifapi_netif_add(netif, &ipaddr, &netmask, &gw)) != ERR_OK) {
		if (dev->netdev_ops->ndo_uninit)
			dev->netdev_ops->ndo_uninit(dev);
		free(netif);
		return -EFAULT;
	}

	dev->mtu = netif->mtu;

	set_bit(__LINK_STATE_PRESENT, &dev->state);

	tcpip_callback(__netif_set_status_callback, netif);
	tcpip_callback(__netif_set_link_callback, netif);

	return 0;
}

void unregister_netdev(struct net_device *dev)
{
	struct netif *netif = dev->netif;

	clear_bit(__LINK_STATE_START, &dev->state);

	if (netif != NULL) {
		netifapi_netif_remove(netif);
		memset(netif, 0, sizeof(struct netif));
		dev->netif = NULL;
		free(netif);
	}

	if (dev->netdev_ops->ndo_stop)
		dev->netdev_ops->ndo_stop(dev);
	dev->flags &= ~IFF_UP;

	if (dev->netdev_ops->ndo_uninit)
		dev->netdev_ops->ndo_uninit(dev);

	netif_auto_change_default(netif);
}

void netif_carrier_on(struct net_device *dev)
{
	 if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &dev->state)) {
		 netif_set_link_up(dev->netif);
	 }
}

void netif_carrier_off(struct net_device *dev)
{
	if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &dev->state)) {
		netif_set_link_down(dev->netif);
	}
}
