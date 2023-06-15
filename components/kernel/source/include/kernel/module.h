#ifndef _KERNEL_MODULE_H
#define _KERNEL_MODULE_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>

struct mod_init {
	const char *name;
	int  (*init)(void);
	int  (*exit)(void);
	bool initialized;
};

#define __define_initcall(n, f1, f2, layer, clayer, priority) \
static __attribute__((__used__)) __attribute__ ((section(".initcall." clayer #priority ".init"))) struct mod_init mod_init_##layer##_##n = { \
	.name = #n, \
	.init = f1, \
	.exit = f2, \
	.initialized = false \
};

#define module_core(n, f1, f2, priority)                                    \
	__define_initcall(n, f1, f2, core, "core", priority)
#define module_postcore(n, f1, f2, priority)                                \
	__define_initcall(n, f1, f2, postcore, "postcore", priority)
#define module_arch(n, f1, f2, priority)                                    \
	__define_initcall(n, f1, f2, arch, "arch", priority)
#define module_system(n, f1, f2, priority)                                  \
	__define_initcall(n, f1, f2, system, "system", priority)
#define module_driver(n, f1, f2, priority)                                  \
	__define_initcall(n, f1, f2, driver, "driver", priority)
#define module_driver_late(n, f1, f2, priority)                             \
	__define_initcall(n, f1, f2, driverlate, "driverlate", priority)
#define module_others(n, f1, f2, priority)                                  \
	__define_initcall(n, f1, f2, others, "others", priority)

#define __initcall(fn) module_others(fn, fn, NULL, 4)
#define __exitcall(fn) module_others(fn, NULL, fn, 4)

int module_init(const char *name);
int module_exit(const char *name);
int module_init2(const char *name, int n_exclude, char *excludes[]);
int module_exit2(const char *name, int n_exclude, char *excludes[]);

#ifdef __cplusplus
}
#endif

#endif
