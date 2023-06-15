#include <generated/br2_autoconf.h>


#ifdef CONFIG_SOC_HC16XX
#include "hc_16xx_key_adc.c"
#elif defined(CONFIG_SOC_HC15XX)
#include "hc_15xx_key_adc.c"
#endif

