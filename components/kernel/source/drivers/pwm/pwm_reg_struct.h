#ifndef __PWM_REG_STRUCT_H__
#define __PWM_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct pwm_reg {

	union {
		struct {
			uint32_t reserved0:			16;
			/**
			bitpos: [[29:16]]
			 */
			uint32_t pwm_clksel:			14;
			/**
			bitpos: [[30]]
			 */
			uint32_t pwm_clken:			1;
			/**
			bitpos: [[31]]
			 */
			uint32_t pwm_enable:			1;
		};
		uint32_t val;
	} clk_ctrl;	/* REG_PWM_BASE + 0x0 */

#ifdef CONFIG_SOC_HC16XX
	union {
		struct {
			uint8_t reserved0:			4;
			/**
			bitpos: [[4]]
			 */
			uint8_t polarity:			1;
			uint8_t reserved5:			2;
			/**
			bitpos: [[7]]
			 */
			uint8_t channel_en:			1;
		};
		uint8_t val;
	} channel_ctrl[6];	/* REG_PWM_BASE + 0x4 */

	uint8_t reserved_5; /* REG_PWM_BASE + 0xa */
	uint8_t reserved_6; /* REG_PWM_BASE + 0xb */

	union {
		struct {
			/**
			bitpos: [[15:0]]
			 */
			uint32_t low_level_counter:			16;
			/**
			bitpos: [[31:16]]
			 */
			uint32_t high_level_counter:			16;
		};
		uint32_t val;
	} divder_ctrl[6];	/* REG_PWM_BASE + 0xc */

#elif defined(CONFIG_SOC_HC15XX)
union {
		struct {
			uint32_t low_level_counter:		16;
			uint32_t high_level_counter:            16;
			uint32_t reserved0:			4;
			uint32_t polarity:			1;
			uint32_t reserved5:			2;
			uint32_t channel_en:			1;
			uint32_t reserved8:			24;
		};
		uint32_t val[2];
	} divder_ctrl[6];



#endif
} pwm_reg_t;


#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __PWM_REG_STRUCT_H__ */
