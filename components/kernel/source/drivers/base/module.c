#define LOG_TAG "MODULE"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <kernel/list.h>
#include <kernel/ld.h>
#include <kernel/vfs.h>
#include <kernel/module.h>
#include <kernel/elog.h>

int module_init2(const char *name, int n_exclude, char *excludes[])
{
	struct mod_init *mod_start = (struct mod_init *)&_module_init_start;
	struct mod_init *mod_end = (struct mod_init *)&_module_init_end;
	struct mod_init *p;
	int i, res;
	bool group_init = true;
	bool skip;

	if (!strcmp(name,"core")){
		log_d("init core\n");
		mod_start = (struct mod_init *)&_module_init_core_start;
		mod_end = (struct mod_init *)&_module_init_core_end;
	} else if (!strcmp(name,"postcore")){
		log_d("init postcore\n");
		mod_start = (struct mod_init *)&_module_init_postcore_start;
		mod_end = (struct mod_init *)&_module_init_postcore_end;
	} else if (!strcmp(name,"arch")){
		mod_start = (struct mod_init *)&_module_init_arch_start;
		mod_end = (struct mod_init *)&_module_init_arch_end;
	} else if (!strcmp(name,"system")){
		mod_start = (struct mod_init *)&_module_init_system_start;
		mod_end = (struct mod_init *)&_module_init_system_end;
	} else if (!strcmp(name,"driver")){
		mod_start = (struct mod_init *)&_module_init_driver_start;
		mod_end = (struct mod_init *)&_module_init_driver_end;
	} else if (!strcmp(name,"driver_late")){
		mod_start = (struct mod_init *)&_module_init_driverlate_start;
		mod_end = (struct mod_init *)&_module_init_driverlate_end;
	} else if (!strcmp(name,"others")){
		mod_start = (struct mod_init *)&_module_init_others_start;
		mod_end = (struct mod_init *)&_module_init_others_end;
	} else if (!strcmp(name, "all")) {
		log_d("init all\n");
		mod_start = (struct mod_init *)&_module_init_start;
		mod_end = (struct mod_init *)&_module_init_end;
	} else {
		group_init = false;
	}

	if (group_init) {
		for (p = mod_start; p < mod_end; p++) {
			skip = false;
			for (i = 0; i < n_exclude; i++) {
				if (!strcmp(p->name, excludes[i])) {
					skip = true;
					break;
				}
			}

			if (skip || p->initialized == true) {
				continue;
			}

			if (p->init) {
				log_d("module init %s\n", p->name);
				res = p->init();
				if (res) {
					log_w("    --> init %s failed.\n", p->name);
					return res;
				}
			}
			p->initialized = true;
		}

		return 0;
	}

	for (p = mod_start; p < mod_end; p++) {
		if (!strcmp(p->name, name)) {
			if (p->initialized == true) {
				return 0;
			}
			if (p->init) {
				log_w("    --> init %s ...\n", p->name);
				res = p->init();
				if (res) {
					log_e("    --> init %s failed.\n", p->name);
					return res;
				}
				log_w("    --> init %s done\n", p->name);
			}
			p->initialized = true;

			return 0;
		}
	}

	return -ENODEV;
}

int module_init(const char *name)
{
	return module_init2(name, 0, NULL);
}

int module_exit2(const char *name, int n_exclude, char *excludes[])
{
	struct mod_init *mod_start = (struct mod_init *)&_module_init_start;
	struct mod_init *mod_end = (struct mod_init *)&_module_init_end;
	struct mod_init *p;
	int i;
	bool skip;

	if (!strcmp(name, "all")) {
		for (p = mod_start; p < mod_end; p++) {
			skip = false;
			for (i = 0; i < n_exclude; i++) {
				if (!strcmp(p->name, excludes[i])) {
					skip = true;
					break;
				}
			}

			if (skip || p->initialized == false) {
				continue;
			}
			if (p->exit)
				p->exit();
			p->initialized = false;
		}

		return 0;
	}

	for (p = mod_start; p < mod_end; p++) {
		if (!strcmp(p->name, name)) {
			if (p->initialized == false) {
				return 0;
			}
			if (p->exit)
				p->exit();
			p->initialized = false;

			return 0;
		}
	}

	return -ENODEV;
}

int module_exit(const char *name)
{
	return module_exit2(name, 0, NULL);
}
