#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

typedef  signed char    int8_t;     /*  range :  -128 to 127               */
typedef  unsigned char  uint8_t;    /*  range :  0 to 255                  */
typedef  signed short   int16_t;    /*  range :  -32768 to 32767           */
typedef  unsigned short uint16_t;   /*  range :  0 to 65535                */
typedef  signed long    int32_t;    /*  range :  -2147483648 to 2147483647 */
typedef  unsigned long  uint32_t;   /*  range :  0 to 4294967295           */
typedef  signed long long    int64_t;    /*  range :  -2^63 to 2^63-1      */
typedef  unsigned long  long uint64_t;   /*  range :  0 to 2^64-1          */
typedef signed long intptr_t;
typedef unsigned long uintptr_t;
//typedef  unsigned long  size_t;   /*  range :  0 to 4294967295           */

typedef uint32_t fdt32_t;

#define EXTRACT_BYTE(n)	((unsigned long long)((uint8_t *)&x)[n])
static inline uint16_t fdt16_to_cpu(uint16_t x)
{
	return (EXTRACT_BYTE(0) << 8) | EXTRACT_BYTE(1);
}
#define cpu_to_fdt16(x) fdt16_to_cpu(x)

static inline uint32_t fdt32_to_cpu(uint32_t x)
{
	return (EXTRACT_BYTE(0) << 24) | (EXTRACT_BYTE(1) << 16) | (EXTRACT_BYTE(2) << 8) | EXTRACT_BYTE(3);
}
#define cpu_to_fdt32(x) fdt32_to_cpu(x)

static inline uint64_t fdt64_to_cpu(uint64_t x)
{
	return (EXTRACT_BYTE(0) << 56) | (EXTRACT_BYTE(1) << 48) | (EXTRACT_BYTE(2) << 40) | (EXTRACT_BYTE(3) << 32)
		| (EXTRACT_BYTE(4) << 24) | (EXTRACT_BYTE(5) << 16) | (EXTRACT_BYTE(6) << 8) | EXTRACT_BYTE(7);
}
#define cpu_to_fdt64(x) fdt64_to_cpu(x)
#undef EXTRACT_BYTE

#ifdef __cplusplus
}
#endif

#endif /* _LIBFDT_ENV_H */
