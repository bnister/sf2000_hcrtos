#ifndef _HC_KEY_ADC_H_
#define _HC_KEY_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/io.h>
#include <linux/timer.h>
/*base addr 0xb8818400*/

typedef struct {
	uint32_t key_min_val;
	uint32_t key_max_val;
	uint32_t key_code;
} hc_adckey_map_s;

struct adc_priv_1512
{
	struct input_dev 		*input;
	struct device 			*dev;
	int 				irq;
	void __iomem 			*base;

	int 				key_code;
	int 				key_down_val;

	int 				keymap_len;
	hc_adckey_map_s 		*key_map;

	int 				down_val_st;
	int 				down_code_st;

	unsigned long 			keyup_jiffies;
	uint32_t 			timeout;

	struct timer_list 		timer_keyup;
	int				key_up_val;

	uint 				val_cmp;

	unsigned int 			def_key_value;
	int				key_state;	/* 0: key down, 1: repead key down */
	uint32_t 			deviation;
	uint8_t				efuse_adjust;	/* 0 : invalid, others : valid, high priority */
	uint8_t 			flash_adjust;	/* 0 : invalid, others : valid, low priority */
	uint8_t 			dyn_adjust;	/* 0 : invalid, others : valid, low priority */
	uint8_t				dyn_adjust_trained;	/* train the dynamic adjust each time power on */
	uint8_t 			store_adjust;	/* 0 : invalid, others : valid, low priority */
	int16_t 			dyn_adjust_temp;
	int 				dts_refer_value;
	struct work_s			work;
	int				work_first_read;
};

#ifdef __cplusplus
}
#endif

#endif /* _HC_KEY_ADC_H_ */
