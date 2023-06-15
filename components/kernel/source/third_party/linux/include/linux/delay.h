#ifndef _LINUX_DELAY_H
#define _LINUX_DELAY_H

#include <unistd.h>
#include <freertos/FreeRTOS.h>

#if configTICK_RATE_HZ >= 1000
#define MAX_UDELAY_MS   1
#elif configTICK_RATE_HZ <= 200
#define MAX_UDELAY_MS   5
#else
#define MAX_UDELAY_MS   (1000 / configTICK_RATE_HZ)
#endif

extern void udelay(unsigned long us);

#define mdelay(n) (\
        (__builtin_constant_p(n) && (n)<=MAX_UDELAY_MS) ? udelay((n)*1000) : \
	        ({unsigned long __ms=(n); while (__ms--) udelay(1000);}))

extern void msleep(unsigned int msecs);
#define usleep_range(min, max) usleep(min)

#endif /* defined(_LINUX_DELAY_H) */
