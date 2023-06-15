#ifndef _LINUX_UTSNAME_H
#define _LINUX_UTSNAME_H

#define __NEW_UTS_LEN 64

struct new_utsname {
	char *sysname;
	char *release;
};

struct uts_namespace {
	struct new_utsname name;
};
extern struct uts_namespace init_uts_ns;

static inline struct new_utsname *init_utsname(void)
{
	return &init_uts_ns.name;
}

#endif /* _LINUX_UTSNAME_H */
