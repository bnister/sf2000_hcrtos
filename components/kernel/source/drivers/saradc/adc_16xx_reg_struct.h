#ifndef __ADC_REG_STRUCT_H__
#define __ADC_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

#define wait_ch_counter_pluser_en(channel) wait_ch##channel##_counter_end_pluse_int_en
#define ch_counter_pluser_en(channel) ch##channel##_counter_end_pluse_int_en
#define wait_ch_counter_pluser_st(channel) wait_ch##channel##_counter_end_pluse_int_st
#define ch_conter_pluset_st(channel) ch##channel##_counter_end_pluse_int_st
#define ch_wait_val(x) ch_wait_val_##x

typedef volatile struct adc_reg {

	uint32_t reserved_0; /* REG_ADC_BASE + 0x0 */
	uint32_t reserved_4; /* REG_ADC_BASE + 0x4 */
	uint32_t reserved_8; /* REG_ADC_BASE + 0x8 */
	//uint32_t reserved_c; /* REG_ADC_BASE + 0xc */

	union {
		struct {
			uint32_t saradc_pwd:		1;
			uint32_t reserved1:			4;
			uint32_t adc8b_sw_ldo_en_core:	1;
			uint32_t adc8b_sw_ldo_vout_sel_core: 2;
			uint32_t sar_clksel_core_div:		8;
			uint32_t sar_clksel_core:			3;
			uint32_t reserved0:		13;
		};
		uint32_t val;
	} saradc_ctl;



	uint32_t reserved_10; /* REG_ADC_BASE + 0x10 */
	uint32_t reserved_14; /* REG_ADC_BASE + 0x14 */
	uint32_t reserved_18; /* REG_ADC_BASE + 0x18 */
	uint32_t reserved_1c; /* REG_ADC_BASE + 0x1c */
	uint32_t reserved_20; /* REG_ADC_BASE + 0x20 */
	uint32_t reserved_24; /* REG_ADC_BASE + 0x24 */
	uint32_t reserved_28; /* REG_ADC_BASE + 0x28 */
	uint32_t reserved_2c; /* REG_ADC_BASE + 0x2c */
	uint32_t reserved_30; /* REG_ADC_BASE + 0x30 */
	uint32_t reserved_34; /* REG_ADC_BASE + 0x34 */
	uint32_t reserved_38; /* REG_ADC_BASE + 0x38 */
	uint32_t reserved_3c; /* REG_ADC_BASE + 0x3c */
	uint32_t reserved_40; /* REG_ADC_BASE + 0x40 */
	uint32_t reserved_44; /* REG_ADC_BASE + 0x44 */
	uint32_t reserved_48; /* REG_ADC_BASE + 0x48 */
	uint32_t reserved_4c; /* REG_ADC_BASE + 0x4c */
	uint32_t reserved_50; /* REG_ADC_BASE + 0x50 */
	uint32_t reserved_54; /* REG_ADC_BASE + 0x54 */
	uint32_t reserved_58; /* REG_ADC_BASE + 0x58 */
	uint32_t reserved_5c; /* REG_ADC_BASE + 0x5c */
	uint32_t reserved_60; /* REG_ADC_BASE + 0x60 */
	uint32_t reserved_64; /* REG_ADC_BASE + 0x64 */
	uint32_t reserved_68; /* REG_ADC_BASE + 0x68 */
	uint32_t reserved_6c; /* REG_ADC_BASE + 0x6c */
	uint32_t old_read_ch; /* REG_ADC_BASE + 0x70 */
//	uint32_t reserved_74; /* REG_ADC_BASE + 0x74 */
	union {

		struct {
			uint32_t sar_en:		1;
			uint32_t reserved0:		31;
		};
		uint32_t val;
	} saradc_en;

	uint32_t reserved_78; /* REG_ADC_BASE + 0x78 */
	uint32_t reserved_7c; /* REG_ADC_BASE + 0x7c */
	uint32_t reserved_80; /* REG_ADC_BASE + 0x80 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint32_t meas_data_10bit_d2_0_sel:			1;
			/**
			bitpos: [[1]]
			 */
			uint32_t meas_data_10bit_d2_1_sel:			1;
			/**
			bitpos: [[2]]
			 */
			uint32_t meas_data_10bit_d2_2_sel:			1;
			/**
			bitpos: [[3]]
			 */
			uint32_t meas_data_10bit_d3_0_sel:			1;
			/**
			bitpos: [[4]]
			 */
			uint32_t meas_data_10bit_d3_1_sel:			1;
			/**
			bitpos: [[5]]
			 */
			uint32_t meas_data_10bit_d3_2_sel:			1;
			uint32_t reserved6:			2;
			/**
			bitpos: [[8]]
			 */
			uint32_t data_10bit_d2_0_sel:			1;
			/**
			bitpos: [[9]]
			 */
			uint32_t data_10bit_d2_1_sel:			1;
			/**
			bitpos: [[10]]
			 */
			uint32_t data_10bit_d2_2_sel:			1;
			/**
			bitpos: [[11]]
			 */
			uint32_t data_10bit_d3_0_sel:			1;
			/**
			bitpos: [[12]]
			 */
			uint32_t data_10bit_d3_1_sel:			1;
			/**
			bitpos: [[13]]
			 */
			uint32_t data_10bit_d3_2_sel:			1;
			uint32_t reserved14:			2;
			/**
			bitpos: [[16]]
			 */
			uint32_t sar_ctrl_reg40_sel:			1;
			/**
			bitpos: [[17]]
			 */
			uint32_t sar_ctrl_reg44_sel:			1;
			/**
			bitpos: [[18]]
			 */
			uint32_t sar_ctrl_reg58_sel:			1;
			/**
			bitpos: [[19]]
			 */
			uint32_t sar_ctrl_reg5c_sel:			1;
			uint32_t reserved20:			12;
		};
		uint32_t val;
	} channel_read;	/* REG_ADC_BASE + 0x84 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint16_t wait_ch0_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[1]]
			 */
			uint16_t wait_ch1_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[2]]
			 */
			uint16_t wait_ch2_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[3]]
			 */
			uint16_t wait_ch3_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[4]]
			 */
			uint16_t wait_ch4_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[5]]
			 */
			uint16_t wait_ch5_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[6]]
			 */
			uint16_t out_wait_end_int_en:			1;
			uint16_t reserved7:			1;
			/**
			bitpos: [[8]]
			 */
			uint16_t ch0_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[9]]
			 */
			uint16_t ch1_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[10]]
			 */
			uint16_t ch2_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[11]]
			 */
			uint16_t ch3_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[12]]
			 */
			uint16_t ch4_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[13]]
			 */
			uint16_t ch5_counter_end_pluse_int_en:			1;
			uint16_t reserved14:			2;
			/**
			bitpos: [[16]]
			*/
		};
		uint16_t val;
	} enable_ctl; /* REG_ADC_BASE + 0x86 */

	union {
		struct {
			/**
			bitpos: [[16]]
			 */
			uint16_t wait_ch0_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[17]]
			 */
			uint16_t wait_ch1_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[18]]
			 */
			uint16_t wait_ch2_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[19]]
			 */
			uint16_t wait_ch3_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[20]]
			 */
			uint16_t wait_ch4_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[21]]
			 */
			uint16_t wait_ch5_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[22]]
			 */
			uint16_t out_wait_state_end_int_st:			1;
			uint16_t reserved23:			1;
			/**
			bitpos: [[24]]
			 */
			uint16_t ch0_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[25]]
			 */
			uint16_t ch1_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[26]]
			 */
			uint16_t ch2_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[27]]
			 */
			uint16_t ch3_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[28]]
			 */
			uint16_t ch4_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[29]]
			 */
			uint16_t ch5_counter_end_pluse_int_st:			1;
			uint16_t eserved30:			2;	
		};
		uint16_t val;
	} status_ctl; /* REG_ADC_BASE + 0x88 */

#if 0
	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint32_t wait_ch0_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[1]]
			 */
			uint32_t wait_ch1_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[2]]
			 */
			uint32_t wait_ch2_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[3]]
			 */
			uint32_t wait_ch3_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[4]]
			 */
			uint32_t wait_ch4_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[5]]
			 */
			uint32_t wait_ch5_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[6]]
			 */
			uint32_t out_wait_state_end_int_en:			1;
			uint32_t reserved7:			1;
			/**
			bitpos: [[8]]
			 */
			uint32_t ch0_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[9]]
			 */
			uint32_t ch1_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[10]]
			 */
			uint32_t ch2_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[11]]
			 */
			uint32_t ch3_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[12]]
			 */
			uint32_t ch4_counter_end_pluse_int_en:			1;
			/**
			bitpos: [[13]]
			 */
			uint32_t ch5_counter_end_pluse_int_en:			1;
			uint32_t reserved14:			2;
			/**
			bitpos: [[16]]
			 */
			uint32_t wait_ch0_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[17]]
			 */
			uint32_t wait_ch1_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[18]]
			 */
			uint32_t wait_ch2_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[19]]
			 */
			uint32_t wait_ch3_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[20]]
			 */
			uint32_t wait_ch4_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[21]]
			 */
			uint32_t wait_ch5_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[22]]
			 */
			uint32_t out_wait_state_end_int_st:			1;
			uint32_t reserved23:			1;
			/**
			bitpos: [[24]]
			 */
			uint32_t ch0_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[25]]
			 */
			uint32_t ch1_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[26]]
			 */
			uint32_t ch2_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[27]]
			 */
			uint32_t ch3_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[28]]
			 */
			uint32_t ch4_counter_end_pluse_int_st:			1;
			/**
			bitpos: [[29]]
			 */
			uint32_t ch5_counter_end_pluse_int_st:			1;
			uint32_t reserved30:			2;
		};
		uint32_t val;
	} int_enable;	/* REG_ADC_BASE + 0x88 */
#endif



	union {
		struct {
			/**
			bitpos: [[1:0]]
			 */
			uint32_t touch_panel_mode_sel:			2;
			/**
			bitpos: [[2]]
			 */
			uint32_t turn_disable:			1;
			/**
			bitpos: [[3]]
			 */
			uint32_t new_arch_sel_driver_inv:			1;
			/**
			bitpos: [[6:4]]
			 */
			uint32_t new_arch_adc8b_sel_reg:			3;
			/**
			bitpos: [[7]]
			 */
			uint32_t new_arch_adc8b_sel_source:			1;
			/**
			bitpos: [[8]]
			 */
			uint32_t new_arch_sw_driver_en_source:			1;
			/**
			bitpos: [[9]]
			 */
			uint32_t new_arch_sw_driver_en_reg:			1;
			/**
			bitpos: [[10]]
			 */
			uint32_t new_arch_sw_driver_stby_reg:			1;
			/**
			bitpos: [[11]]
			 */
			uint32_t new_arch_sw_driver_stby_source:			1;
			/**
			bitpos: [[12]]
			 */
			uint32_t new_arch_sw_driver_sel_source:			1;
			/**
			bitpos: [[13]]
			 */
			uint32_t new_arch_sw_driver_sel_reg:			1;
			/**
			bitpos: [[14]]
			 */
			uint32_t new_arch_adc8b_sel_debug:			1;
			/**
			bitpos: [[15]]
			 */
			uint32_t new_arc_mode_sel:			1;
			/**
			bitpos: [[16]]
			 */
			uint32_t ch_wait_val_0:			1;
			/**
			bitpos: [[17]]
			 */
			uint32_t ch1_wait_val:			1;
			/**
			bitpos: [[18]]
			 */
			uint32_t ch2_wait_val:			1;
			/**
			bitpos: [[19]]
			 */
			uint32_t ch3_wait_val:			1;
			/**
			bitpos: [[20]]
			 */
			uint32_t ch4_wait_val:			1;
			/**
			bitpos: [[21]]
			 */
			uint32_t ch5_wait_val:			1;
			/**
			bitpos: [[23:22]]
			 */
			uint32_t out_pad_data_select_1_0:			2;
			/**
			bitpos: [[24]]
			 */
			uint32_t def_val_dis_ch0:			1;
			/**
			bitpos: [[25]]
			 */
			uint32_t def_val_dis_ch1:			1;
			/**
			bitpos: [[26]]
			 */
			uint32_t def_val_dis_ch2:			1;
			/**
			bitpos: [[27]]
			 */
			uint32_t def_val_dis_ch3:			1;
			/**
			bitpos: [[28]]
			 */
			uint32_t def_val_dis_ch4:			1;
			/**
			bitpos: [[29]]
			 */
			uint32_t def_val_dis_ch5:			1;
			/**
			bitpos: [[31:30]]
			 */
			uint32_t out_pad_data_select_3_2:			2;
		};
		uint32_t val;
	} ctrl_reg;	/* REG_ADC_BASE + 0x8c */

	union {
		struct {
			uint8_t ch:				8;
		};
		uint8_t val;
	}count_end_thr[6];	/* REG_ADC_BASE + 0x90 */

	union {
		struct {
			uint8_t ch:				8;
		};
		uint8_t val;
	}ave_thr[6];	/*REG_ADC_BASE + 0x96 */

	union {
		struct {
			uint8_t ch:				8;
		};
		uint8_t val;
	}def_val[6];	/* REG_ADC_BASE + 0x9c */

	union {
		struct {
			uint8_t ch:				8;
		};
		uint8_t val;
	}cmp_def_val[6];	/* REG_ADC_BASE + 0xa2 */

	union {
		struct {
			uint16_t ch:				16;
		};
		uint16_t val;
	}count_thr[6];		/* REG_ADC_BASE + 0xa8 */

	uint32_t reserved_b4; /* REG_ADC_BASE + 0xb4 */
	uint32_t reserved_b8; /* REG_ADC_BASE + 0xb8 */
	uint32_t reserved_bc; /* REG_ADC_BASE + 0xbc */

	union {
		struct {
			uint8_t ch:				8;
		};
		uint8_t val;
	}read_data[8];		/* REG_ADC_BASE + 0xc0 */
} adc_reg_t;

typedef volatile struct adc_wait_reg{
	union 
	{
		struct 
		{
			uint8_t wait_ch_ave_counter: 	8;
		};
		uint8_t val;
	}new_saradc_ch_ctrl[6];	/* 0x1880 _ 0x160 */
	

	uint16_t reserved_0;

	union 
	{
		struct 
		{
			uint8_t wait_ch_counter_threshold: 	8;
			uint8_t reserved_0: 	8;
		};
		uint16_t val;
	}new_saradc_wait_ch_counter_threshold[6];	/* 0x1880 _ 0x168 */

}adc_wait_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __ADC_REG_STRUCT_H__ */
