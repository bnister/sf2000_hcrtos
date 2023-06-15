#include "pok.h"

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

#include <kernel/ld.h>

#include <linux/io.h>
#include <hcuapi/iocbase.h>

extern unsigned long POKCTRL;
extern unsigned long POK_OUT_INTR;

#define read_buf_len 128
static char read_buf[read_buf_len];

struct pok_device
{
	void	*pok_iobase;
	int	     pok_irq;
	int	     pok_status; 
	SemaphoreHandle_t pok_lock;   
	wait_queue_head_t wait;
};

struct pok_device hc_pok = { 0 };

static int pok_open(struct file *filep)
{
	// struct inode *inode = filep->f_inode;

	return 0;
}

static int pok_close(struct file *filep)
{
	return 0;
}

static ssize_t pok_read(struct file *filep, char * buf,size_t size)
{
	sprintf(read_buf,"%d",hc_pok.pok_status);
	memcpy(buf,read_buf,size);
	// printf("pok status in drvier: %d\n",hc_pok.pok_status);
	
	return 0;
}

static ssize_t pok_write(struct file *filep, const char *buf, size_t size)
{
	// xSemaphoreTake(hc_pok.pok_lock, portMAX_DELAY);
	// xSemaphoreGive(hc_pok.pok_lock);

	return 0;
}

static int pok_poll(struct file *filep, poll_table *wait)
{
	int mask = 0;

	// poll_wait(filep, &hc_pok.wait, wait);

	// if (hc_pok.pok_status)
	// 	mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int pok_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	switch (cmd)
	{
		case POK_READ:
			memcpy((int*)arg,&hc_pok.pok_status,sizeof(int));
			break;

		default:
			break;
	}

	return 0;
}

static const struct file_operations pok_fops = {
	.open 	= 	pok_open,
	.close 	= 	pok_close,
	.read 	= 	pok_read,
	.write 	= 	pok_write,
	.poll 	= 	pok_poll,
	.ioctl 	= 	pok_ioctl
};

static void hc_pok_interrupt(uint32_t priv)
{
	struct pok_device *hs = (struct pok_device *)priv;
	
	//read irq status and save status
	static uint32_t POK_REG_STATUS = 0;
	POK_REG_STATUS = readl(hs->pok_iobase);

	if (((POK_REG_STATUS & POK_OUT33_LEVEL) ==0 )&& (POK_REG_STATUS & POK_FALL33_INT_FLAG))
	{
		printf("the power is less than 2.5V\r\n");
		/*set Pok_ou33_rise_int_enable and set thereshold [0,0]*/
		writel(0xC000401,hs->pok_iobase);
		hs->pok_status = 0;
		// printf("pok status is %d in irq\r\n",hs->pok_status);
	}
	if ((POK_REG_STATUS & POK_RISE33_INT_FLAG))
	{
		printf("Power reaches 3.1V\r\n");
		/*set Pok_ou33_fall_int_enable and set thereshold [1,1]*/
		writel(0xC000831,hs->pok_iobase);
		hs->pok_status = 1;
		// printf("pok status is %d in irq\r\n",hs->pok_status);
	} 

	//clear irq	status
	writeb(0xC,hs->pok_iobase + 0x3);

	return;
}

static int pok_init(void)
{
	struct pok_device *pok_dev = &hc_pok;

	pok_dev->pok_lock = xSemaphoreCreateMutex();
	init_waitqueue_head(&pok_dev->wait);

    pok_dev->pok_iobase = (void *)&POKCTRL;
    pok_dev->pok_irq 	= (int)&POK_OUT_INTR;

	writel(0x0,  pok_dev->pok_iobase);
	writeb(0x08, pok_dev->pok_iobase+0x1);
	writeb(0x00, pok_dev->pok_iobase+0x2);
	writeb(0x0C, pok_dev->pok_iobase+0x3);
	writeb(0x31, pok_dev->pok_iobase);
	
	pok_dev->pok_status = 1;

	xPortInterruptInstallISR(pok_dev->pok_irq,hc_pok_interrupt,(uint32_t)pok_dev);

	return 0;	

}

static int pok_driver_init(void)
{
	int np;
	const char *status;
	const char *path;
	do{
		np = fdt_get_node_offset_by_path("/hcrtos/pok");
		if(np < 0)
		{
			printf("no found pok in dts\n");
			break;
		}
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled"))
			break;

		if (fdt_get_property_string_index(np, "devpath", 0, &path))
			break;


		pok_init();

		register_driver(path,&pok_fops,0666,NULL);
	}while(0);

	return 0;
}

module_system(pok_module, pok_driver_init, NULL, 0)