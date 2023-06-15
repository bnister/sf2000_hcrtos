#ifndef _HC_KEY_ADC_H_
#define _HC_KEY_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/io.h>
#include <linux/timer.h>

typedef struct {
	uint32_t key_min_val;
	uint32_t key_max_val;
	uint32_t key_code;
} hc_adckey_map_s;

struct hc_key_adc_priv {
	struct input_dev 		*input;
	struct device			*dev;
	int				irq;
	void __iomem			*base;
	int				ch_id;

	int				key_code;
	int 				key_down_val;
	int				key_up_val;
	int				key_num;
	int				key_down_flag;		
	int				key_up_flag;

	unsigned long			keyup_jiffies;
	uint32_t			timeout;

	struct timer_list		timer_keyup;	
	int				keymap_len;
	hc_adckey_map_s			*key_map;

	int				down_val_st;
	int				down_code_st;

	int 				def_val;

	int				key_state;	/* 0: key down, 1: repead key down */
	int				val_cmp;
	uint32_t 			deviation;
	uint8_t				efuse_adjust;	/* 0 : invalid, others : valid, high priority */
	uint8_t 			flash_adjust;	/* 0 : invalid, others : valid, low priority */
	uint8_t 			dyn_adjust;	/* 0 : invalid, others : valid, low priority */
	uint8_t				dyn_adjust_trained;	/* train the dynamic adjust each time power on */
	uint8_t 			store_adjust;	/* 0 : invalid, others : valid, low priority */
	int16_t 			dyn_adjust_temp;
	int 				dts_refer_value;
	struct work_s			work;
};

extern void get_map_info(struct hc_key_adc_priv *priv);

#ifdef __cplusplus
}
#endif

#endif /* _HC_KEY_ADC_H_ */
