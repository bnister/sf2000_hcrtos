#ifndef _LINUX_BYTEORDER_LITTLE_ENDIAN_H
#define _LINUX_BYTEORDER_LITTLE_ENDIAN_H

#include <linux/types.h>

#define __le32_to_cpu(x) ((u32)(u32)(x))
#define __le32_to_cpus(x) do { (void)(x); } while (0)

#define __le16_to_cpu(x) ((u16)(u16)(x))
#define __le16_to_cpus(x) do { (void)(x); } while (0)

#define __cpu_to_le16(x) ((u16)(u16)(x))
#define __cpu_to_le32(x) ((u32)(u32)(x))
#define __cpu_to_be16(x) ((u16)__swab16((x)))
#define __cpu_to_be32(x) ((u32)__swab32((x)))
#define __cpu_to_le64(x) ((__le64)(__u64)(x))
#define __le64_to_cpu(x) ((__u64)(__le64)(x))

#define ___constant_swab16(x) ((__u16)(				\
	(((__u16)(x) & (__u16)0x00ffU) << 8) |			\
	(((__u16)(x) & (__u16)0xff00U) >> 8)))

#define ___constant_swab32(x) ((__u32)(				\
	(((__u32)(x) & (__u32)0x000000ffUL) << 24) |		\
	(((__u32)(x) & (__u32)0x0000ff00UL) <<  8) |		\
	(((__u32)(x) & (__u32)0x00ff0000UL) >>  8) |		\
	(((__u32)(x) & (__u32)0xff000000UL) >> 24)))

#define __swab16(x) ___constant_swab16(x)
#define __swab32(x) ___constant_swab32(x)

static inline __attribute_const__ __u16 __fswab16(__u16 val)
{
	return ___constant_swab16(val);
}

static inline __u16 __swab16p(const __u16 *p)
{
	return __swab16(*p);
}
static inline __u32 __swab32p(const __u32 *p)
{
	return __swab32(*p);
}

static inline __u16 __be16_to_cpup(const __be16 *p)
{
	return __swab16p((__u16 *)p);
}
static inline __u32 __be32_to_cpup(const __be32 *p)
{
	return __swab32p((__u32 *)p);
}

#define __be16_to_cpu(x) __swab16((x))
#define __be32_to_cpu(x) __swab32((x))
#define __le16_to_cpup(x) (*(u16 *)(x))
#define __le32_to_cpup(x) (*(u32 *)(x))

#endif /* _LINUX_BYTEORDER_LITTLE_ENDIAN_H */
