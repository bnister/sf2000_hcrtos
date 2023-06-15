#ifndef _LINUX_NLS_H
#define _LINUX_NLS_H

#include <linux/types.h>


/* Arbitrary Unicode character */
typedef u32 unicode_t;

// typedef u16 wchar_t;
#include <stddef.h>

/* Byte order for UTF-16 strings */
enum utf16_endian {
	UTF16_HOST_ENDIAN,
	UTF16_LITTLE_ENDIAN,
	UTF16_BIG_ENDIAN
};

extern int utf16s_to_utf8s(const uint16_t *pwcs, int len,
		enum utf16_endian endian, u8 *s, int maxlen);
extern int utf32_to_utf8(unicode_t u, u8 *s, int maxout);
int utf16s_to_utf8s(const uint16_t *pwcs, int inlen, enum utf16_endian endian,
		u8 *s, int maxout);
extern int utf8s_to_utf16s(const u8 *s, int inlen, enum utf16_endian endian,
		uint16_t *pwcs, int maxout);

#endif /* _LINUX_NLS_H */
