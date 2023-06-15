#ifndef __GPIO_CTRL_H__
#define __GPIO_CTRL_H__

#include <hcuapi/pinpad.h>
#include <stdint.h> //uint32_t

#ifdef __linux__

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t gpio_pinset_t;

#define GPIO_DIR_INPUT			(0 << 0)
#define GPIO_DIR_OUTPUT			(1 << 0)
#define GPIO_IRQ_RISING			(1 << 4)
#define GPIO_IRQ_FALLING		(1 << 5)


int gpio_configure(pinpad_e padctl, gpio_pinset_t pinset);
void gpio_set_output(pinpad_e padctl, bool val);
int gpio_get_input(pinpad_e padctl);
void gpio_close(pinpad_e padctl);


#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif //end of __linux__

#endif //end of __GPIO_CTRL_H__
