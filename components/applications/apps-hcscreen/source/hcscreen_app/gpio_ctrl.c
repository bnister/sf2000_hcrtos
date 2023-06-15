/*
gpio_ctrl.c, get/set the GPIO value
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdbool.h> //bool
#include <dirent.h>
#include "glist.h"

#ifdef __linux__
//only valid in OS linux

#include "gpio_ctrl.h"
#include "com_api.h"

static glist *m_gpio_list = NULL;

typedef struct{
	int number;
	int direction;
}gpio_handle_t;

static int file_exist(const char *path)
{
	DIR *dirp;
    if ((dirp = opendir(path)) == NULL){
        return 0;
    }else{
        closedir(dirp);
        return 1;
    }
}

static void gpio_dir_set(int num, int dir)
{
	int fd = -1;
	char str_buf[64];

	// echo in > /sys/class/gpio/gpio8/direction
	sprintf(str_buf, "/sys/class/gpio/gpio%d/direction", num);
	fd = open(str_buf, O_WRONLY);
	if (fd < 0){
		printf("%s(), line:%d. open %s fail!\n", __func__, __LINE__, str_buf);
		return;
	}
	if (GPIO_DIR_INPUT == dir)
		sprintf(str_buf, "in");
	else
		sprintf(str_buf, "out");

	write(fd, str_buf, strlen(str_buf));
	close(fd);

}

static void *gpio_open(int num, int dir)
{
	int fd = -1;
	gpio_handle_t *gpio_handle = NULL;
	char str_buf[64];


	//step 1: create GPIO node
	sprintf(str_buf, "/sys/class/gpio/gpio%d", num);
	//if /sys/class/gpio/gpio8 is exist, not need create again.
	if (!file_exist(str_buf)){

		fd = open("/sys/class/gpio/export", O_WRONLY);
		if (fd < 0){
			printf("%s(), line:%d. open export fail!, fd:%d\n", __func__, __LINE__, fd);
			return NULL;
		}
		sprintf(str_buf, "%d", num);
		write(fd, str_buf, strlen(str_buf));
		close(fd);
		api_sleep_ms(10);
	}

	//step 2: set direction of GPIO
	gpio_dir_set(num, dir);

	gpio_handle = (gpio_handle_t*)malloc(sizeof(gpio_handle_t));
	memset(gpio_handle, 0, sizeof(gpio_handle_t));
	gpio_handle->number = num;
	gpio_handle->direction = dir;

	return (void*)gpio_handle;
}

static int _gpio_close(void *gpio_handle)
{
	gpio_handle_t *handle = (gpio_handle_t*)gpio_handle;
	char str_buf[64] = {0};

	if (!handle)
		return API_FAILURE;

	//"echo 8 > /sys/class/gpio/unexport"
	int fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd < 0){
		printf("%s(), line:%d. open unexport fail!\n", __func__, __LINE__);
		return API_FAILURE;	
	}
	sprintf(str_buf, "%d", handle->number);
	write(fd, str_buf, strlen(str_buf));
	close(fd);

	return API_SUCCESS;	
}


static int gpio_read(void *gpio_handle)
{
	char str_buf[64];
	int ret;
	gpio_handle_t *handle = (gpio_handle_t*)gpio_handle;
	int fd = -1;
	int value = -1;

	if (!handle)
		return API_FAILURE;

	char buffer[16]={0};


	//"cat /sys/class/gpio/gpio8/value"
	sprintf(str_buf, "/sys/class/gpio/gpio%d/value", handle->number);
	fd = open(str_buf, O_RDWR);

	if (fd < 0){
		//printf("%s(), line:%d. open %s fail!\n", __func__, __LINE__, str_buf);
		return value;
	}

	ret = read(fd, buffer, sizeof(buffer));
	if (ret > 0){
		value = atoi(buffer);
	}
	close(fd);
	//printf("ret=%d, value:%d, buffer:%s\n", ret, value, buffer);
	
	return value;
}

static int gpio_write(void *gpio_handle, bool high)
{
	char str_buf[64];
	gpio_handle_t *handle = (gpio_handle_t*)gpio_handle;
	int fd = -1;

	sprintf(str_buf, "/sys/class/gpio/gpio%d/value", handle->number);
	fd = open(str_buf, O_WRONLY);
	if (fd < 0){
		printf("%s(), line:%d. open %s fail!\n", __func__, __LINE__, str_buf);
		return API_FAILURE;
	}

	if (high)
		sprintf(str_buf, "1");
	else
		sprintf(str_buf, "0");

	write(fd, str_buf, strlen(str_buf));
	close(fd);
	return API_SUCCESS;
}

static gpio_handle_t *gpio_get_by_number(pinpad_e padctl)
{
    glist *list_tmp = NULL;
	gpio_handle_t *gpio_handle = NULL;

	list_tmp = m_gpio_list;
	while (list_tmp){
		gpio_handle = (gpio_handle_t*)list_tmp->data;
		if (gpio_handle->number == padctl){
			return (void*)gpio_handle;
		}
		list_tmp = list_tmp->next;
	}
	return NULL;
}

int gpio_configure(pinpad_e padctl, gpio_pinset_t pinset)
{
	if (padctl == INVALID_VALUE_32)
		return -1;

	gpio_handle_t *gpio_handle = gpio_get_by_number(padctl);
	if (NULL == gpio_handle){
		//create new GPIO
		gpio_handle = gpio_open(padctl, pinset);
		if (NULL == gpio_handle)
			return -1;
		m_gpio_list = glist_append(m_gpio_list, (void*)gpio_handle);
	}else{
		if (gpio_handle->direction == pinset)
			return 0;

		//config GPIO dir.
		gpio_dir_set(padctl, pinset);

	}

	return 0;
}

void gpio_set_output(pinpad_e padctl, bool val)
{
	if (padctl == INVALID_VALUE_32)
		return;

	gpio_handle_t *gpio_handle = gpio_get_by_number(padctl);
	if (gpio_handle){
		gpio_write(gpio_handle, val);
	}

}

int gpio_get_input(pinpad_e padctl)
{
	if (padctl == INVALID_VALUE_32)
		return -1;

	gpio_handle_t *gpio_handle = gpio_get_by_number(padctl);
	if (gpio_handle){
		return gpio_read(gpio_handle);
	}
	return -1;
}

void gpio_close(pinpad_e padctl)
{
	if (padctl == INVALID_VALUE_32)
		return;

	gpio_handle_t *gpio_handle = gpio_get_by_number(padctl);
	if (gpio_handle){
		_gpio_close(gpio_handle);
		m_gpio_list = glist_remove(m_gpio_list, gpio_handle);
		//printf("%s(), m_gpio_list=0x%x\n", __func__, m_gpio_list);
		if (gpio_handle)
			free(gpio_handle);
	}
}

#endif