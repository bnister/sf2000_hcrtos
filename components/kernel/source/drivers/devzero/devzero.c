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
#include <generated/br2_autoconf.h>

static ssize_t devzero_read(struct file *filep, char *buffer, size_t len)
{
	memset(buffer, 0, len);
	return len;
}

static int devzero_poll(struct file *filep, poll_table *wait)
{
	return (POLLIN | POLLOUT);
}

static const struct file_operations devzero_fops =
{
  dummy_open,    /* open */
  dummy_close,   /* close */
  devzero_read,  /* read */
  dummy_write,   /* write */
  NULL,          /* seek */
  NULL,          /* ioctl */
  devzero_poll   /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  , NULL         /* unlink */
#endif
};

static int devzero_module_init(void)
{
	register_driver("/dev/zero", &devzero_fops, 0666, NULL);
	return 0;
}

module_arch(devzero, devzero_module_init, NULL, 0)
