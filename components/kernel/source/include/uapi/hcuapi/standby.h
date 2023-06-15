#ifndef _HCUAPI_STANDBY_H_
#define _HCUAPI_STANDBY_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/pinpad.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

#define STANDBY_SET_WAKEUP_BY_IR		_IOW(STANDBY_IOCBASE, 0, struct standby_ir_setting)
#define STANDBY_SET_WAKEUP_BY_GPIO		_IOW(STANDBY_IOCBASE, 1, struct standby_gpio_setting)
#define STANDBY_SET_WAKEUP_BY_SARADC		_IOW(STANDBY_IOCBASE, 2, struct standby_saradc_setting)
#define STANDBY_ENTER				_IO (STANDBY_IOCBASE, 3)
#define STANDBY_SET_PWROFF_DDR			_IOW(STANDBY_IOCBASE, 4, struct standby_pwroff_ddr_setting)
#define STANDBY_LOCKER_REQUEST			_IOW(STANDBY_IOCBASE, 5, struct standby_locker)
#define STANDBY_LOCKER_RELEASE			_IOW(STANDBY_IOCBASE, 6, struct standby_locker)
#define STANDBY_GET_BOOTUP_MODE			_IOR(STANDBY_IOCBASE, 7, enum standby_bootup_mode)

typedef enum standby_bootup_mode {
	STANDBY_BOOTUP_COLD_BOOT,
	STANDBY_BOOTUP_WARM_BOOT,
} standby_bootup_mode_e;

struct standby_locker {
	char name[64];
};

struct standby_gpio_setting {
	pinpad_e pin;
	int polarity;
};

struct standby_ir_setting {
	int num_of_scancode;
	uint32_t scancode[16];
};

struct standby_saradc_setting {
	int channel;
	int min;
	int max;
};

struct standby_pwroff_ddr_setting {
	pinpad_e pin;
	int polarity;
};

#endif
