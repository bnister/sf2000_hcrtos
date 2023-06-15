#include <malloc.h>
#include <string.h>

#include <kernel/vfs.h>
#include <hcuapi/mmz.h>
#include <kernel/module.h>

static int mmz_ioctl(struct file *file , int cmd , unsigned long arg)
{
	int ret = 0;
	struct mmz_blk *mmz = (struct mmz_blk *)arg;

	switch(cmd)
	{
	case MMZ_TOTAL:
		mmz->size = mmz_total(mmz->id);
		break;
	case MMZ_FREE:
		mmz_free(mmz->id, (void *)mmz->addr);
		break;
	case MMZ_MEMALIGN:
		mmz->addr = (uint32_t)mmz_memalign(mmz->id, mmz->alignment, mmz->size);
		if (!mmz->addr) {
			ret = -1;
		}
		break;
	default: {
		ret = -1;
		break;
	}
	}

	return ret;
}

static const struct file_operations mmz_fops = {
	.ioctl = mmz_ioctl,
	.open = dummy_open,
	.close = dummy_close,
	.read = dummy_read,
	.write = dummy_write,
};

int mmz_init(void)
{
	return register_driver("/dev/mmz" , &mmz_fops , 0666 , NULL);
}

module_driver(mmz, mmz_init, NULL, 0)
