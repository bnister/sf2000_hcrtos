#ifndef __ASM_BUG_H
#define __ASM_BUG_H

#include <assert.h>
#include <stdio.h>
#include <linux/compiler.h>
#include <linux/printk.h>

#define BUG() assert(0)
#define BUG_ON(C) assert(!(C))
#define __WARN()		pr_warn("WARNING: at %s:%d %pS()\n", __FILE__, __LINE__, __builtin_return_address(0));

#define WARN(condition, format...)                                             \
	({                                                                     \
		int __ret_warn_on = !!(condition);                             \
		if (unlikely(__ret_warn_on)) {                                 \
			__WARN();                                              \
			printf(format);                                        \
		}                                                              \
		unlikely(__ret_warn_on);                                       \
	})

#define WARN_ON(condition)                                                     \
	({                                                                     \
		int __ret_warn_on = !!(condition);                             \
		if (unlikely(__ret_warn_on))                                   \
			__WARN();                                              \
		unlikely(__ret_warn_on);                                       \
	})

#define WARN_ON_ONCE(condition)                                                \
	({                                                                     \
		static bool __warned;                                          \
		int __ret_warn_once = !!(condition);                           \
                                                                               \
		if (unlikely(__ret_warn_once))                                 \
			if (WARN_ON(!__warned))                                \
				__warned = true;                               \
		unlikely(__ret_warn_once);                                     \
	})

#define WARN_ONCE(condition, format...)                                        \
	({                                                                     \
		static bool __warned;                                          \
		int __ret_warn_once = !!(condition);                           \
                                                                               \
		if (unlikely(__ret_warn_once))                                 \
			if (WARN(!__warned, format))                           \
				__warned = true;                               \
		unlikely(__ret_warn_once);                                     \
	})

#endif /* __ASM_BUG_H */
