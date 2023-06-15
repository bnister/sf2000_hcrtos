#include "../hc_porting.h"

#include "libusbi.h"
#include "hcrtos_usbfs.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/errno.h>
#include <linux/usb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <pthread.h>
#include <kernel/delay.h>

static pthread_t libusb_linux_event_thread;
static void linux_netlink_event_thread_main(void *arg);
static TaskHandle_t hotplug_poll_task = NULL;
static bool is_busy = false;


int linux_netlink_start_event_monitor(void)
{
	if(!hotplug_poll_task)
		xTaskCreate(linux_netlink_event_thread_main, (const char *)"libusb_hotplug", 
					configTASK_STACK_DEPTH,
					NULL, portPRI_TASK_NORMAL, 
					&hotplug_poll_task);
	return LIBUSB_SUCCESS;
}


int linux_netlink_stop_event_monitor(void)
{
	if (hotplug_poll_task != NULL){
		while(is_busy == true)
			msleep(100);
		vTaskDelete(hotplug_poll_task);
		hotplug_poll_task = NULL;
	}
	return LIBUSB_SUCCESS;
}


static int linux_netlink_read_message(void)
{
	uint8_t busnum, devaddr;
	bool detached;
	char sys_name[64] = { 0 };
	struct hotplug_msg msg;
	BaseType_t ret;

	if(!linux_hotplug_msg_queue)
		return -1;

	ret = xQueueReceive(linux_hotplug_msg_queue,
					&msg,
					0);
	if(ret != pdTRUE)
		return -1;
	
	detached = msg.detached;
	devaddr = msg.devaddr;
	busnum = msg.busnum;

	sprintf(&sys_name[0], "/dev/bus/usb/%3.3u/%3.3u", 
			busnum,
			devaddr);
	
	usbi_dbg("%s %s\n", sys_name,
				(detached) ? "plug out" : "plug in");
	
	printf(" ==> (linux_netlink_read_message) Receive libusb_notify: %s, busnum:%u, devnum:%u\n",
				sys_name, msg.busnum, msg.devaddr);

	/* signal device is available (or not) to all contexts */
	if (detached)
		linux_device_disconnected(busnum, devaddr);
	else
		linux_hotplug_enumerate(busnum, devaddr, sys_name);

	return 0;
}

static void linux_netlink_event_thread_main(void *arg)
{
	int ret;
	uint8_t busnum, devaddr;
	bool detached;
	char sys_name[64] = { 0 };
	struct hotplug_msg msg;

	usbi_dbg("netlink event thread entering");

	while(1){
		is_busy = false;

		if(!linux_hotplug_msg_queue){
			msleep(100);
			continue;
		}

		ret = xQueueReceive(linux_hotplug_msg_queue,
						&msg,
						portMAX_DELAY);
		if(ret != pdTRUE)
			continue;

		msleep(100);

		usbi_mutex_static_lock(&linux_hotplug_lock);

		is_busy = true;
		detached = msg.detached;
		devaddr = msg.devaddr;
		busnum = msg.busnum;

		sprintf(&sys_name[0], "/dev/bus/usb/%3.3u/%3.3u", 
				busnum,
				devaddr);
		
		usbi_dbg("%s %s\n", sys_name, 
				(detached) ? "plug out" : "plug in");

		// printf(" ==> (linux_netlink_event_thread_main) Receive libusb_notify: %s, busnum:%u, devnum:%u\n",
		// 		sys_name, msg.busnum, msg.devaddr);

		/* signal device is available (or not) to all contexts */
		if (detached)
			linux_device_disconnected(busnum, devaddr);
		else
			linux_hotplug_enumerate(busnum, devaddr, sys_name);
			
		usbi_mutex_static_unlock(&linux_hotplug_lock);
	}

	usbi_dbg("netlink event thread exiting");
	return;
}


void linux_netlink_hotplug_poll(void)
{
	int r;

	usbi_mutex_static_lock(&linux_hotplug_lock);
	do {
		r = linux_netlink_read_message();
	} while (r == 0);
	usbi_mutex_static_unlock(&linux_hotplug_lock);
}
