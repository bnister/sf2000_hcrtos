#include <errno.h>
#include <kernel/drivers/spi.h>
#include <nuttx/fs/fs.h>
#include <hcuapi/spidev.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <kernel/lib/fdt_api.h>

/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
				| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)

struct spidev_data {
	struct spi_device	*spi;

	struct mutex		buf_lock;
	void			*tx_buffer;
	void			*rx_buffer;
	unsigned		speed_hz;
};

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
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

static inline ssize_t
spidev_sync_write(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->tx_buffer,
			.len		= len,
			.speed_hz	= spidev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

static inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->rx_buffer,
			.len		= len,
			.speed_hz	= spidev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

static ssize_t
spidev_read(struct file *filp, char *buf, size_t count)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;

	spidev = filp->f_priv;

	mutex_lock(&spidev->buf_lock);
	spidev->rx_buffer = buf;
	status = spidev_sync_read(spidev, count);
	mutex_unlock(&spidev->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char *buf,
		size_t count)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;
	unsigned long		missing;

	spidev = filp->f_priv;

	mutex_lock(&spidev->buf_lock);
	spidev->tx_buffer = (void *)buf;
	status = spidev_sync_write(spidev, count);
	mutex_unlock(&spidev->buf_lock);

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

static int
spidev_ioctl(struct file *filp, int cmd, unsigned long arg)
{
	int			err = 0;
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
		if (cmd == SPI_IOC_WR_MODE)
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

static int spidev_open(struct file *filp)
{
	int			status = -ENXIO;
	struct inode		*inode = filp->f_inode;
	struct spidev_data	*spidev = inode->i_private;

	filp->f_priv = spidev;

	return 0;
}

static int spidev_close(struct file *filp)
{
	filp->f_priv = NULL;

	return 0;
}

static const struct file_operations spidev_fops = {
	.open = spidev_open, /* open */
	.close = spidev_close, /* close */
	.read = spidev_read, /* read */
	.write = spidev_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = spidev_ioctl, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int spidev_probe(const char *master, const char *node)
{
	int rc = 0;
	int np;
	const char *path;
	struct spi_device *spi = NULL;
	struct spidev_data *spidev = NULL;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return 0;

	spi = kzalloc(sizeof(*spi), GFP_KERNEL);
	if (!spi) {
		rc = -ENOMEM;
		goto err_probe;
	}

	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev) {
		rc = -ENOMEM;
		goto err_probe;
	}

	spi->controller = spi_find_controller(master);
	if (!spi->controller) {
		rc = -EFAULT;
		goto err_probe;
	}

	spi->master = spi->controller;
	spi->max_speed_hz = 50 * 1000 * 1000;
	spi->bits_per_word = 8;
	spi->chip_select = 0;
	spi->mode = SPI_MODE_0;
	spi->cs_gpio = PINPAD_INVALID;

	rc = spi_add_device(spi);
	if (rc)
		goto err_probe;

	spidev->spi = spi;
	spidev->speed_hz = spi->max_speed_hz;
	mutex_init(&spidev->buf_lock);

	rc = register_driver(path, &spidev_fops, 0666, (void *)spidev);
	if (rc < 0) {
		printf("ERROR: register_driver() failed: %d\n", rc);
		goto err_probe;
	}

	return 0;

err_probe:
	if (spi)
		kfree(spi);
	if (spidev)
		kfree(spidev);

	return rc;
}

static int spidev_init(void)
{
	int rc = 0;

	rc = spidev_probe("hc-sf-spi", "/hcrtos/sfspi/spidev@0");
	rc = spidev_probe("hc-spi", "/hcrtos/spi-master/spidev@1");
	rc = spidev_probe("hc-gpio-spi", "/hcrtos/spi-gpio/spidev@2");

	return rc;
}

module_driver(spidev, spidev_init, NULL, 0)
