#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <hcuapi/gpio.h>
#include <kernel/drivers/spi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#define LOG_TAG "SPIDRV"
#include <kernel/elog.h>

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static LIST_HEAD(spi_ctlr_list);

static inline void mutex_lock(QueueHandle_t mutex)
{
	xSemaphoreTake(mutex, portMAX_DELAY);
}

static inline void mutex_unlock(QueueHandle_t mutex)
{
	xSemaphoreGive(mutex);
}

int spi_register_controller(struct spi_controller *ctlr)
{
	if (!ctlr->name || !ctlr->transfer_one) {
		log_e("transfer_one not implemented\n");
		return -EINVAL;
	}

	ctlr->bus_lock_mutex = xSemaphoreCreateMutex();
	if (ctlr->bus_lock_mutex == NULL) {
		log_e("mutex create fail\n");
		return -EFAULT;
	}

	ctlr->dummy_tx = NULL;
	ctlr->dummy_rx = NULL;

	list_add_tail(&ctlr->list, &spi_ctlr_list);

	return 0;
}

struct spi_controller *spi_find_controller(const char *name)
{
	struct spi_controller *curr, *next;

	list_for_each_entry_safe (curr, next, &spi_ctlr_list, list) {
		if (!strcmp(name, curr->name)) {
			return curr;
		}
	}

	return NULL;
}

static void spi_set_cs(struct spi_device *spi, bool enable)
{
	if (spi->mode & SPI_CS_HIGH)
		enable = !enable;

	if (gpio_is_valid(spi->cs_gpio))
		gpio_set_output(spi->cs_gpio, !enable);
	else if (spi->controller->set_cs)
		spi->controller->set_cs(spi, !enable);
}

int spi_setup(struct spi_device *spi)
{
	unsigned bad_bits, ugly_bits;
	int status = 0;

	/* check mode to prevent that DUAL and QUAD set at the same time
	 */
	if (((spi->mode & SPI_TX_DUAL) && (spi->mode & SPI_TX_QUAD)) ||
	    ((spi->mode & SPI_RX_DUAL) && (spi->mode & SPI_RX_QUAD))) {
		log_e("setup: can not select dual and quad at the same time\n");
		return -EINVAL;
	}

	/* if it is SPI_3WIRE mode, DUAL and QUAD should be forbidden
	 */
	if ((spi->mode & SPI_3WIRE) &&
	    (spi->mode &
	     (SPI_TX_DUAL | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)))
		return -EINVAL;

	/* help drivers fail *cleanly* when they need options
	 * that aren't supported with their current controller
	 */
	bad_bits = spi->mode & ~spi->controller->mode_bits;
	ugly_bits = bad_bits &
		    (SPI_TX_DUAL | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD);
	if (ugly_bits) {
		log_w("setup: ignoring unsupported mode bits %x\n", ugly_bits);
		spi->mode &= ~ugly_bits;
		bad_bits &= ~ugly_bits;
	}
	if (bad_bits) {
		log_e("setup: unsupported mode bits %x\n", bad_bits);
		return -EINVAL;
	}

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	if (!spi->max_speed_hz)
		spi->max_speed_hz = spi->controller->max_speed_hz;

	if (spi->controller->setup)
		status = spi->controller->setup(spi);

	spi_set_cs(spi, false);

	log_d("setup mode %d, %s%s%s%s%u bits/w, %u Hz max --> %d\n",
	      (int)(spi->mode & (SPI_CPOL | SPI_CPHA)),
	      (spi->mode & SPI_CS_HIGH) ? "cs_high, " : "",
	      (spi->mode & SPI_LSB_FIRST) ? "lsb, " : "",
	      (spi->mode & SPI_3WIRE) ? "3wire, " : "",
	      (spi->mode & SPI_LOOP) ? "loopback, " : "", spi->bits_per_word,
	      spi->max_speed_hz, status);

	return status;
}

int spi_add_device(struct spi_device *spi)
{
	int status;

	if (!spi) {
		log_e("error parameter!\n");
		return -EINVAL;
	}

	if (!spi->controller) {
		log_e("No spi controller for device\n");
		return -EFAULT;
	}

	if (spi->chip_select >= spi->controller->num_chipselect) {
		log_e("cs%d >= max %d\n", spi->chip_select,
		      spi->controller->num_chipselect);
		return -EINVAL;
	}

	status = spi_setup(spi);
	if (status < 0) {
		log_e("can't setup %s, status %d\n", spi->name, status);
	}

	return status;
}

static int __spi_validate(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_controller *ctlr = spi->controller;
	struct spi_transfer *xfer;
	int w_size;

	if (list_empty(&msg->transfers))
		return -EINVAL;

	/* Half-duplex links include original MicroWire, and ones with
	 * only one data pin like SPI_3WIRE (switches direction) or where
	 * either MOSI or MISO is missing.  They can also be caused by
	 * software limitations.
	 */
	if ((ctlr->flags & SPI_MASTER_HALF_DUPLEX) ||
	    (spi->mode & SPI_3WIRE)) {
		unsigned flags = ctlr->flags;

		list_for_each_entry (xfer, &msg->transfers, transfer_list) {
			if (xfer->rx_buf && xfer->tx_buf)
				return -EINVAL;
			if ((flags & SPI_MASTER_NO_TX) && xfer->tx_buf)
				return -EINVAL;
			if ((flags & SPI_MASTER_NO_RX) && xfer->rx_buf)
				return -EINVAL;
		}
	}

	/**
	 * Set transfer bits_per_word and max speed as spi device default if
	 * it is not set for this transfer.
	 * Set transfer tx_nbits and rx_nbits as single transfer default
	 * (SPI_NBITS_SINGLE) if it is not set for this transfer.
	 */
	list_for_each_entry (xfer, &msg->transfers, transfer_list) {
		if (!xfer->bits_per_word)
			xfer->bits_per_word = spi->bits_per_word;

		if (!xfer->speed_hz)
			xfer->speed_hz = spi->max_speed_hz;
		if (!xfer->speed_hz)
			xfer->speed_hz = ctlr->max_speed_hz;

		if (ctlr->max_speed_hz &&
		    xfer->speed_hz > ctlr->max_speed_hz)
			xfer->speed_hz = ctlr->max_speed_hz;

		/*
		 * SPI transfer length should be multiple of SPI word size
		 * where SPI word size should be power-of-two multiple
		 */
		if (xfer->bits_per_word <= 8)
			w_size = 1;
		else if (xfer->bits_per_word <= 16)
			w_size = 2;
		else
			w_size = 4;

		/* No partial transfers accepted */
		if (xfer->len % w_size)
			return -EINVAL;

		if (xfer->tx_buf && !xfer->tx_nbits)
			xfer->tx_nbits = SPI_NBITS_SINGLE;
		if (xfer->rx_buf && !xfer->rx_nbits)
			xfer->rx_nbits = SPI_NBITS_SINGLE;
		/* check transfer tx/rx_nbits:
		 * 1. check the value matches one of single, dual and quad
		 * 2. check tx/rx_nbits match the mode in spi_device
		 */
		if (xfer->tx_buf) {
			if (xfer->tx_nbits != SPI_NBITS_SINGLE &&
			    xfer->tx_nbits != SPI_NBITS_DUAL &&
			    xfer->tx_nbits != SPI_NBITS_QUAD)
				return -EINVAL;
			if ((xfer->tx_nbits == SPI_NBITS_DUAL) &&
			    !(spi->mode & (SPI_TX_DUAL | SPI_TX_QUAD)))
				return -EINVAL;
			if ((xfer->tx_nbits == SPI_NBITS_QUAD) &&
			    !(spi->mode & SPI_TX_QUAD))
				return -EINVAL;
		}
		/* check transfer rx_nbits */
		if (xfer->rx_buf) {
			if (xfer->rx_nbits != SPI_NBITS_SINGLE &&
			    xfer->rx_nbits != SPI_NBITS_DUAL &&
			    xfer->rx_nbits != SPI_NBITS_QUAD)
				return -EINVAL;
			if ((xfer->rx_nbits == SPI_NBITS_DUAL) &&
			    !(spi->mode & (SPI_RX_DUAL | SPI_RX_QUAD)))
				return -EINVAL;
			if ((xfer->rx_nbits == SPI_NBITS_QUAD) &&
			    !(spi->mode & SPI_RX_QUAD))
				return -EINVAL;
		}
	}

	return 0;
}

static int spi_map_msg(struct spi_controller *ctlr, struct spi_message *msg)
{
	struct spi_transfer *xfer;
	unsigned int max_tx, max_rx;

	if (ctlr->flags & (SPI_MASTER_MUST_RX | SPI_MASTER_MUST_TX)) {
		max_tx = 0;
		max_rx = 0;

		list_for_each_entry (xfer, &msg->transfers, transfer_list) {
			if ((ctlr->flags & SPI_MASTER_MUST_TX) &&
			    !xfer->tx_buf)
				max_tx = max(xfer->len, max_tx);
			if ((ctlr->flags & SPI_MASTER_MUST_RX) &&
			    !xfer->rx_buf)
				max_rx = max(xfer->len, max_rx);
		}

		if (max_tx) {
			ctlr->dummy_tx = memalign(4, max_tx);
			if (!ctlr->dummy_tx)
				return -ENOMEM;
			memset(ctlr->dummy_tx, 0, max_tx);
		}

		if (max_rx) {
			ctlr->dummy_rx = memalign(4, max_rx);
			if (!ctlr->dummy_rx) {
				return -ENOMEM;
			}
		}

		if (max_tx || max_rx) {
			list_for_each_entry (xfer, &msg->transfers,
					     transfer_list) {
				if (!xfer->tx_buf)
					xfer->tx_buf = (const void *)ctlr->dummy_tx;
				if (!xfer->rx_buf)
					xfer->rx_buf = ctlr->dummy_rx;
			}
		}
	}

	return 0;
}

static inline int spi_unmap_msg(struct spi_controller *ctlr,
				struct spi_message *msg)
{
	if (ctlr->flags & (SPI_MASTER_MUST_RX | SPI_MASTER_MUST_TX)) {
		if (ctlr->dummy_tx != NULL) {
			free(ctlr->dummy_tx);
			ctlr->dummy_tx = NULL;
		}

		if (ctlr->dummy_rx != NULL) {
			free(ctlr->dummy_rx);
			ctlr->dummy_rx = NULL;
		}
	}

	return 0;
}

static int spi_transfer_one_message(struct spi_controller *ctlr,
				    struct spi_message *msg)
{
	struct spi_transfer *xfer;
	bool keep_cs = false;
	int ret = 0;

	ret = spi_map_msg(ctlr, msg);
	if (ret) {
		spi_unmap_msg(ctlr, msg);
		return ret;
	}

	spi_set_cs(msg->spi, true);

	list_for_each_entry (xfer, &msg->transfers, transfer_list) {
		if (xfer->tx_buf || xfer->rx_buf) {
			ret = ctlr->transfer_one(ctlr, msg->spi, xfer);
			if (ret < 0) {
				log_e("SPI transfer failed: %d\n", ret);
				goto out;
			}
		} else {
			if (xfer->len)
				log_e("Bufferless transfer has length %u\n",
				      xfer->len);
		}

		if (xfer->delay_usecs)
			usleep(xfer->delay_usecs);

		if (xfer->cs_change) {
			if (list_is_last(&xfer->transfer_list,
					 &msg->transfers)) {
				keep_cs = true;
			} else {
				spi_set_cs(msg->spi, false);
				usleep(10);
				spi_set_cs(msg->spi, true);
			}
		}

		msg->actual_length += xfer->len;
	}

out:
	if (ret < 0 || !keep_cs) {
		spi_set_cs(msg->spi, false);
	}

	spi_unmap_msg(ctlr, msg);

	if (msg->complete)
		msg->complete(msg->context);

	return ret;
}

int spi_sync(struct spi_device *spi, struct spi_message *message)
{
	int status;
	struct spi_controller *ctlr = spi->controller;

	status = __spi_validate(spi, message);
	if (status != 0)
		return status;

	message->spi = spi;

	mutex_lock(ctlr->bus_lock_mutex);

	if (ctlr->prepare_message) {
		status = ctlr->prepare_message(ctlr, message);
		if (status) {
			log_e("failed to prepare message: %d\n", status);
			mutex_unlock(ctlr->bus_lock_mutex);
			return status;
		}
	}

	status = spi_transfer_one_message(ctlr, message);

	if (ctlr->unprepare_message) {
		status = ctlr->unprepare_message(ctlr, message);
		if (status) {
			log_e("failed to unprepare message: %d\n", status);
		}
	}

	mutex_unlock(ctlr->bus_lock_mutex);

	return status;
}

static void __spi_async(void *parameter)
{
	struct spi_message *message = (struct spi_message *)parameter;
	spi_sync(message->spi, message);
}

int spi_async(struct spi_device *spi, struct spi_message *message)
{
	message->spi = spi;
	work_queue(HPWORK, &message->work, __spi_async, (void *)message, 0);
	return 0;
}
