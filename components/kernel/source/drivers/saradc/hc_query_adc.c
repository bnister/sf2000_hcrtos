#include <generated/br2_autoconf.h>

#if defined(CONFIG_SOC_HC16XX) && defined(CONFIG_POLL_ADC)
#include "hc_16xx_query_adc.c"
#elif defined(CONFIG_SOC_HC15XX) && defined(CONFIG_POLL_ADC)
#include "hc_15xx_query_adc.c"
#endif

