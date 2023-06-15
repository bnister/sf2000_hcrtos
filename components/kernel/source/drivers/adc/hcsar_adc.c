/****************************************************************************
 * /hcsar_adc.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "sar_adc"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <kernel/completion.h>
#include <kernel/list.h>
#include <kernel/soc/soc_common.h>
#include <nuttx/wqueue.h>
#include <hcuapi/pinmux.h>
#include <kernel/lib/fdt_api.h>
#include <generated/br2_autoconf.h>
#include <kernel/elog.h>


#include "adc.h"
#include "sar_adc.h"





#ifdef CONFIG_HCHIP_ADC

/* Some ADC peripheral must be enabled */

#if defined(CONFIG_HCHIP_ADC1) || defined(CONFIG_IMXRT_ADC2)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ADC_MAX_CHANNELS  16

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct hcsar_dev_s
{
  FAR const struct adc_callback_s *cb;  /* Upper driver callback */
  uint8_t  intf;                        /* ADC number (i.e. ADC1, ADC2) */
  uint32_t base;                        /* ADC register base */
  uint8_t  initialized;                 /* ADC initialization counter */
  int      irq;                         /* ADC IRQ number */
  int      nchannels;                   /* Number of configured ADC channels */
  uint8_t  chanlist[ADC_MAX_CHANNELS];  /* ADC channel list */
  uint8_t  current;                     /* Current channel being converted */
  uint8_t clk_sel;
  uint8_t clk_div;//
  uint8_t repeat_num;
  uint8_t jitter_threshold;//offset
  uint8_t init_val; //adc³õÊ¼²Î¿¼Öµ
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void adc_putreg(FAR struct hcsar_dev_s *priv, uint32_t offset,
                       uint32_t value);
static uint32_t adc_getreg(FAR struct hcsar_dev_s *priv, uint32_t offset);
static void adc_modifyreg(FAR struct hcsar_dev_s *priv, uint32_t offset,
                          uint32_t clearbits, uint32_t setbits);

/* ADC methods */

static int  adc_bind(FAR struct adc_dev_s *dev,
                     FAR const struct adc_callback_s *callback);
static void adc_reset(FAR struct adc_dev_s *dev);
static int  adc_setup(FAR struct adc_dev_s *dev);
static void adc_shutdown(FAR struct adc_dev_s *dev);
static void adc_rxint(FAR struct adc_dev_s *dev, bool enable);
static int  adc_ioctl(FAR struct adc_dev_s *dev, int cmd, unsigned long arg);
static void  adc_interrupt(uint32_t arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/
extern unsigned long SAR_ADC_INTR;
extern unsigned long SARADC_BASE;

static const struct adc_ops_s g_adcops =
{
  .ao_bind     = adc_bind,
  .ao_reset    = adc_reset,
  .ao_setup    = adc_setup,
  .ao_shutdown = adc_shutdown,
  .ao_rxint    = adc_rxint,
  .ao_ioctl    = adc_ioctl,
};

#ifdef CONFIG_HCHIP_ADC1
static struct hcsar_dev_s g_adcpriv1 =
{
  .irq         = 45,//SAR_ADC_INTR,
  .intf        = 1,
  .initialized = 0,
  .base        = 0xb8818400,//SARADC_BASE,
  .init_val   = 248,
  .clk_div   = 0xff,
  .repeat_num = 0xf,
  .jitter_threshold = 0xf,
  .clk_sel    =0,
};

static struct adc_dev_s g_adcdev1 =
{
  .ad_ops      = &g_adcops,
  .ad_priv     = &g_adcpriv1,
};
/*
gpio_pinset_t g_adcpinlist1[ADC_MAX_CHANNELS] =
{
    GPIO_ADC1_CH0,
    GPIO_ADC1_CH1,
    GPIO_ADC1_CH2,
    GPIO_ADC1_CH3,
    GPIO_ADC1_CH4,
    GPIO_ADC1_CH5,
    GPIO_ADC1_CH6,
    GPIO_ADC1_CH7,
    GPIO_ADC1_CH8,
    GPIO_ADC1_CH9,
    GPIO_ADC1_CH10,
    GPIO_ADC1_CH11,
    GPIO_ADC1_CH12,
    GPIO_ADC1_CH13,
    GPIO_ADC1_CH14,
    GPIO_ADC1_CH15,
 
};*/   
#endif

#ifdef CONFIG_IMXRT_ADC2
static struct hcsar_dev_s g_adcpriv2 =
{
  .irq         = IMXRT_IRQ_ADC2,
  .intf        = 2,
  .initialized = 0,
  .base        = IMXRT_ADC2_BASE,
};

static struct adc_dev_s g_adcdev2 =
{
  .ad_ops      = &g_adcops,
  .ad_priv     = &g_adcpriv2,
};

gpio_pinset_t g_adcpinlist2[ADC_MAX_CHANNELS] =
{
    GPIO_ADC2_CH0,
    GPIO_ADC2_CH1,
    GPIO_ADC2_CH2,
    GPIO_ADC2_CH3,
    GPIO_ADC2_CH4,
    GPIO_ADC2_CH5,
    GPIO_ADC2_CH6,
    GPIO_ADC2_CH7,
    GPIO_ADC2_CH8,
    GPIO_ADC2_CH9,
    GPIO_ADC2_CH10,
    GPIO_ADC2_CH11,
    GPIO_ADC2_CH12,
    GPIO_ADC2_CH13,
    GPIO_ADC2_CH14,
    GPIO_ADC2_CH15,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void adc_putreg(FAR struct hcsar_dev_s *priv, uint32_t offset,
                       uint32_t value)
{
  REG32_WRITE(priv->base + offset, value);
}

static uint32_t adc_getreg(FAR struct hcsar_dev_s *priv, uint32_t offset)
{
  return REG32_READ(priv->base + offset);//getreg32(priv->base + offset);
}

static void adc_modifyreg(FAR struct hcsar_dev_s *priv, uint32_t offset,
                          uint32_t clearbits, uint32_t setbits)
{
  //modifyreg32(priv->base + offset, clearbits, setbits);
  
}
static void sar_adc_set_repeat_num(struct adc_dev_s * dev,uint8_t repeat_num)
{ 
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG1);
	set_val = (reg_val&(~(SAR_REPEAT_NUM_MASK<<SAR_REPEAT_NUM_BIT)))|((repeat_num&(SAR_REPEAT_NUM_MASK))<<SAR_REPEAT_NUM_BIT);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG1,set_val);
}

static void sar_adc_set_debounce(struct adc_dev_s * dev,uint8_t debounce_val)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG2);
	set_val = (reg_val&(~(SAR_DEBOUNCE_VALUE_MASK<<SAR_DEBOUNCE_VALUE_BIT)))|((debounce_val&(SAR_DEBOUNCE_VALUE_MASK))<<SAR_DEBOUNCE_VALUE_BIT);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG2,set_val);
}

static void sar_adc_set_clk_sel(struct adc_dev_s * dev,uint8_t clk_sel)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG3);
	set_val = (reg_val&(~(SAR_CLK_SEL_MASK<<SAR_CLK_SEL_BIT)))|((clk_sel&(SAR_CLK_SEL_MASK))<<SAR_CLK_SEL_BIT);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG3,set_val);
}


static void sar_adc_set_clk_div(struct adc_dev_s * dev,uint8_t clk_div)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG1);
	set_val = (reg_val&(~(SAR_CLK_DIV_NUM_MASK<<SAR_CLK_DIV_NUM_BIT)))|((clk_div&(SAR_CLK_DIV_NUM_MASK))<<SAR_CLK_DIV_NUM_BIT);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG1,set_val);
}

static void sar_adc_set_default_value(struct adc_dev_s * dev,uint8_t default_val)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG2);
	set_val = (reg_val&(~(SAR_DEFAULT_VALUE_MASK<<SAR_DEFAULT_VALUE_BIT)))|((default_val&(SAR_DEFAULT_VALUE_MASK))<<SAR_DEFAULT_VALUE_BIT);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG2,set_val);
}

u32 gadc_data = 0;
static uint32_t get_sar_adc_val(struct adc_dev_s * dev)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG0);
	gadc_data = (reg_val>>SAR_DOUT_BIT)&SAR_DOUT_MASK;
	//log_i("[%s][%d]:ret %d",__FUNCTION__,__LINE__,ret);
	return (reg_val>>SAR_DOUT_BIT)&SAR_DOUT_MASK;//ret;
}

static uint8_t clear_sar_adc_int_status(struct adc_dev_s * dev)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG0);
	set_val = (reg_val&(~(SAR_INT_ST_MASK<<SAR_INT_ST_BIT)))|((1&(SAR_INT_ST_MASK))<<SAR_INT_ST_BIT);
	//log_i("[%s][%d]\n",__FUNCTION__,__LINE__);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG0,set_val);
}

static uint8_t get_sar_adc_int_status(struct adc_dev_s * dev)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG0);
	return (reg_val>>SAR_INT_ST_BIT)&(SAR_INT_ST_MASK);
}

uint32_t sar_adc_return_val(struct adc_dev_s * dev)
{
	return get_sar_adc_val(dev);
}


/****************************************************************************
 * Name: adc_bind
 *
 * Description:
 *   Bind the upper-half driver callbacks to the lower-half implementation.
 *   This must be called early in order to receive ADC event notifications.
 *
 ****************************************************************************/

static int adc_bind(FAR struct adc_dev_s *dev,
                    FAR const struct adc_callback_s *callback)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;

	DEBUGASSERT(priv != NULL);
	priv->cb = callback;
	return OK;
}

/****************************************************************************
 * Name: adc_reset
 *
 * Description:
 *   Reset the ADC device.  Called early to initialize the hardware. This
 *   is called, before adc_setup() and on error conditions.
 *
 ****************************************************************************/

static void adc_reset(FAR struct adc_dev_s *dev)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;

	//log_i("%s enter\n",__FUNCTION__);
	portENTER_CRITICAL();

	if (priv->initialized > 0)
	{
		goto exit_leave_critical;
	}
	switch (priv->intf)
	{
#ifdef CONFIG_HCHIP_ADC1
		case 1:
			//imxrt_clockall_adc1();
			break;
#endif
		default:
			log_e("ERROR: Tried to reset non-existing ADC: %d\n", priv->intf);
			goto exit_leave_critical;
	}
	//portEXIT_CRITICAL();

	uint32_t reg_val = 0;
	uint32_t set_val = 0;

	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG3);
	set_val = (reg_val&(~(SAR_PWD_CORE_MASK<<SAR_PWD_CORE_BIT)))|((0&(SAR_PWD_CORE_MASK))<<SAR_PWD_CORE_BIT);
	//log_i("[%s][%d]:sar_init_val=%d\n",__FUNCTION__,__LINE__,priv->init_val);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG3,set_val);
#if 0
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG1);
	set_val = (reg_val&(~(SAR_EN_MASK<<SAR_EN_BIT)))|((1)<<SAR_EN_BIT);
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG1,set_val);
#endif
	//sar_adc_set_debounce(dev,0xf);
	//sar_adc_set_jitter_threshold(dev,0xf);
	//sar_adc_set_clk_sel(dev,0);
	sar_adc_set_default_value(dev,priv->init_val);
	sar_adc_set_clk_div(dev,priv->clk_div);
	sar_adc_set_repeat_num(dev,priv->repeat_num);//sar_adc_set_debounce(dev,adc_config->repeat_num);
	sar_adc_set_debounce(dev,priv->jitter_threshold);
	//sar_adc_set_jitter_threshold(dev,adc_config->jitter_threshold);
	sar_adc_set_clk_sel(dev,priv->clk_sel);


exit_leave_critical:
	portEXIT_CRITICAL();
	return;

}

/****************************************************************************
 * Name: adc_setup
 *
 * Description:
 *   Configure the ADC. This method is called the first time that the ADC
 *   device is opened.  This will occur when the port is first opened.
 *   This setup includes configuring and attaching ADC interrupts.
 *   Interrupts are all disabled upon return.
 *
 ****************************************************************************/

static int adc_setup(FAR struct adc_dev_s *dev)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	int ret = OK;
	/* Do nothing when the ADC device is already set up */
	if (priv->initialized > 0)
	{
		return OK;
	}
	log_i(" register ISR\n");

	priv->initialized++;
	xPortInterruptInstallISR(priv->irq, adc_interrupt,(uint32_t)dev);
	priv->current = 0;

	// set ADC channel?  to do here...
#if 0
	int ret = irq_attach(priv->irq, adc_interrupt, dev);
	if (ret < 0)
	{
		ainfo("irq_attach failed: %d\n", ret);
		return ret;
	}

	up_enable_irq(priv->irq);

	/* Start the first conversion */

	priv->current = 0;
	adc_putreg(priv, IMXRT_ADC_HC0_OFFSET,
			ADC_HC_ADCH(priv->chanlist[priv->current]));
#endif
	return ret;
}

/****************************************************************************
 * Name: adc_shutdown
 *
 * Description:
 *   Disable the ADC.  This method is called when the ADC device is closed.
 *   This method reverses the operation the setup method.
 *
 ****************************************************************************/

static void adc_shutdown(FAR struct adc_dev_s *dev)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;

	/* Shutdown the ADC device only when not in use */

	priv->initialized--;

	if (priv->initialized > 0)
	{
		return;
	}

	/* Disable ADC interrupts, both at the level of the ADC device and at the
	 * level of the NVIC.
	 */
	portENTER_CRITICAL();
	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG0);
	set_val = reg_val&(~(SAR_INT_EN_MASK<<SAR_INT_EN_BIT));
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG0,set_val);

	reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG1);
	set_val = reg_val&(~(SAR_EN_MASK<<SAR_EN_BIT));
	REG32_WRITE(priv->base+SAR_ADC_CTRL_REG1,set_val);
	portENTER_CRITICAL();

	/* Then detach the ADC interrupt handler. */
	xPortInterruptRemoveISR(priv->irq, adc_interrupt);
}

/****************************************************************************
 * Name: adc_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts
 *
 ****************************************************************************/

static void adc_rxint(FAR struct adc_dev_s *dev, bool enable)
{
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t reg_val = 0;
	uint32_t set_val = 0;

	if(enable == 1)
	{
		reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG0);
		set_val = (reg_val&(~(SAR_INT_EN_MASK<<SAR_INT_EN_BIT)))|((1)<<SAR_INT_EN_BIT);
		REG32_WRITE(priv->base+SAR_ADC_CTRL_REG0,set_val);

		reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG1);
		set_val = (reg_val&(~(SAR_EN_MASK<<SAR_EN_BIT)))|((1)<<SAR_EN_BIT);
		REG32_WRITE(priv->base+SAR_ADC_CTRL_REG1,set_val);
	}
	else if(enable == 0)
	{
		reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG0);
		set_val = reg_val&(~(SAR_INT_EN_MASK<<SAR_INT_EN_BIT));
		REG32_WRITE(priv->base+SAR_ADC_CTRL_REG0,set_val);

		reg_val = REG32_READ(priv->base+SAR_ADC_CTRL_REG1);
		set_val = reg_val&(~(SAR_EN_MASK<<SAR_EN_BIT));
		REG32_WRITE(priv->base+SAR_ADC_CTRL_REG1,set_val);
	}
}

/****************************************************************************
 * Name: adc_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method.
 *
 * Input Parameters:
 *   dev - pointer to device structure used by the driver
 *   cmd - command
 *   arg - arguments passed with command
 *
 * Returned Value:
 *
 ****************************************************************************/

static int adc_ioctl(FAR struct adc_dev_s *dev, int cmd, unsigned long arg)
{
	/* No ioctl commands supported */

	/* TODO: ANIOC_TRIGGER, for SW triggered conversion */
	log_d("%s No ioctl commands supported now...\n",__FUNCTION__);

	return -ENOTTY;
}

/****************************************************************************
 * Name: adc_interrupt
 *
 * Description:
 *   ADC interrupt handler
 *
 ****************************************************************************/

static void adc_interrupt( uint32_t arg)
{
	FAR struct adc_dev_s *dev = (FAR struct adc_dev_s *)arg;
	FAR struct hcsar_dev_s *priv = (FAR struct hcsar_dev_s *)dev->ad_priv;
	uint32_t data;

	clear_sar_adc_int_status(dev);
	data = sar_adc_return_val(dev);
	//log_d("%s, %x\n", __FUNCTION__,(u32)data);
	if(priv && priv->cb)
	{
		priv->cb->au_receive(dev,priv->chanlist[priv->current],(int32_t)data);
		priv->current++;

		if (priv->current >= priv->nchannels)
		{
			/* Restart the conversion sequence from the beginning */

			priv->current = 0;
		}
	}

#if 0
	if ((adc_getreg(priv, IMXRT_ADC_HS_OFFSET) & ADC_HS_COCO0) != 0)
	{
		/* Read data. This also clears the COCO bit. */

		data = (int32_t)adc_getreg(priv, IMXRT_ADC_R0_OFFSET);

		if (priv->cb != NULL)
		{
			DEBUGASSERT(priv->cb->au_receive != NULL);
			priv->cb->au_receive(dev, priv->chanlist[priv->current],  data);
		}

		/* Set the channel number of the next channel that will complete
		 * conversion.
		 */

		priv->current++;

		if (priv->current >= priv->nchannels)
		{
			/* Restart the conversion sequence from the beginning */

			priv->current = 0;
		}

		/* Start the next conversion */

		adc_modifyreg(priv, IMXRT_ADC_HC0_OFFSET, ADC_HC_ADCH_MASK,
				ADC_HC_ADCH(priv->chanlist[priv->current]));
	}

	/* There are no interrupt flags left to clear */
#endif
	//return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: imxrt_adcinitialize
 *
 * Description:
 *   Initialize the adc
 *
 * Input Parameters:
 *   intf      - ADC number (1 or 2)
 *   chanlist  - The list of channels
 *   nchannels - Number of channels
 *
 * Returned Value:
 *   Valid can device structure reference on success; a NULL on failure
 *
 ****************************************************************************/

FAR struct adc_dev_s *hc_adcinitialize(int intf,
		FAR const uint8_t *chanlist,
		int nchannels)
{
	FAR struct adc_dev_s *dev;
	FAR struct hcsar_dev_s *priv;

	DEBUGASSERT(nchannels > 0);

	switch (intf)
	{
#ifdef CONFIG_HCHIP_ADC1
		case 1:
			{
				dev = &g_adcdev1;
				break;
			}
#endif /* CONFIG_HCHIP_ADC1 */

#ifdef CONFIG_IMXRT_ADC2
		case 2:
			{
				dev = &g_adcdev2;
				break;
			}
#endif /* CONFIG_IMXRT_ADC2 */

		default:
			{
				log_e("ERROR: Tried to initialize invalid ADC: %d\n", intf);
				return NULL;
			}
	}

	priv = (FAR struct hcsar_dev_s *)dev->ad_priv;

	priv->nchannels = nchannels;
	memcpy(priv->chanlist, chanlist, nchannels);

	//log_d("intf: %d nchannels: %d\n", priv->intf, priv->nchannels);

	return dev;
}

#endif /* CONFIG_HCHIP_ADC1 || CONFIG_IMXRT_ADC2 */

#endif /* CONFIG_HCHIP_ADC */
