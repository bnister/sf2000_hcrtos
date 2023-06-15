#ifndef __SMBOX_REG_H__
#define __SMBOX_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_SMBOX_BASE(i)		( REG_SMBOX##i##_BASE )


#define R_SMBOX_MBOX_INT_TRIG(i)		( __REG_SMBOX_BASE(i) + 0x0 )

/**
 bitpos: [0]
 Trig another cpu interrupt
 */
#define F_SMBOX_0000_INT_TRIG 0x00000001
#define F_SMBOX_0000_INT_TRIG_M 0x00000001
#define F_SMBOX_0000_INT_TRIG_V 0x1
#define F_SMBOX_0000_INT_TRIG_S 0

#define R_SMBOX_MBOX_INT_CLR(i)		( __REG_SMBOX_BASE(i) + 0x4 )

/**
 bitpos: [0]
 Clear another cpu interrupt
 */
#define F_SMBOX_0004_INT_CLR 0x00000001
#define F_SMBOX_0004_INT_CLR_M 0x00000001
#define F_SMBOX_0004_INT_CLR_V 0x1
#define F_SMBOX_0004_INT_CLR_S 0

#define R_SMBOX_MBOX_INT_ENST(i)		( __REG_SMBOX_BASE(i) + 0x8 )

/**
 bitpos: [0]
 Enable receive interrupt
 */
#define F_SMBOX_0008_INT_REC_EN 0x00000001
#define F_SMBOX_0008_INT_REC_EN_M 0x00000001
#define F_SMBOX_0008_INT_REC_EN_V 0x1
#define F_SMBOX_0008_INT_REC_EN_S 0
/**
 bitpos: [1]
 Enable callback interrupt
 */
#define F_SMBOX_0008_INT_CBK_EN 0x00000002
#define F_SMBOX_0008_INT_CBK_EN_M 0x00000002
#define F_SMBOX_0008_INT_CBK_EN_V 0x1
#define F_SMBOX_0008_INT_CBK_EN_S 1
/**
 bitpos: [8]
 Receive interrupt status
 */
#define F_SMBOX_0008_INT_REC_STA 0x00000100
#define F_SMBOX_0008_INT_REC_STA_M 0x00000100
#define F_SMBOX_0008_INT_REC_STA_V 0x1
#define F_SMBOX_0008_INT_REC_STA_S 8
/**
 bitpos: [16]
 Callback interrupt status
 */
#define F_SMBOX_0008_INT_CBK_STA 0x00010000
#define F_SMBOX_0008_INT_CBK_STA_M 0x00010000
#define F_SMBOX_0008_INT_CBK_STA_V 0x1
#define F_SMBOX_0008_INT_CBK_STA_S 16

#define R_SMBOX_MBOX_DATA0(i)		( __REG_SMBOX_BASE(i) + 0xC )

/**
 bitpos: [31:0]
 data descriptor register0
 */
#define F_SMBOX_000C_DATA 0xFFFFFFFF
#define F_SMBOX_000C_DATA_M 0xFFFFFFFF
#define F_SMBOX_000C_DATA_V 0xFFFFFFFF
#define F_SMBOX_000C_DATA_S 0

#define R_SMBOX_MBOX_DATA1(i)		( __REG_SMBOX_BASE(i) + 0x10 )

/**
 bitpos: [31:0]
 data descriptor register1
 */
#define F_SMBOX_0010_DATA 0xFFFFFFFF
#define F_SMBOX_0010_DATA_M 0xFFFFFFFF
#define F_SMBOX_0010_DATA_V 0xFFFFFFFF
#define F_SMBOX_0010_DATA_S 0

#define R_SMBOX_MBOX_DATA2(i)		( __REG_SMBOX_BASE(i) + 0x14 )

/**
 bitpos: [31:0]
 data descriptor register2
 */
#define F_SMBOX_0014_DATA 0xFFFFFFFF
#define F_SMBOX_0014_DATA_M 0xFFFFFFFF
#define F_SMBOX_0014_DATA_V 0xFFFFFFFF
#define F_SMBOX_0014_DATA_S 0

#define R_SMBOX_MBOX_DATA3(i)		( __REG_SMBOX_BASE(i) + 0x18 )

/**
 bitpos: [31:0]
 data descriptor register3
 */
#define F_SMBOX_0018_DATA 0xFFFFFFFF
#define F_SMBOX_0018_DATA_M 0xFFFFFFFF
#define F_SMBOX_0018_DATA_V 0xFFFFFFFF
#define F_SMBOX_0018_DATA_S 0

#define R_SMBOX_MBOX_DATA4(i)		( __REG_SMBOX_BASE(i) + 0x1C )

/**
 bitpos: [31:0]
 data descriptor register4
 */
#define F_SMBOX_001C_DATA 0xFFFFFFFF
#define F_SMBOX_001C_DATA_M 0xFFFFFFFF
#define F_SMBOX_001C_DATA_V 0xFFFFFFFF
#define F_SMBOX_001C_DATA_S 0

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __SMBOX_REG_H__ */
