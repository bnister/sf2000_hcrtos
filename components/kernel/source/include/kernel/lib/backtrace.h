#ifndef __BACKTRACE_H
#define __BACKTRACE_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern void show_stack(TaskHandle_t task);
extern int stacktrace_get_ra(unsigned int *save_ra, int ra_cnt, int omit_cnt);

#endif
