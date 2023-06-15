/****************************************************************************
 * drivers/adc/adc.c
 *   Copyright (C) 2008-2009 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

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
#include <nuttx/wqueue.h>

#include <kernel/lib/fdt_api.h>
#include <generated/br2_autoconf.h>
#include <fcntl.h>
#include <nuttx/semaphore.h>
#include <linux/kfifo.h>
#include <hcuapi/input.h>

#define LOG_TAG "ADC"
#include <kernel/elog.h>

#include "adc.h"
#include "sar_adc.h"
/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int     adc_open(FAR struct file *filep);
static int     adc_close(FAR struct file *filep);
static ssize_t adc_read(FAR struct file *fielp, FAR char *buffer,
		size_t buflen);
static int     adc_ioctl(FAR struct file *filep, int cmd, unsigned long arg);
static int     adc_receive(FAR struct adc_dev_s *dev, uint8_t ch,
		int32_t data);
static void    adc_notify(FAR struct adc_dev_s *dev);
static int     adc_poll(FAR struct file *filep, poll_table *wait);
void hadc_event(struct adc_dev_s *dev, unsigned int type,  unsigned int code, int value);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_adc_fops =
{
	adc_open,     /* open */
	adc_close,    /* close */
	adc_read,     /* read */
	0,            /* write */
	0,            /* seek */
	adc_ioctl,    /* ioctl */
	adc_poll      /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
		, NULL        /* unlink */
#endif
};

static const struct adc_callback_s g_adc_callback =
{
	adc_receive   /* au_receive */
};


/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adc_open
 *
 * Description:
 *   This function is called whenever the ADC device is opened.
 *
 ****************************************************************************/

static int adc_open(FAR struct file *filep)
{
	FAR struct inode     *inode = filep->f_inode;
	FAR struct adc_dev_s *dev   = inode->i_private;
	uint8_t               tmp;
	int                   ret = 0;

	/* If the port is the middle of closing, wait until the close is
	 * finished.
	 */
	log_i("open adc device\n");

#if 1	
	ret = nxsem_wait(&dev->ad_closesem);
	if (ret >= 0)
	{
		/* Increment the count of references to the device.  If this is the
		 * first time that the driver has been opened for this device, then
		 * initialize the device.
		 */

		tmp = dev->ad_ocount + 1;
		if (tmp == 0)
		{
			/* More than 255 opens; uint8_t overflows to zero */

			ret = -EMFILE;
		}
		else
		{
			/* Check if this is the first time that the driver has been
			 * opened.
			 */

			if (tmp == 1)
			{
				/* Yes.. perform one time hardware initialization. */

				//irqstate_t flags = enter_critical_section();
				ret = dev->ad_ops->ao_setup(dev);
				if (ret == OK)
				{
					/* Mark the FIFOs empty */

					dev->ad_recv.af_head = 0;
					dev->ad_recv.af_tail = 0;

					/* Finally, Enable the ADC RX interrupt */

					dev->ad_ops->ao_rxint(dev, true);

					/* Save the new open count on success */

					dev->ad_ocount = tmp;
				}

				//leave_critical_section(flags);
			}
		}

		nxsem_post(&dev->ad_closesem);
	}
#endif
	return ret;
}

/****************************************************************************
 * Name: adc_close
 *
 * Description:
 *   This routine is called when the ADC device is closed.
 *   It waits for the last remaining data to be sent.
 *
 ****************************************************************************/

static int adc_close(FAR struct file *filep)
{
	FAR struct inode     *inode = filep->f_inode;
	FAR struct adc_dev_s *dev   = inode->i_private;
	//irqstate_t            flags;
	int                   ret = 0;

	//log_i("%s enter\n", __FUNCTION__);
#if 1
	ret = nxsem_wait(&dev->ad_closesem);
	if (ret >= 0)
	{
		/* Decrement the references to the driver.  If the reference count will
		 * decrement to 0, then uninitialize the driver.
		 */

		if (dev->ad_ocount > 1)
		{
			dev->ad_ocount--;
			nxsem_post(&dev->ad_closesem);
		}
		else
		{
			/* There are no more references to the port */

			dev->ad_ocount = 0;

			/* Free the IRQ and disable the ADC device */

			//flags = enter_critical_section();    /* Disable interrupts */
			dev->ad_ops->ao_shutdown(dev);       /* Disable the ADC */
			//leave_critical_section(flags);

			nxsem_post(&dev->ad_closesem);
		}
	}
#endif
	return ret;
}

/****************************************************************************
 * Name: adc_read
 ****************************************************************************/
//extern u32 gadc_data;
static ssize_t adc_read(FAR struct file *filep, FAR char *buffer,
                        size_t buflen)
{
 	struct inode *inode = filep->f_inode;
	struct adc_dev_s *dev = inode->i_private;

	if (kfifo_out(&dev->kfifo, (struct input_event *)buffer, 1))
		return sizeof(struct input_event);
	else
		return 0;
}

/****************************************************************************
 * Name: adc_ioctl
 ****************************************************************************/

static int adc_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	FAR struct inode *inode = filep->f_inode;
	FAR struct adc_dev_s *dev = inode->i_private;
	int ret = 0;
	log_i("%s enter\n", __FUNCTION__);
	ret = dev->ad_ops->ao_ioctl(dev, cmd, arg);
	return ret;
}

/****************************************************************************
 * Name: adc_receive
 ****************************************************************************/

static int adc_receive(FAR struct adc_dev_s *dev, uint8_t ch, int32_t data)
{
	hadc_event(dev, EV_KEY,ch,data);
	return OK;
}

/****************************************************************************
 * Name: adc_pollnotify
 ****************************************************************************/

static void adc_pollnotify(FAR struct adc_dev_s *dev, uint32_t type)
{
	log_i("adc_pollnotify enter\n");
#if 0
	for (i = 0; i < CONFIG_ADC_NPOLLWAITERS; i++)
	{
		struct pollfd *fds = dev->fds[i];
		if (fds)
		{
			fds->revents |= type;
			nxsem_post(fds->sem);
		}
	}
#endif	
}

/****************************************************************************
 * Name: adc_notify
 ****************************************************************************/

static void adc_notify(FAR struct adc_dev_s *dev)
{
	log_i("adc_notify enter\n");
#if 0
	/* If there are threads waiting on poll() for data to become available,
	 * then wake them up now.
	 */

	adc_pollnotify(dev, POLLIN);

	/* If there are threads waiting for read data, then signal one of them
	 * that the read data is available.
	 */

	if (dev->ad_nrxwaiters > 0)
	{
		//nxsem_post(&fifo->af_sem);
	}
#endif	
}

/****************************************************************************
 * Name: adc_poll
 ****************************************************************************/

static int adc_poll(FAR struct file *filep, poll_table *wait)
{
	FAR struct inode     *inode = filep->f_inode;
	FAR struct adc_dev_s *dev   = inode->i_private;
	//log_i("adc_poll enter\n");
	/* Interrupts must be disabled while accessing the list of poll structures
	 * and ad_recv FIFO.
	 */
#if 1 // add poll method
	int mask = 0;

	poll_wait(filep, &dev->wait, wait);

	if (!kfifo_is_empty(&dev->kfifo))
		mask |= POLLIN | POLLRDNORM;

	return mask;
#endif	
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#define ADC1_NCHANNELS 1///3
//#define ADC2_NCHANNELS 3

/****************************************************************************
 * Private Data
 ****************************************************************************/
static const uint8_t g_chanlist1[ADC1_NCHANNELS] =
{
	/* Arduino Interface pins A0 to A2 */
	0
};
#if 0
static const uint8_t g_chanlist1[ADC1_NCHANNELS] =
{
	/* Arduino Interface pins A0 to A2 */
	15,
	0,
	9,
};
#endif
/****************************************************************************
 * Name: adc_register
 ****************************************************************************/

int adc_register(FAR const char *path, FAR struct adc_dev_s *dev)
{
	int ret = 0;
	//log_i("adc_register enter\n");

	DEBUGASSERT(path != NULL && dev != NULL);
#if 1
	/* Bind the upper-half callbacks to the lower half ADC driver */

	DEBUGASSERT(dev->ad_ops != NULL && dev->ad_ops->ao_bind != NULL);
	ret = dev->ad_ops->ao_bind(dev, &g_adc_callback);
	if (ret < 0)
	{
		//aerr("ERROR: Failed to bind callbacks: %d\n", ret);
		log_i("bind callback error\n");

		return ret;
	}

	/* Initialize the ADC device structure */
	dev->ad_ocount = 0;

	/* Initialize semaphores */

	//nxsem_init(&dev->ad_recv.af_sem, 0, 0);
	nxsem_init(&dev->ad_closesem, 0, 1);

	/* The receive semaphore is used for signaling and, hence, should not have
	 * priority inheritance enabled.
	 */

	//nxsem_set_protocol(&dev->ad_recv.af_sem, SEM_PRIO_NONE);

	/* Reset the ADC hardware */
	DEBUGASSERT(dev->ad_ops->ao_reset != NULL);
	dev->ad_ops->ao_reset(dev);

	/* Register the ADC character driver */
	ret = register_driver(path, &g_adc_fops, 0444, dev);
	if (ret < 0)
	{
		log_i(" adc register_driver fail\n");
		//nxsem_destroy(&dev->ad_recv.af_sem);
		nxsem_destroy(&dev->ad_closesem);
	}
#endif
	return ret;
}

static int adc_module_init(void)
{
	int np =0;
	char name[128];
	const char *path = NULL;
	struct adc_dev_s *adc_dev = NULL;

	sprintf(name, "/hcrtos/adc");
	np = fdt_node_probe_by_path(name);
	if (np < 0)
		return 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		log_i("get property devpath fail\n");

	adc_dev = hc_adcinitialize(1, g_chanlist1, ADC1_NCHANNELS);
	if (adc_dev == NULL)
	{
		log_e("ERROR: Failed to get ADC interface for ADC1\n");
		return -ENODEV;
	}
	INIT_KFIFO(adc_dev->kfifo);
	init_waitqueue_head(&adc_dev->wait);

	adc_register(path, adc_dev);

}
module_driver(adc, adc_module_init, NULL, 1)

void hadc_event(struct adc_dev_s *dev, unsigned int type, unsigned int code, int value)
{
	struct input_event event = { type,  code, value };

	kfifo_put(&dev->kfifo, event);
	wake_up(&dev->wait);
}

int adc_register_device(struct adc_dev_s *dev)
{
	int id = 0;//fls32(id_bitmap);
	char path[128];

	dev->id = id;
	//id_bitmap &= ~BIT(id);

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/adc%d", id);

	//register_driver(path, &input_ops, 0666, dev);

	return 0;
}

void adc_unregister_device(struct adc_dev_s *dev)
{
	char path[128];
	int id = dev->id;

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/adc%d", id);
	unregister_driver(path);
}

