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
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>

#include <kernel/list.h>
#include <nuttx/wqueue.h>
#include <hcuapi/virtuart.h>

struct virtuart_dev
{
	uint8_t rx_buf[CONFIG_VIRTUART_RX_BUF_SIZE];
	uint8_t tx_buf[CONFIG_VIRTUART_TX_BUF_SIZE];
	uint32_t rx_rd;
	uint32_t rx_wt;
	uint32_t tx_rd;
	uint32_t tx_wt;
	struct work_s work;
	wait_queue_head_t wait;
	wait_queue_head_t proxy_wait;
	SemaphoreHandle_t rx_sem;
	SemaphoreHandle_t tx_sem;
};

struct virtuart_dev g_dev = { 0 };

static ssize_t virtuart_read(struct file *filep, char *buf, size_t size)
{
	size_t i = 0;

	xSemaphoreTake(g_dev.rx_sem, portMAX_DELAY);

	for (i = 0; i < size; i++) {
		if (g_dev.rx_rd == g_dev.rx_wt)
			break;
		buf[i] = g_dev.rx_buf[g_dev.rx_rd++];
		g_dev.rx_rd %= CONFIG_VIRTUART_RX_BUF_SIZE;
	}

	xSemaphoreGive(g_dev.rx_sem);

	return i;
}

static ssize_t virtuart_write(struct file *filep, const char *buf, size_t size)
{
	size_t i;

	if (!uxInterruptNesting)
		xSemaphoreTake(g_dev.tx_sem, portMAX_DELAY);

	for (i = 0; i < size; i++) {
		if (buf[i] == '\n') {
			g_dev.tx_buf[g_dev.tx_wt++] = '\r';
			g_dev.tx_wt %= CONFIG_VIRTUART_TX_BUF_SIZE;
		}
		g_dev.tx_buf[g_dev.tx_wt++] = buf[i];
		g_dev.tx_wt %= CONFIG_VIRTUART_TX_BUF_SIZE;
	}

	if (!uxInterruptNesting)
		xSemaphoreGive(g_dev.tx_sem);

	if (i > 0) {
		wake_up(&g_dev.proxy_wait);
		work_notifier_signal2(VIRTUART_NOTIFY_POLLIN, NULL, 0);
	}

	return size;
}

static int virtuart_poll(struct file *filep, poll_table *wait)
{
	int mask = 0;

	poll_wait(filep, &g_dev.wait, wait);

	if (g_dev.rx_rd != g_dev.rx_wt)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static ssize_t virtuart_proxy_read(struct file *filep, char *buf, size_t size)
{
	size_t i = 0;

	xSemaphoreTake(g_dev.tx_sem, portMAX_DELAY);

	for (i = 0; i < size; i++) {
		if (g_dev.tx_rd == g_dev.tx_wt)
			break;
		buf[i] = g_dev.tx_buf[g_dev.tx_rd++];
		g_dev.tx_rd %= CONFIG_VIRTUART_TX_BUF_SIZE;
	}

	xSemaphoreGive(g_dev.tx_sem);

	return i;
}

static ssize_t virtuart_proxy_write(struct file *filep, const char *buf, size_t size)
{
	size_t i;

	xSemaphoreTake(g_dev.rx_sem, portMAX_DELAY);

	for (i = 0; i < size; i++) {
		g_dev.rx_buf[g_dev.rx_wt++] = buf[i];
		g_dev.rx_wt %= CONFIG_VIRTUART_RX_BUF_SIZE;
	}

	xSemaphoreGive(g_dev.rx_sem);

	if (i > 0)
		wake_up(&g_dev.wait);

	return size;
}

static int virtuart_proxy_poll(struct file *filep, poll_table *wait)
{
	int mask = 0;

	poll_wait(filep, &g_dev.proxy_wait, wait);

	if (g_dev.tx_rd != g_dev.tx_wt)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static const struct file_operations virtuart_fops = {
	.open = dummy_open,
	.close = dummy_close,
	.read = virtuart_read,
	.write = virtuart_write,
	.poll = virtuart_poll,
};

static const struct file_operations virtuart_proxy_fops = {
	.open = dummy_open,
	.close = dummy_close,
	.read = virtuart_proxy_read,
	.write = virtuart_proxy_write,
	.poll = virtuart_proxy_poll,
};

static int virtuart_driver_probe(const char *node)
{
	int np;
	const char *path;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return 0;

	g_dev.rx_rd = 0;
	g_dev.rx_wt = 0;
	g_dev.rx_sem = xSemaphoreCreateMutex();
	g_dev.tx_sem = xSemaphoreCreateMutex();
	init_waitqueue_head(&g_dev.wait);
	init_waitqueue_head(&g_dev.proxy_wait);

	register_driver(path, &virtuart_fops, 0666, NULL);
	register_driver("/dev/virtuart_proxy", &virtuart_proxy_fops, 0666, NULL);

	return 0;
}

static int virtuart_driver_init(void)
{
	int rc = 0;
	rc = virtuart_driver_probe("/hcrtos/virtuart");
	return rc;
}

static int quit_virtconsole(int argc, char *argv[])
{
	work_notifier_signal2(VIRTUART_NOTIFY_EXIT, NULL, 0);
	return 0;
}

CONSOLE_CMD(quit, NULL, quit_virtconsole, CONSOLE_CMD_MODE_SELF,
	    "Quit virtual uart console")

module_arch(virtuart, virtuart_driver_init, NULL, 0)
