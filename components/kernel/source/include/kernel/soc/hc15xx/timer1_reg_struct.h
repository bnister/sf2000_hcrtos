#ifndef __TIMER1_REG_STRUCT_H__
#define __TIMER1_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct timer1_reg {

	union {
		struct {
			/**
			bitpos: [[31:0]]
			Timer counter of second
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} aim_sec;	/* REG_TIMER1_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[9:0]]
			Timer counter of ms
			 */
			uint32_t ms:			10;
			uint32_t reserved10:			6;
			/**
			bitpos: [[31:16]]
			tick counts per ms
			 */
			uint32_t tick_per_ms:			16;
		};
		uint32_t val;
	} aim_ms;	/* REG_TIMER1_BASE + 0x4 */

	union {
		struct {
			uint32_t reserved0:			2;
			/**
			bitpos: [[2]]
			Enable timer
			 */
			uint32_t en:			1;
			/**
			bitpos: [[3]]
			Interrupt status of timer
			 */
			uint32_t int_st:			1;
			/**
			bitpos: [[4]]
			Interrupt enable
			 */
			uint32_t int_en:			1;
			uint32_t reserved5:			11;
			/**
			bitpos: [[25:16]]
			Timer current ms val
			 */
			uint32_t cur_ms:			10;
			uint32_t reserved26:			6;
		};
		uint32_t val;
	} ctrl;	/* REG_TIMER1_BASE + 0x8 */

	union {
		struct {
			/**
			bitpos: [[31:0]]
			Timer current sec val
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} cur_sec;	/* REG_TIMER1_BASE + 0xc */

} timer1_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __TIMER1_REG_STRUCT_H__ */
