#ifndef __FDT_API_H
#define __FDT_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <linux/types.h>

void fdt_early_setup(void);
void fdt_setup(void);
int fdt_get_property_u_32_array(int offset, const char *name, u32 *outval, int length);
int fdt_get_property_u_8_array(int offset, const char *name, u8 *outval, int length);
int fdt_get_property_u_32_index(int offset, const char *name, int index, u32 *outval);
int fdt_get_property_string_index(int offset, const char *name, int index,const char **outval);
int fdt_property_count_strings(int offset, const char *name);
int fdt_get_node_offset_by_compatible(const char *compatible);
const void *fdt_get_property_data_by_name(int offset, const char *name, int *length);
int fdt_get_node_offset_by_path(const char *path);
const char *fdt_get_property_alias(const char *name);
int of_scan_flat_dt(int (*it)(unsigned long node, const char *uname, int depth, void *data),
		    void *data);
int fdt_node_probe_by_path(const char *path);
bool fdt_property_read_bool(int offset,const char *propname);
int fdt_property_read(int offset, const char *name, int* len);
int fdt_get_property_alias_id(int offset,const char *name);
int fdt_get_node_offset_by_phandle(uint32_t phandle);
int fdt_parse_phandle(int offset, const char *phandle_name, int index);
int fdt_set_property_u32(void *fdt, int nodeoffset, const char *name,
				  uint32_t val);

#ifdef __cplusplus
}
#endif

#endif
