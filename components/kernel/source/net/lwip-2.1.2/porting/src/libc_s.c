#include <stdarg.h>
#include <securec.h>

int snprintf_s(char *strDest, size_t destMax, size_t count, const char *format, ...)
{
    int ret;                    /* If initialization causes  e838 */
    va_list argList;

    va_start(argList, format);
    ret = vsnprintf(strDest, count, format, argList);
    va_end(argList);
    (void)argList;              /* To clear e438 last value assigned not used , the compiler will optimize this code */

    return ret;
}

errno_t memset_s(void *dest, size_t destMax, int c, size_t count)
{
        (void)memset(dest, c, count);
        return EOK;
}

errno_t memcpy_s(void *dest, size_t destMax, const void *src, size_t count)
{
        (void)memcpy(dest, src, count);
        return EOK;
}

errno_t strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count)
{
	(void)strncpy(strDest, strSrc, count);
	return EOK;
}
