#include <string.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <kernel/ld.h>
#include <hcuapi/standby.h>
#include "standby_priv.h"
#include <stdio.h>
#include <kernel/io.h>

static int standby_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case STANDBY_SET_WAKEUP_BY_IR:
		rc = standby_set_wakeup_by_ir((struct standby_ir_setting *)arg);
		break;

	case STANDBY_SET_WAKEUP_BY_GPIO:
		rc = standby_set_wakeup_by_gpio((struct standby_gpio_setting *)arg);
		break;

	case STANDBY_SET_WAKEUP_BY_SARADC:
		rc = standby_set_wakeup_by_saradc((struct standby_saradc_setting *)arg);
		break;

	case STANDBY_ENTER:
		standby_enter();
		break;

	case STANDBY_SET_PWROFF_DDR:
		rc = standby_set_ddr((struct standby_pwroff_ddr_setting *)arg);
		break;

	case STANDBY_LOCKER_REQUEST:
		rc = standby_request((struct standby_locker *)arg);
		break;

	case STANDBY_LOCKER_RELEASE:
		rc = standby_release((struct standby_locker *)arg);
		break;
	case STANDBY_GET_BOOTUP_MODE:
		rc = standby_get_bootup_mode((enum standby_bootup_mode *)arg);
		break;

	default:
		break;
	}
	return 0;
}

static const struct file_operations g_standbyops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = dummy_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = standby_ioctl, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int standby_init(void)
{
	size_t bss_sz = (size_t)((void *)&__STANDBY_BSS_START -
				 (void *)&__STANDBY_BSS_END);

	REG32_CLR_BIT(0xb8800064, BIT1);
	REG32_SET_BIT(0xb8802000, BIT0);
	REG32_WRITE(0xb8802244, 0x00000000);
	upmode_n = REG32_READ(0xb8802248);
	REG32_WRITE(0xb8802244, 0x80000000);
	REG32_WRITE(0xb8802248, 0x00000000);
	REG32_WRITE(0xb8802244, 0x00000000);
	memset((void *)&__STANDBY_BSS_START, 0, bss_sz);
	standby_get_dts_param();
	register_driver("/dev/standby", &g_standbyops , 0666, NULL);
	standby_lock_list_init();

	return 0;
}
module_driver(standby, standby_init, NULL, 0)
