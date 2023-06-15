#ifndef __SARADC_REG_STRUCT_H__
#define __SARADC_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct saradc_reg {

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint32_t sar_int_en:			1;
			uint32_t reserved1:			7;
			/**
			bitpos: [[8]]
			 */
			uint32_t sar_int_st:			1;
			uint32_t reserved9:			7;
			/**
			bitpos: [[23:16]]
			 */
			uint32_t sar_dout:			8;
			uint32_t reserved24:			8;
		};
		uint32_t val;
	} sar_ctrl_reg0;	/* REG_SARADC_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint32_t sar_en:			1;
			uint32_t reserved1:			7;
			/**
			bitpos: [[15:8]]
			 */
			uint32_t clk_div_num:			8;
			uint32_t reserved16:			8;
			/**
			bitpos: [[31:24]]
			 */
			uint32_t repeat_num:			8;
		};
		uint32_t val;
	} sar_ctrl_reg1;	/* REG_SARADC_BASE + 0x4 */

	union {
		struct {
			/**
			bitpos: [[7:0]]
			 */
			uint32_t default_value:			8;
			/**
			bitpos: [[15:8]]
			 */
			uint32_t debounce_value:			8;
			uint32_t reserved16:			16;
		};
		uint32_t val;
	} sar_ctrl_reg2;	/* REG_SARADC_BASE + 0x8 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint32_t sar_pwd_core:			1;
			uint32_t reserved1:			3;
			/**
			bitpos: [[4]]
			 */
			uint32_t sar_inpsel_core:			1;
			uint32_t reserved5:			11;
			/**
			bitpos: [[17:16]]
			 */
			uint32_t sar_clk_sel:			2;
			uint32_t reserved18:			10;
			/**
			bitpos: [[31:28]]
			 */
			uint32_t sar_anatst_core:			4;
		};
		uint32_t val;
	} sar_ctrl_reg3;	/* REG_SARADC_BASE + 0xc */

} saradc_1512_reg_t;

extern saradc_1512_reg_t SARADC0;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __SARADC_REG_STRUCT_H__ */
