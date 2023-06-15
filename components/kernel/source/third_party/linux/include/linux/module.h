#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

#include <stdbool.h>
#include <linux/export.h>

#define MODULE_DESCRIPTION(...)
#define MODULE_AUTHOR(...)
#define MODULE_LICENSE(...)
#define MODULE_ALIAS(...)
#define module_param(...);
#define MODULE_PARM_DESC(...);
#define MODULE_VERSION(...)

#define module_param_array(name, type, nump, perm)

struct module {
};

static inline bool module_requested_async_probing(struct module *module)
{
	return false;
}

#define module_init __module_init_discard
#define module_exit __module_exit_discard
#include <kernel/module.h>
#undef module_init
#undef module_exit

#define module_init(x)  __initcall(x);
#define module_exit(x)                                                         \
	static int ___##x(void)                                                 \
	{                                                                      \
		x();                                                           \
		return 0;                                                      \
	}                                                                      \
	__exitcall(___##x);

#endif /* _LINUX_MODULE_H */
