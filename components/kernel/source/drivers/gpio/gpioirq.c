#include <generated/br2_autoconf.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <stdio.h>
#include <sys/errno.h>

#include <kernel/list.h>
#include <kernel/ld.h>
#include <kernel/soc/soc_common.h>
#include <malloc.h>

#include <hcuapi/gpio.h>
#include "hc_gpio.h"

static struct gpio_irq_param *__gpio_isr_head[PINPAD_MAX] = { NULL };

static void * ctrlreg[4] = {
	(void *)&GPIOLCTRL,
	(void *)&GPIOBCTRL,
	(void *)&GPIORCTRL,
	(void *)&GPIOTCTRL,
};

static void gpio_irq_clear(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + ISR_REG;

	REG32_SET_BIT(reg, bit);

	return ;
}
	
static int gpio_irq_mode_config(pinpad_e padctl, gpio_pinset_t pinset)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg_fall = ctrlreg[padctl / 32] + FALL_IER_REG;

	void *reg_ris  = ctrlreg[padctl / 32] + RIS_IER_REG;

	if (GPIO_IRQ_FALLING & pinset)
		REG32_SET_BIT(reg_fall, bit);
	else
		REG32_CLR_BIT(reg_fall, bit);

	if (GPIO_IRQ_RISING & pinset)
		REG32_SET_BIT(reg_ris , bit);
	else
		REG32_CLR_BIT(reg_ris , bit);

	return 0;
}

int gpio_irq_enable(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + IER_REG;

	REG32_SET_BIT(reg, bit);

	return 0;
}

int gpio_irq_disable(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + IER_REG;

	REG32_CLR_BIT(reg, bit);

	return 0;
}

void gpio_interrupt(uint32_t param)
{
	gpio_irq_param_s *p = (gpio_irq_param_s *)param;
	pinpad_e padctl;
	uint32_t bit;
	void *reg;
	void (*proc)(uint32_t);
	int (*proc2)(int, void *);

	if (p == NULL)
		return;

	padctl = p->padctl;
	bit = BIT(padctl % 32);
	reg = ctrlreg[padctl / 32] + ISR_REG;

	if (!REG32_GET_BIT(reg, bit)) {
		return;
	}

	REG32_SET_BIT(reg, bit);

	while (p != NULL) {
		if (p->type == 0) {
			proc = (typeof(proc))p->callback;
			proc(p->param);
		} else {
			proc2 = (typeof(proc2))p->callback;
			proc2((int)p->padctl + 256, (void *)p->param);
		}

		p = p->next;
	}

	return;
}

static int __gpio_irq_request(pinpad_e padctl, void (*callback)(uint32_t), uint32_t param, int type)
{
	gpio_irq_param_s *irq_param;

	if (!gpio_is_valid(padctl))
		return -EINVAL;

	irq_param = (gpio_irq_param_s *)malloc(sizeof(gpio_irq_param_s));	
	if (irq_param == NULL)
		return -ENOMEM;

	irq_param->padctl = padctl;
	irq_param->param = param;
	irq_param->callback = callback;
	irq_param->next = NULL;
	irq_param->type = type;

	taskENTER_CRITICAL();
	if (__gpio_isr_head[padctl] == NULL) {
		__gpio_isr_head[padctl] = irq_param;
		xPortInterruptInstallISR((uint32_t)&GPIO_INTR, gpio_interrupt,
					  (uint32_t)__gpio_isr_head[padctl]);
		gpio_irq_enable(padctl);
	} else {
		irq_param->next = __gpio_isr_head[padctl];
		__gpio_isr_head[padctl] = irq_param;
	}
	taskEXIT_CRITICAL();

	return 0;
}

int gpio_irq_request2(pinpad_e padctl, void (*callback)(int, uint32_t), uint32_t param)
{
	return __gpio_irq_request(padctl, (void (*)(uint32_t))callback, param, 1);
}

int gpio_irq_request(pinpad_e padctl, void (*callback)(uint32_t), uint32_t param)
{
	return __gpio_irq_request(padctl, (void (*)(uint32_t))callback, param, 0);
}

int gpio_irq_free(pinpad_e padctl, uint32_t param)
{
	gpio_irq_param_s *p, *pr;
	int ret = 0;

	if (!gpio_is_valid(padctl))
		return -EINVAL;

	taskENTER_CRITICAL();
	p = __gpio_isr_head[padctl];
	pr = p;
	if (p == NULL) {
		/* not found */
		taskEXIT_CRITICAL();
		return -EINVAL;
	}

	while (param != p->param && p->next != NULL) {
		pr = p;
		p = p->next;
	}

	if (param == p->param) {
		if (p == __gpio_isr_head[padctl]) {
			__gpio_isr_head[padctl] = p->next;
		} else {
			pr->next = p->next;
		}

		if (__gpio_isr_head[padctl] == NULL) {
			gpio_irq_disable(padctl);
			/* no more gpio irq for the padctl */
			xPortInterruptRemoveISR(
				(uint32_t)&GPIO_INTR,
				(void (*)(uint32_t))(__gpio_isr_head[padctl]));
		}
		free(p);
	} else {
		/* not found */
		ret = -EINVAL;
	}

	taskEXIT_CRITICAL();
	return ret;
}

int gpio_config_irq(pinpad_e padctl, gpio_pinset_t pinset)
{
	gpio_irq_mode_config(padctl, pinset);
}
