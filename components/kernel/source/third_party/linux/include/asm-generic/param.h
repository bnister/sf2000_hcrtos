/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_PARAM_H
#define __ASM_GENERIC_PARAM_H

#include <freertos/FreeRTOS.h>

#ifdef HZ
#undef HZ
#endif
# define HZ		configTICK_RATE_HZ		/* Internal kernel timer frequency */

#endif /* __ASM_GENERIC_PARAM_H */
