#ifndef __LINUX_NETLINK_H
#define __LINUX_NETLINK_H

#include <asm-generic/page.h>
#include <linux/skbuff.h>
#include <uapi/linux/netlink.h>

#define NLMSG_GOODSIZE	SKB_WITH_OVERHEAD(PAGE_SIZE)
#define NLMSG_DEFAULT_SIZE (NLMSG_GOODSIZE - NLMSG_HDRLEN)

struct nlmsghdr *
__nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq, int type, int len, int flags);

#endif	/* __LINUX_NETLINK_H */
