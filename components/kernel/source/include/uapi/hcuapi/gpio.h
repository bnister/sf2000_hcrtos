#ifndef _GPIO_H_
#define _GPIO_H_

#ifdef __HCRTOS__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <kernel/io.h>
#include <hcuapi/pinmux.h>

/******************************************************************************
* gpio_inset_t:
*  	 0 ~ 3bit: gpio dir
*  	 4 ~ 7bit: irq mode 
******************************************************************************/
typedef uint8_t gpio_pinset_t;

/******************************************************************************
 * gpio_pinset_t member bit wide
 *****************************************************************************/
#define GPIO_DIR_MUSK			NBITS_M(0, 4)
#define GPIO_IRQ_MUSK			NBITS_M(4, 4)

/******************************************************************************
 * gpio_pinset_t member start pos
 *****************************************************************************/
#define GPIO_DIR_INPUT			(0 << 0)
#define GPIO_DIR_OUTPUT			(1 << 0)
#define GPIO_IRQ_RISING			(1 << 4)
#define GPIO_IRQ_FALLING		(1 << 5)

extern int gpio_configure(pinpad_e padctl, gpio_pinset_t pinset);

extern void gpio_set_output(pinpad_e padctl, bool val);
extern int gpio_get_input(pinpad_e padctl);

extern int gpio_irq_request(pinpad_e padctl, void (*callback)(uint32_t),
			    uint32_t param);
extern int gpio_irq_request2(pinpad_e padctl, void (*callback)(int, uint32_t),
			    uint32_t param);
extern int gpio_irq_free(pinpad_e padctl, uint32_t param);

extern int gpio_config_irq(pinpad_e padctl, gpio_pinset_t pinset);

int gpio_irq_enable(pinpad_e padctl);
int gpio_irq_disable(pinpad_e padctl);

#define gpio_is_valid(padctl) ((pinpad_e)(padctl) < PINPAD_MAX)

#ifdef __cplusplus
}
#endif

#endif

#endif /* _GPIO_H_ */
