#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include "adc_reg_struct.h"

#include "hc_touch_adc.h"

#include <kernel/ld.h>
#include <linux/io.h>
#include <stdio.h>


int current_hz = 0;

struct touch_adc_param ch0_param = {0};
struct touch_adc_param ch1_param = {0};

void deal_data(struct touch_adc_priv *param) {

	struct touch_adc_priv *priv = param;

	static uint32_t ch0_diff0 = 0,ch0_diff1 = 0;
	static uint32_t ch1_diff0 = 0,ch1_diff1 = 0;
	int x,y ;

	if ((ch1_param.num == ch_buf_len)&&(ch0_param.num == ch_buf_len))
	{	
		ch0_diff0 = abs(ch0_param.ch_buf[0]-ch0_param.ch_buf[1]);
		ch0_diff1 = abs(ch0_param.ch_buf[1]-ch0_param.ch_buf[2]);
		ch1_diff0 = abs(ch1_param.ch_buf[0]-ch1_param.ch_buf[1]);
		ch1_diff1 = abs(ch1_param.ch_buf[1]-ch1_param.ch_buf[2]);

		if((ch0_diff0<2)&&(ch0_diff1<2)&&(ch1_diff0<2)&&(ch1_diff1<2))
		{
			x = (ch1_param.ch_buf[(ch_buf_len -1)/2] * x_len)/205;
			y = (ch0_param.ch_buf[(ch_buf_len -1)/2] * y_len)/217;
			
			ch0_param.num 	= 0;
			ch1_param.num 	= 0;
			// printf("(%4d %4d)\n",x,y);			
			input_report_abs(priv->input,ABS_X,x);
			input_report_abs(priv->input,ABS_Y,y);
			input_sync(priv->input);
		}
		else
		{
			ch0_param.num 	= 0;
			ch1_param.num 	= 0;			
		}
	}	
}

static void hc_touch_adc_interrupt(uint32_t param) {
	
	struct touch_adc_priv *priv = (struct touch_adc_priv *)param;
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	uint8_t adc_val;
	uint16_t irq_status;

	irq_status = reg->status_ctl.val;

	if((irq_status & CH0_1_CEPI_ST))
	{
		adc_val = reg->read_data[1].val;
		if(ch1_param.num < ch_buf_len)
		{
			ch1_param.ch_buf[ch1_param.num++] = adc_val;
		}

		adc_val = reg->read_data[0].val;
		if(ch0_param.num < ch_buf_len)
		{
			ch0_param.ch_buf[ch0_param.num++] = adc_val;
		}
		deal_data(priv);
	}

	reg->status_ctl.val |= 0x3f;
}

static int touch_adc_open(struct input_dev *dev)
{
	struct touch_adc_priv *priv = input_get_drvdata(dev);
	adc_reg_t *reg = (adc_reg_t *)priv->base;

	reg->saradc_en.sar_en = 0x01;

	return 0;
}

static void touch_adc_close(struct input_dev *dev)
{
	struct touch_adc_priv *priv = input_get_drvdata(dev);
	adc_reg_t *reg = (adc_reg_t *)priv->base;

	reg->saradc_en.sar_en = 0x00;

	return ;
}

static int touch_adc_ioctl(struct file *filep, int cmd, unsigned long arg)
{

	adc_reg_t *reg = (adc_reg_t *)&ADCCTRL;

	switch (cmd) {
		case SET_CLK_200_HZ:
			reg->saradc_en.sar_en = 0x00;
			reg->saradc_ctl.sar_clksel_core 			= 0x02;
			reg->saradc_ctl.sar_clksel_core_div			= 0x1E;
			reg->saradc_en.sar_en = 0x01;
			current_hz = 200;
			break;
		case SET_CLK_250_HZ:
			reg->saradc_en.sar_en = 0x00;
			reg->saradc_ctl.sar_clksel_core 			= 0x04;
			reg->saradc_ctl.sar_clksel_core_div			= 0x32;
			reg->saradc_en.sar_en = 0x01;
			current_hz = 250;
			break;
		case SET_CLK_300_HZ:
			reg->saradc_en.sar_en = 0x00;
			reg->saradc_ctl.sar_clksel_core 			= 0x03;
			reg->saradc_ctl.sar_clksel_core_div			= 0x34;
			reg->saradc_en.sar_en = 0x01;
			current_hz = 300;
			break;
		case SET_CLK_350_HZ:
			reg->saradc_en.sar_en = 0x00;
			reg->saradc_ctl.sar_clksel_core 			= 0x05;
			reg->saradc_ctl.sar_clksel_core_div			= 0x58;
			reg->saradc_en.sar_en = 0x01;
			current_hz = 350;
			break;
		case SET_CLK_400_HZ:
			reg->saradc_en.sar_en = 0x00;
			reg->saradc_ctl.sar_clksel_core 			= 0x06;
			reg->saradc_ctl.sar_clksel_core_div			= 0x9c;
			reg->saradc_en.sar_en = 0x01;
			current_hz = 400;
			break;
		case GET_CURRENT_CLK_HZ:
			memcpy((int*)arg,&current_hz,sizeof(int));
			break;
		default:
			break;
	}

	return 0;
}

static void touch_adc_init(struct touch_adc_priv *priv)
{
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	adc_wait_reg_t *reg_wait = (adc_wait_reg_t *)priv->base_wait;

	priv->input->open 	= touch_adc_open;
	priv->input->close 	= touch_adc_close;
	priv->input->ioctl 	= touch_adc_ioctl;

	for (int i = 0; i < ch_num; i++)
	{
		reg->count_end_thr[priv->ch_id[i]].ch			= 0x01;
		reg->ave_thr[priv->ch_id[i]].ch					= 0x00;
		reg->ave_thr[priv->ch_id[i]].ch 				= 0x00;
		reg->def_val[priv->ch_id[i]].ch 				= 0xdc;
		reg->cmp_def_val[priv->ch_id[i]].ch 			= 0x08;
		reg->count_thr[priv->ch_id[i]].ch				= 0x1f;
		reg_wait->new_saradc_ch_ctrl[priv->ch_id[i]].wait_ch_ave_counter = 0x10;
		reg_wait->new_saradc_wait_ch_counter_threshold[priv->ch_id[i]].wait_ch_counter_threshold = 0x20;
	}

	reg->enable_ctl.ch0_counter_end_pluse_int_en = 0x01;
	reg->enable_ctl.ch1_counter_end_pluse_int_en = 0x01;
	reg->enable_ctl.ch2_counter_end_pluse_int_en = 0x01;
	reg->enable_ctl.ch3_counter_end_pluse_int_en = 0x01;

	reg->ctrl_reg.touch_panel_mode_sel 	= 0x01;
	reg->ctrl_reg.new_arc_mode_sel		= 0x01;
	reg->ctrl_reg.ch_wait_val_0			= 0x01;

	reg->saradc_ctl.saradc_pwd					= 0x00;
	reg->saradc_ctl.adc8b_sw_ldo_en_core 		= 0x01;
	reg->saradc_ctl.adc8b_sw_ldo_vout_sel_core 	= 0x00;
	reg->saradc_ctl.sar_clksel_core 			= 0x03;
	reg->saradc_ctl.sar_clksel_core_div			= 0x34;

	current_hz = 300;

	// reg->saradc_en.sar_en = 0x01;

}

static int touch_adc_probe(char *node)
{
	int np;
	int ret = -EINVAL;
	struct touch_adc_priv *priv;
	
	np = fdt_get_node_offset_by_path(node);
	if (np < 0)
	{
		printf("no find touch_adc in dts\n");
		return -ENOMEM;
	}

	priv = kzalloc(sizeof(struct touch_adc_priv),GFP_KERNEL);
	if (!priv)
	{
		return -ENOMEM;
	}

	priv->input = input_allocate_device();
	if(!priv->input)
	{
		goto err;
	}
	
	priv->irq = (int)&SAR_ADC_INTR;
	if (priv->irq < 0)
	{
		ret = priv->irq;
		goto err;
	}

	priv->base 		= (void*)&ADCCTRL;
	priv->base_wait = (void *)(0xb8800160);

	ret = fdt_get_property_u_32_array(np,"channel_id",priv->ch_id,ch_num);
	if (ret < 0)
	{
		printf("no find channel id\n");
		goto err;
	}

	touch_adc_init(priv);

	input_set_drvdata(priv->input,priv);

	ret = input_register_device(priv->input);
	if (ret < 0)
	{
		printf("input register fail\n");
	}
	
	xPortInterruptInstallISR(priv->irq,hc_touch_adc_interrupt,(uint32_t)priv);

	return 0;

err:
	input_free_device(priv->input);
	kfree(priv);

	return ret;
}

static int touch_adc_driver_init(void)
{
	int ret;
	ret = touch_adc_probe("/hcrtos/touch-adc");
	return ret;
}

module_driver(hc_touch_adc, touch_adc_driver_init, NULL, 1)
