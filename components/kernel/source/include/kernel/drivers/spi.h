#ifndef __KERNEL_SPI_H__
#define __KERNEL_SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <kernel/list.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <kernel/io.h>
#include <hcuapi/pinpad.h>
#include <linux/minmax.h>
#include <linux/spi/spi-mem.h>
#include <nuttx/wqueue.h>

struct spi_message;
struct spi_transfer;
struct spi_device;
struct spi_controller;

#define spi_master                      spi_controller

/**
 * struct spi_device - Master side proxy for an SPI slave device
 * @bits_per_word: Data transfers involve one or more words; word sizes
 *	like eight or 12 bits are common.  In-memory wordsizes are
 *	powers of two bytes (e.g. 20 bit samples use 32 bits).
 *	This may be changed by the device's driver, or left at the
 *	default (0) indicating protocol words are eight bit bytes.
 *	The spi_transfer.bits_per_word can override this for each transfer.
 * @chip_select: Chipselect, distinguishing chips handled by @master.
 * @mode: The spi mode defines how data is clocked out and in.
 *	This may be changed by the device's driver.
 *	The "active low" default for chipselect mode can be overridden
 *	(by specifying SPI_CS_HIGH) as can the "MSB first" default for
 *	each word in a transfer (by specifying SPI_LSB_FIRST).
 * @irq: Negative, or the number passed to request_irq() to receive
 *	interrupts from this device.
 * @cs_gpio: gpio number of the chipselect line (optional)
 */
struct spi_device {
	const char		*name;

	struct spi_controller	*controller;
	struct spi_controller	*master;

	uint32_t		max_speed_hz;
	uint8_t			bits_per_word;
	uint8_t			chip_select;
	uint16_t		mode;
#define	SPI_CPHA	0x01			/* clock phase */
#define	SPI_CPOL	0x02			/* clock polarity */
#define	SPI_MODE_0	(0|0)			/* (original MicroWire) */
#define	SPI_MODE_1	(0|SPI_CPHA)
#define	SPI_MODE_2	(SPI_CPOL|0)
#define	SPI_MODE_3	(SPI_CPOL|SPI_CPHA)
#define	SPI_CS_HIGH	0x04			/* chipselect active high? */
#define	SPI_LSB_FIRST	0x08			/* per-word bits-on-wire */
#define	SPI_3WIRE	0x10			/* SI/SO signals shared */
#define	SPI_LOOP	0x20			/* loopback mode */
#define	SPI_NO_CS	0x40			/* 1 dev/bus, no chipselect */
#define	SPI_READY	0x80			/* slave pulls low to pause */
#define	SPI_TX_DUAL	0x100			/* transmit with 2 wires */
#define	SPI_TX_QUAD	0x200			/* transmit with 4 wires */
#define	SPI_RX_DUAL	0x400			/* receive with 2 wires */
#define	SPI_RX_QUAD	0x800			/* receive with 4 wires */
#define	SPI_RX_OCTAL	0x1000			/* receive with 2 wires */
#define	SPI_TX_OCTAL	0x2000			/* receive with 4 wires */

	int			irq;
	pinpad_e		cs_gpio;	/* chip select gpio */
	void			*controller_state;
	void			*priv;
};

/**
 * struct spi_controller - interface to SPI master controller
 * @num_chipselect: chipselects are used to distinguish individual
 *	SPI slaves, and are numbered from zero to num_chipselects.
 *	each slave has a chipselect signal, but it's common that not
 *	every chipselect is connected to a slave.
 * @mode_bits: flags understood by this controller driver
 * @max_speed_hz: Highest supported transfer speed
 * @bus_lock_mutex: mutex for SPI bus locking
 */
struct spi_controller {
	struct list_head	list;

	const struct spi_controller_mem_ops *mem_ops;

	const char		*name;

	uint16_t		num_chipselect;

	uint16_t		bus_num;

	/* spi_device.mode flags understood by this controller driver */

	uint32_t		max_xfer_size;

	uint16_t		mode_bits;

	uint32_t		bits_per_word_mask;

#define SPI_BPW_MASK(bits) BIT((bits) - 1)
#define SPI_BIT_MASK(bits) (((bits) == 32) ? ~0U : (BIT(bits) - 1))
#define SPI_BPW_RANGE_MASK(min, max) (SPI_BIT_MASK(max) - SPI_BIT_MASK(min - 1))

        /* limits on transfer speed */
        uint32_t                     min_speed_hz;
        uint32_t                     max_speed_hz;

	/* other constraints relevant to this driver */
	uint16_t		flags;
#define SPI_MASTER_HALF_DUPLEX	BIT(0)		/* can't do full duplex */
#define SPI_MASTER_NO_RX	BIT(1)		/* can't do buffer read */
#define SPI_MASTER_NO_TX	BIT(2)		/* can't do buffer write */
#define SPI_MASTER_MUST_RX	BIT(3)		/* requires rx */
#define SPI_MASTER_MUST_TX	BIT(4)		/* requires tx */

	QueueHandle_t		bus_lock_mutex;

	/* Setup mode and clock, etc (spi driver may call many times).
	 *
	 * IMPORTANT:  this may be called when transfers to another
	 * device are active.  DO NOT UPDATE SHARED REGISTERS in ways
	 * which could break those transfers.
	 */
	int                     (*transfer)(struct spi_device *spi,
                                        struct spi_message *mesg);

	/* called on release() to free memory provided by spi_master */
	void                    (*cleanup)(struct spi_device *spi);
	int (*setup)(struct spi_device *spi);

	void (*set_cs)(struct spi_device *spi, bool enable);
	int (*prepare_message)(struct spi_controller *master,
			       struct spi_message *message);
	int (*prepare_transfer_hardware)(struct spi_master *master);
	int (*transfer_one_message)(struct spi_master *master,
                            struct spi_message *mesg);
	int (*unprepare_transfer_hardware)(struct spi_master *master);
	int (*transfer_one)(struct spi_controller *master, struct spi_device *spi,
			    struct spi_transfer *transfer);
	int (*unprepare_message)(struct spi_controller *master,
				 struct spi_message *message);

	size_t (*max_transfer_size)(struct spi_device *spi);
	size_t (*max_message_size)(struct spi_device *spi);

	/* dummy data for full duplex devices */
	void			*dummy_rx;
	void			*dummy_tx;

	void			*priv;
};

struct spi_transfer {
	const void	*tx_buf;
	void		*rx_buf;
	unsigned	len;

	unsigned	dummy_data:1;
	unsigned	cs_change:1;
	unsigned	tx_nbits:3;
	unsigned	rx_nbits:3;
#define	SPI_NBITS_SINGLE	0x01 /* 1bit transfer */
#define	SPI_NBITS_DUAL		0x02 /* 2bits transfer */
#define	SPI_NBITS_QUAD		0x04 /* 4bits transfer */
	uint8_t		bits_per_word;
	uint16_t	delay_usecs;
	uint32_t	speed_hz;

	struct list_head transfer_list;
};

struct spi_message {
	struct list_head	transfers;

	struct spi_device	*spi;

	int			status;
	unsigned		actual_length;

	void			(*complete)(void *context);
	void			*context;

	struct work_s		work;
};

static inline void *spi_controller_get_devdata(struct spi_controller *ctlr)
{
	return ctlr->priv;
}

static inline void spi_controller_set_devdata(struct spi_controller *ctlr,
					      void *data)
{
	ctlr->priv = data;
}

#define spi_master_get_devdata(_ctlr)	spi_controller_get_devdata(_ctlr)
#define spi_master_set_devdata(_ctlr, _data)	\
	spi_controller_set_devdata(_ctlr, _data)

static inline void spi_message_init(struct spi_message *m)
{
	memset(m, 0, sizeof *m);
	INIT_LIST_HEAD(&m->transfers);
}

static inline void
spi_message_add_tail(struct spi_transfer *t, struct spi_message *m)
{
	list_add_tail(&t->transfer_list, &m->transfers);
}

static inline void
spi_transfer_del(struct spi_transfer *t)
{
	list_del(&t->transfer_list);
}

extern int spi_sync(struct spi_device *spi, struct spi_message *message);
extern int spi_async(struct spi_device *spi, struct spi_message *message);

static inline int
spi_write(struct spi_device *spi, const void *buf, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= buf,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}

static inline int
spi_read(struct spi_device *spi, void *buf, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= buf,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}

static inline void
spi_message_init_with_transfers(struct spi_message *m,
struct spi_transfer *xfers, unsigned int num_xfers)
{
	unsigned int i;

	spi_message_init(m);
	for (i = 0; i < num_xfers; ++i)
		spi_message_add_tail(&xfers[i], m);
}

static inline int
spi_sync_transfer(struct spi_device *spi, struct spi_transfer *xfers,
	unsigned int num_xfers)
{
	struct spi_message msg;

	spi_message_init_with_transfers(&msg, xfers, num_xfers);

	return spi_sync(spi, &msg);
}

extern struct spi_controller *spi_find_controller(const char *name);
extern int spi_register_controller(struct spi_controller *ctlr);
extern int spi_add_device(struct spi_device *spi);
extern int spi_setup(struct spi_device *spi);

#define spi_register_master(_ctlr)      spi_register_controller(_ctlr)
#define spi_master_get_devdata(_ctlr)   spi_controller_get_devdata(_ctlr)
#define spi_master_set_devdata(_ctlr, _data)    \
	spi_controller_set_devdata(_ctlr, _data)

static inline size_t
spi_max_message_size(struct spi_device *spi)
{
	struct spi_controller *ctlr = spi->controller;

	if (!ctlr->max_message_size)
		return SIZE_MAX;
	return ctlr->max_message_size(spi);
}

static inline size_t
spi_max_transfer_size(struct spi_device *spi)
{
	struct spi_controller *ctlr = spi->controller;
	size_t tr_max = SIZE_MAX;
	size_t msg_max = spi_max_message_size(spi);

	if (ctlr->max_transfer_size)
		tr_max = ctlr->max_transfer_size(spi);

	/* transfer size limit must not be greater than messsage size limit */
	return min(tr_max, msg_max);
}

static inline void spi_set_drvdata(struct spi_device *spi, void *data)
{
	spi->priv = data;
}

#ifdef __cplusplus
}
#endif

#endif
