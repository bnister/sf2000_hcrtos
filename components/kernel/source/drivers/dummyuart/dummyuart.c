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

static const struct file_operations g_dummyops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = dummy_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = NULL, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int sci_dummy_probe(const char *node)
{
	int np;
	const char *path;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return 0;

	register_driver(path, &g_dummyops, 0666, NULL);

	return 0;
}

static int dummysci_module_init(void)
{
	int rc = 0;

	rc |= sci_dummy_probe("/hcrtos/uart_dummy");

	return rc;
}

module_arch(dummyuart, dummysci_module_init, NULL, 0)
