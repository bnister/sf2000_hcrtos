#include <string.h>
#include <generated/br2_autoconf.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <stdlib.h>
#include <kernel/lib/libfdt/fdt.h>
#include <kernel/lib/libfdt/libfdt.h>

#define FDT_MAX_DEPTH 12
/* Location of FDT yet to be loaded. */
/* This may be in read-only memory, so can't be manipulated directly. */
/* Location of FDT on heap. */
/* This is the copy we actually manipulate. */
static struct fdt_header *fdtp = NULL;
/* Location of FDT in kernel or module. */
/* This won't be set if FDT is loaded from disk or memory. */
/* If it is set, we'll update it when fdt_copy() gets called. */

#define __swab32(x)                                                            \
	((u32)((((u32)(x) & (u32)0x000000ffUL) << 24) |                        \
	       (((u32)(x) & (u32)0x0000ff00UL) << 8) |                         \
	       (((u32)(x) & (u32)0x00ff0000UL) >> 8) |                         \
	       (((u32)(x) & (u32)0xff000000UL) >> 24)))

u32 fdt32_to_cpup(const u32 *p)
{
	return __swab32(*(u32 *)(p));
}

#ifdef CONFIG_OF_EMBED

#define STR2(m)  #m
#define STR(m) STR2(m)

__asm__ (
	".section .rodata\n"
	".balign  16\n"
	".globl   __dtb_start\n"
"__dtb_start:\n"
	".incbin "STR(CONFIG_DEFAULT_DEVICE_TREE)"\n"\
	\
	".balign 64\n"
	".globl   __dtb_end\n"
"__dtb_end:\n"
	".globl   __dtb_size\n"
"__dtb_size:\n"
	".word __dtb_end - __dtb_start\n");

extern const char __dtb_start;
extern const char __dtb_end;
extern const int  __dtb_size;
#endif

static void *get_fdt(void)
{
#ifdef CONFIG_OF_EMBED
	if (&__dtb_start != &__dtb_end)
		return (void *)&__dtb_start;
#elif defined(CONFIG_OF_SEPARATE)
	return (void *)(*(uint32_t *)((void *)&MSYSIO0 + 4));
#else
# error "No device tree provided!"
#endif
	return NULL;
}

void fdt_early_setup(void)
{
	void *dt;

	dt = get_fdt();
	if (!dt) {
		asm volatile("nop;.word 0x1000ffff;nop;");
	}

	fdtp = dt;
}

void fdt_setup(void)
{
	int size;
	void *dt;

	dt = get_fdt();
	if (!dt) {
		asm volatile("nop;.word 0x1000ffff;nop;");
	}

	fdtp = dt;
	(void)size;
#ifdef CONFIG_OF_SEPARATE
	size = fdt_totalsize(dt);
	fdtp = malloc(size);
	if (!fdtp)
		asm volatile("nop;.word 0x1000ffff;nop;");
	memcpy(fdtp, dt, size);
#endif
}

int fdt_get_property_u_32_array(int offset, const char *name, u32 *outval,
				int length)
{
	const u32 *val;

	if (offset < 0)
		return 1;

	val = fdt_getprop(fdtp, offset, name,
				     NULL); //* get the data of dts_buff.

	if (NULL == val)
		return 1;
	while (length--)
		*outval++ = fdt32_to_cpup((u32 *)val++);
	return 0;
}

int fdt_get_property_u_8_array(int offset, const char *name, u8 *outval,
			       int length)
{
	const u8 *val;

	if (offset < 0)
		return 1;

	val = fdt_getprop(fdtp, offset, name,
				    NULL); //* get the data of dts_buff.

	if (NULL == val)
		return 1;

	while (length--)
		*outval++ = *val++;
	return 0;
}

int fdt_get_property_u_32_index(int offset, const char *name, int index,
				u32 *outval)
{
	const u32 *val;

	if (offset < 0)
		return 1;

	val = fdt_getprop(fdtp, offset, name,
				     NULL); //* get the data of dts_buff.

	if (NULL == val)
		return 1;

	*outval = fdt32_to_cpu(*(val + index));
	return 0;
}

int fdt_get_property_string_index(int offset, const char *name, int index,
				  const char **outval)
{
	int i;
	int l = 0, total = 0;
	int len;
	const char *p;

	if (offset < 0)
		return 1;

	p = fdt_get_property_data_by_name(offset, name, &len);

	if (NULL == p)
		return 1;
	for (i = 0; total < len; total += l, p += l) {
		l = strlen(p) + 1;
		if (i++ == index) {
			*outval = p;
			return 0;
		}
	}
	return 1;
}

int fdt_property_count_strings(int offset, const char *name)
{
	int i;
	int l = 0, total = 0;
	int len;
	const char *p;

	if (offset < 0)
		return 0;

	p = fdt_get_property_data_by_name(offset, name, &len);

	if (NULL == p)
		return 0;
	for (i = 0; total < len; total += l, p += l) {
		l = strlen(p) + 1;
		i++;
	}

	return i;
}

bool fdt_property_read_bool(int offset, const char *propname)
{
	int length;

	if (offset < 0)
		return false;

	const struct fdt_property *prop =
		fdt_get_property(fdtp, offset, propname, &length);

	return prop ? true : false;
}

int of_device_is_compatible(int offset, const char *compat)
{
	int l = 0, total = 0;
	int len;
	const char *p;

	if (compat && compat[0]) {
		p = fdt_get_property_data_by_name(offset, "hcrtos-compatible", &len);
		if (NULL == p)
			return 0;

		for (; total < len; total += l, p += l) {
			l = strlen(p) + 1;
			if (strncasecmp(p, compat, strlen(compat)) == 0)
				return 1;
		}
	}

	return 0;
}

int fdt_property_read(int offset, const char *name, int *len)
{
	return (int)(fdt_property_read_bool(offset, name));
}

int fdt_get_node_offset_by_compatible(const char *compatible)
{
	int O = fdt_path_offset(fdtp, "/");

	O = fdt_node_offset_by_compatible(fdtp, O, compatible);

	return O;
}

int fdt_get_node_offset_by_path(const char *path)
{
	return fdt_path_offset(fdtp, path);
}

int fdt_node_probe_by_path(const char *path)
{
	int np;
	const char *status;

	np = fdt_get_node_offset_by_path(path);
	if (np < 0)
		return -1;

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		!strcmp(status, "disabled")) {
		return -1;
	}

	return np;
}

const void *fdt_get_property_data_by_name(int offset, const char *name,
					  int *length)
{
	const struct fdt_property *prop;

	if (offset < 0)
		return NULL;

	prop = fdt_get_property(fdtp, offset, name, length);
	if (NULL == prop)
		return NULL;

	return prop->data;
}

const char *fdt_get_property_alias(const char *name)
{
	return fdt_get_alias(fdtp, name);
}

int fdt_get_property_alias_id(int offset,const char *name)
{
	return fdt_get_alias_id(fdtp, name);
}

int fdt_get_node_offset_by_phandle(uint32_t phandle)
{
	return fdt_node_offset_by_phandle(fdtp, phandle);
}

static inline const char *kbasename(const char *path)
{
	const char *tail = strrchr(path, '/');
	return tail ? tail + 1 : path;
}

int of_scan_flat_dt(int (*it)(unsigned long node, const char *uname, int depth, void *data),
		    void *data)
{
	const void *blob = fdtp;
	const char *pathp;
	int offset, rc = 0, depth = -1;

	if (!blob)
		return 0;

	for (offset = fdt_next_node(blob, -1, &depth);
	     offset >= 0 && depth >= 0 && !rc;
	     offset = fdt_next_node(blob, offset, &depth)) {

		pathp = fdt_get_name(blob, offset, NULL);
		if (*pathp == '/')
			pathp = kbasename(pathp);
		rc = it(offset, pathp, depth, data);
	}
	return rc;
}

int fdt_parse_phandle(int offset, const char *phandle_name, int index)
{
	u32 np = 0;
	int handle = 0;

	if (!fdt_get_property_u_32_index(offset, phandle_name, index, &np)) {
		handle = fdt_get_node_offset_by_phandle(np);
	}

	if (handle > 0)
		return handle;
	else {
		/*
		 * The offset 0 is the begining of DTB
		 * Makesure the requested phandle is not the begining node of DTB
		 */
		return 0;
	}
}

int fdt_set_property_u32(void *fdt, int nodeoffset, const char *name,
				  uint32_t val)
{
	return fdt_setprop_u32(fdt, nodeoffset, name, val);
}
