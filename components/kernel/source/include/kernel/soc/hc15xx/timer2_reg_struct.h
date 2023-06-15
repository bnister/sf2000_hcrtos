#ifndef __TIMER2_REG_STRUCT_H__
#define __TIMER2_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct timer2_reg {

	union {
		struct {
			/**
			bitpos: [[31:0]]
			Timer counter
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} cnt;	/* REG_TIMER2_BASE + 0x0 */

	uint32_t reserved_4; /* REG_TIMER2_BASE + 0x4 */

	union {
		struct {
			/**
			bitpos: [[1:0]]
			Timer clkdiv，27M 工作时钟
			00:32 分频，每个tick约1.18微秒
			01:64 分频，每个tick约2.37微秒
			10:128 分频，每个tick约4.74微秒
			11:256分频，每个tick约9.48微秒
			 */
			uint32_t clkdiv:			2;
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
			Enable Timer overflow interrupt
			 */
			uint32_t int_en:			1;
			uint32_t reserved5:			27;
		};
		uint32_t val;
	} ctrl;	/* REG_TIMER2_BASE + 0x8 */

} timer2_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __TIMER2_REG_STRUCT_H__ */
