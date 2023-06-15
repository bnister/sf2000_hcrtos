#ifndef _HC_GPIO_H_
#define _HC_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hcuapi/gpio.h>
#include <kernel/ld.h>
#include <kernel/list.h>



#if CONFIG_SOC_HC15XX
#define IER_REG                         0x00
#define RIS_IER_REG                     0x04
#define FALL_IER_REG                    0x08
#define INPUT_ST_REG                    0x0c
#define OUTPUT_VAL_REG                  0x10
#define DIR_REG                         0x14
#define ISR_REG                         0x18
#else
#define IER_REG				0x00
#define RIS_IER_REG			0x04
#define FALL_IER_REG 			0x08
#define INPUT_ST_REG			0x0c
#define OUTPUT_VAL_REG			0x10
#define DIR_REG				0x14
#define ISR_REG				0x18
#endif
#define DIR_INPUT			0x00
#define DIR_OUTPUT			0x01

typedef struct gpio_irq_param {
	pinpad_e padctl;
	uint32_t param;
	int type;
	void (*callback)(uint32_t);
	struct gpio_irq_param *next;
} gpio_irq_param_s;

#ifdef __cplusplus
}
#endif

#endif /* _HC_GPIO_H_ */
