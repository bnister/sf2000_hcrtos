#ifndef __WS2811_H_
#define __WS2811_H_

#include <uapi/hcuapi/pinpad.h>

void *ws2811_start(pinpad_e pin, int tim);
int ws2811_stop(void *handle);

#endif
