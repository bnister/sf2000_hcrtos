#ifndef __I2C_REG_H__
#define __I2C_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_I2C_BASE(i)		( REG_I2C##i##_BASE )


#define R_I2C_HCR(i)		( __REG_I2C_BASE(i) + 0x0 )

/**
 bitpos: [0]
 */
#define F_I2C_0000_ST 0x01
#define F_I2C_0000_ST_M 0x01
#define F_I2C_0000_ST_V 0x1
#define F_I2C_0000_ST_S 0
/**
 bitpos: [3:1]
 */
#define F_I2C_0000_CP 0x0E
#define F_I2C_0000_CP_M 0x0E
#define F_I2C_0000_CP_V 0x7
#define F_I2C_0000_CP_S 1
/**
 bitpos: [4]
 */
#define F_I2C_0000_SR_EN 0x10
#define F_I2C_0000_SR_EN_M 0x10
#define F_I2C_0000_SR_EN_V 0x1
#define F_I2C_0000_SR_EN_S 4
/**
 bitpos: [5]
 */
#define F_I2C_0000_EDDC_EN 0x20
#define F_I2C_0000_EDDC_EN_M 0x20
#define F_I2C_0000_EDDC_EN_V 0x1
#define F_I2C_0000_EDDC_EN_S 5
/**
 bitpos: [6]
 */
#define F_I2C_0000_DEV_NOT_EXIST_EN 0x40
#define F_I2C_0000_DEV_NOT_EXIST_EN_M 0x40
#define F_I2C_0000_DEV_NOT_EXIST_EN_V 0x1
#define F_I2C_0000_DEV_NOT_EXIST_EN_S 6
/**
 bitpos: [7]
 */
#define F_I2C_0000_HOST_CTRL_EN 0x80
#define F_I2C_0000_HOST_CTRL_EN_M 0x80
#define F_I2C_0000_HOST_CTRL_EN_V 0x1
#define F_I2C_0000_HOST_CTRL_EN_S 7

#define R_I2C_HSR(i)		( __REG_I2C_BASE(i) + 0x1 )

/**
 bitpos: [0]
 */
#define F_I2C_0001_FIFO_EMPTY 0x01
#define F_I2C_0001_FIFO_EMPTY_M 0x01
#define F_I2C_0001_FIFO_EMPTY_V 0x1
#define F_I2C_0001_FIFO_EMPTY_S 0
/**
 bitpos: [1]
 */
#define F_I2C_0001_FIFO_FULL 0x02
#define F_I2C_0001_FIFO_FULL_M 0x02
#define F_I2C_0001_FIFO_FULL_V 0x1
#define F_I2C_0001_FIFO_FULL_S 1
/**
 bitpos: [2]
 */
#define F_I2C_0001_FIFO_UNDERRUN 0x04
#define F_I2C_0001_FIFO_UNDERRUN_M 0x04
#define F_I2C_0001_FIFO_UNDERRUN_V 0x1
#define F_I2C_0001_FIFO_UNDERRUN_S 2
/**
 bitpos: [3]
 */
#define F_I2C_0001_FIFO_OVERRUN 0x08
#define F_I2C_0001_FIFO_OVERRUN_M 0x08
#define F_I2C_0001_FIFO_OVERRUN_V 0x1
#define F_I2C_0001_FIFO_OVERRUN_S 3
/**
 bitpos: [4]
 */
#define F_I2C_0001_FIFO_ERROR 0x10
#define F_I2C_0001_FIFO_ERROR_M 0x10
#define F_I2C_0001_FIFO_ERROR_V 0x1
#define F_I2C_0001_FIFO_ERROR_S 4
/**
 bitpos: [5]
 */
#define F_I2C_0001_HOST_BUSY 0x20
#define F_I2C_0001_HOST_BUSY_M 0x20
#define F_I2C_0001_HOST_BUSY_V 0x1
#define F_I2C_0001_HOST_BUSY_S 5
/**
 bitpos: [6]
 */
#define F_I2C_0001_DEVICE_NOT_EXIST 0x40
#define F_I2C_0001_DEVICE_NOT_EXIST_M 0x40
#define F_I2C_0001_DEVICE_NOT_EXIST_V 0x1
#define F_I2C_0001_DEVICE_NOT_EXIST_S 6
/**
 bitpos: [7]
 */
#define F_I2C_0001_DEVICE_BUSY 0x80
#define F_I2C_0001_DEVICE_BUSY_M 0x80
#define F_I2C_0001_DEVICE_BUSY_V 0x1
#define F_I2C_0001_DEVICE_BUSY_S 7

#define R_I2C_IER(i)		( __REG_I2C_BASE(i) + 0x2 )

/**
 bitpos: [0]
 */
#define F_I2C_0002_TRANS_DONE_INT_EN 0x01
#define F_I2C_0002_TRANS_DONE_INT_EN_M 0x01
#define F_I2C_0002_TRANS_DONE_INT_EN_V 0x1
#define F_I2C_0002_TRANS_DONE_INT_EN_S 0
/**
 bitpos: [1]
 */
#define F_I2C_0002_DEV_NOT_EXIST_INT_EN 0x02
#define F_I2C_0002_DEV_NOT_EXIST_INT_EN_M 0x02
#define F_I2C_0002_DEV_NOT_EXIST_INT_EN_V 0x1
#define F_I2C_0002_DEV_NOT_EXIST_INT_EN_S 1
/**
 bitpos: [2]
 */
#define F_I2C_0002_DEV_BUSY_INT_EN 0x04
#define F_I2C_0002_DEV_BUSY_INT_EN_M 0x04
#define F_I2C_0002_DEV_BUSY_INT_EN_V 0x1
#define F_I2C_0002_DEV_BUSY_INT_EN_S 2
/**
 bitpos: [3]
 */
#define F_I2C_0002_ARBITER_LOST_INT_EN 0x08
#define F_I2C_0002_ARBITER_LOST_INT_EN_M 0x08
#define F_I2C_0002_ARBITER_LOST_INT_EN_V 0x1
#define F_I2C_0002_ARBITER_LOST_INT_EN_S 3
/**
 bitpos: [4]
 */
#define F_I2C_0002_SLAVE_SELEC_INT_EN 0x10
#define F_I2C_0002_SLAVE_SELEC_INT_EN_M 0x10
#define F_I2C_0002_SLAVE_SELEC_INT_EN_V 0x1
#define F_I2C_0002_SLAVE_SELEC_INT_EN_S 4
/**
 bitpos: [5]
 */
#define F_I2C_0002_STOP_INT_EN 0x20
#define F_I2C_0002_STOP_INT_EN_M 0x20
#define F_I2C_0002_STOP_INT_EN_V 0x1
#define F_I2C_0002_STOP_INT_EN_S 5
/**
 bitpos: [6]
 */
#define F_I2C_0002_FIFO_FULL_INT_EN 0x40
#define F_I2C_0002_FIFO_FULL_INT_EN_M 0x40
#define F_I2C_0002_FIFO_FULL_INT_EN_V 0x1
#define F_I2C_0002_FIFO_FULL_INT_EN_S 6
#define F_I2C_0002_FF
/**
 bitpos: [7]
 */
#define F_I2C_0002_FIFO_EMPTY_INT_EN 0x80
#define F_I2C_0002_FIFO_EMPTY_INT_EN_M 0x80
#define F_I2C_0002_FIFO_EMPTY_INT_EN_V 0x1
#define F_I2C_0002_FIFO_EMPTY_INT_EN_S 7

#define R_I2C_ISR(i)		( __REG_I2C_BASE(i) + 0x3 )

/**
 bitpos: [0]
 */
#define F_I2C_0003_TRANS_DONE_INT 0x01
#define F_I2C_0003_TRANS_DONE_INT_M 0x01
#define F_I2C_0003_TRANS_DONE_INT_V 0x1
#define F_I2C_0003_TRANS_DONE_INT_S 0
/**
 bitpos: [1]
 */
#define F_I2C_0003_DEV_NOT_EXIST_INT 0x02
#define F_I2C_0003_DEV_NOT_EXIST_INT_M 0x02
#define F_I2C_0003_DEV_NOT_EXIST_INT_V 0x1
#define F_I2C_0003_DEV_NOT_EXIST_INT_S 1
/**
 bitpos: [2]
 */
#define F_I2C_0003_DEV_BUSY_INT 0x04
#define F_I2C_0003_DEV_BUSY_INT_M 0x04
#define F_I2C_0003_DEV_BUSY_INT_V 0x1
#define F_I2C_0003_DEV_BUSY_INT_S 2
/**
 bitpos: [3]
 */
#define F_I2C_0003_ARBITER_LOST_INT 0x08
#define F_I2C_0003_ARBITER_LOST_INT_M 0x08
#define F_I2C_0003_ARBITER_LOST_INT_V 0x1
#define F_I2C_0003_ARBITER_LOST_INT_S 3
/**
 bitpos: [4]
 */
#define F_I2C_0003_SLAVE_SELEC_INT 0x10
#define F_I2C_0003_SLAVE_SELEC_INT_M 0x10
#define F_I2C_0003_SLAVE_SELEC_INT_V 0x1
#define F_I2C_0003_SLAVE_SELEC_INT_S 4
/**
 bitpos: [5]
 */
#define F_I2C_0003_STOP_INT 0x20
#define F_I2C_0003_STOP_INT_M 0x20
#define F_I2C_0003_STOP_INT_V 0x1
#define F_I2C_0003_STOP_INT_S 5
/**
 bitpos: [6]
 */
#define F_I2C_0003_FIFO_FULL_INT 0x40
#define F_I2C_0003_FIFO_FULL_INT_M 0x40
#define F_I2C_0003_FIFO_FULL_INT_V 0x1
#define F_I2C_0003_FIFO_FULL_INT_S 6
/**
 bitpos: [7]
 */
#define F_I2C_0003_FIFO_EMPTY_INT 0x80
#define F_I2C_0003_FIFO_EMPTY_INT_M 0x80
#define F_I2C_0003_FIFO_EMPTY_INT_V 0x1
#define F_I2C_0003_FIFO_EMPTY_INT_S 7

#define R_I2C_SAR(i)		( __REG_I2C_BASE(i) + 0x4 )

/**
 bitpos: [0]
 */
#define F_I2C_0004_RESERVED 0x01
#define F_I2C_0004_RESERVED_M 0x01
#define F_I2C_0004_RESERVED_V 0x1
#define F_I2C_0004_RESERVED_S 0
/**
 bitpos: [7:1]
 */
#define F_I2C_0004_SLAVE_ADDRESS 0xFE
#define F_I2C_0004_SLAVE_ADDRESS_M 0xFE
#define F_I2C_0004_SLAVE_ADDRESS_V 0x7F
#define F_I2C_0004_SLAVE_ADDRESS_S 1

#define R_I2C_SSAR(i)		( __REG_I2C_BASE(i) + 0x5 )

/**
 bitpos: [7:0]
 */
#define F_I2C_0005_SUB_ADDRESS 0xFF
#define F_I2C_0005_SUB_ADDRESS_M 0xFF
#define F_I2C_0005_SUB_ADDRESS_V 0xFF
#define F_I2C_0005_SUB_ADDRESS_S 0
/**
 bitpos: [7:0]
 */

#define R_I2C_HPCC(i)		( __REG_I2C_BASE(i) + 0x6 )

/**
 bitpos: [7:0]
 */

#define R_I2C_LPCC(i)		( __REG_I2C_BASE(i) + 0x7 )

/**
 bitpos: [7:0]
 */

#define R_I2C_PSUR(i)		( __REG_I2C_BASE(i) + 0x8 )

/**
 bitpos: [7:0]
 */

#define R_I2C_PSDR(i)		( __REG_I2C_BASE(i) + 0x9 )

/**
 bitpos: [7:0]
 */

#define R_I2C_RSUR(i)		( __REG_I2C_BASE(i) + 0xA )

/**
 bitpos: [7:0]
 */

#define R_I2C_SHDR(i)		( __REG_I2C_BASE(i) + 0xB )


#define R_I2C_FCR(i)		( __REG_I2C_BASE(i) + 0xC )

/**
 bitpos: [5:0]
 */
#define F_I2C_000C_BYTE_COUNT 0x3F
#define F_I2C_000C_BYTE_COUNT_M 0x3F
#define F_I2C_000C_BYTE_COUNT_V 0x3F
#define F_I2C_000C_BYTE_COUNT_S 0
/**
 bitpos: [6]
 */
#define F_I2C_000C_RESERVED 0x40
#define F_I2C_000C_RESERVED_M 0x40
#define F_I2C_000C_RESERVED_V 0x1
#define F_I2C_000C_RESERVED_S 6
/**
 bitpos: [7]
 */
#define F_I2C_000C_FIFO_FLUSH 0x80
#define F_I2C_000C_FIFO_FLUSH_M 0x80
#define F_I2C_000C_FIFO_FLUSH_V 0x1
#define F_I2C_000C_FIFO_FLUSH_S 7

#define R_I2C_DEV_CONTROL(i)		( __REG_I2C_BASE(i) + 0xD )

/**
 bitpos: [1:0]
 */
#define F_I2C_000D_DATA_HOLD_SEL 0x03
#define F_I2C_000D_DATA_HOLD_SEL_M 0x03
#define F_I2C_000D_DATA_HOLD_SEL_V 0x3
#define F_I2C_000D_DATA_HOLD_SEL_S 0
/**
 bitpos: [2]
 */
#define F_I2C_000D_SLAVE_10BIT_ADDR 0x04
#define F_I2C_000D_SLAVE_10BIT_ADDR_M 0x04
#define F_I2C_000D_SLAVE_10BIT_ADDR_V 0x1
#define F_I2C_000D_SLAVE_10BIT_ADDR_S 2
/**
 bitpos: [3]
 */
#define F_I2C_000D_FIN_SET 0x08
#define F_I2C_000D_FIN_SET_M 0x08
#define F_I2C_000D_FIN_SET_V 0x1
#define F_I2C_000D_FIN_SET_S 3
/**
 bitpos: [4]
 */
#define F_I2C_000D_SCL_DISCTRL_EN 0x10
#define F_I2C_000D_SCL_DISCTRL_EN_M 0x10
#define F_I2C_000D_SCL_DISCTRL_EN_V 0x1
#define F_I2C_000D_SCL_DISCTRL_EN_S 4
/**
 bitpos: [5]
 */
#define F_I2C_000D_SLAVE_DDC_EN 0x20
#define F_I2C_000D_SLAVE_DDC_EN_M 0x20
#define F_I2C_000D_SLAVE_DDC_EN_V 0x1
#define F_I2C_000D_SLAVE_DDC_EN_S 5
/**
 bitpos: [6]
 */
#define F_I2C_000D_SLAVE_SEND_DATA_EN 0x40
#define F_I2C_000D_SLAVE_SEND_DATA_EN_M 0x40
#define F_I2C_000D_SLAVE_SEND_DATA_EN_V 0x1
#define F_I2C_000D_SLAVE_SEND_DATA_EN_S 6
/**
 bitpos: [7]
 */
#define F_I2C_000D_RESERVED 0x80
#define F_I2C_000D_RESERVED_M 0x80
#define F_I2C_000D_RESERVED_V 0x1
#define F_I2C_000D_RESERVED_S 7
/**
 bitpos: [7:0]
 */

#define R_I2C_EDDC_ADDR(i)		( __REG_I2C_BASE(i) + 0xE )

/**
 bitpos: [7:0]
 */

#define R_I2C_SEG_POINTER(i)		( __REG_I2C_BASE(i) + 0xF )

/**
 bitpos: [7:0]
 */

#define R_I2C_DATA_REGISTER(i)		( __REG_I2C_BASE(i) + 0x10 )


#define R_I2C_ADDR_GOT(i)		( __REG_I2C_BASE(i) + 0x11 )

/**
 bitpos: [0]
 */
#define F_I2C_0011_R_W_BIT_GOT 0x01
#define F_I2C_0011_R_W_BIT_GOT_M 0x01
#define F_I2C_0011_R_W_BIT_GOT_V 0x1
#define F_I2C_0011_R_W_BIT_GOT_S 0
/**
 bitpos: [7:1]
 */
#define F_I2C_0011_SLAVE_ADDRESS_GOT 0xFE
#define F_I2C_0011_SLAVE_ADDRESS_GOT_M 0xFE
#define F_I2C_0011_SLAVE_ADDRESS_GOT_V 0x7F
#define F_I2C_0011_SLAVE_ADDRESS_GOT_S 1
/**
 bitpos: [7:0]
 */

#define R_I2C_SADDR_GOT(i)		( __REG_I2C_BASE(i) + 0x12 )


#define R_I2C_SSR(i)		( __REG_I2C_BASE(i) + 0x13 )

/**
 bitpos: [0]
 */
#define F_I2C_0013_SLAVE_SELECTED 0x01
#define F_I2C_0013_SLAVE_SELECTED_M 0x01
#define F_I2C_0013_SLAVE_SELECTED_V 0x1
#define F_I2C_0013_SLAVE_SELECTED_S 0
/**
 bitpos: [1]
 */
#define F_I2C_0013_SLAVE_TB_SELECTED 0x02
#define F_I2C_0013_SLAVE_TB_SELECTED_M 0x02
#define F_I2C_0013_SLAVE_TB_SELECTED_V 0x1
#define F_I2C_0013_SLAVE_TB_SELECTED_S 1
/**
 bitpos: [3:2]
 */
#define F_I2C_0013_RESERVED 0x0C
#define F_I2C_0013_RESERVED_M 0x0C
#define F_I2C_0013_RESERVED_V 0x3
#define F_I2C_0013_RESERVED_S 2
/**
 bitpos: [6:4]
 */
#define F_I2C_0013_SLAVE_STATUS 0x70
#define F_I2C_0013_SLAVE_STATUS_M 0x70
#define F_I2C_0013_SLAVE_STATUS_V 0x7
#define F_I2C_0013_SLAVE_STATUS_S 4
/**
 bitpos: [7]
 */
#define F_I2C_0013_RUNNING 0x80
#define F_I2C_0013_RUNNING_M 0x80
#define F_I2C_0013_RUNNING_V 0x1
#define F_I2C_0013_RUNNING_S 7

#define R_I2C_IER1(i)		( __REG_I2C_BASE(i) + 0x20 )

/**
 bitpos: [0]
 */
#define F_I2C_0020_FIFO_TRIGGER_INT_EN 0x01
#define F_I2C_0020_FIFO_TRIGGER_INT_EN_M 0x01
#define F_I2C_0020_FIFO_TRIGGER_INT_EN_V 0x1
#define F_I2C_0020_FIFO_TRIGGER_INT_EN_S 0
/**
 bitpos: [7:1]
 */
#define F_I2C_0020_RESERVED 0xFE
#define F_I2C_0020_RESERVED_M 0xFE
#define F_I2C_0020_RESERVED_V 0x7F
#define F_I2C_0020_RESERVED_S 1

#define R_I2C_ISR1(i)		( __REG_I2C_BASE(i) + 0x21 )

/**
 bitpos: [0]
 */
#define F_I2C_0021_FIFO_TRIGGER_INT 0x01
#define F_I2C_0021_FIFO_TRIGGER_INT_M 0x01
#define F_I2C_0021_FIFO_TRIGGER_INT_V 0x1
#define F_I2C_0021_FIFO_TRIGGER_INT_S 0
/**
 bitpos: [7:1]
 */
#define F_I2C_0021_RESERVED 0xFE
#define F_I2C_0021_RESERVED_M 0xFE
#define F_I2C_0021_RESERVED_V 0x7F
#define F_I2C_0021_RESERVED_S 1

#define R_I2C_FIFO_TREGGER(i)		( __REG_I2C_BASE(i) + 0x22 )

/**
 bitpos: [5:0]
 */
#define F_I2C_0022_FIFO_TRIGGER_LEVE 0x3F
#define F_I2C_0022_FIFO_TRIGGER_LEVE_M 0x3F
#define F_I2C_0022_FIFO_TRIGGER_LEVE_V 0x3F
#define F_I2C_0022_FIFO_TRIGGER_LEVE_S 0
/**
 bitpos: [7:6]
 */
#define F_I2C_0022_RESERVED 0xC0
#define F_I2C_0022_RESERVED_M 0xC0
#define F_I2C_0022_RESERVED_V 0x3
#define F_I2C_0022_RESERVED_S 6
/**
 bitpos: [7:0]
 */

#define R_I2C_SCB_BC_12_5(i)		( __REG_I2C_BASE(i) + 0x23 )


#define R_I2C_SCB_SSAR_EN(i)		( __REG_I2C_BASE(i) + 0x24 )

/**
 bitpos: [1:0]
 */
#define F_I2C_0024_OFFSET_BYTE_SELECT 0x03
#define F_I2C_0024_OFFSET_BYTE_SELECT_M 0x03
#define F_I2C_0024_OFFSET_BYTE_SELECT_V 0x3
#define F_I2C_0024_OFFSET_BYTE_SELECT_S 0
/**
 bitpos: [3:2]
 */
#define F_I2C_0024_DEV_OFFSET_BYTE1 0x0C
#define F_I2C_0024_DEV_OFFSET_BYTE1_M 0x0C
#define F_I2C_0024_DEV_OFFSET_BYTE1_V 0x3
#define F_I2C_0024_DEV_OFFSET_BYTE1_S 2
/**
 bitpos: [4]
 */
#define F_I2C_0024_AUTO_POWER_CTRL_OFF 0x10
#define F_I2C_0024_AUTO_POWER_CTRL_OFF_M 0x10
#define F_I2C_0024_AUTO_POWER_CTRL_OFF_V 0x1
#define F_I2C_0024_AUTO_POWER_CTRL_OFF_S 4
/**
 bitpos: [7:5]
 */
#define F_I2C_0024_RESERVED 0xE0
#define F_I2C_0024_RESERVED_M 0xE0
#define F_I2C_0024_RESERVED_V 0x7
#define F_I2C_0024_RESERVED_S 5
/**
 bitpos: [7:0]
 */

#define R_I2C_DEV_OFFSET_BYTE2(i)		( __REG_I2C_BASE(i) + 0x25 )

/**
 bitpos: [7:0]
 */

#define R_I2C_DEV_OFFSET_BYTE3(i)		( __REG_I2C_BASE(i) + 0x26 )

/**
 bitpos: [7:0]
 */

#define R_I2C_DEV_OFFSET_BYTE4(i)		( __REG_I2C_BASE(i) + 0x27 )

/**
 bitpos: [7:0]
 */

#define R_I2C_DDC_ADDR_GOT(i)		( __REG_I2C_BASE(i) + 0x28 )


#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __I2C_REG_H__ */
