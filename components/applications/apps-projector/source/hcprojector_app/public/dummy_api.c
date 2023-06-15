/*
dummy_api.c: Define some empty apis for special modules that unsupport. 
	So that reduce macros.
 */
#include "app_config.h"
#include "dummy_api.h"

/*
#################### bluetooth #########################
 */
#ifndef BR2_PACKAGE_BLUETOOTH

int bluetooth_init(const char *uart_path, bluetooth_callback_t callback)
{
	return 0;
}
int bluetooth_deinit(void)
{
	return 0;
}
int bluetooth_poweron(void)
{
	return 0;
}
int bluetooth_poweroff(void)
{
	return 0;
}
int bluetooth_scan(void)
{
	return 0;
}
int bluetooth_stop_scan(void)
{
	return 0;
}
int bluetooth_connect(unsigned char *mac)
{
	return 0;
}
int bluetooth_is_connected(void)
{
	return 0;
}
int bluetooth_disconnect(void)
{
	return 0;
}
int bluetooth_set_gpio_backlight(unsigned char value)
{
	return 0;
}
int bluetooth_set_gpio_mutu(unsigned char value)
{
	return 0;
}
int bluetooth_set_cvbs_aux_mode(void)
{
	return 0;
}
int bluetooth_set_cvbs_fiber_mode(void)
{
	return 0;
}

#endif