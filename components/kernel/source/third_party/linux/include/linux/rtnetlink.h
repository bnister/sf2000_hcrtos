#ifndef __LINUX_RTNETLINK_H
#define __LINUX_RTNETLINK_H

#include <uapi/linux/rtnetlink.h>

static inline int rtnl_is_locked(void)
{
	return 0;
}
#define rtnl_lock() do {} while (0)
#define rtnl_unlock() do {} while (0)

#endif	/* __LINUX_RTNETLINK_H */
