/****************************************************************************
 * components/kernel/source/drivers/adc/hc_adc.h
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

#ifndef __DRIVERS_ADC_HCSAR_ADC_H
#define __DRIVERS_ADC_HCSAR_ADC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#define CONFIG_HCHIP_ADC
#define CONFIG_HCHIP_ADC1

#define SAR_INT_EN_BIT				(0)
#define SAR_INT_EN_MASK				(1)

#define SAR_INT_ST_BIT				(8)
#define SAR_INT_ST_MASK				(1)

#define SAR_DOUT_BIT				(16)
#define SAR_DOUT_MASK				(0xFF)

#define SAR_EN_BIT					(0)
#define SAR_EN_MASK					(0x1)

#define SAR_CLK_DIV_NUM_BIT			(8)
#define SAR_CLK_DIV_NUM_MASK		(0xFF)

#define SAR_REPEAT_NUM_BIT		(24)
#define SAR_REPEAT_NUM_MASK		(0xFF)


#define SAR_DEBOUNCE_VALUE_BIT		(8)
#define SAR_DEBOUNCE_VALUE_MASK		(0xFF)

#define SAR_DEFAULT_VALUE_BIT		(0)
#define SAR_DEFAULT_VALUE_MASK		(0xFF)

#define SAR_OFFSET_VALUE_BIT		(8)
#define SAR_OFFSET_VALUE_MASK		(0xFF)

#define SAR_PWD_CORE_BIT			(0)
#define SAR_PWD_CORE_MASK			(0x1)

#define SAR_INPSEL_CORE_BIT			(4)
#define SAR_INPSEL_CORE_MASK		(0x1)

#define SAR_CLK_SEL_BIT			(16)
#define SAR_CLK_SEL_MASK		(0x3)

#define SAR_ANATST_CORE_BIT			(28)
#define SAR_ANATST_CORE_MASK		(0xF)

enum SAR_ADC_HW_REG_ADDR
{
	SAR_ADC_CTRL_REG0=0x0,
	SAR_ADC_CTRL_REG1=0x4,
	SAR_ADC_CTRL_REG2=0x8,
	SAR_ADC_CTRL_REG3=0xc,
};

/****************************************************************************
 * Name: hc_adcinitialize
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

#ifdef CONFIG_HCHIP_ADC
FAR struct adc_dev_s *hc_adcinitialize(int intf,
                                          FAR const uint8_t *chanlist,
                                          int nchannels);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_ADC_HCSAR_ADC_H*/
