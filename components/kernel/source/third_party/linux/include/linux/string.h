/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

#include <string.h>
#define kstrdup(a, b) strdup(a)
#define kmemdup(src, len, gfp)                                                 \
	({                                                                     \
		void *____p;                                                   \
		____p = malloc(len);                                           \
		if (____p)                                                     \
			memcpy(____p, src, len);                               \
		____p;                                                         \
	})

#endif /* _LINUX_STRING_H_ */
