#ifndef _HCUAPI_SYSDATA_H_
#define _HCUAPI_SYSDATA_H_

#include <hcuapi/tvtype.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

typedef enum HCFOTA_REBOOT_OTA_DETECT_MODE {
	HCFOTA_REBOOT_OTA_DETECT_NONE,
	HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE,
	HCFOTA_REBOOT_OTA_DETECT_USB_HOST,
	HCFOTA_REBOOT_OTA_DETECT_SD,
	HCFOTA_REBOOT_OTA_DETECT_NETWORK,
} hcfota_reboot_ota_detect_mode_e;

struct sysdata {
	char product_id[16];
	uint32_t firmware_version;
	uint32_t ota_detect_modes;
	tvtype_e tvtype;
	uint8_t volume;
	uint8_t flip_mode;
	uint8_t lcd_pwm_backlight;	//<! 0 ~ 100
	uint8_t lcd_pwm_vcom;		//<! 0 ~ 100
	uint8_t adc_adjust_value;
	uint8_t reserved[127];
};

#ifdef __HCRTOS__
int sys_get_sysdata(struct sysdata *sysdata);
int sys_get_sysdata_tvtype(tvtype_e *tvtype);
int sys_get_sysdata_volume(uint8_t *volume);
int sys_get_sysdata_flip_mode(uint8_t *mode);
int sys_get_sysdata_lcd_pwm_backlight(uint8_t *backlight);
int sys_get_sysdata_lcd_pwm_vcom(uint8_t *vcom);
int sys_get_sysdata_adc_adjust_value(uint8_t *value);
int sys_set_sysdata_adc_adjust_value(uint8_t value);
#endif

#endif
