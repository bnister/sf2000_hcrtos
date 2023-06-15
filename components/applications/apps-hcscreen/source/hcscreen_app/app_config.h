/*
app_config.h: the global config header file for application
 */
#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#ifdef __linux__
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <sys/types.h> //uint
#else
#include <generated/br2_autoconf.h>
#endif


#ifdef __HCRTOS__
    #if CONFIG_WDT_AUTO_FEED 
      #define WATCHDOG_KERNEL_FEED
    #endif
#endif


#endif
