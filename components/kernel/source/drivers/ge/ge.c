#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/drivers/input.h>
#include <kernel/io.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/log2.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/limits.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <kernel/ld.h>
#include <asm-generic/page.h>
#include <linux/mm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>

#include <hcuapi/ge.h>
#include <kernel/drivers/hc_clk_gate.h>

#define MODULE_NAME "/dev/ge"

#if 0
#define HCGE_DBG printk
#else
#define HCGE_DBG(...)                                                          \
	do {                                                                   \
	} while (0)
#endif

#define NODE_ADDR(phy_addr_start) ((phy_addr_start)&0xFFFFFFFE)
#define NODE_WRAP(phy_addr_start) ((phy_addr_start)&0x01)
#define NODE_CLEAR_WRAP(phy_addr_start)                                        \
	do {                                                                   \
		phy_addr_start = phy_addr_start & 0xFFFFFFFE;                  \
	} while (0)

#define NODE_SET_WRAP(phy_addr_start)                                          \
	do {                                                                   \
		phy_addr_start = phy_addr_start | 0x1;                         \
	} while (0)

#define NODE_SET_ADDR(phy_addr_start, addr)                                    \
	do {                                                                   \
		phy_addr_start = addr | (phy_addr_start & 0x1);                \
	} while (0)

#define GE_CMDQ_BUF_SIZE_DEFAULT (0x3c000)

struct ge_dev {
	int irq;
	int iomode_flag;
	spinlock_t lock;
	dma_addr_t cmdq_buf_phy;
	void *cmdq_buf_virt;
	u32 cmdq_buf_size;
	void __iomem *reg_base;
	void __iomem *reg_base_phy;
	void __iomem *sys_reg_base;
	u32 reg_size;
	bool irq_is_install;
	wait_queue_head_t wq;
};

static struct ge_dev *g_ge_dev = NULL;

static void hc_ge_irq(uint32_t param)
{
	struct ge_dev *ge = (struct ge_dev *)param;
	volatile struct cmdq_node_ctx *cmdq_node_ctx =
		(volatile struct cmdq_node_ctx *)ge->cmdq_buf_virt;
	u32 buffer_wraped = 0;
	u32 val = 0;
	unsigned long status = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&ge->lock, flags);

	/*clear INT status.*/
	status = readl(ge->reg_base + 0x08);
	if (status) {
		writel(status, ge->reg_base + 0x08);
	}

	if (1 == ge->iomode_flag) {
		wake_up(&ge->wq);
		spin_unlock_irqrestore(&ge->lock, flags);
		return;
	}

	/*all work done, mark GE is finished so that userspace could see it.*/
	cmdq_node_ctx->serial_num++; /* increase serial*/
	buffer_wraped = NODE_WRAP(cmdq_node_ctx->phy_addr_start);
	if (NODE_ADDR(cmdq_node_ctx->phy_addr_start) ==
	    readl(ge->reg_base + 0x10)) {
		cmdq_node_ctx->is_finish = 1;
		wake_up(&ge->wq);
	} else {
		/*new node available, so go on.*/
		if (buffer_wraped) {
			/*finish tail, now wrap over to start.*/
			if (readl(ge->reg_base + 0x10) ==
			    cmdq_node_ctx->point_wrap_addr) {
				/*clear wrap flag.*/
				NODE_CLEAR_WRAP(cmdq_node_ctx->phy_addr_start);

				/*start from beginning.*/
				writel(cmdq_node_ctx->phy_addr_min,
				       ge->reg_base + 0x10);
				writel(NODE_ADDR(
					       cmdq_node_ctx->phy_addr_start) -
					       4,
				       ge->reg_base + 0x14);
			} else {
				/* tail do not finish, finish tail first */
				writel(cmdq_node_ctx->point_wrap_addr - 4,
				       ge->reg_base + 0x14);
			}
		} else {
			/*no wrap, just start tailing.*/
			writel(NODE_ADDR(cmdq_node_ctx->phy_addr_start) - 4,
			       ge->reg_base + 0x14);
		}

		val = readl(ge->reg_base + 0x00);
		val |= 0x80000000; /*SW access memroy.*/
		val |= (1 << 17); /*enable HQ IRQ.*/
		writel(val, ge->reg_base + 0x00);

		val = readl(ge->reg_base + 0x04);
		val |= 0x2;
		writel(val, ge->reg_base + 0x04); /*start HQ.*/
	}

	spin_unlock_irqrestore(&ge->lock, flags);

	return;
}
#if 0
static void hcge_enable_irq(struct ge_dev *ge, bool val)
{
	val = readl((void __iomem *)(ge->sys_reg_base + 0x38));
	if(val) {
		val |= (1 << 4);
	} else {
		val &= ~(1 << 4);
	}
	writel(val, (void __iomem *)(ge->sys_reg_base + 0x38));

}
#endif

int hcge_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	int ret = 0;
	struct ge_dev *ge = g_ge_dev;
	if (!ge)
		return -ENODEV;

	switch (cmd) {
	case HCGE_REQUEST_IRQ:
		if (!ge->irq_is_install) {
			HCGE_DBG("GE register IRQ\n");
			xPortInterruptInstallISR((uint32_t)ge->irq, hc_ge_irq,
						 (uint32_t)ge);
			ge->irq_is_install = 1;
		} else {
			HCGE_DBG("GE IRQ already registered\n");
		}

		break;

	case HCGE_FREE_IRQ:
#if 0
		if( ge->irq_is_install == 1) {
			devm_free_irq(ge->dev, ge->irq, ge);
			ge->irq_is_install = 0;
		}
#else
		/*hcge_enable_irq(ge, false);*/
#endif
		break;

	case HCGE_RESET: {
		u32 val = 0;
		val = readl((void __iomem *)(ge->sys_reg_base + 0x80));
		val |= (1 << 4);
		writel(val, (void __iomem *)(ge->sys_reg_base + 0x80));

		usleep(10000);

		val = readl((void __iomem *)(ge->sys_reg_base + 0x80));
		val &= ~(1 << 4);
		writel(val, (void __iomem *)(ge->sys_reg_base + 0x80));
		break;
	}

	case HCGE_SET_CLOCK: {
		u32 val = 0;
		val = readl((void __iomem *)(ge->sys_reg_base + 0x7c));
		val &= ~(3 << 18);
		val |= ((arg & 0x3) << 18);
		writel(val, (void __iomem *)(ge->sys_reg_base + 0x7c));
		break;
	}

	case HCGE_SYNC_TIMEOUT: {
		u32 timeout = msecs_to_jiffies(arg);
		volatile struct cmdq_node_ctx *cmdq_node_ctx =
			(volatile struct cmdq_node_ctx *)ge->cmdq_buf_virt;

		ret = wait_event_interruptible_timeout(
			ge->wq,
			cmdq_node_ctx->is_finish ||
				(!(readl(ge->reg_base + 0x8) & (1 << 31))),
			timeout);
		if (ret <= 0) { /*timeout or receive a signal.*/
			HCGE_DBG("wait ge command time out, timeout = %d\n",
				 timeout);
#if 0
			HCGE_DBG("cmdq_node_ctx->is_finish: %d, hq_fst_ptr: 0x%08x, hq_lst_ptr: 0x%08x, gpu status: 0x%08x\n",
			         cmdq_node_ctx->is_finish, readl(ge->reg_base + 0x10), readl(ge->reg_base + 0x14),
			         readl(ge->reg_base + 0x8));
#endif
			return (-2); /*timeout as osal_flag_wait return?.*/
		} else if (!(readl(ge->reg_base + 0x10) & (1 << 31)) &&
			   !cmdq_node_ctx->is_finish) {
			HCGE_DBG(
				"idle: 0x%08x, hq_fst_ptr: 0x%08x, hq_lst_ptr: 0x%08x\n",
				readl(ge->reg_base + 0x8),
				readl(ge->reg_base + 0x10),
				readl(ge->reg_base + 0x14));
		}
		ret = 0;
		break;
	}

	case HCGE_GET_CMDQ_BUFINFO: {
		struct cmdq_buf_info *temp = (struct cmdq_buf_info *)arg;
		temp->addr = (uint32_t)ge->cmdq_buf_phy;
		temp->size = ge->cmdq_buf_size;
		HCGE_DBG("GE get cmdq addr 0x%x size 0x%x\n", (int)temp->addr,
			 (int)temp->size);
		break;
	}
	case HCGE_GET_GE_REGISTER: {
		void **addr = (void **)arg;
		*addr = (void *)g_ge_dev->reg_base;
		break;
	}
	case FIOC_MMAP: { /* Get color plane info */
		void **ppv = (void **)((uintptr_t)arg);
		/* Return the address corresponding to the start of frame buffer. */

		*ppv = (void *)(g_ge_dev->cmdq_buf_virt);

		ret = 0;
		break;
	}

	default:
		break;
	}

	return ret;
}

static int hcge_open(struct file *file)
{
	if (!g_ge_dev)
		return -ENODEV;
	return 0;
}

static int hcge_close(struct file *file)
{
	return 0;
}

static struct file_operations hcge_ops = {
	.open = hcge_open,
	.close = hcge_close,
	.ioctl = hcge_ioctl,
	.read = dummy_read,
	.write = dummy_write,
};

static int hcge_probe(const char *node)
{
	struct ge_dev *ge;
	int ret = 0;
	int np = fdt_node_probe_by_path(node);
	hc_clk_enable(GE_CLK);

	if (np < 0) {
		return 0;
	}

	ge = kmalloc(sizeof(struct ge_dev), GFP_KERNEL);
	if (!ge) {
		dev_err(&pdev->dev, "No enough memory\n");
		return -ENOMEM;
	}
	memset(ge, 0, sizeof(struct ge_dev));

	fdt_get_property_u_32_index(np, "reg", 0, (u32 *)&ge->reg_base);
	fdt_get_property_u_32_index(np, "reg", 1, (u32 *)&ge->reg_size);

	fdt_get_property_u_32_index(np, "reg", 2, (u32 *)&ge->sys_reg_base);

	ge->reg_base_phy = ge->reg_base;

	ge->irq = (int)&GE_INTR;
	xPortInterruptInstallISR((uint32_t)ge->irq, hc_ge_irq, (uint32_t)ge);
	ge->irq_is_install = true;

	fdt_get_property_u_32_index(np, "cmdq_buf_size", 0,
				    (u32 *)&ge->cmdq_buf_size);
	if (ge->cmdq_buf_size == 0)
		ge->cmdq_buf_size = PAGE_ALIGN(GE_CMDQ_BUF_SIZE_DEFAULT);
	else
		ge->cmdq_buf_size = PAGE_ALIGN(ge->cmdq_buf_size);

	ge->cmdq_buf_virt = dma_alloc_coherent(
		NULL, PAGE_ALIGN(ge->cmdq_buf_size), &ge->cmdq_buf_phy, 0);
	if (!ge->cmdq_buf_virt) {
		printk("alloc dma buffer error\n");
		ret = -ENOMEM;
		ge->cmdq_buf_virt = NULL;
		goto err_hdl;
	}

	spin_lock_init(&ge->lock);

	init_waitqueue_head(&ge->wq);

	g_ge_dev = ge;
	printk("Init GE success.\n");

	return register_driver(MODULE_NAME, &hcge_ops, 0666, NULL);
err_hdl:
	if (ge) {
		if (ge->cmdq_buf_virt)
			dma_free_coherent(NULL, PAGE_ALIGN(ge->cmdq_buf_size),
					  ge->cmdq_buf_virt, (dma_addr_t)ge->cmdq_buf_phy);
		if (ge->irq_is_install)
			xPortInterruptRemoveISR((uint32_t)ge->irq, hc_ge_irq);
		free(ge);
	}
	g_ge_dev = NULL;

	return ret;
}

static int __exit hcge_remove(void)
{
	if (!g_ge_dev)
		return -ENODEV;

	if (g_ge_dev) {
		if (g_ge_dev->cmdq_buf_virt)
			dma_free_coherent(NULL, PAGE_ALIGN(g_ge_dev->cmdq_buf_size),
					  g_ge_dev->cmdq_buf_virt, (dma_addr_t)g_ge_dev->cmdq_buf_phy);
		if (g_ge_dev->irq_is_install)
			xPortInterruptRemoveISR((uint32_t)g_ge_dev->irq,
						hc_ge_irq);
		free(g_ge_dev);
		g_ge_dev = NULL;
	}

	return 0;
}

static int ge_init(void)
{
	hcge_probe("/hcrtos/ge@18806000");
	return 0;
}

static int ge_exit(void)
{
	unregister_driver(MODULE_NAME);
	hcge_remove();
}

module_driver(ge, ge_init, ge_exit, 1)
