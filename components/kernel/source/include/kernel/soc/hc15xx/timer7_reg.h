#ifndef __TIMER7_REG_H__
#define __TIMER7_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_TIMER7_BASE		( REG_TIMER7_BASE )


#define R_TIMER7_CNT		( __REG_TIMER7_BASE + 0x0 )

/**
 bitpos: [31:0]
 Timer counter
 */
#define F_TIMER7_0000_DATA 0xFFFFFFFF
#define F_TIMER7_0000_DATA_M 0xFFFFFFFF
#define F_TIMER7_0000_DATA_V 0xFFFFFFFF
#define F_TIMER7_0000_DATA_S 0

#define R_TIMER7_CTRL		( __REG_TIMER7_BASE + 0x8 )

/**
 bitpos: [2]
 Enable Timer
 */
#define F_TIMER7_0008_EN 0x00000004
#define F_TIMER7_0008_EN_M 0x00000004
#define F_TIMER7_0008_EN_V 0x1
#define F_TIMER7_0008_EN_S 2
/**
 bitpos: [3]
 Timer overflow flag
 */
#define F_TIMER7_0008_OVERFLOW 0x00000008
#define F_TIMER7_0008_OVERFLOW_M 0x00000008
#define F_TIMER7_0008_OVERFLOW_V 0x1
#define F_TIMER7_0008_OVERFLOW_S 3
/**
 bitpos: [4]
 Timer overflow interrupt enable
 */
#define F_TIMER7_0008_INT_EN 0x00000010
#define F_TIMER7_0008_INT_EN_M 0x00000010
#define F_TIMER7_0008_INT_EN_V 0x1
#define F_TIMER7_0008_INT_EN_S 4

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __TIMER7_REG_H__ */
