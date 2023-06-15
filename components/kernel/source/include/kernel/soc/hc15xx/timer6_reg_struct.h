#ifndef __TIMER6_REG_STRUCT_H__
#define __TIMER6_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct timer6_reg {

	union {
		struct {
			/**
			bitpos: [[31:0]]
			Timer counter
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} cnt;	/* REG_TIMER6_BASE + 0x0 */

	uint32_t reserved_4; /* REG_TIMER6_BASE + 0x4 */

	union {
		struct {
			uint32_t reserved0:			2;
			/**
			bitpos: [[2]]
			Enable Timer
			 */
			uint32_t en:			1;
			/**
			bitpos: [[3]]
			Timer overflow flag
			 */
			uint32_t overflow:			1;
			/**
			bitpos: [[4]]
			Timer overflow interrupt enable
			 */
			uint32_t int_en:			1;
			uint32_t reserved5:			27;
		};
		uint32_t val;
	} ctrl;	/* REG_TIMER6_BASE + 0x8 */

} timer6_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __TIMER6_REG_STRUCT_H__ */
