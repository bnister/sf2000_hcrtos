#ifndef _HCUAPI_WATCHDOG_H_
#define _HCUAPI_WATCHDOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hcuapi/iocbase.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

#define WDIOC_START			_IO (WATCHDOG_IOCBASE, 0)
#define WDIOC_STOP			_IO (WATCHDOG_IOCBASE, 1)
#define WDIOC_KEEPALIVE			_IO (WATCHDOG_IOCBASE, 2)
#define WDIOC_SETMODE			_IO (WATCHDOG_IOCBASE, 3) //!< refer to WDT_MODE_*
#define WDIOC_SETTIMEOUT		_IO (WATCHDOG_IOCBASE, 4) //!< unit in us
#define WDIOC_GETTIMEOUT		_IOR(WATCHDOG_IOCBASE, 5, unsigned int) //!< unit in us
#define WDIOC_GETTIMELEFT		_IOR(WATCHDOG_IOCBASE, 6, unsigned int) //!< unit in us

#define WDIOC_NOTIFY_TIMEOUT		_IO (WATCHDOG_IOCBASE, 7) //!< notification when work in mode WDT_MODE_TIMER

#define WDT_MODE_WATCHDOG		(1 << 0) //!< default mode, reset chipset when timer overflow
#define WDT_MODE_TIMER			(1 << 1) //!< do not reset chipset when timer overflow

#ifdef __cplusplus
}
#endif

#endif
