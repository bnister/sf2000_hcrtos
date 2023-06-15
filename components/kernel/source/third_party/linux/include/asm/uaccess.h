#ifndef _ASM_UACCESS_H
#define _ASM_UACCESS_H

#define access_ok(type, addr, size) 1
#define copy_to_user(dest, src, len)                                           \
	({                                                                     \
		long __cu_len;                                                 \
		__cu_len = (0);                                                \
		memcpy(dest, src, len);                                        \
		__cu_len;                                                      \
	})

#define copy_from_user(dest, src, len)                                         \
	({                                                                     \
		long __cu_len;                                                 \
		__cu_len = (0);                                                \
		memcpy(dest, src, len);                                        \
		__cu_len;                                                      \
	})

#endif /* _ASM_UACCESS_H */
