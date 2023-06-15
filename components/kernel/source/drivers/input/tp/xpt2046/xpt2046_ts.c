#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include <stdio.h>
#include <kernel/drivers/spi.h>
#include <kernel/drivers/spi.h>
#include <nuttx/fs/fs.h>

#include <errno.h>
#include <kernel/drivers/spi.h>
#include <nuttx/fs/fs.h>
#include <hcuapi/spidev.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <kernel/lib/fdt_api.h>
#include <linux/device.h>

#include "xpt2046_ts.h"
#include <hcuapi/gpio.h>
#include <linux/byteorder/generic.h>
#include <linux/byteorder/little_endian.h>
#include "calibration_ts.h"
#include <linux/delay.h>


#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
				| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)


#define TS_POLL_DELAY	(10 * 1000000)	/* ns delay before the first sample */
#define TS_POLL_PERIOD	(20 * 1000000)	/* ns delay between samples */

// #define DEBUG
#if DEBUG
	#define xpt2046printInfo(msg...)	printf(msg);
	#define xpt2046printDebug(msg...)	printf(KERN_ERR msg);
#else
	#define xpt2046printInfo(msg...)
	#define xpt2046printDebug(msg...)
#endif

struct spidev_data {
	struct spi_device	*spi;

	struct mutex		buf_lock;
	void			*tx_buffer;
	void			*rx_buffer;
	unsigned		speed_hz;
};

static int xpt2046_open(struct file *filp)
{
	struct inode		*inode = filp->f_inode;
	struct spidev_data	*spidev = inode->i_private;

	filp->f_priv = spidev;

	return 0;
}

static ssize_t xpt2046_write(struct file *filp, const char *buf,size_t count)
{
	struct inode		*inode = filp->f_inode;
	struct spidev_data	*spidev = inode->i_private;

	filp->f_priv = spidev;

	return 0;
}

static ssize_t xpt2046_read(struct file *filp, char *buf, size_t count)
{
	struct inode		*inode = filp->f_inode;
	struct spidev_data	*spidev = inode->i_private;

	filp->f_priv = spidev;

	return 0;
}

static ssize_t spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	int status;
	struct spi_device *spi;

	spi = spidev->spi;

	if (spi == NULL)
		status = -EINVAL;
	else
		status = spi_sync(spi, message);

	if (status == 0)
		status = message->actual_length;

	return status;
}

static int spidev_message(struct spidev_data *spidev,
		struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
	struct spi_message	msg;
	struct spi_transfer	*k_xfers;
	struct spi_transfer	*k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned		n, total;
	int			status = -EFAULT;

	spi_message_init(&msg);
	k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	total = 0;
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		k_tmp->len = u_tmp->len;

		total += k_tmp->len;
		/* Since the function returns the total length of transfers
		 * on success, restrict the total to positive int values to
		 * avoid the return value looking like an error.  Also check
		 * each transfer length to avoid arithmetic overflow.
		 */
		if (total > INT_MAX || k_tmp->len > INT_MAX) {
			status = -EMSGSIZE;
			goto done;
		}

		if (u_tmp->rx_buf) {
			k_tmp->rx_buf = (void *)u_tmp->rx_buf;
		}
		if (u_tmp->tx_buf) {
			k_tmp->tx_buf = (void *)u_tmp->tx_buf;
		}

		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->tx_nbits = u_tmp->tx_nbits;
		k_tmp->rx_nbits = u_tmp->rx_nbits;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay_usecs = u_tmp->delay_usecs;
		k_tmp->speed_hz = u_tmp->speed_hz;
		if (!k_tmp->speed_hz)
			k_tmp->speed_hz = spidev->speed_hz;
#ifdef VERBOSE
		dev_dbg(&spidev->spi->dev,
			"  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
			u_tmp->len,
			u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ? : spidev->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ? : spidev->spi->max_speed_hz);
#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	status = spidev_sync(spidev, &msg);
	if (status < 0)
		goto done;

	status = total;

done:
	kfree(k_xfers);
	return status;
}

static int xpt2046_ioctl(struct file *filp, int cmd, unsigned long arg)
{
    int			retval = 0;
	struct spidev_data	*spidev;
	struct spi_device	*spi;
	uint32_t		tmp;
	unsigned		n_ioc;
	struct spi_ioc_transfer	*ioc;
	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOCBASE)
		return -ENOTTY;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->f_priv;
	spi = spidev->spi;

	if (spi == NULL)
		return -EINVAL;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	mutex_lock(&spidev->buf_lock);

	switch (cmd) {
	/* read requests */
	case SPI_IOC_RD_MODE:
		*(uint8_t *)arg = spi->mode & SPI_MODE_MASK;
		retval = 0;
		break;
	case SPI_IOC_RD_MODE32:
		*(uint32_t *)arg = spi->mode & SPI_MODE_MASK;
		retval = 0;
		break;
	case SPI_IOC_RD_LSB_FIRST:
		*(uint8_t *)arg = (spi->mode & SPI_LSB_FIRST) ?  1 : 0;
		retval = 0;
		break;
	case SPI_IOC_RD_BITS_PER_WORD:
		*(uint8_t *)arg = spi->bits_per_word;
		retval = 0;
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:
		*(uint32_t *)arg = spidev->speed_hz;
		retval = 0;
		break;

	/* write requests */
	case SPI_IOC_WR_MODE:
	case SPI_IOC_WR_MODE32:
		if ((uint)cmd == SPI_IOC_WR_MODE)
			tmp = *(uint8_t *)arg;
		else
			tmp = *(uint32_t *)arg;
		retval = 0;
		if (retval == 0) {
			uint32_t save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (uint16_t)tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
		tmp = *(uint8_t *)arg;
		retval = 0;
		if (retval == 0) {
			uint32_t save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
		tmp = *(uint8_t *)arg;
		retval = 0;
		if (retval == 0) {
			uint8_t save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
		tmp = *(uint32_t *)arg;
		retval = 0;
		if (retval == 0) {
			uint32_t save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval >= 0)
				spidev->speed_hz = tmp;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
			spi->max_speed_hz = save;
		}
		break;

	default:
		/* segmented and/or full-duplex I/O request */
		/* Check message and copy into scratch area */
		n_ioc = _IOC_SIZE(cmd) / sizeof(struct spi_ioc_transfer);
		ioc = (struct spi_ioc_transfer *)arg;
		if (!ioc || !n_ioc)
			break;	/* n_ioc is also 0 */

		/* translate to spi_message, execute */
		retval = spidev_message(spidev, ioc, n_ioc);
		break;
	}

	mutex_unlock(&spidev->buf_lock);
	return retval;
}

static const struct file_operations xpt2046_fops = {
	.open   = xpt2046_open,
	.close  = NULL,
	.read   = xpt2046_read,
	.write  = xpt2046_write,
	.seek   = NULL,
	.ioctl  = xpt2046_ioctl,
	.poll   = NULL
};

/* ------------------------------------------------------------------------- */

static int get_pendown_state(struct xpt2046 *ts)
{
	if (ts->get_pendown_state)
		return ts->get_pendown_state();

	// return !gpio_get_value(ts->gpio_pendown);
	return !gpio_get_input(ts->gpio_pendown);
}

static int xpt2046_verifyAndConvert(struct xpt2046 *ts,int adx, int ady,int *x, int *y)
{

	xpt2046printInfo("%s:(%d,%d)(%d/%d)\n",__FUNCTION__, adx, ady, *x, *y);
	
	if((*x< ts->x_min) || (*x > ts->x_max))
		return 1;

	if((*y< ts->y_min) || (*y > ts->y_max + ts->touch_virtualkey_length))
		return 1;


	return 0;
}

static void xpt2046_rx(void *xpt)
{
	struct xpt2046		*ts = xpt;
	struct xpt2046_packet	*packet = ts->packet;
	unsigned		Rt = 1;
	u16			x, y;
	int 		cal_x,cal_y;
	/* xpt2046_rx_val() did in-place conversion (including byteswap) from
	 * on-the-wire format as part of debouncing to get stable readings.
	 */
	x = packet->tc.x;
	y = packet->tc.y;

	//printk(KERN_ERR"***>%s:x=%d,y=%d\n",__FUNCTION__,x,y);

	/* range filtering */
	if (x == MAX_12BIT)
		x = 0;

	/* Sample found inconsistent by debouncing or pressure is beyond
	 * the maximum. Don't report it to user space, repeat at least
	 * once more the measurement
	 */
	if (packet->tc.ignore) {

		xpt2046printInfo("%s:ignored=%d\n",__FUNCTION__,packet->tc.ignore);
	
		mod_timer(&ts->timer, jiffies + TS_POLL_PERIOD / 1000000);
		return;
	}

	/* Maybe check the pendown state before reporting. This discards
	 * false readings when the pen is lifted.
	 */
	if (ts->penirq_recheck_delay_usecs) {
		udelay(ts->penirq_recheck_delay_usecs);
		if (!get_pendown_state(ts))
		{
			xpt2046printInfo("%s:get_pendown_state(ts)==0,discard false reading\n",__FUNCTION__);
			Rt = 0;
		}
	}

	/* NOTE: We can't rely on the pressure to determine the pen down
	 * state, even this controller has a pressure sensor.  The pressure
	 * value can fluctuate for quite a while after lifting the pen and
	 * in some cases may not even settle at the expected value.
	 *
	 * The only safe way to check for the pen up condition is in the
	 * timer by reading the pen signal state (it's a GPIO _and_ IRQ).
	 */
	if (Rt) {
		struct input_dev *input = ts->input;
		
		TouchPanelCalibrateAPoint(x, y, &cal_x, &cal_y);
    
    	cal_x = cal_x / 4;
    	cal_y = cal_y / 4;
		gADPoint.x = x;
        gADPoint.y = y;
		
		if (ts->swap_xy)
			swap(cal_x, cal_y);	
		
		if(xpt2046_verifyAndConvert(ts,cal_x,cal_y,&cal_x,&cal_y))
		{
			xpt2046printInfo("%s:xpt2046_verifyAndConvert fail\n",__FUNCTION__);
			// input_report_abs(input, ABS_X, cal_x);
			// input_report_abs(input, ABS_Y, cal_y);
			// input_sync(input);
			// printf("x:%d,y:%d\n",cal_x,cal_y);
			xpt2046printInfo("x:%d,y:%d\n",cal_x,cal_y);
			goto out;
		}
		
		if (!ts->pendown) {
			input_report_key(input, BTN_TOUCH, 1);
			ts->pendown = 1;
			xpt2046printInfo("%s:input_report_key(pen down)\n",__FUNCTION__);
		}	
#if	1	
		input_report_abs(input, ABS_X, cal_x);
		input_report_abs(input, ABS_Y, cal_y);
		input_sync(input);
		//printf("x:%d,y:%d\n",cal_x,cal_y);
		xpt2046printInfo("x:%d,y:%d\n",cal_x,cal_y);

#else
		input_report_abs(input, ABS_MT_PRESSURE, 10);
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, 1);
		input_report_abs(input, ABS_MT_POSITION_X, cal_x);
		input_report_abs(input, ABS_MT_POSITION_Y, cal_x);
		input_mt_sync(input);
		
		input_sync(ts->input);
#endif
		xpt2046printInfo("%s:input_report_abs(%4d/%4d)\n",__FUNCTION__,cal_x, cal_y);
	}
out:
	mod_timer(&ts->timer, jiffies + TS_POLL_PERIOD / 1000000);
}

static void xpt2046_rx_val(void *xpt)
{
	struct xpt2046 *ts = xpt;
	struct xpt2046_packet *packet = ts->packet;
	struct spi_message *m;
	struct spi_transfer *t;
	int val;
	int action;
	int status;
	
	m = &ts->msg[ts->msg_idx];
	t = list_entry(m->transfers.prev, struct spi_transfer, transfer_list);

	/* adjust:  on-wire is a must-ignore bit, a BE12 value, then padding;
	 * built from two 8 bit values written msb-first.
	 */
	val = (__be16_to_cpup((__be16 *)t->rx_buf) >> 3) & 0x0fff;

	xpt2046printInfo("%s:value=%d\n",__FUNCTION__,val);
	
	action = ts->filter(ts->filter_data, ts->msg_idx, &val);
	switch (action) {
	case XPT2046_FILTER_REPEAT:
		xpt2046printInfo("%s:XPT2046_FILTER_REPEAT:value=%d\n",__FUNCTION__,val);
		break;
	case XPT2046_FILTER_IGNORE:
		packet->tc.ignore = 1;
		/* Last message will contain xpt2046_rx() as the
		 * completion function.
		 */
		 xpt2046printInfo("%s:XPT2046_FILTER_IGNORE:value=%d\n",__FUNCTION__,val);
		m = ts->last_msg;
		break;
	case XPT2046_FILTER_OK:
		*(u16 *)t->rx_buf = val;
		packet->tc.ignore = 0;
		m = &ts->msg[++ts->msg_idx];
		xpt2046printInfo("%s:XPT2046_FILTER_OK:value=%d\n",__FUNCTION__,val);
		break;
	default:
		// BUG();
		xpt2046printInfo("%d BUG\n",__LINE__);
	}
	ts->wait_for_sync();
	status = spi_async(ts->spi, m);
	if (status)
		dev_err(&ts->spi->dev, "spi_async --> %d\n",
				status);
}

static void null_wait_for_sync(void)
{
	
}

static int xpt2046_read12_dfr(struct spi_device	*spi, unsigned command)
{
	// struct spi_device	*spi = to_spi_device(dev);
	struct xpt2046		*ts = spi->priv;
	struct dfr_req		*req = kzalloc(sizeof *req, GFP_KERNEL);
	int			status;

	if (!req)
		return -ENOMEM;

	spi_message_init(&req->msg);

	/* take sample */
	req->command = (u8) command;
	req->xfer[0].tx_buf = &req->command;
	req->xfer[0].len = 1;
	spi_message_add_tail(&req->xfer[0], &req->msg);

	req->xfer[1].rx_buf = &req->sample;
	req->xfer[1].len = 2;
	spi_message_add_tail(&req->xfer[1], &req->msg);

	/* converter in low power mode & enable PENIRQ */
	req->pwrdown= PWRDOWN;
	req->xfer[2].tx_buf = &req->pwrdown;
	req->xfer[2].len = 1;
	spi_message_add_tail(&req->xfer[2], &req->msg);

	req->xfer[3].rx_buf = &req->dummy;
	req->xfer[3].len = 2;
	CS_CHANGE(req->xfer[3]);
	spi_message_add_tail(&req->xfer[3], &req->msg);

	ts->irq_disabled = 1;
	gpio_irq_disable(spi->irq);
	status = spi_sync(spi, &req->msg);
	ts->irq_disabled = 0;
	gpio_irq_enable(spi->irq);
	
	if (status == 0) {
		/* on-wire is a must-ignore bit, a BE12 value, then padding */
		status = be16_to_cpu(req->sample);
		status = status >> 3;
		status &= 0x0fff;
		xpt2046printInfo("***>%s:status=%d\n",__FUNCTION__,status);
	}

	kfree(req);
	return status;
}

static int xpt2046_debounce(void *xpt, int data_idx, int *val)
{
	struct xpt2046		*ts = xpt;
	static int average_val[2];
	
	xpt2046printInfo("%s:%d,%d,%d,%d,%d,%d,%d,%d\n",__FUNCTION__,
		data_idx,ts->last_read,
	  ts->read_cnt,ts->debounce_max,
		abs(ts->last_read - *val),ts->debounce_tol,
		ts->read_rep,ts->debounce_rep);
	
	/* discard the first sample. */
	 //on info_it50, the top-left area(1cmx1cm top-left square ) is not responding cause the first sample is invalid, @sep 17th
	if(!ts->read_cnt)
	{
		//udelay(100);
		ts->read_cnt++;
		return XPT2046_FILTER_REPEAT;
	}
	if(*val == 4095 || *val == 0)
	{
		ts->read_cnt = 0;
		ts->last_read = 0;
		memset(average_val,0,sizeof(average_val));
		xpt2046printInfo("%s:*val == 4095 || *val == 0\n",__FUNCTION__);
		return XPT2046_FILTER_IGNORE;
	}


	if (ts->read_cnt==1 || (abs(ts->last_read - *val) > ts->debounce_tol)) {
		/* Start over collecting consistent readings. */
		ts->read_rep = 1;
		average_val[data_idx] = *val;
		/* Repeat it, if this was the first read or the read
		 * wasn't consistent enough. */
		if (ts->read_cnt < ts->debounce_max) {
			ts->last_read = *val;
			ts->read_cnt++;
			return XPT2046_FILTER_REPEAT;
		} else {
			/* Maximum number of debouncing reached and still
			 * not enough number of consistent readings. Abort
			 * the whole sample, repeat it in the next sampling
			 * period.
			 */
			ts->read_cnt = 0;
			ts->last_read = 0;
			memset(average_val,0,sizeof(average_val));
			xpt2046printInfo("%s:XPT2046_FILTER_IGNORE\n",__FUNCTION__);
			return XPT2046_FILTER_IGNORE;
		}
	} 
	else {
		average_val[data_idx] += *val;
		
		if (++ts->read_rep >= ts->debounce_rep) {
			/* Got a good reading for this coordinate,
			 * go for the next one. */
			ts->read_cnt = 0;
			ts->read_rep = 0;
			ts->last_read = 0;
			*val = average_val[data_idx]/(ts->debounce_rep);
			return XPT2046_FILTER_OK;
		} else {
			/* Read more values that are consistent. */
			ts->read_cnt++;
			
			return XPT2046_FILTER_REPEAT;
		}
	}
}

static int device_suspended(struct xpt2046 *ts)
{
	// struct xpt2046 *ts = dev_get_drvdata(dev);
	return ts->is_suspended || ts->disabled;
}

static void xpt2046_timer(struct timer_list *param)
{
    // xpt2046printInfo("t\n");

	//struct xpt2046  *ts = from_timer(param, struct xpt2046, timer);

	struct xpt2046  *ts = container_of(param, struct xpt2046, timer);
	int		status = 0;
	
	spin_lock(&ts->lock);

	if (unlikely(!get_pendown_state(ts) ||
		     device_suspended(ts->spi->priv))) {
		if (ts->pendown) {
			struct input_dev *input = ts->input;
			input_report_key(input, BTN_TOUCH, 0);
			input_sync(input);
			//input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
			//input_mt_sync(input);
			//input_sync(ts->input);

			ts->pendown = 0;
			gFirstIrq = 1;
			
			xpt2046printInfo("%s:input_report_key(The touchscreen up)\n",__FUNCTION__);
		}

		/* measurement cycle ended */
		if (!device_suspended(ts->spi->priv)) {
			// xpt2046printInfo("%s:device_suspended==0\n",__FUNCTION__);
			ts->irq_disabled = 0;
			gpio_irq_enable(ts->spi->irq);
		}
		ts->pending = 0;
	} else {
		if(gFirstIrq)
		{
			xpt2046printInfo("%s:delay 15 ms!!!\n",__FUNCTION__);
			gFirstIrq = 0;
			mdelay(15);
			xpt2046printInfo("%s:end delay!!!\n",__FUNCTION__);
		}
		/* pen is still down, continue with the measurement */
		xpt2046printInfo("%s:pen is still down, continue with the measurement\n",__FUNCTION__);
		ts->msg_idx = 0;
		ts->wait_for_sync();
		status = spi_async(ts->spi, &ts->msg[0]);
		if (status)
			dev_err(&ts->spi->dev, "spi_async --> %d\n", status);
	}

	spin_unlock(&ts->lock);
	return;
	return;
}

static void xpt2046_irq(uint32_t param)
{
	struct xpt2046 *ts = (void *)param;
	unsigned long flags;
	
	spin_lock_irqsave(&ts->lock, flags);

	if (likely(get_pendown_state(ts))) {
		if (!ts->irq_disabled) {
			/* The ARM do_simple_IRQ() dispatcher doesn't act
			 * like the other dispatchers:  it will report IRQs
			 * even after they've been disabled.  We work around
			 * that here.  (The "generic irq" framework may help...)
			 */
			ts->irq_disabled = 1;
			// disable_irq_nosync(ts->spi->irq);
			gpio_irq_disable(ts->spi->irq);
			ts->pending = 1;
			mod_timer(&ts->timer, jiffies + TS_POLL_DELAY / 1000000);
		}
	}
	spin_unlock_irqrestore(&ts->lock, flags);

	return ;

}

int tp_calib_init(void)
{
    // unsigned char ret;
    // int cali_num = 5;
    int screen_x[5], screen_y[5];
    int uncali_x[5], uncali_y[5];
    int tst_uncali_x, tst_uncali_y, tst_cali_x, tst_cali_y;
    
    screen_x[0] = 15; screen_y[0] = 15;
    screen_x[1] = 15; screen_y[1] = 595;
    screen_x[2] = 785; screen_y[2] = 15;
    screen_x[3] = 785; screen_y[3] = 595;
    screen_x[4] = 400; screen_y[4] = 300;
  #if 0
    uncali_x[0] = 3810; uncali_y[0] = 3830;
    uncali_x[1] = 3810; uncali_y[1] = 340;
    uncali_x[2] = 240; uncali_y[2] = 3830;
    uncali_x[3] = 240; uncali_y[3] = 335;
    uncali_x[4] = 2028; uncali_y[4] = 2130;

  uncali_x[0] = 3180; uncali_y[0] = 3495;
 uncali_x[1] = 3795; uncali_y[1] = 630;
 uncali_x[2] = 220; uncali_y[2] = 3410;
 uncali_x[3] = 940; uncali_y[3] = 768;
 uncali_x[4] = 1950; uncali_y[4] = 2100;

 #else
     uncali_x[0] = 3780; uncali_y[0] = 3800;
    uncali_x[1] = 3780; uncali_y[1] = 360;
    uncali_x[2] = 250; uncali_y[2] = 3810;
    uncali_x[3] = 260; uncali_y[3] = 350;
    uncali_x[4] = 2020; uncali_y[4] = 2120;
	
 #endif
    // ret = tp_calib_iface_init(cali_num, screen_x, screen_y, uncali_x, uncali_y);
	// if(ret ==0)
	// {
	// 	 xpt2046printInfo("TouchPanelSetCalibration OK.\n");
	// }
	// else
	// {
	// 	 xpt2046printInfo("TouchPanelSetCalibration FAIL.\n");
	// }

    tst_uncali_x = 2028;
    tst_uncali_y = 2130;
    
    TouchPanelCalibrateAPoint(tst_uncali_x, tst_uncali_y,
                              &tst_cali_x, &tst_cali_y);
    
    xpt2046printInfo("(%d, %d) >> (%d, %d)\n", tst_uncali_x, tst_uncali_y,
                                     tst_cali_x/4, tst_cali_y/4);
    
    tst_uncali_x = 400;
    tst_uncali_y = 400;
    
    TouchPanelCalibrateAPoint(tst_uncali_x, tst_uncali_y,
                              &tst_cali_x, &tst_cali_y);
    
    xpt2046printInfo("(%d, %d) >> (%d, %d)\n", tst_uncali_x, tst_uncali_y,
                                     tst_cali_x/4, tst_cali_y/4);

    tst_uncali_x = 3800;
    tst_uncali_y = 3800;
    
    TouchPanelCalibrateAPoint(tst_uncali_x, tst_uncali_y,
                              &tst_cali_x, &tst_cali_y);
    
    xpt2046printInfo("(%d, %d) >> (%d, %d)\n", tst_uncali_x, tst_uncali_y,
                                     tst_cali_x/4, tst_cali_y/4);

    tst_uncali_x = 400;
    tst_uncali_y = 3800;
    
    TouchPanelCalibrateAPoint(tst_uncali_x, tst_uncali_y,
                              &tst_cali_x, &tst_cali_y);
    
    xpt2046printInfo("(%d, %d) >> (%d, %d)\n", tst_uncali_x, tst_uncali_y,
                                     tst_cali_x/4, tst_cali_y/4);

    tst_uncali_x = 3800;
    tst_uncali_y = 400;
    
    TouchPanelCalibrateAPoint(tst_uncali_x, tst_uncali_y,
                              &tst_cali_x, &tst_cali_y);
    
    xpt2046printInfo("(%d, %d) >> (%d, %d)\n", tst_uncali_x, tst_uncali_y,
                                     tst_cali_x/4, tst_cali_y/4);

	return 0;
									 
}

static int xpt2046_probe(const char *master)
{   
    int np, ret;
	const char *path;
	struct spi_device *spi = NULL;
	struct spidev_data *spidev = NULL;
	gpio_pinset_t int_pinset = GPIO_DIR_INPUT | GPIO_IRQ_FALLING /* | GPIO_IRQ_RISING */;
	unsigned long pin_num = 0;
	
	struct xpt2046			*ts;
	struct xpt2046_packet		*packet;
	struct input_dev		*input_dev;
	struct spi_message		*m;
	struct spi_transfer		*x;
	int				vref;

	spi = kzalloc(sizeof(*spi), GFP_KERNEL);
	if (!spi) {
		ret = -ENOMEM;
		goto err_probe;
	}

    np = fdt_get_node_offset_by_path("/hcrtos/spi-gpio/xpt2046");
    if (np < 0){
        xpt2046printInfo("no find np in dts\n");
        return -1;
    }
	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return -1;
    
	if (fdt_get_property_u_32_index(np, "touch-gpio", 0, (u32 *)&pin_num))
		return -1;
	spi->irq = pin_num;
	gpio_configure(spi->irq,int_pinset);

	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev) {
		ret = -ENOMEM;
		goto err_probe;
	}

    spi->controller = spi_find_controller(master);
	if (!spi->controller) {
		ret = -EFAULT;
		goto err_probe;
	}

    spi->master = spi->controller;
	spi->max_speed_hz = 50 * 1000 * 1000;
	spi->bits_per_word = 8;
	spi->chip_select = 0;
	spi->mode = SPI_MODE_0;
	spi->cs_gpio = PINPAD_INVALID;

	ret = spi_add_device(spi);
	if (ret)
		goto err_probe;

	spidev->spi = spi;
	spidev->speed_hz = spi->max_speed_hz;
	mutex_init(&spidev->buf_lock);

	ret = register_driver(path, &xpt2046_fops, 0666, (void *)spidev);
	if (ret < 0) {
		xpt2046printInfo("ERROR: register_driver() failed: %d\n", ret);
		goto err_probe;
	}

	ts = kzalloc(sizeof(struct xpt2046), GFP_KERNEL);
	packet = kzalloc(sizeof(struct xpt2046_packet), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !packet || !input_dev) {
		ret = -ENOMEM;
	}

	spi_set_drvdata(spi, ts);
	// dev_set_drvdata(&spi->dev,ts);
	ts->packet = packet;
	ts->spi = spi;
	ts->input = input_dev;
	ts->swap_xy =0;// pdata->swap_xy;
	gFirstIrq = 1;
	ts->gpio_pendown = spi->irq;

	ret = gpio_irq_request(spi->irq,xpt2046_irq,(uint32_t)ts);
	if (ret < 0) {
		xpt2046printInfo("ERROR: gpio_irq_request() failed: %d\n", ret);
		goto err_probe;
	}

	timer_setup(&ts->timer, xpt2046_timer, 0);
	spin_lock_init(&ts->lock);

	ts->model = 2046;

	ts->debounce_max = DEBOUNCE_MAX;
	if (ts->debounce_max < DEBOUNCE_REP)
		ts->debounce_max = DEBOUNCE_REP;
	ts->debounce_tol = DEBOUNCE_TOL;
	ts->debounce_rep = DEBOUNCE_REP;
	ts->filter = xpt2046_debounce;
	ts->filter_data = ts;	
	ts->penirq_recheck_delay_usecs = PENIRQ_RECHECK_DELAY_USECS;

	ts->wait_for_sync = null_wait_for_sync;
	ts->x_min = 0;//pdata->x_min;
	ts->x_max =SCREEN_MAX_X;// pdata->x_max;
	ts->y_min =0;// pdata->y_min;
	ts->y_max =SCREEN_MAX_Y;// pdata->y_max;

	ts->touch_virtualkey_length = 0;

	input_set_abs_params(input_dev, ABS_X,
			ts->x_min ? : 0,
			ts->x_max ? : MAX_12BIT,
			0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			ts->y_min ? : 0,
			ts->y_max ? : MAX_12BIT,
			0, 0);
	ret = input_register_device(input_dev);
	
	vref = 1;

	m = &ts->msg[0];
	x = ts->xfer;

	spi_message_init(m);
/* y- still on; turn on only y+ (and ADC) */
	packet->read_y = READ_Y(vref);
	x->tx_buf = &packet->read_y;
	x->len = 1;
	spi_message_add_tail(x, m);
	x++;
	x->rx_buf = &packet->tc.y;
	x->len = 2;
	spi_message_add_tail(x, m);

	m->complete = xpt2046_rx_val;
	m->context = ts;

	m++;
	spi_message_init(m);

	/* turn y- off, x+ on, then leave in lowpower */
	x++;
	packet->read_x = READ_X(vref);
	x->tx_buf = &packet->read_x;
	x->len = 1;
	spi_message_add_tail(x, m);

	x++;
	x->rx_buf = &packet->tc.x;
	x->len = 2;
	spi_message_add_tail(x, m);

	m->complete = xpt2046_rx_val;
	m->context = ts;

	/* power down */
	m++;
	spi_message_init(m);

	x++;
	packet->pwrdown = PWRDOWN;
	x->tx_buf = &packet->pwrdown;
	x->len = 1;
	spi_message_add_tail(x, m);

	x++;
	x->rx_buf = &packet->dummy;
	x->len = 2;
	CS_CHANGE(*x);
	spi_message_add_tail(x, m);

	m->complete = xpt2046_rx;
	m->context = ts;

	ts->last_msg = m;


	xpt2046_read12_dfr(spi,READ_X(1));

	tp_calib_init();
	//zdx add

	gpio_irq_enable(spi->irq);

    return 0;

err_probe:
	if (spi)
		kfree(spi);
	if (spidev)
		kfree(spidev);
    return ret;
}
static int hc_xpt2046_init(void)
{
    int ret = 0;

    ret = xpt2046_probe("hc-gpio-spi");

    return ret;
}

module_driver(hc_xpt2046,hc_xpt2046_init,NULL,1)
