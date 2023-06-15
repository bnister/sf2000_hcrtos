#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <kernel/io.h>
#include <linux/completion.h>
#include <stdlib.h>

# define might_sleep() do { } while (0)
void complete_and_exit(struct completion *, long);
#define simple_strtoul(cp, endp, base) strtoul(cp, endp, base)

#endif
