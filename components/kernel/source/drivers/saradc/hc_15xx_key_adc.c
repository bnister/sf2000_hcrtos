#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include <stdio.h>

#include "adc_15xx_reg_struct.h"
#include "hc_15xx_key_adc.h"
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>

#define TIME_TO_WORK	25

static void hc_key_adc_up(struct timer_list *param)
{
	struct adc_priv_1512 *priv = from_timer(priv, param, timer_keyup);
	input_report_key(priv->input, priv->key_code, 0);
	input_sync(priv->input);
	priv->key_state = 0;

	return;
}

static void hc_key_adc_down(struct adc_priv_1512 *priv)
{
	priv->timeout = 200000;
	if (priv->key_state == 0) {
		input_event(priv->input, EV_MSC, MSC_SCAN, priv->key_code);
		input_report_key(priv->input, priv->key_code, 1);
		priv->key_state = 1;
	}
	if (priv->key_state == 1) {
		input_event(priv->input, EV_MSC, MSC_SCAN, priv->key_code);
		input_sync(priv->input);
	}

	priv->key_up_val = priv->key_down_val;
	priv->keyup_jiffies = jiffies + usecs_to_jiffies(priv->timeout);
	mod_timer(&priv->timer_keyup, priv->keyup_jiffies);
	return;
}

static int hc_key_adc_val_detection(struct adc_priv_1512 *priv)
{
	int i, ret;
	uint32_t down_val = priv->key_down_val * 2000 / 255;
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	down_val = priv->key_down_val * priv->dts_refer_value / priv->dyn_adjust;
	for (i = 0; i < priv->keymap_len; i++) {
		if (down_val >= priv->key_map[i].key_min_val &&
		    down_val <= priv->key_map[i].key_max_val) {
			priv->val_cmp = 0;
			if (i == priv->down_code_st)
				priv->down_val_st++;
			else {
				priv->down_code_st = i;
				priv->down_val_st = 0;
			}
			break;
		}
	}

	if (i >= priv->keymap_len)
		priv->val_cmp++;

	if (priv->val_cmp > 10) {
		priv->val_cmp = 0;
		reg->sar_ctrl_reg1.sar_en = 0x00;
		priv->dyn_adjust =
		reg->sar_ctrl_reg2.default_value = reg->sar_ctrl_reg0.sar_dout;
		if (priv->dyn_adjust == 0 && priv->flash_adjust == 0)
			priv->dyn_adjust = 1;
		reg->sar_ctrl_reg1.sar_en = 0x01;
	}

	if (priv->down_val_st > 1) {
		priv->down_val_st = 0;
		ret = priv->down_code_st;
		priv->down_code_st = -1;
		priv->key_code = priv->key_map[i].key_code;

		return ret;
	} else {
		return -1;
	}

	return ret;
}

static void dyn_adjust_train_work(void *param)
{
	struct adc_priv_1512 *priv = (struct adc_priv_1512 *)param;
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	int16_t value = 0;

	reg->sar_ctrl_reg0.sar_int_en = 0x00;
	reg->sar_ctrl_reg1.repeat_num = 0x00;

	if (priv->work_first_read == 0) {
		work_queue(HPWORK, &priv->work, dyn_adjust_train_work, (void *)priv, TIME_TO_WORK);
		priv->work_first_read = 1;
		return;
	}
	if (reg->sar_ctrl_reg0.sar_int_st == 0x00) {
		value = (int16_t)reg->sar_ctrl_reg0.sar_dout;

		/* Now it is no key pressed at this moment */
		if (priv->dyn_adjust_temp == 0) {
			priv->dyn_adjust_temp = value;
			//priv->work_first_read = 0;
			work_queue(HPWORK, &priv->work, dyn_adjust_train_work, (void *)priv, TIME_TO_WORK);
			return;
		}
		if (abs(value - priv->dyn_adjust_temp) > 2) {
			priv->dyn_adjust_temp = value;
			priv->work_first_read = 0;
			work_queue(HPWORK, &priv->work, dyn_adjust_train_work, (void *)priv, TIME_TO_WORK);
			return;
		}

		/* training finished */
		priv->dyn_adjust = (uint8_t)value;

		if (priv->store_adjust) {
			int16_t tmp = (int16_t)priv->flash_adjust;
			if (abs(value - tmp) > 2) {
				/* new trained adjustment, update to flash */
				sys_set_sysdata_adc_adjust_value(priv->dyn_adjust);
				reg->sar_ctrl_reg1.repeat_num = 0x07;
			}
		}
		priv->dyn_adjust_trained = 1;
	}
	reg->sar_ctrl_reg1.repeat_num = 0x07;
	reg->sar_ctrl_reg0.sar_int_en = 0x01;
}

static void hc_1512_saradc_interrupt(uint32_t param)
{
	struct adc_priv_1512 *priv = (struct adc_priv_1512 *)param;
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	reg->sar_ctrl_reg0.sar_int_en = 0x00;
	priv->key_down_val = reg->sar_ctrl_reg0.sar_dout;

	if (priv->flash_adjust != 0 || priv->dyn_adjust_trained != 0 ||reg->sar_ctrl_reg1.repeat_num != 0 ) {
		if (hc_key_adc_val_detection(priv) >= 0) {
			hc_key_adc_down(priv);
		}
	}
	reg->sar_ctrl_reg0.sar_int_en = 0x01;
	reg->sar_ctrl_reg0.sar_int_st = 0x01;

	if (priv->dyn_adjust_trained == 0 && priv->down_code_st != -1) {
		work_queue(HPWORK, &priv->work, dyn_adjust_train_work, (void *)priv, TIME_TO_WORK);
	}
}

static int hc_key_adc_open(struct input_dev *dev)
{
	struct adc_priv_1512 *priv = input_get_drvdata(dev);
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	reg->sar_ctrl_reg0.sar_int_en = 0x01;
	reg->sar_ctrl_reg1.sar_en = 0x01;

	return 0;
}

static void hc_key_adc_close(struct input_dev *dev)
{
	struct adc_priv_1512 *priv = input_get_drvdata(dev);
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;
	reg->sar_ctrl_reg0.sar_int_en = 0x00;
	reg->sar_ctrl_reg1.sar_en = 0x00;
	return;
}

static int hc_1512_saradc_init(struct adc_priv_1512 *priv)
{
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	priv->down_code_st = -1;

	priv->input->open = hc_key_adc_open;
	priv->input->close = hc_key_adc_close;

	timer_setup(&priv->timer_keyup, hc_key_adc_up, 0);

	reg->sar_ctrl_reg3.val = 0x00;

	reg->sar_ctrl_reg2.debounce_value = 0x00;
	reg->sar_ctrl_reg2.default_value = 0x00;

	reg->sar_ctrl_reg3.sar_clk_sel = 0x01;

	reg->sar_ctrl_reg1.repeat_num = 0x03;
	reg->sar_ctrl_reg1.clk_div_num = 0x0f;
	reg->sar_ctrl_reg1.sar_en = 0x01;

	usleep(20000);

	reg->sar_ctrl_reg0.sar_int_en = 0x0;

	reg->sar_ctrl_reg2.debounce_value = 0x05;
	reg->sar_ctrl_reg1.repeat_num = 0x07;
	reg->sar_ctrl_reg1.clk_div_num = 0xff;

	reg->sar_ctrl_reg1.sar_en = 0x00;

	priv->key_down_val = 0x00;
	priv->key_up_val = 0x00;
	priv->val_cmp = 0x00;

	return 0;
}

static void hc_key_adc_adjust_init(struct adc_priv_1512 *priv)
{
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	if (!sys_get_sysdata_adc_adjust_value(&priv->flash_adjust)) {
		priv->dyn_adjust = priv->flash_adjust;
		priv->store_adjust = 1;
	}
	if (priv->dyn_adjust != 0) {
		reg->sar_ctrl_reg2.default_value = priv->dyn_adjust;
	} else  {
		reg->sar_ctrl_reg1.sar_en = 0x01;
		usleep(1000);
		reg->sar_ctrl_reg2.default_value = reg->sar_ctrl_reg0.sar_dout;
		usleep(1000);
		priv->dyn_adjust = reg->sar_ctrl_reg0.sar_dout;
		if (priv->dyn_adjust == 0)
			priv->dyn_adjust = 1;
		reg->sar_ctrl_reg1.sar_en = 0x00;
	}
}

static int hc_1512_saradc_probe(char *node)
{
	int np;
	int ret = -EINVAL;
	struct adc_priv_1512 *priv;

	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		printf("no found %s", node);
		return 0;
	}

	priv = kzalloc(sizeof(struct adc_priv_1512), GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}

	priv->input = input_allocate_device();
	if (!priv->input)
		goto err;

	priv->irq = (int)&SAR_ADC_INTR;
	if (priv->irq < 0) {
		ret = priv->irq;
		goto err;
	}

	priv->base = (void *)&ADCCTRL;

	ret = fdt_get_property_u_32_index(np, "key-num", 0, &priv->keymap_len);
	if (ret < 0) {
		printf("can't get property key-num\n");
	}
	priv->key_map = (hc_adckey_map_s *)malloc(sizeof(hc_adckey_map_s) *
						  priv->keymap_len);
	ret = fdt_get_property_u_32_array(np, "key-map", (u32 *)priv->key_map,
					  priv->keymap_len * 3);
	if (ret < 0) {
		printf("\n\ncan't get ke-map\n\n");
	}

	ret = fdt_get_property_u_32_index(np, "adc_ref_voltage", 0, &priv->dts_refer_value);
	if (ret < 0) {
		printf("can't get property adc_ref_voltage\n");
	}
	if (priv->dts_refer_value > 2000) /*mv*/
		priv->dts_refer_value = 2000;

	ret = hc_1512_saradc_init(priv);
	if (ret < 0) {
		goto err;
	}
	hc_key_adc_adjust_init(priv);

	input_set_drvdata(priv->input, priv);
	ret = input_register_device(priv->input);
	xPortInterruptInstallISR(priv->irq, hc_1512_saradc_interrupt,
				 (uint32_t)priv);
	return 0;

err:
	input_free_device(priv->input);
	kfree(priv);

	return ret;
}

static int hc_1512_saradc_driver_init(void)
{
	int ret = 0;

	ret = hc_1512_saradc_probe("/hcrtos/adc");

	return ret;
}

module_driver(saradc_module, hc_1512_saradc_driver_init, NULL, 1)
