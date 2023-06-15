#ifndef __WDT_REG_STRUCT_H__
#define __WDT_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct wdt_reg {
	uint32_t count; /* REG_WDT_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[1:0]]
			This field specifies the clock divisor for counting frequency.
			00: counting frequency = 27MHz frequency / 32
			01: counting frequency = 27MHz frequency / 64
			10: counting frequency = 27MHz frequency / 128
			11: counting frequency = 27MHz frequency / 256
			 */
			uint32_t clkdiv:			2;
			/**
			bitpos: [[2]]
			Timer enable
			 */
			uint32_t en:			1;
			/**
			bitpos: [[3]]
			Timer overflow status (RO), it is cleared when count is reloaded with value != 0xffffffff or ien = 0
			 */
			uint32_t overflow:			1;
			/**
			bitpos: [[4]]
			Timer interrupt enable
			 */
			uint32_t ien:			1;
			/**
			bitpos: [[5]]
			Watchdog reset enable
			 */
			uint32_t wdten:			1;
			uint32_t reserved6:			26;
		};
		uint32_t val;
	} conf;	/* REG_WDT_BASE + 0x4 */

} wdt_reg_t;


#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __WDT_REG_STRUCT_H__ */
