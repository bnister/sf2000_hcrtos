#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <kernel/completion.h>
#include <kernel/list.h>
#include <kernel/ld.h>
#include <kernel/delay.h>
#include <kernel/soc/soc_common.h>
#include <nuttx/wqueue.h>
#include <hcuapi/sci.h>
#include <hcuapi/pinmux.h>
#include <kernel/lib/fdt_api.h>
#include <generated/br2_autoconf.h>

static int devnull_poll(struct file *filep, poll_table *wait)
{
	return (POLLIN | POLLOUT);
}

static const struct file_operations devnull_fops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = dummy_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = NULL, /* ioctl */
	.poll = devnull_poll /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int devnull_module_init(void)
{
	register_driver("/dev/null", &devnull_fops, 0666, NULL);
	return 0;
}

module_arch(devnull, devnull_module_init, NULL, 0)
