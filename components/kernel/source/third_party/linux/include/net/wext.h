#ifndef __NET_WEXT_H
#define __NET_WEXT_H

#include <net/iw_handler.h>

struct netif;

int wext_handle_ioctl(struct netif *netif, struct ifreq *ifr, unsigned int cmd,
		      void *arg);

struct iw_statistics *get_wireless_stats(struct net_device *dev);
int call_commit_handler(struct net_device *dev);

int ioctl_private_call(struct net_device *dev, struct iwreq *iwr,
		       unsigned int cmd, struct iw_request_info *info,
		       iw_handler handler);
int compat_private_call(struct net_device *dev, struct iwreq *iwr,
			unsigned int cmd, struct iw_request_info *info,
			iw_handler handler);
int iw_handler_get_private(struct net_device *		dev,
			   struct iw_request_info *	info,
			   union iwreq_data *		wrqu,
			   char *			extra);

#endif /* __NET_WEXT_H */
