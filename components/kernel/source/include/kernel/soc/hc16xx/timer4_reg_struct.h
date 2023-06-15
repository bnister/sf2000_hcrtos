#ifndef __TIMER4_REG_STRUCT_H__
#define __TIMER4_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct timer4_reg {

	union {
		struct {
			/**
			bitpos: [[31:0]]
			Timer counter
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} cnt;	/* REG_TIMER4_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[31:0]]
			Timer target count
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} aim;	/* REG_TIMER4_BASE + 0x4 */

	union {
		struct {
			/**
			bitpos: [[1:0]]
			Timer clkdiv，27M 工作时钟
			00:16 分频，每个tick约593纳秒
			01:32 分频，每个tick约1.18微秒
			10:64 分频，每个tick约2.37微秒
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
			Timer interrupt status
			 */
			uint32_t int_st:			1;
			/**
			bitpos: [[4]]
			Timer interrupt enable
			 */
			uint32_t int_en:			1;
			uint32_t reserved5:			27;
		};
		uint32_t val;
	} ctrl;	/* REG_TIMER4_BASE + 0x8 */

} timer4_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __TIMER4_REG_STRUCT_H__ */
