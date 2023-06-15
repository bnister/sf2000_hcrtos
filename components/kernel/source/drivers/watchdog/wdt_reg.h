#ifndef __WDT_REG_H__
#define __WDT_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_WDT_BASE		( REG_WDT_BASE )

/**
 bitpos: [31:0]
 */

#define R_WDT_COUNT		( __REG_WDT_BASE + 0x0 )


#define R_WDT_CONF		( __REG_WDT_BASE + 0x4 )

/**
 bitpos: [1:0]
 This field specifies the clock divisor for counting frequency.
 00: counting frequency = 27MHz frequency / 32
 01: counting frequency = 27MHz frequency / 64
 10: counting frequency = 27MHz frequency / 128
 11: counting frequency = 27MHz frequency / 256
 */
#define F_WDT_0004_CLKDIV 0x00000003
#define F_WDT_0004_CLKDIV_M 0x00000003
#define F_WDT_0004_CLKDIV_V 0x3
#define F_WDT_0004_CLKDIV_S 0
/**
 bitpos: [2]
 Timer enable
 */
#define F_WDT_0004_EN 0x00000004
#define F_WDT_0004_EN_M 0x00000004
#define F_WDT_0004_EN_V 0x1
#define F_WDT_0004_EN_S 2
/**
 bitpos: [3]
 Timer overflow status (RO), it is cleared when count is reloaded with value != 0xffffffff or ien = 0
 */
#define F_WDT_0004_OVERFLOW 0x00000008
#define F_WDT_0004_OVERFLOW_M 0x00000008
#define F_WDT_0004_OVERFLOW_V 0x1
#define F_WDT_0004_OVERFLOW_S 3
/**
 bitpos: [4]
 Timer interrupt enable
 */
#define F_WDT_0004_IEN 0x00000010
#define F_WDT_0004_IEN_M 0x00000010
#define F_WDT_0004_IEN_V 0x1
#define F_WDT_0004_IEN_S 4
/**
 bitpos: [5]
 Watchdog reset enable
 */
#define F_WDT_0004_WDTEN 0x00000020
#define F_WDT_0004_WDTEN_M 0x00000020
#define F_WDT_0004_WDTEN_V 0x1
#define F_WDT_0004_WDTEN_S 5

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __WDT_REG_H__ */
