#ifndef __TIMER3_REG_H__
#define __TIMER3_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_TIMER3_BASE		( REG_TIMER3_BASE )


#define R_TIMER3_CNT		( __REG_TIMER3_BASE + 0x0 )

/**
 bitpos: [31:0]
 Timer counter
 */
#define F_TIMER3_0000_DATA 0xFFFFFFFF
#define F_TIMER3_0000_DATA_M 0xFFFFFFFF
#define F_TIMER3_0000_DATA_V 0xFFFFFFFF
#define F_TIMER3_0000_DATA_S 0

#define R_TIMER3_CTRL		( __REG_TIMER3_BASE + 0x8 )

/**
 bitpos: [1:0]
 Timer clkdiv，27M 工作时钟
 00:32 分频，每个tick约1.18微秒
 01:64 分频，每个tick约2.37微秒
 10:128 分频，每个tick约4.74微秒
 11:256分频，每个tick约9.48微秒
 */
#define F_TIMER3_0008_CLKDIV 0x00000003
#define F_TIMER3_0008_CLKDIV_M 0x00000003
#define F_TIMER3_0008_CLKDIV_V 0x3
#define F_TIMER3_0008_CLKDIV_S 0
/**
 bitpos: [2]
 Enable Timer
 */
#define F_TIMER3_0008_EN 0x00000004
#define F_TIMER3_0008_EN_M 0x00000004
#define F_TIMER3_0008_EN_V 0x1
#define F_TIMER3_0008_EN_S 2
/**
 bitpos: [3]
 Timer overflow flag
 */
#define F_TIMER3_0008_OVERFLOW 0x00000008
#define F_TIMER3_0008_OVERFLOW_M 0x00000008
#define F_TIMER3_0008_OVERFLOW_V 0x1
#define F_TIMER3_0008_OVERFLOW_S 3
/**
 bitpos: [4]
 Enable Timer overflow interrupt
 */
#define F_TIMER3_0008_INT_EN 0x00000010
#define F_TIMER3_0008_INT_EN_M 0x00000010
#define F_TIMER3_0008_INT_EN_V 0x1
#define F_TIMER3_0008_INT_EN_S 4

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __TIMER3_REG_H__ */
