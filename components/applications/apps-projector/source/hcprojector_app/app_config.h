/*
app_config.h: the global config header file for application
 */
#ifndef __HCDEMO_CONFIG_H__
#define __HCDEMO_CONFIG_H__

#ifdef __linux__
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <sys/types.h> //uint
#else
#include <generated/br2_autoconf.h>
#endif


#ifdef CONFIG_APPS_PROJECTOR_HDMIIN
#define HDMIIN_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_CVBSIN
#define CVBSIN_SUPPORT
#endif


#ifdef CONFIG_APPS_PROJECTOR_BLUETOOTH
#define BLUETOOTH_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_DLNA
#define DLNA_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_AIRCAST
#define AIRCAST_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_MIRACAST
#define MIRACAST_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_USBMIRROR
#define USBMIRROR_SUPPORT
#endif

#if defined(DLNA_SUPPORT) || defined(AIRCAST_SUPPORT) || defined(MIRACAST_SUPPORT)
#define NETWORK_SUPPORT
#define WIFI_SUPPORT
#endif


#if defined(DLNA_SUPPORT) || defined(AIRCAST_SUPPORT) || defined(MIRACAST_SUPPORT) || defined(USBMIRROR_SUPPORT)
#define CAST_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_VMOTOR_DRIVE
#define PROJECTOR_VMOTOR_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_MAIN_PAGE
#define MAIN_PAGE_SUPPORT
#endif

#define PROJECTER_C1_VERSION  0 //customer 1
#define PROJECTER_C2_VERSION  0 //customer 2
#define PROJECTER_C2_D3000_VERSION  0 //customer 2

#ifdef CONFIG_APPS_PROJECTOR_CVBS_AUDIO_I2SI_I2SO
#define CVBS_AUDIO_I2SI_I2SO    // this macro is for c1 ddr3 custmboard
#endif

#ifdef CONFIG_APPS_PROJECTOR_CAST_720P
#define CAST_720P_SUPPORT  1 // default 1080p
#endif

#define HTTPD_SERVICE_SUPPORT


//dump air/miracast/usb mirror ES data to U-disk
//#define MIRROR_ES_DUMP_SUPPORT

#ifdef BR2_PACKAGE_FFMPEG_SWSCALE
  #define  RTOS_SUBTITLE_SUPPORT
#endif

#if defined(BR2_PACKAGE_PREBUILTS_LIBSPECTRUM) || defined(BR2_PACKAGE_PREBUILTS_SPECTRUM)
  #define AUDIO_SPECTRUM_SUPPORT
#endif

#ifdef BR2_PACKAGE_FFMPEG_SWSCALE
  #define FFMPEG_SWSCALE_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_KEYSTONE
	#define KEYSTONE_SUPPORT 1
#endif

#ifdef CONFIG_APPS_PROJECTOR_BACKLIGHT_MONITOR
#define BACKLIGHT_MONITOR_SUPPORT
#endif

#ifdef CONFIG_SOC_HC15XX
	#define CASTING_CLOSE_FB_SUPPORT 1  // close fb during mirroring
#else
	#define CASTING_CLOSE_FB_SUPPORT 0
#endif

#ifdef __HCRTOS__
    #if CONFIG_WDT_AUTO_FEED 
      #define WATCHDOG_KERNEL_FEED
    #endif
#endif

#ifdef CONFIG_APPS_PROJECTOR_LVGL_RESOLUTION_240P
  	#define LVGL_RESOLUTION_240P_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_LVGL_MBOX_STANDBY
#define LVGL_MBOX_STANDBY_SUPPORT
#endif

#ifdef WIFI_SUPPORT
  //#define AUTO_HTTP_UPGRADE_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_SYS_ZOOM
  #define SYS_ZOOM_SUPPORT 1
#endif

#ifdef BR2_PACKAGE_LIBCURL
  #define LIBCURL_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_USB_AUTO_UPGRADE
  #define USB_AUTO_UPGRADE
#endif

#endif
