#ifndef _IO_H
#define _IO_H

#include "types.h"

#define ARRAY_SIZE(n) (sizeof(n)/sizeof(n[0]))

//Register Bits{{
#define BIT31		0x80000000
#define BIT30		0x40000000
#define BIT29		0x20000000
#define BIT28		0x10000000
#define BIT27		0x08000000
#define BIT26		0x04000000
#define BIT25		0x02000000
#define BIT24		0x01000000
#define BIT23		0x00800000
#define BIT22		0x00400000
#define BIT21		0x00200000
#define BIT20		0x00100000
#define BIT19		0x00080000
#define BIT18		0x00040000
#define BIT17		0x00020000
#define BIT16		0x00010000
#define BIT15		0x00008000
#define BIT14		0x00004000
#define BIT13		0x00002000
#define BIT12		0x00001000
#define BIT11		0x00000800
#define BIT10		0x00000400
#define BIT9		0x00000200
#define BIT8		0x00000100
#define BIT7		0x00000080
#define BIT6		0x00000040
#define BIT5		0x00000020
#define BIT4		0x00000010
#define BIT3		0x00000008
#define BIT2		0x00000004
#define BIT1		0x00000002
#define BIT0		0x00000001
//}}

//Registers Operation {{
#define MIPS_CACHED_ADDR(p) ((typeof(p))((((unsigned long)(p)) | 0x80000000) & 0x9FFFFFFF))
#define MIPS_UNCACHED_ADDR(p) ((typeof(p))(((unsigned long)(p)) | 0xA0000000))
#define PHY_ADDR(p) ((typeof(p))(((unsigned long)(p)) & 0x1FFFFFFF))

#ifndef __ASSEMBLER__
#define BIT(nr)				(1UL << (nr))
#else
#define BIT(nr)				(1 << (nr))
#endif

#ifndef __ASSEMBLER__
#define NBITS_V(n)			((1UL << (n)) - 1)
#else
#define NBITS_V(n)			((1 << (n)) - 1)
#endif

#define NBITS_M(s, n)			(NBITS_V(n) << s)

#ifndef __ASSEMBLER__

#define __ALIGN_KERNEL(x, a)            __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)             __ALIGN_KERNEL((x), (a))
#define ALIGN_DOWN(x, a)        __ALIGN_KERNEL((x) - ((a) - 1), (a))
#define __ALIGN_MASK(x, mask)   __ALIGN_KERNEL_MASK((x), (mask))
#define PTR_ALIGN(p, a)         ((typeof(p))ALIGN((unsigned long)(p), (a)))
#define PTR_ALIGN_DOWN(p, a)    ((typeof(p))ALIGN_DOWN((unsigned long)(p), (a)))
#define IS_ALIGNED(x, a)                (((x) & ((typeof(x))(a) - 1)) == 0)

#define REG32_WRITE_SYNC(_r, _v)                                               \
	({                                                                     \
		(*(volatile uint32_t *)(_r)) = (_v);                           \
		while ((*(volatile uint32_t *)(_r)) != (_v))                   \
			;                                                      \
	})

#define REG16_WRITE_SYNC(_r, _v)                                               \
	({                                                                     \
		(*(volatile uint16_t *)(_r)) = (_v);                           \
		while ((*(volatile uint16_t *)(_r)) != (_v))                   \
			;                                                      \
	})

#define REG8_WRITE_SYNC(_r, _v)                                                \
	({                                                                     \
		(*(volatile uint8_t *)(_r)) = (_v);                            \
		while ((*(volatile uint8_t *)(_r)) != (_v))                    \
			;                                                      \
	})

//write value to register
#define REG32_WRITE(_r, _v) ({                                                                                           \
            (*(volatile uint32_t *)(_r)) = (_v);                                                                       \
        })

#define REG16_WRITE(_r, _v) ({                                                                                           \
            (*(volatile uint16_t *)(_r)) = (_v);                                                                       \
        })

#define REG8_WRITE(_r, _v) ({                                                                                           \
            (*(volatile uint8_t *)(_r)) = (_v);                                                                       \
        })

//read value from register
#define REG32_READ(_r) ({                                                                                                \
            (*(volatile uint32_t *)(_r));                                                                              \
        })

#define REG16_READ(_r) ({                                                                                                \
            (*(volatile uint16_t *)(_r));                                                                              \
        })

#define REG8_READ(_r) ({                                                                                                \
            (*(volatile uint8_t *)(_r));                                                                              \
        })

//get bit or get bits from register
#define REG32_GET_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint32_t*)(_r) & (_b));                                                                        \
        })

#define REG16_GET_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint16_t*)(_r) & (_b));                                                                        \
        })

#define REG8_GET_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint8_t*)(_r) & (_b));                                                                        \
        })

//set bit or set bits to register
#define REG32_SET_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint32_t*)(_r) |= (_b));                                                                       \
        })

#define REG16_SET_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint16_t*)(_r) |= (_b));                                                                       \
        })

#define REG8_SET_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint8_t*)(_r) |= (_b));                                                                       \
        })

//clear bit or clear bits of register
#define REG32_CLR_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint32_t*)(_r) &= ~(_b));                                                                      \
        })

#define REG16_CLR_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint16_t*)(_r) &= ~(_b));                                                                      \
        })

#define REG8_CLR_BIT(_r, _b)  ({                                                                                        \
            (*(volatile uint8_t*)(_r) &= ~(_b));                                                                      \
        })

//set bits of register controlled by mask
#define REG32_SET_BITS(_r, _b, _m) ({                                                                                    \
            (*(volatile uint32_t*)(_r) = (*(volatile uint32_t*)(_r) & ~(_m)) | ((_b) & (_m)));                         \
        })

#define REG16_SET_BITS(_r, _b, _m) ({                                                                                    \
            (*(volatile uint16_t*)(_r) = (*(volatile uint16_t*)(_r) & ~(_m)) | ((_b) & (_m)));                         \
        })

#define REG8_SET_BITS(_r, _b, _m) ({                                                                                    \
            (*(volatile uint8_t*)(_r) = (*(volatile uint8_t*)(_r) & ~(_m)) | ((_b) & (_m)));                         \
        })

//get field from register, uses field _S & _V to determine mask
#define REG32_GET_FIELD(_r, _f) ({                                                                                       \
            ((REG32_READ(_r) >> (_f##_S)) & (_f##_V));                                                                   \
        })

#define REG16_GET_FIELD(_r, _f) ({                                                                                       \
            ((REG16_READ(_r) >> (_f##_S)) & (_f##_V));                                                                   \
        })

#define REG8_GET_FIELD(_r, _f) ({                                                                                       \
            ((REG8_READ(_r) >> (_f##_S)) & (_f##_V));                                                                   \
        })

//set field of a register from variable, uses field _S & _V to determine mask
#define REG32_SET_FIELD(_r, _f, _v) ({                                                                                   \
            (REG32_WRITE((_r),((REG32_READ(_r) & ~(_f))|(((_v) & (_f##_V))<<(_f##_S)))));                \
        })

#define REG16_SET_FIELD(_r, _f, _v) ({                                                                                   \
            (REG16_WRITE((_r),((REG16_READ(_r) & ~(_f))|(((_v) & (_f##_V))<<(_f##_S)))));                \
        })

#define REG8_SET_FIELD(_r, _f, _v) ({                                                                                   \
            (REG8_WRITE((_r),((REG8_READ(_r) & ~(_f))|(((_v) & (_f##_V))<<(_f##_S)))));                \
        })

#define REG32_SET_FIELD2(_r, _shift, _nbits, _v) ({ \
		(REG32_WRITE((_r), ((REG32_READ(_r) & ~(NBITS_M((_shift), (_nbits)))) | (((_v) & (NBITS_V((_nbits))))<<(_shift))))); \
	})

#define REG16_SET_FIELD2(_r, _shift, _nbits, _v) ({ \
		(REG16_WRITE((_r), ((REG16_READ(_r) & ~(NBITS_M((_shift), (_nbits)))) | (((_v) & (NBITS_V((_nbits))))<<(_shift))))); \
	})

#define REG8_SET_FIELD2(_r, _shift, _nbits, _v) ({ \
		(REG8_WRITE((_r), ((REG8_READ(_r) & ~(NBITS_M((_shift), (_nbits)))) | (((_v) & (NBITS_V((_nbits))))<<(_shift))))); \
	})

#define REG32_GET_FIELD2(_r, _shift, _nbits) ({ \
		((REG32_READ(_r) & (NBITS_M((_shift), (_nbits)))) >> (_shift)); \
	})

#define REG16_GET_FIELD2(_r, _shift, _nbits) ({ \
		((REG16_READ(_r) & (NBITS_M((_shift), (_nbits)))) >> (_shift)); \
	})

#define REG8_GET_FIELD2(_r, _shift, _nbits) ({ \
		((REG8_READ(_r) & (NBITS_M((_shift), (_nbits)))) >> (_shift)); \
	})


//get field value from a variable
#define VALUE_GET_FIELD(_r, _f) (((_r) >> (_f##_S)) & (_f##_V))

//set field value to a variable
#define VALUE_SET_FIELD(_r, _f, _v) ((_r)=(((_r) & ~(_f))|((_v)<<(_f##_S))))

//generate a value from a field value
#define FIELD_TO_VALUE(_f, _v) (((_v)<<_f##_S) & (_f))

#define VALUE_GET_BITS(_r, _shift, _nbits) (((_r) >> (_shift)) & (NBITS_V(_nbits)))
#define VALUE_SET_BITS(_r, _shift, _nbits, _v) (((_r) & ~(NBITS_V(_nbits) << (_shift))) | (((_v) & NBITS_V(_nbits)) << (_shift)))

static inline void REG32_READ_REP(void *addr, uint32_t *dst, int count)
{
	while (--count >= 0) {
		uint32_t data = REG32_READ(addr);
		*dst = data;
		dst++;
	}
}

static inline void REG16_READ_REP(void *addr, uint16_t *dst, int count)
{
	while (--count >= 0) {
		uint16_t data = REG16_READ(addr);
		*dst = data;
		dst++;
	}
}

static inline void REG8_READ_REP(void *addr, uint8_t *dst, int count)
{
	while (--count >= 0) {
		uint8_t data = REG8_READ(addr);
		*dst = data;
		dst++;
	}
}

static inline void REG8_WRITE_REP(void *addr, const uint8_t *src, int count)
{
	while (--count >= 0) {
		REG8_WRITE(addr, *src);
		src++;
	}
}

static inline void REG16_WRITE_REP(void *addr, const uint16_t *src, int count)
{
	while (--count >= 0) {
		REG16_WRITE(addr, *src);
		src++;
	}
}

static inline void REG32_WRITE_REP(void *addr, const uint32_t *src, int count)
{
	while (--count >= 0) {
		REG32_WRITE(addr, *src);
		src++;
	}
}

#define __GCC_VERSION__ (__GNUC__ * 10000		\
		     + __GNUC_MINOR__ * 100	\
		     + __GNUC_PATCHLEVEL__)

#if __GCC_VERSION__ >= 40400
#define ___HAVE_BUILTIN_BSWAP32___
#define ___HAVE_BUILTIN_BSWAP64___
#endif

#if __GCC_VERSION__ >= 40800
#define ___HAVE_BUILTIN_BSWAP16___
#endif

#define ___CONSTANT_SWAB16(x) ((uint16_t)(				\
	(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |			\
	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))

#define ___CONSTANT_SWAB32(x) ((uint32_t)(				\
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |		\
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |		\
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |		\
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

static inline __attribute__((__const__)) uint16_t __FSWAB16(uint16_t val)
{
#ifdef ___HAVE_BUILTIN_BSWAP16___
	return __builtin_bswap16(val);
#else
	return ___CONSTANT_SWAB16(val);
#endif
}

static inline __attribute__((__const__)) uint32_t __FSWAB32(uint32_t val)
{
#ifdef ___HAVE_BUILTIN_BSWAP32___
	return __builtin_bswap32(val);
#else
	return ___CONSTANT_SWAB32(val);
#endif
}

#define __SWAB16(x)				\
	(__builtin_constant_p((uint16_t)(x)) ?	\
	___CONSTANT_SWAB16(x) :			\
	__FSWAB16(x))

#define __SWAB32(x)				\
	(__builtin_constant_p((uint32_t)(x)) ?	\
	___CONSTANT_SWAB32(x) :			\
	__FSWAB32(x))

#endif /* !__ASSEMBLER__ */
//}}

#endif /* _IO_H */
