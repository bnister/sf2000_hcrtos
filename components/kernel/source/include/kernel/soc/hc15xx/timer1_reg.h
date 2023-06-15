#ifndef __TIMER1_REG_H__
#define __TIMER1_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_TIMER1_BASE		( REG_TIMER1_BASE )


#define R_TIMER1_AIM_SEC		( __REG_TIMER1_BASE + 0x0 )

/**
 bitpos: [31:0]
 Timer counter of second
 */
#define F_TIMER1_0000_DATA 0xFFFFFFFF
#define F_TIMER1_0000_DATA_M 0xFFFFFFFF
#define F_TIMER1_0000_DATA_V 0xFFFFFFFF
#define F_TIMER1_0000_DATA_S 0

#define R_TIMER1_AIM_MS		( __REG_TIMER1_BASE + 0x4 )

/**
 bitpos: [9:0]
 Timer counter of ms
 */
#define F_TIMER1_0004_MS 0x000003FF
#define F_TIMER1_0004_MS_M 0x000003FF
#define F_TIMER1_0004_MS_V 0x3FF
#define F_TIMER1_0004_MS_S 0
/**
 bitpos: [31:16]
 tick counts per ms
 */
#define F_TIMER1_0004_TICK_PER_MS 0xFFFF0000
#define F_TIMER1_0004_TICK_PER_MS_M 0xFFFF0000
#define F_TIMER1_0004_TICK_PER_MS_V 0xFFFF
#define F_TIMER1_0004_TICK_PER_MS_S 16

#define R_TIMER1_CTRL		( __REG_TIMER1_BASE + 0x8 )

/**
 bitpos: [2]
 Enable timer
 */
#define F_TIMER1_0008_EN 0x00000004
#define F_TIMER1_0008_EN_M 0x00000004
#define F_TIMER1_0008_EN_V 0x1
#define F_TIMER1_0008_EN_S 2
/**
 bitpos: [3]
 Interrupt status of timer
 */
#define F_TIMER1_0008_INT_ST 0x00000008
#define F_TIMER1_0008_INT_ST_M 0x00000008
#define F_TIMER1_0008_INT_ST_V 0x1
#define F_TIMER1_0008_INT_ST_S 3
/**
 bitpos: [4]
 Interrupt enable
 */
#define F_TIMER1_0008_INT_EN 0x00000010
#define F_TIMER1_0008_INT_EN_M 0x00000010
#define F_TIMER1_0008_INT_EN_V 0x1
#define F_TIMER1_0008_INT_EN_S 4
/**
 bitpos: [25:16]
 Timer current ms val
 */
#define F_TIMER1_0008_CUR_MS 0x03FF0000
#define F_TIMER1_0008_CUR_MS_M 0x03FF0000
#define F_TIMER1_0008_CUR_MS_V 0x3FF
#define F_TIMER1_0008_CUR_MS_S 16

#define R_TIMER1_CUR_SEC		( __REG_TIMER1_BASE + 0xC )

/**
 bitpos: [31:0]
 Timer current sec val
 */
#define F_TIMER1_000C_DATA 0xFFFFFFFF
#define F_TIMER1_000C_DATA_M 0xFFFFFFFF
#define F_TIMER1_000C_DATA_V 0xFFFFFFFF
#define F_TIMER1_000C_DATA_S 0

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __TIMER1_REG_H__ */
