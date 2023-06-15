#ifndef __SOC_COMMON_H__
#define __SOC_COMMON_H__

#include <generated/br2_autoconf.h>

#include <kernel/io.h>
#ifdef CONFIG_SOC_HC15XX
#include "hc15xx/uart_reg.h"
#include "hc15xx/uart_reg_struct.h"
#include "hc15xx/timer0_reg.h"
#include "hc15xx/timer0_reg_struct.h"
#include "hc15xx/timer1_reg.h"
#include "hc15xx/timer1_reg_struct.h"
#include "hc15xx/timer2_reg.h"
#include "hc15xx/timer2_reg_struct.h"
#include "hc15xx/timer3_reg.h"
#include "hc15xx/timer3_reg_struct.h"
#include "hc15xx/timer4_reg.h"
#include "hc15xx/timer4_reg_struct.h"
#include "hc15xx/timer5_reg.h"
#include "hc15xx/timer5_reg_struct.h"
#include "hc15xx/timer6_reg.h"
#include "hc15xx/timer6_reg_struct.h"
#include "hc15xx/timer7_reg.h"
#include "hc15xx/timer7_reg_struct.h"
#elif defined(CONFIG_SOC_HC16XX)
#include "hc16xx/uart_reg.h"
#include "hc16xx/uart_reg_struct.h"
#include "hc16xx/timer0_reg.h"
#include "hc16xx/timer0_reg_struct.h"
#include "hc16xx/timer1_reg.h"
#include "hc16xx/timer1_reg_struct.h"
#include "hc16xx/timer2_reg.h"
#include "hc16xx/timer2_reg_struct.h"
#include "hc16xx/timer3_reg.h"
#include "hc16xx/timer3_reg_struct.h"
#include "hc16xx/timer4_reg.h"
#include "hc16xx/timer4_reg_struct.h"
#include "hc16xx/timer5_reg.h"
#include "hc16xx/timer5_reg_struct.h"
#include "hc16xx/timer6_reg.h"
#include "hc16xx/timer6_reg_struct.h"
#include "hc16xx/timer7_reg.h"
#include "hc16xx/timer7_reg_struct.h"
#endif

#endif /* __SOC_COMMON_H__ */
