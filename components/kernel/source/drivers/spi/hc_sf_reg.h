#ifndef __HC_SF_REG_H__
#define __HC_SF_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_SF_BASE		( REG_SF_BASE )


#define R_SF_CPU_DMA_CTRL		( __REG_SF_BASE + 0x0 )

/**
 bitpos: [0]
 */
#define F_SF_0000_DMA_STOP_CMD 0x00000001
#define F_SF_0000_DMA_STOP_CMD_M 0x00000001
#define F_SF_0000_DMA_STOP_CMD_V 0x1
#define F_SF_0000_DMA_STOP_CMD_S 0
/**
 bitpos: [1]
 */
#define F_SF_0000_DMA_STOP_INT_EN 0x00000002
#define F_SF_0000_DMA_STOP_INT_EN_M 0x00000002
#define F_SF_0000_DMA_STOP_INT_EN_V 0x1
#define F_SF_0000_DMA_STOP_INT_EN_S 1
/**
 bitpos: [10:9]
 */
#define F_SF_0000_SPI_MODE 0x00000600
#define F_SF_0000_SPI_MODE_M 0x00000600
#define F_SF_0000_SPI_MODE_V 0x3
#define F_SF_0000_SPI_MODE_S 9
/**
 bitpos: [16]
 */
#define F_SF_0000_SEND_DUMMY 0x00010000
#define F_SF_0000_SEND_DUMMY_M 0x00010000
#define F_SF_0000_SEND_DUMMY_V 0x1
#define F_SF_0000_SEND_DUMMY_S 16
/**
 bitpos: [24]
 */
#define F_SF_0000_PIO_REQ 0x01000000
#define F_SF_0000_PIO_REQ_M 0x01000000
#define F_SF_0000_PIO_REQ_V 0x1
#define F_SF_0000_PIO_REQ_S 24
/**
 bitpos: [25]
 */
#define F_SF_0000_PIO_REQ_INT_EN 0x02000000
#define F_SF_0000_PIO_REQ_INT_EN_M 0x02000000
#define F_SF_0000_PIO_REQ_INT_EN_V 0x1
#define F_SF_0000_PIO_REQ_INT_EN_S 25

#define R_SF_CLK_DIV_EXT		( __REG_SF_BASE + 0x4 )

/**
 bitpos: [5:0]
 */
#define F_SF_0004_CLK_DIV_EXT 0x0000003F
#define F_SF_0004_CLK_DIV_EXT_M 0x0000003F
#define F_SF_0004_CLK_DIV_EXT_V 0x3F
#define F_SF_0004_CLK_DIV_EXT_S 0

#define R_SF_DMA_MEM_ADDR		( __REG_SF_BASE + 0x58 )

/**
 bitpos: [29:0]
 */
#define F_SF_0058_DMA_MEM_ADDR 0x3FFFFFFF
#define F_SF_0058_DMA_MEM_ADDR_M 0x3FFFFFFF
#define F_SF_0058_DMA_MEM_ADDR_V 0x3FFFFFFF
#define F_SF_0058_DMA_MEM_ADDR_S 0

#define R_SF_DMA_FLASH_ADDR		( __REG_SF_BASE + 0x5C )

/**
 bitpos: [25:0]
 */
#define F_SF_005C_DMA_FLASH_ADDR 0x03FFFFFF
#define F_SF_005C_DMA_FLASH_ADDR_M 0x03FFFFFF
#define F_SF_005C_DMA_FLASH_ADDR_V 0x3FFFFFF
#define F_SF_005C_DMA_FLASH_ADDR_S 0

#define R_SF_DMA_LEN		( __REG_SF_BASE + 0x60 )

/**
 bitpos: [23:0]
 */
#define F_SF_0060_DMA_LEN 0x00FFFFFF
#define F_SF_0060_DMA_LEN_M 0x00FFFFFF
#define F_SF_0060_DMA_LEN_V 0xFFFFFF
#define F_SF_0060_DMA_LEN_S 0

#define R_SF_DMA_CTRL		( __REG_SF_BASE + 0x64 )

/**
 bitpos: [0]
 */
#define F_SF_0064_CS_SEL 0x00000001
#define F_SF_0064_CS_SEL_M 0x00000001
#define F_SF_0064_CS_SEL_V 0x1
#define F_SF_0064_CS_SEL_S 0
/**
 bitpos: [5]
 */
#define F_SF_0064_DMA_START 0x00000020
#define F_SF_0064_DMA_START_M 0x00000020
#define F_SF_0064_DMA_START_V 0x1
#define F_SF_0064_DMA_START_S 5
/**
 bitpos: [6]
 */
#define F_SF_0064_DMA_INC_VALID 0x00000040
#define F_SF_0064_DMA_INC_VALID_M 0x00000040
#define F_SF_0064_DMA_INC_VALID_V 0x1
#define F_SF_0064_DMA_INC_VALID_S 6
/**
 bitpos: [7]
 */
#define F_SF_0064_DMA_DIR 0x00000080
#define F_SF_0064_DMA_DIR_M 0x00000080
#define F_SF_0064_DMA_DIR_V 0x1
#define F_SF_0064_DMA_DIR_S 7
/**
 bitpos: [17]
 */
#define F_SF_0064_ARBIT_MODE 0x00020000
#define F_SF_0064_ARBIT_MODE_M 0x00020000
#define F_SF_0064_ARBIT_MODE_V 0x1
#define F_SF_0064_ARBIT_MODE_S 17
/**
 bitpos: [18]
 */
#define F_SF_0064_ACCESS_REQ 0x00040000
#define F_SF_0064_ACCESS_REQ_M 0x00040000
#define F_SF_0064_ACCESS_REQ_V 0x1
#define F_SF_0064_ACCESS_REQ_S 18
/**
 bitpos: [19]
 */
#define F_SF_0064_ACCESS_MODE 0x00080000
#define F_SF_0064_ACCESS_MODE_M 0x00080000
#define F_SF_0064_ACCESS_MODE_V 0x1
#define F_SF_0064_ACCESS_MODE_S 19
/**
 bitpos: [20]
 */
#define F_SF_0064_DMA_INT_EN 0x00100000
#define F_SF_0064_DMA_INT_EN_M 0x00100000
#define F_SF_0064_DMA_INT_EN_V 0x1
#define F_SF_0064_DMA_INT_EN_S 20
/**
 bitpos: [21]
 */
#define F_SF_0064_ARBIT_INT_EN 0x00200000
#define F_SF_0064_ARBIT_INT_EN_M 0x00200000
#define F_SF_0064_ARBIT_INT_EN_V 0x1
#define F_SF_0064_ARBIT_INT_EN_S 21
/**
 bitpos: [22]
 */
#define F_SF_0064_BYTE_TRANSFER_EN 0x00400000
#define F_SF_0064_BYTE_TRANSFER_EN_M 0x00400000
#define F_SF_0064_BYTE_TRANSFER_EN_V 0x1
#define F_SF_0064_BYTE_TRANSFER_EN_S 22
/**
 bitpos: [24]
 */
#define F_SF_0064_AUTO_PAUSE_EN 0x01000000
#define F_SF_0064_AUTO_PAUSE_EN_M 0x01000000
#define F_SF_0064_AUTO_PAUSE_EN_V 0x1
#define F_SF_0064_AUTO_PAUSE_EN_S 24
/**
 bitpos: [27:25]
 */
#define F_SF_0064_AUTO_PAUSE_THR 0x0E000000
#define F_SF_0064_AUTO_PAUSE_THR_M 0x0E000000
#define F_SF_0064_AUTO_PAUSE_THR_V 0x7
#define F_SF_0064_AUTO_PAUSE_THR_S 25
/**
 bitpos: [28]
 */
#define F_SF_0064_DUMMY_FOR_ADDR 0x10000000
#define F_SF_0064_DUMMY_FOR_ADDR_M 0x10000000
#define F_SF_0064_DUMMY_FOR_ADDR_V 0x1
#define F_SF_0064_DUMMY_FOR_ADDR_S 28
/**
 bitpos: [29]
 */
#define F_SF_0064_DUMMY_INC_EN 0x20000000
#define F_SF_0064_DUMMY_INC_EN_M 0x20000000
#define F_SF_0064_DUMMY_INC_EN_V 0x1
#define F_SF_0064_DUMMY_INC_EN_S 29
/**
 bitpos: [30]
 */
#define F_SF_0064_DUMMY_BYTE_EXCHANGE 0x40000000
#define F_SF_0064_DUMMY_BYTE_EXCHANGE_M 0x40000000
#define F_SF_0064_DUMMY_BYTE_EXCHANGE_V 0x1
#define F_SF_0064_DUMMY_BYTE_EXCHANGE_S 30
/**
 bitpos: [31]
 */
#define F_SF_0064_PRE_READ_EN 0x80000000
#define F_SF_0064_PRE_READ_EN_M 0x80000000
#define F_SF_0064_PRE_READ_EN_V 0x1
#define F_SF_0064_PRE_READ_EN_S 31

#define R_SF_CONF0		( __REG_SF_BASE + 0x98 )

/**
 bitpos: [7:0]
 */
#define F_SF_0098_INSTRUCT 0xFF
#define F_SF_0098_INSTRUCT_M 0xFF
#define F_SF_0098_INSTRUCT_V 0xFF
#define F_SF_0098_INSTRUCT_S 0

#define R_SF_CONF1		( __REG_SF_BASE + 0x99 )

/**
 bitpos: [0]
 */
#define F_SF_0099_DATA_HIT 0x01
#define F_SF_0099_DATA_HIT_M 0x01
#define F_SF_0099_DATA_HIT_V 0x1
#define F_SF_0099_DATA_HIT_S 0
/**
 bitpos: [1]
 */
#define F_SF_0099_DUMMY_HIT 0x02
#define F_SF_0099_DUMMY_HIT_M 0x02
#define F_SF_0099_DUMMY_HIT_V 0x1
#define F_SF_0099_DUMMY_HIT_S 1
/**
 bitpos: [2]
 */
#define F_SF_0099_ADDR_HIT 0x04
#define F_SF_0099_ADDR_HIT_M 0x04
#define F_SF_0099_ADDR_HIT_V 0x1
#define F_SF_0099_ADDR_HIT_S 2
/**
 bitpos: [3]
 */
#define F_SF_0099_CODE_HIT 0x08
#define F_SF_0099_CODE_HIT_M 0x08
#define F_SF_0099_CODE_HIT_V 0x1
#define F_SF_0099_CODE_HIT_S 3
/**
 bitpos: [5:4]
 */
#define F_SF_0099_DUMMY_NUM 0x30
#define F_SF_0099_DUMMY_NUM_M 0x30
#define F_SF_0099_DUMMY_NUM_V 0x3
#define F_SF_0099_DUMMY_NUM_S 4
/**
 bitpos: [6]
 */
#define F_SF_0099_CONTINUE_READ 0x40
#define F_SF_0099_CONTINUE_READ_M 0x40
#define F_SF_0099_CONTINUE_READ_V 0x1
#define F_SF_0099_CONTINUE_READ_S 6
/**
 bitpos: [7]
 */
#define F_SF_0099_CONTINUE_WRITE 0x80
#define F_SF_0099_CONTINUE_WRITE_M 0x80
#define F_SF_0099_CONTINUE_WRITE_V 0x1
#define F_SF_0099_CONTINUE_WRITE_S 7

#define R_SF_CONF2		( __REG_SF_BASE + 0x9A )

/**
 bitpos: [2:0]
 */
#define F_SF_009A_MODE 0x07
#define F_SF_009A_MODE_M 0x07
#define F_SF_009A_MODE_V 0x7
#define F_SF_009A_MODE_S 0
/**
 bitpos: [4:3]
 */
#define F_SF_009A_ADDR_BYTES 0x18
#define F_SF_009A_ADDR_BYTES_M 0x18
#define F_SF_009A_ADDR_BYTES_V 0x3
#define F_SF_009A_ADDR_BYTES_S 3
/**
 bitpos: [5]
 */
#define F_SF_009A_CONTINUE_COUNT_EN 0x20
#define F_SF_009A_CONTINUE_COUNT_EN_M 0x20
#define F_SF_009A_CONTINUE_COUNT_EN_V 0x1
#define F_SF_009A_CONTINUE_COUNT_EN_S 5
/**
 bitpos: [6]
 */
#define F_SF_009A_RX_READY 0x40
#define F_SF_009A_RX_READY_M 0x40
#define F_SF_009A_RX_READY_V 0x1
#define F_SF_009A_RX_READY_S 6
/**
 bitpos: [7]
 */
#define F_SF_009A_ODD_DIV_SETTING 0x80
#define F_SF_009A_ODD_DIV_SETTING_M 0x80
#define F_SF_009A_ODD_DIV_SETTING_V 0x1
#define F_SF_009A_ODD_DIV_SETTING_S 7

#define R_SF_CONF3		( __REG_SF_BASE + 0x9B )

/**
 bitpos: [3:0]
 */
#define F_SF_009B_CLK_DIV 0x0F
#define F_SF_009B_CLK_DIV_M 0x0F
#define F_SF_009B_CLK_DIV_V 0xF
#define F_SF_009B_CLK_DIV_S 0
/**
 bitpos: [4]
 */
#define F_SF_009B_WJ_CTRL 0x10
#define F_SF_009B_WJ_CTRL_M 0x10
#define F_SF_009B_WJ_CTRL_V 0x1
#define F_SF_009B_WJ_CTRL_S 4
/**
 bitpos: [5]
 */
#define F_SF_009B_HOLDJ_EN 0x20
#define F_SF_009B_HOLDJ_EN_M 0x20
#define F_SF_009B_HOLDJ_EN_V 0x1
#define F_SF_009B_HOLDJ_EN_S 5
/**
 bitpos: [7:6]
 */
#define F_SF_009B_SIZE 0xC0
#define F_SF_009B_SIZE_M 0xC0
#define F_SF_009B_SIZE_V 0x3
#define F_SF_009B_SIZE_S 6

#define R_SF_CONF4		( __REG_SF_BASE + 0x9C )

/**
 bitpos: [31:0]
 */
#define F_SF_009C_DUMMY_DATA 0xFFFFFFFF
#define F_SF_009C_DUMMY_DATA_M 0xFFFFFFFF
#define F_SF_009C_DUMMY_DATA_V 0xFFFFFFFF
#define F_SF_009C_DUMMY_DATA_S 0

#define R_SF_INT_ST		( __REG_SF_BASE + 0xA0 )

/**
 bitpos: [0]
 */
#define F_SF_00A0_DMA_INT_ST 0x00000001
#define F_SF_00A0_DMA_INT_ST_M 0x00000001
#define F_SF_00A0_DMA_INT_ST_V 0x1
#define F_SF_00A0_DMA_INT_ST_S 0
/**
 bitpos: [8]
 */
#define F_SF_00A0_ARBIT_INT_ST 0x00000100
#define F_SF_00A0_ARBIT_INT_ST_M 0x00000100
#define F_SF_00A0_ARBIT_INT_ST_V 0x1
#define F_SF_00A0_ARBIT_INT_ST_S 8
/**
 bitpos: [18]
 */
#define F_SF_00A0_DMA_STOP_ST 0x00040000
#define F_SF_00A0_DMA_STOP_ST_M 0x00040000
#define F_SF_00A0_DMA_STOP_ST_V 0x1
#define F_SF_00A0_DMA_STOP_ST_S 18
/**
 bitpos: [24]
 */
#define F_SF_00A0_PIO_REQ_INT_ST 0x01000000
#define F_SF_00A0_PIO_REQ_INT_ST_M 0x01000000
#define F_SF_00A0_PIO_REQ_INT_ST_V 0x1
#define F_SF_00A0_PIO_REQ_INT_ST_S 24
/**
 bitpos: [25]
 */
#define F_SF_00A0_DMA_LEN_ERR_ST 0x02000000
#define F_SF_00A0_DMA_LEN_ERR_ST_M 0x02000000
#define F_SF_00A0_DMA_LEN_ERR_ST_V 0x1
#define F_SF_00A0_DMA_LEN_ERR_ST_S 25

#define R_SF_ACCESS_CONF		( __REG_SF_BASE + 0xA8 )

/**
 bitpos: [7:0]
 */
#define F_SF_00A8_LATENCY 0x000000FF
#define F_SF_00A8_LATENCY_M 0x000000FF
#define F_SF_00A8_LATENCY_V 0xFF
#define F_SF_00A8_LATENCY_S 0
/**
 bitpos: [15:8]
 */
#define F_SF_00A8_HI_PRIO_CNT 0x0000FF00
#define F_SF_00A8_HI_PRIO_CNT_M 0x0000FF00
#define F_SF_00A8_HI_PRIO_CNT_V 0xFF
#define F_SF_00A8_HI_PRIO_CNT_S 8

#define R_SF_ADDR_CONF		( __REG_SF_BASE + 0xB8 )

/**
 bitpos: [7:0]
 */
#define F_SF_00B8_ADDR_BYTE_PROG 0x000000FF
#define F_SF_00B8_ADDR_BYTE_PROG_M 0x000000FF
#define F_SF_00B8_ADDR_BYTE_PROG_V 0xFF
#define F_SF_00B8_ADDR_BYTE_PROG_S 0
/**
 bitpos: [8]
 */
#define F_SF_00B8_ADDR_BYTE_PROG_EN 0x00000100
#define F_SF_00B8_ADDR_BYTE_PROG_EN_M 0x00000100
#define F_SF_00B8_ADDR_BYTE_PROG_EN_V 0x1
#define F_SF_00B8_ADDR_BYTE_PROG_EN_S 8
/**
 bitpos: [31:0]
 */

#define R_SF_SQI_COUNT		( __REG_SF_BASE + 0xBC )


#define R_SF_WRITE_PROTECT_START		( __REG_SF_BASE + 0xC0 )

/**
 bitpos: [23:2]
 */
#define F_SF_00C0_ADDR 0x00FFFFFC
#define F_SF_00C0_ADDR_M 0x00FFFFFC
#define F_SF_00C0_ADDR_V 0x3FFFFF
#define F_SF_00C0_ADDR_S 2
/**
 bitpos: [31]
 */
#define F_SF_00C0_ENABLE 0x80000000
#define F_SF_00C0_ENABLE_M 0x80000000
#define F_SF_00C0_ENABLE_V 0x1
#define F_SF_00C0_ENABLE_S 31

#define R_SF_WRITE_PROTECT_END		( __REG_SF_BASE + 0xC4 )

/**
 bitpos: [23:2]
 */
#define F_SF_00C4_ADDR 0x00FFFFFC
#define F_SF_00C4_ADDR_M 0x00FFFFFC
#define F_SF_00C4_ADDR_V 0x3FFFFF
#define F_SF_00C4_ADDR_S 2

#define R_SF_TIMING_CTRL		( __REG_SF_BASE + 0xC8 )

/**
 bitpos: [0]
 */
#define F_SF_00C8_CS_SETUP_EN 0x00000001
#define F_SF_00C8_CS_SETUP_EN_M 0x00000001
#define F_SF_00C8_CS_SETUP_EN_V 0x1
#define F_SF_00C8_CS_SETUP_EN_S 0
/**
 bitpos: [7:1]
 */
#define F_SF_00C8_CS_SETUP 0x000000FE
#define F_SF_00C8_CS_SETUP_M 0x000000FE
#define F_SF_00C8_CS_SETUP_V 0x7F
#define F_SF_00C8_CS_SETUP_S 1
/**
 bitpos: [8]
 */
#define F_SF_00C8_CS_HOLD_EN 0x00000100
#define F_SF_00C8_CS_HOLD_EN_M 0x00000100
#define F_SF_00C8_CS_HOLD_EN_V 0x1
#define F_SF_00C8_CS_HOLD_EN_S 8
/**
 bitpos: [15:9]
 */
#define F_SF_00C8_CS_HOLD 0x0000FE00
#define F_SF_00C8_CS_HOLD_M 0x0000FE00
#define F_SF_00C8_CS_HOLD_V 0x7F
#define F_SF_00C8_CS_HOLD_S 9
/**
 bitpos: [16]
 */
#define F_SF_00C8_CS_DE_ASSERT_EN 0x00010000
#define F_SF_00C8_CS_DE_ASSERT_EN_M 0x00010000
#define F_SF_00C8_CS_DE_ASSERT_EN_V 0x1
#define F_SF_00C8_CS_DE_ASSERT_EN_S 16
/**
 bitpos: [23:17]
 */
#define F_SF_00C8_CS_DE_ASSERT 0x00FE0000
#define F_SF_00C8_CS_DE_ASSERT_M 0x00FE0000
#define F_SF_00C8_CS_DE_ASSERT_V 0x7F
#define F_SF_00C8_CS_DE_ASSERT_S 17
/**
 bitpos: [24]
 */
#define F_SF_00C8_CS_PROGRAM_EN 0x01000000
#define F_SF_00C8_CS_PROGRAM_EN_M 0x01000000
#define F_SF_00C8_CS_PROGRAM_EN_V 0x1
#define F_SF_00C8_CS_PROGRAM_EN_S 24
/**
 bitpos: [26:25]
 */
#define F_SF_00C8_CS_PROGRAM 0x06000000
#define F_SF_00C8_CS_PROGRAM_M 0x06000000
#define F_SF_00C8_CS_PROGRAM_V 0x3
#define F_SF_00C8_CS_PROGRAM_S 25

#define R_SF_CRC_CTRL		( __REG_SF_BASE + 0xD0 )

/**
 bitpos: [0]
 */
#define F_SF_00D0_CRC_VALID 0x00000001
#define F_SF_00D0_CRC_VALID_M 0x00000001
#define F_SF_00D0_CRC_VALID_V 0x1
#define F_SF_00D0_CRC_VALID_S 0
/**
 bitpos: [8]
 */
#define F_SF_00D0_CRC_CLEAR 0x00000100
#define F_SF_00D0_CRC_CLEAR_M 0x00000100
#define F_SF_00D0_CRC_CLEAR_V 0x1
#define F_SF_00D0_CRC_CLEAR_S 8
/**
 bitpos: [31:0]
 */

#define R_SF_CRC_INIT_VALUE		( __REG_SF_BASE + 0xD4 )

/**
 bitpos: [31:0]
 */

#define R_SF_CRC_RESULT		( __REG_SF_BASE + 0xD8 )


#define R_SF_POWER		( __REG_SF_BASE + 0xDC )

/**
 bitpos: [0]
 */
#define F_SF_00DC_PFLASH_EN 0x00000001
#define F_SF_00DC_PFLASH_EN_M 0x00000001
#define F_SF_00DC_PFLASH_EN_V 0x1
#define F_SF_00DC_PFLASH_EN_S 0
/**
 bitpos: [1]
 */
#define F_SF_00DC_SFLASH_EN 0x00000002
#define F_SF_00DC_SFLASH_EN_M 0x00000002
#define F_SF_00DC_SFLASH_EN_V 0x1
#define F_SF_00DC_SFLASH_EN_S 1
/**
 bitpos: [2]
 */
#define F_SF_00DC_DMA_EN 0x00000004
#define F_SF_00DC_DMA_EN_M 0x00000004
#define F_SF_00DC_DMA_EN_V 0x1
#define F_SF_00DC_DMA_EN_S 2

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __SF_REG_H__ */
