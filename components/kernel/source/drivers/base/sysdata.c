#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>

int sys_get_sysdata(struct sysdata *sysdata)
{
	int fd;
	struct persistentmem_node node;

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int sys_get_sysdata_tvtype(tvtype_e *tvtype)
{
	struct sysdata sysdata;

	if (!sys_get_sysdata(&sysdata)) {
		*tvtype = sysdata.tvtype;
		return 0;
	}

	return -1;
}

int sys_get_sysdata_volume(uint8_t *volume)
{
	struct sysdata sysdata;

	if (!sys_get_sysdata(&sysdata)) {
		*volume = sysdata.volume;
		return 0;
	}

	return -1;
}

int sys_get_sysdata_flip_mode(uint8_t *mode)
{
	struct sysdata sysdata;

	if (!sys_get_sysdata(&sysdata)) {
		*mode = sysdata.flip_mode;
		return 0;
	}

	return -1;
}

int sys_get_sysdata_lcd_pwm_backlight(uint8_t *backlight)
{
	struct sysdata sysdata;

	if (!sys_get_sysdata(&sysdata)) {
		*backlight = sysdata.lcd_pwm_backlight;
		return 0;
	}

	return -1;
}

int sys_get_sysdata_lcd_pwm_vcom(uint8_t *vcom)
{
	struct sysdata sysdata;

	if (!sys_get_sysdata(&sysdata)) {
		*vcom = sysdata.lcd_pwm_vcom;
		return 0;
	}

	return -1;
}

int sys_get_sysdata_adc_adjust_value(uint8_t *value)
{
	struct sysdata sysdata;

	if (!sys_get_sysdata(&sysdata)) {
		*value = sysdata.adc_adjust_value;
		return 0;
	}

	return -1;
}

int sys_set_sysdata_adc_adjust_value(uint8_t value)
{
	int fd;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = offsetof(struct sysdata, adc_adjust_value);
	node.size = sizeof(sysdata.adc_adjust_value);
	node.buf = &value;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}
