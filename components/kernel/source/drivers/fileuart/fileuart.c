#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/sys-blocking-notify.h>
#include <linux/notifier.h>
#include <kernel/notify.h>

#include <kernel/list.h>
#include <nuttx/wqueue.h>

#define CONFIG_FILEUART_TX_BUF_SIZE 1024

struct fileuart_dev
{
	int fd;
	uint8_t tx_buf[CONFIG_FILEUART_TX_BUF_SIZE];
	char devname[DISK_NAME_LEN];
	uint32_t tx_rd;
	uint32_t tx_wt;
	struct work_s work;
	wait_queue_head_t wait;
	SemaphoreHandle_t sem;
};

static struct fileuart_dev g_dev = { 0 };

static int fileuart_fs_mount_notify(struct notifier_block *self, unsigned long action, void *dev)
{
	char log_file_path[512] = { 0 };

	switch (action) {
	case USB_MSC_NOTIFY_MOUNT: {
		xSemaphoreTake(g_dev.sem, portMAX_DELAY);
		if (g_dev.fd < 0) {
			snprintf(log_file_path, sizeof(log_file_path), "/media/%s/log.txt", (char *)dev);
			g_dev.fd = open(log_file_path, O_CREAT | O_WRONLY);
			if (g_dev.fd >= 0) {
				memcpy(g_dev.devname, dev, DISK_NAME_LEN);
			}
		}
		xSemaphoreGive(g_dev.sem);
		break;
	}
	case USB_MSC_NOTIFY_UMOUNT: {
		xSemaphoreTake(g_dev.sem, portMAX_DELAY);
		if (g_dev.fd >= 0) {
			if (!strncmp(g_dev.devname, dev, strlen(g_dev.devname))) {
				close(g_dev.fd);
				memset(g_dev.devname, 0, DISK_NAME_LEN);
				g_dev.fd = -1;
			}
		}
		xSemaphoreGive(g_dev.sem);
		break;
	}
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block fileuart_fs_mount = {
       .notifier_call = fileuart_fs_mount_notify,
};

static void fileuart_delayed_work(void *parameter)
{
	xSemaphoreTake(g_dev.sem, portMAX_DELAY);

	if (g_dev.tx_rd < g_dev.tx_wt) {
		write(g_dev.fd, g_dev.tx_buf + g_dev.tx_rd, g_dev.tx_wt - g_dev.tx_rd);
		fsync(g_dev.fd);
	} else if (g_dev.tx_rd > g_dev.tx_wt) {
		write(g_dev.fd, g_dev.tx_buf + g_dev.tx_rd, CONFIG_FILEUART_TX_BUF_SIZE - g_dev.tx_rd);
		if (g_dev.tx_wt > 0)
			write(g_dev.fd, g_dev.tx_buf, g_dev.tx_wt);
		fsync(g_dev.fd);
	}

	g_dev.tx_rd = g_dev.tx_wt;

	xSemaphoreGive(g_dev.sem);
}

static ssize_t fileuart_write(struct file *filep, const char *buf, size_t size)
{
	size_t i;

	if (g_dev.fd < 0)
		return 0;

	if (!uxInterruptNesting) {
		xSemaphoreTake(g_dev.sem, portMAX_DELAY);
		write(g_dev.fd, buf, size);
		fsync(g_dev.fd);
		xSemaphoreGive(g_dev.sem);
	} else {
		for (i = 0; i < size; i++) {
			if (buf[i] == '\n') {
				g_dev.tx_buf[g_dev.tx_wt++] = '\r';
				g_dev.tx_wt %= CONFIG_FILEUART_TX_BUF_SIZE;
			}
			g_dev.tx_buf[g_dev.tx_wt++] = buf[i];
			g_dev.tx_wt %= CONFIG_FILEUART_TX_BUF_SIZE;
		}
		if (i > 0) {
			if (work_available(&g_dev.work)) {
				work_queue(LPWORK, &g_dev.work, fileuart_delayed_work, NULL, 0);
			}
		}
	}
	return size;
}

static int fileuart_poll(struct file *filep, poll_table *wait)
{
	int mask = 0;

	poll_wait(filep, &g_dev.wait, wait);

	mask |= POLLOUT | POLLWRNORM;

	return mask;
}

static const struct file_operations fileuart_fops = {
	.open = dummy_open,
	.close = dummy_close,
	.write = fileuart_write,
	.poll = fileuart_poll,
};

static int fileuart_driver_probe(const char *node)
{
	int np;
	const char *path;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return 0;

	g_dev.sem = xSemaphoreCreateMutex();
	g_dev.fd = -1;
	init_waitqueue_head(&g_dev.wait);
	sys_register_notify(&fileuart_fs_mount);

	register_driver(path, &fileuart_fops, 0666, NULL);

	return 0;
}

static int fileuart_driver_init(void)
{
	int rc = 0;
	rc = fileuart_driver_probe("/hcrtos/fileuart");
	return rc;
}

module_arch(fileuart, fileuart_driver_init, NULL, 0)
