#ifndef __I2C_REG_STRUCT_H__
#define __I2C_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct i2c_reg {

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t st:			1;
			/**
		bitpos: [[3:1]]
			 */
			uint8_t cp:			3;
			/**
			bitpos: [[4]]
			 */
			uint8_t sr_en:			1;
			/**
			bitpos: [[5]]
			 */
			uint8_t eddc_en:			1;
			/**
			bitpos: [[6]]
			 */
			uint8_t dev_not_exist_en:			1;
			/**
			bitpos: [[7]]
			 */
			uint8_t host_ctrl_en:			1;
		};
		uint8_t val;
	} hcr;	/* REG_I2C_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t fifo_empty:			1;
			/**
			bitpos: [[1]]
			 */
			uint8_t fifo_full:			1;
			/**
			bitpos: [[2]]
			 */
			uint8_t fifo_underrun:			1;
			/**
			bitpos: [[3]]
			 */
			uint8_t fifo_overrun:			1;
			/**
			bitpos: [[4]]
			 */
			uint8_t fifo_error:			1;
			/**
			bitpos: [[5]]
			 */
			uint8_t host_busy:			1;
			/**
			bitpos: [[6]]
			 */
			uint8_t device_not_exist:			1;
			/**
			bitpos: [[7]]
			 */
			uint8_t device_busy:			1;
		};
		uint8_t val;
	} hsr;	/* REG_I2C_BASE + 0x1 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t trans_done_int_en:			1;
			/**
			bitpos: [[1]]
			 */
			uint8_t dev_not_exist_int_en:			1;
			/**
			bitpos: [[2]]
			 */
			uint8_t dev_busy_int_en:			1;
			/**
			bitpos: [[3]]
			 */
			uint8_t arbiter_lost_int_en:			1;
			/**
			bitpos: [[4]]
			 */
			uint8_t slave_selec_int_en:			1;
			/**
			bitpos: [[5]]
			 */
			uint8_t stop_int_en:			1;
			/**
			bitpos: [[6]]
			 */
			uint8_t fifo_full_int_en:			1;
			/**
			bitpos: [[7]]
			 */
			uint8_t fifo_empty_int_en:			1;
		};
		uint8_t val;
	} ier;	/* REG_I2C_BASE + 0x2 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t trans_done_int:			1;
			/**
			bitpos: [[1]]
			 */
			uint8_t dev_not_exist_int:			1;
			/**
			bitpos: [[2]]
			 */
			uint8_t dev_busy_int:			1;
			/**
			bitpos: [[3]]
			 */
			uint8_t arbiter_lost_int:			1;
			/**
			bitpos: [[4]]
			 */
			uint8_t slave_selec_int:			1;
			/**
			bitpos: [[5]]
			 */
			uint8_t stop_int:			1;
			/**
			bitpos: [[6]]
			 */
			uint8_t fifo_full_int:			1;
			/**
			bitpos: [[7]]
			 */
			uint8_t fifo_empty_int:			1;
		};
		uint8_t val;
	} isr;	/* REG_I2C_BASE + 0x3 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t reserved:			1;
			/**
			bitpos: [[7:1]]
			 */
			uint8_t slave_address:			7;
		};
		uint8_t val;
	} sar;	/* REG_I2C_BASE + 0x4 */

	union {
		struct {
			/**
			bitpos: [[7:0]]
			 */
			uint8_t sub_address:			8;
		};
		uint8_t val;
	} ssar;	/* REG_I2C_BASE + 0x5 */
	uint8_t hpcc; /* REG_I2C_BASE + 0x6 */
	uint8_t lpcc; /* REG_I2C_BASE + 0x7 */
	uint8_t psur; /* REG_I2C_BASE + 0x8 */
	uint8_t psdr; /* REG_I2C_BASE + 0x9 */
	uint8_t rsur; /* REG_I2C_BASE + 0xa */
	uint8_t shdr; /* REG_I2C_BASE + 0xb */

	union {
		struct {
			/**
			bitpos: [[5:0]]
			 */
			uint8_t byte_count:			6;
			/**
			bitpos: [[6]]
			 */
			uint8_t reserved:			1;
			/**
			bitpos: [[7]]
			 */
			uint8_t fifo_flush:			1;
		};
		uint8_t val;
	} fcr;	/* REG_I2C_BASE + 0xc */

	union {
		struct {
			/**
			bitpos: [[1:0]]
			 */
			uint8_t data_hold_sel:			2;
			/**
			bitpos: [[2]]
			 */
			uint8_t slave_10bit_addr:			1;
			/**
			bitpos: [[3]]
			 */
			uint8_t fin_set:			1;
			/**
			bitpos: [[4]]
			 */
			uint8_t scl_disctrl_en:			1;
			/**
			bitpos: [[5]]
			 */
			uint8_t slave_ddc_en:			1;
			/**
			bitpos: [[6]]
			 */
			uint8_t slave_send_data_en:			1;
			/**
			bitpos: [[7]]
			 */
			uint8_t reserved:			1;
		};
		uint8_t val;
	} dev_control;	/* REG_I2C_BASE + 0xd */
	uint8_t eddc_addr; /* REG_I2C_BASE + 0xe */
	uint8_t seg_pointer; /* REG_I2C_BASE + 0xf */
	uint8_t data_register; /* REG_I2C_BASE + 0x10 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t r_w_bit_got:			1;
			/**
			bitpos: [[7:1]]
			 */
			uint8_t slave_address_got:			7;
		};
		uint8_t val;
	} addr_got;	/* REG_I2C_BASE + 0x11 */
	uint8_t saddr_got; /* REG_I2C_BASE + 0x12 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t slave_selected:			1;
			/**
			bitpos: [[1]]
			 */
			uint8_t slave_tb_selected:			1;
			/**
			bitpos: [[3:2]]
			 */
			uint8_t reserved:			2;
			/**
			bitpos: [[6:4]]
			 */
			uint8_t slave_status:			3;
			/**
			bitpos: [[7]]
			 */
			uint8_t running:			1;
		};
		uint8_t val;
	} ssr;	/* REG_I2C_BASE + 0x13 */

	uint32_t reserved_14; /* REG_I2C_BASE + 0x14 */
	uint32_t reserved_18; /* REG_I2C_BASE + 0x18 */
	uint32_t reserved_1c; /* REG_I2C_BASE + 0x1c */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t fifo_trigger_int_en:			1;
			/**
			bitpos: [[7:1]]
			 */
			uint8_t reserved:			7;
		};
		uint8_t val;
	} ier1;	/* REG_I2C_BASE + 0x20 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint8_t fifo_trigger_int:			1;
			/**
			bitpos: [[7:1]]
			 */
			uint8_t reserved:			7;
		};
		uint8_t val;
	} isr1;	/* REG_I2C_BASE + 0x21 */

	union {
		struct {
			/**
			bitpos: [[5:0]]
			 */
			uint8_t fifo_trigger_leve:			6;
			/**
			bitpos: [[7:6]]
			 */
			uint8_t reserved:			2;
		};
		uint8_t val;
	} fifo_tregger;	/* REG_I2C_BASE + 0x22 */
	uint8_t scb_bc_12_5; /* REG_I2C_BASE + 0x23 */

	union {
		struct {
			/**
			bitpos: [[1:0]]
			 */
			uint8_t offset_byte_select:			2;
			/**
			bitpos: [[3:2]]
			 */
			uint8_t dev_offset_byte1:			2;
			/**
			bitpos: [[4]]
			 */
			uint8_t auto_power_ctrl_off:			1;
			/**
			bitpos: [[7:5]]
			 */
			uint8_t reserved:			3;
		};
		uint8_t val;
	} scb_ssar_en;	/* REG_I2C_BASE + 0x24 */
	uint8_t dev_offset_byte2; /* REG_I2C_BASE + 0x25 */
	uint8_t dev_offset_byte3; /* REG_I2C_BASE + 0x26 */
	uint8_t dev_offset_byte4; /* REG_I2C_BASE + 0x27 */
	uint8_t ddc_addr_got; /* REG_I2C_BASE + 0x28 */

} i2c_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __I2C_REG_STRUCT_H__ */
