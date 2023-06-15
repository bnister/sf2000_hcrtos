/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_BYTEORDER_GENERIC_H
#define _LINUX_BYTEORDER_GENERIC_H

#include <linux/types.h>
#include <linux/byteorder/little_endian.h>

#define le32_to_cpu __le32_to_cpu
#define le16_to_cpu __le16_to_cpu
#define be32_to_cpu __be32_to_cpu
#define cpu_to_le16 __cpu_to_le16
#define cpu_to_le32 __cpu_to_le32
#define cpu_to_be16 __cpu_to_be16
#define cpu_to_be32 __cpu_to_be32
#define le64_to_cpu __le64_to_cpu
#define be16_to_cpu __be16_to_cpu
#define cpu_to_le64 __cpu_to_le64

#define le16_to_cpus __le16_to_cpus

#define le16_to_cpup __le16_to_cpup
#define le32_to_cpup __le32_to_cpup

#define be32_to_cpup __be32_to_cpup

#undef ntohl
#undef ntohs
#undef htonl
#undef htons

#define ___htonl(x) __cpu_to_be32(x)
#define ___htons(x) __cpu_to_be16(x)
#define ___ntohl(x) __be32_to_cpu(x)
#define ___ntohs(x) __be16_to_cpu(x)

#define htonl(x) ___htonl(x)
#define ntohl(x) ___ntohl(x)
#define htons(x) ___htons(x)
#define ntohs(x) ___ntohs(x)

static inline void le32_to_cpu_array(u32 *buf, unsigned int words)
{
	while (words--) {
		__le32_to_cpus(buf);
		buf++;
	}
}

static inline void le16_add_cpu(__le16 *var, u16 val)
{
	*var = cpu_to_le16(le16_to_cpu(*var) + val);
}

static inline void le32_add_cpu(__le32 *var, u32 val)
{
	*var = cpu_to_le32(le32_to_cpu(*var) + val);
}

#endif /* _LINUX_BYTEORDER_GENERIC_H */
