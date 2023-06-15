#ifndef _NEWLIB_GLUE_INTERNAL_H_
#define _NEWLIB_GLUE_INTERNAL_H_
#include <sys/time.h>

void platform_settime(time_t t, long us);

void platform_gettime(time_t *sec, long *usec);

#endif /* _NEWLIB_GLUE_INTERNAL_H_ */
