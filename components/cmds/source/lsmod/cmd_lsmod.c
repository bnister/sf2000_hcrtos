#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>

extern unsigned long _module_init_start;
extern unsigned long _module_init_core_start;
extern unsigned long _module_init_core_end;
extern unsigned long _module_init_postcore_start;
extern unsigned long _module_init_postcore_end;
extern unsigned long _module_init_arch_start;
extern unsigned long _module_init_arch_end;
extern unsigned long _module_init_system_start;
extern unsigned long _module_init_system_end;
extern unsigned long _module_init_driver_start;
extern unsigned long _module_init_driver_end;
extern unsigned long _module_init_driverlate_start;
extern unsigned long _module_init_driverlate_end;
extern unsigned long _module_init_others_start;
extern unsigned long _module_init_others_end;
extern unsigned long _module_init_end;

static void lsmod_range(struct mod_init *mod_start, struct mod_init *mod_end)
{
	struct mod_init *p;

	if (mod_start == mod_end) {
		printf("\t---none---\n");
		return;
	}

	for (p = mod_start; p < mod_end; p++) {
		printf("\t%s\n", p->name);
	}
}

static int lsmod(int argc, char **argv)
{
	struct mod_init *mod_start;
	struct mod_init *mod_end;
	struct mod_init *p;

	printf("core modules:\n");
	lsmod_range((struct mod_init *)&_module_init_core_start,
		    (struct mod_init *)&_module_init_core_end);
	printf("postcore modules:\n");
	lsmod_range((struct mod_init *)&_module_init_postcore_start,
		    (struct mod_init *)&_module_init_postcore_end);
	printf("arch modules:\n");
	lsmod_range((struct mod_init *)&_module_init_arch_start,
		    (struct mod_init *)&_module_init_arch_end);
	printf("system modules:\n");
	lsmod_range((struct mod_init *)&_module_init_system_start,
		    (struct mod_init *)&_module_init_system_end);
	printf("driver modules:\n");
	lsmod_range((struct mod_init *)&_module_init_driver_start,
		    (struct mod_init *)&_module_init_driver_end);
	printf("driver_late modules:\n");
	lsmod_range((struct mod_init *)&_module_init_driverlate_start,
		    (struct mod_init *)&_module_init_driverlate_end);
	printf("others modules:\n");
	lsmod_range((struct mod_init *)&_module_init_others_start,
		    (struct mod_init *)&_module_init_others_end);

	return 0;
}

CONSOLE_CMD(lsmod, NULL, lsmod, CONSOLE_CMD_MODE_SELF, "list init modules")
