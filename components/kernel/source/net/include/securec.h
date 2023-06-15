
#ifndef __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27
#define __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* define error code */
#if !defined(__STDC_WANT_LIB_EXT1__) || (defined(__STDC_WANT_LIB_EXT1__) && (__STDC_WANT_LIB_EXT1__ == 0))
#ifndef SECUREC_DEFINED_ERRNO_TYPE
#define SECUREC_DEFINED_ERRNO_TYPE
/* just check whether macrodefinition exists. */
#ifndef errno_t
typedef int errno_t;
#endif
#endif
#endif

/* success */
#ifndef EOK
#define EOK (0)
#endif

int snprintf_s(char *strDest, size_t destMax, size_t count, const char *format, ...);
errno_t memset_s(void *dest, size_t destMax, int c, size_t count);
errno_t memcpy_s(void *dest, size_t destMax, const void *src, size_t count);
errno_t strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27 */
