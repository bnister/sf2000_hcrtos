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

#include <kernel/drivers/hc_clk_gate.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>

#define SCI_WAKEUP_IN_HPWORK 0

#define SCI_PARITY_NONE 0x0000
#define SCI_PARITY_EVEN 0x0001
#define SCI_PARITY_ODD 0x0002

#define IIR_PRIOR 0x06

/*
* UART mode master clock, Programmable baud generator divides 
* any input clock by 1 to (216 – 1) and generates the 16X clock 
*/
/* 1.843Mhz, as normal speed mode */
#define UART_CLK_SRC_NORMAL_SPEED_MODE 1843000
/* 54Mhz, as high speed mode */
#define UART_CLK_SRC_HIGH_SPEED_MODE 54000000
/* 14.7456Mhz, as high speed mode, using DPLL, for baudrate 921600 */
#define UART_CLK_SRC_HIGH_SPEED_MODE_DPLL 14745600

#define UART_NORMAL_BAUDRATE		115200
#define UART_GPIO_TX_BAUDRATE		9600
#define TX_SAMPLE_TICK			102

#define UART_TX_TMOUT 5
#define UART_EVENT_RX_AVAIL (1 << 0)
#define UART_EVENT_TX_EMPTY (1 << 1)

struct sci_device {
	int id;
	int refcnt;
	uart_reg_t *reg;
	int32_t irq;
	uint32_t baud_rate;
	uint8_t *rx_buf;
	uint32_t rx_wt;
	uint32_t rx_rd;
	SemaphoreHandle_t rlock;
	SemaphoreHandle_t wlock;

	struct pinmux_setting *active_state;
	struct pinmux_setting *default_state;

#if SCI_WAKEUP_IN_HPWORK
	struct work_s work;
	uint8_t event;
#endif

	struct completion completion;
	struct list_head instance_list;
	unsigned long gpio_tx;
};

struct sci_instance {
	struct list_head list;
	wait_queue_head_t wait;
	int fd;
};

static int uart_gate_array[4] = {
	UART1_CLK,
	UART2_CLK,
	UART3_4_CLK,
	UART3_4_CLK,
};

static struct sci_device *g_scidev[4] = { 0 };

static void sci_16550uart_setting(struct sci_device *dev,
				  struct sci_setting *setting)
{
	if (NULL == dev)
		return;

	xSemaphoreTake(dev->wlock, portMAX_DELAY);
	switch (setting->parity_mode) {
	case PARITY_EVEN:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x1; /* selects even parity */
		break;
	case PARITY_ODD:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	default:
		dev->reg->ulcr.pen = 0x0; /* no Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	};

	switch (setting->bits_mode) {
	case bits_mode_default:
		dev->reg->ulcr.wls = 0x3; /* set  word length as 8 bits */
		dev->reg->ulcr.stb = 0x0; /* set stop bit length  as 1 bit */
		break;
	case bits_mode1:
		dev->reg->ulcr.wls = 0x0; /* set  word length as 5 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 1.5 bit */
		break;
	case bits_mode2:
		dev->reg->ulcr.wls = 0x1; /* set  word length as 6 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 2 bit */
		break;
	case bits_mode3:
		dev->reg->ulcr.wls = 0x2; /* set  word length as 7 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 2 bit */
		break;
	case bits_mode4:
		dev->reg->ulcr.wls = 0x3; /* set  word length as 8 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 2 bit */
		break;
	}

	/* Enable and reset  FIFO,  threshold is 4 bytes */
	dev->reg->uiir.val = 0x47;

	dev->reg->ulsr.val = 0x0; /* Reset line status */
	dev->reg->umcr.val = 0x0;
	dev->reg->umcr.dtr = 0x1;
	dev->reg->umcr.rts = 0x1; /* Reset line status */

	xSemaphoreGive(dev->wlock);
}

static void sci_16550uart_reset(struct sci_device *dev)
{
	switch (dev->id) {
	case 0:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT16);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT16);
		break;
	case 1:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT17);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT17);
		break;
	case 2:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT9);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT9);
		break;
	case 3:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT10);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT10);
		break;
	default:
		break;
	}
}

static void sci_16550uart_set_mode(struct sci_device *dev, uint32_t parity)
{
	uint32_t div = 0;
	uint32_t tmp = 0;
	uint32_t clk_src;

	if (NULL == dev)
		return;

	xSemaphoreTake(dev->wlock, portMAX_DELAY);

	if (dev->baud_rate <= 115200) {
		/* normal speed mode */
		dev->reg->dev_ctrl.fin_sel = 0x0;
		dev->reg->ulcr.dlab = 0x1; /* start to set UART Divisor Latch	*/
		dev->reg->urbr_utbr.val = 0x1 & 0xff;
		/* configuare baud rate as 115200Hz */
		dev->reg->uier.val = (0x1 >> 8) & 0xff;
		dev->reg->ulcr.dlab = 0x0; /*finish seting UART Divisor Latch	*/
		clk_src = UART_CLK_SRC_NORMAL_SPEED_MODE;
	} else {
		/* TODO */
	}

	usleep(100);

	/*while write first char after mode set, takes 5ms*/
	//dev->timeout = 20000;

	/* Disable all interrupt */
	dev->reg->uier.val = 0;

	div = clk_src / (16 * dev->baud_rate);
	if ((clk_src % (16 * dev->baud_rate)) > (8 * dev->baud_rate))
		div += 1;

	dev->reg->ulcr.wls = 0x3; /* set  word length as 8 bits */
	dev->reg->ulcr.stb = 0x0; /* set stop bit length  as 1 bit */
	//dev->reg->ulcr.stb = 0x1;  /* set stop bit length  as 2 bit */

	dev->reg->ulcr.pen = 0x1; /* Parity Enable */
	dev->reg->ulcr.eps = 0x1; /* selects even parity */
	dev->reg->ulcr.sp = 0x0; /* disbale Stick Parity */
	dev->reg->ulcr.break_ctrl = 0x0;
	dev->reg->ulcr.dlab = 0x1; /* start to set UART Divisor Latch  */

	//must be delete before release
	//dev->reg->dev_ctrl.fin_sel = 0x1; /*set high speed mode */

	dev->reg->urbr_utbr.val = div & 0xff;
	dev->reg->uier.val = (div >> 8) & 0xff;

	switch (parity & 0x03) {
	case SCI_PARITY_EVEN:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x1; /* selects even parity */
		break;
	case SCI_PARITY_ODD:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	default:
		dev->reg->ulcr.pen = 0x0; /* no Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	};

	tmp = (parity >> 6) & 0x4;
	tmp |= (~(parity >> 4) & 0x03);

	dev->reg->ulcr.wls = tmp; /* Word Length Select  */
	dev->reg->ulcr.dlab = 0x0; /*finish seting UART Divisor Latch  */

	/* Enable and reset  FIFO,  threshold is 4 bytes */
	dev->reg->uiir.val = 0x47;

	dev->reg->ulsr.val = 0x0; /* Reset line status */
	dev->reg->umcr.val = 0x0;
	dev->reg->umcr.dtr = 0x1;
	dev->reg->umcr.rts = 0x1; /* Reset line status */

	/* Enable receiver interrupt */
	dev->reg->uier.val = 0x0;
	dev->reg->uier.erdvi = 1;
	//dev->reg->tx_fifocnt.val = 16;
	dev->reg->uier.erlsi = 1; /* Enable RX & timeout interrupt */

	dev->reg->uier.ethrei = 1; /* Enable TX interrupt */

	/* signon message or measure TIMEOUT */
	xSemaphoreGive(dev->wlock);
}

#if SCI_WAKEUP_IN_HPWORK
static void sci_isr_bottom_half(void *parameter)
{
	struct sci_device *dev = (struct sci_device *)parameter;
	struct sci_instance *ins;
	struct list_head *curr, *tmp;

	if (dev->event & UART_EVENT_RX_AVAIL) {
		list_for_each_safe (curr, tmp, &dev->instance_list) {
			ins = (struct sci_instance *)curr;
			wake_up(&ins->wait);
		}
	}

	if (dev->event & UART_EVENT_TX_EMPTY) {
		complete(&dev->completion);
	}

	dev->event = 0;
}
#endif

static void sci_isr(uint32_t parameter)
{
	uint8_t istatus = 0;
	struct sci_device *dev = (struct sci_device *)parameter;
	struct sci_instance *ins;
	struct list_head *curr, *tmp;
	int event = 0;

	/* It seems not need read UIIR but LSR for interrupt processing, but need
	* to read UIIR for clear interrupt. If ULSR error ocured, read ULSR and
	* clear it. */
	istatus = dev->reg->uiir.val & 0xf;
	while (!(istatus & 0x1)) {
		if (IIR_PRIOR == istatus) {
			while (dev->reg->ulsr.dr) {
				dev->rx_buf[dev->rx_wt++] =
					dev->reg->urbr_utbr.val;
				dev->rx_wt %= CONFIG_UART_RX_BUF_SIZE;
				event |= UART_EVENT_RX_AVAIL;
			}
		}

		switch (istatus) {
		/* We continue receive data at this condition */
		case 0x0c: /* Character Timer-outIndication */
			while (dev->reg->rx_fifocnt.val > 0) {
				dev->rx_buf[dev->rx_wt++] =
					dev->reg->urbr_utbr.val;
				dev->rx_wt %= CONFIG_UART_RX_BUF_SIZE;
				event |= UART_EVENT_RX_AVAIL;
			}
			break;
		case 0x04: /* Received Data Available */
			while (dev->reg->rx_fifocnt.val > 1) {
				dev->rx_buf[dev->rx_wt++] =
					dev->reg->urbr_utbr.val;
				dev->rx_wt %= CONFIG_UART_RX_BUF_SIZE;
				event |= UART_EVENT_RX_AVAIL;
			}
			dev->reg->ulsr.val &= ~F_UART_0005_DR_M;
			break;

		case 0x02: /* TransmitterHoldingRegister Empty */
			event |= UART_EVENT_TX_EMPTY;
			break;
		case 0x00: /* Modem Status */
		default:
			break;
		}

		istatus = dev->reg->uiir.val & 0xf;
	}

#if SCI_WAKEUP_IN_HPWORK
	dev->event = event;
	if (dev->event != 0 && work_available(&dev->work)) {
		work_queue(HPWORK, &dev->work, sci_isr_bottom_half,
			   (void *)parameter, 0);
	}
#else
	if (event & UART_EVENT_RX_AVAIL) {
		list_for_each_safe (curr, tmp, &dev->instance_list) {
			ins = (struct sci_instance *)curr;
			wake_up(&ins->wait);
		}
	}

	if (event & UART_EVENT_TX_EMPTY) {
		complete(&dev->completion);
	}
#endif
	return;
}

bool sci_16550uart_read(struct sci_device *dev, char *p_ch)
{
	uint8_t received = 0;
	uint32_t cnt = 0;

	if (NULL == dev)
		return false;

	xSemaphoreTake(dev->rlock, portMAX_DELAY);

	received = !(dev->rx_wt == dev->rx_rd);
	while (!received) {
		xSemaphoreGive(dev->rlock);
		msleep(1);

		if(cnt++ > 100)
			return false;

		xSemaphoreTake(dev->rlock, portMAX_DELAY);
		received = !(dev->rx_wt == dev->rx_rd);
	}

	*p_ch = dev->rx_buf[dev->rx_rd++];
	dev->rx_rd %= CONFIG_UART_RX_BUF_SIZE;
	xSemaphoreGive(dev->rlock);

	return true;
}

static void sci_16550uart_gpio_tx_send_byte(struct sci_device *dev ,uint8_t val)
{
	uint8_t cnt = 0;

	taskENTER_CRITICAL();

	gpio_set_output(dev->gpio_tx,0);
	usleep(TX_SAMPLE_TICK);

	for (cnt = 0; cnt < 8; cnt++) {
		if(val & 0x01)
			gpio_set_output(dev->gpio_tx,1);
		else
			gpio_set_output(dev->gpio_tx,0);
		usleep(TX_SAMPLE_TICK);
		val >>= 1;
	}

	gpio_set_output(dev->gpio_tx,1);
	usleep(TX_SAMPLE_TICK);

	taskEXIT_CRITICAL();
}

static void sci_16550uart_gpio_tx_write_bytes(struct sci_device *dev,
				      struct sci_instance *ins, const char *buf,
				      int32_t bytes)
{
	int32_t cnt = 0;

	if (NULL == dev || NULL == buf || bytes == 0)
		return;

	xSemaphoreTake(dev->wlock, portMAX_DELAY);

	do {
		if (ins->fd == STDOUT_FILENO || ins->fd == STDERR_FILENO) {
			if(*buf == '\n')
				sci_16550uart_gpio_tx_send_byte(dev,'\r');
		}
		sci_16550uart_gpio_tx_send_byte(dev,*buf);
		buf++;
		cnt++;
	} while (cnt < bytes);

	xSemaphoreGive(dev->wlock);

	return;
}

static void sci_16550uart_write_bytes(struct sci_device *dev,
				      struct sci_instance *ins, const char *buf,
				      int32_t bytes)
{
#define FIFO_MAX 16
	int sent = 0;
	char ch;
	bool add_cr = true;
	int fifo = 0;

	if (NULL == dev || NULL == buf || bytes == 0)
		return;

	xSemaphoreTake(dev->wlock, portMAX_DELAY);

	do {
		init_completion(&dev->completion);

		fifo = 0;
		do {
			ch = buf[sent];
			if (ins->fd == STDOUT_FILENO || ins->fd == STDERR_FILENO) {
				if (ch == '\n' && add_cr) {
					dev->reg->urbr_utbr.val = '\r';
					fifo++;
					if (fifo == FIFO_MAX) {
						add_cr = false;
						break;
					}
				}
			}
			dev->reg->urbr_utbr.val = ch;
			fifo++;
			sent++;
			add_cr = true;
		} while (fifo < FIFO_MAX && sent < bytes);

		if (wait_for_completion_timeout(&dev->completion,
						UART_TX_TMOUT) == 0) {
			/* timeout, do polling wait */
			while (!dev->reg->ulsr.thre)
				;
		}
	} while (sent < bytes);

	xSemaphoreGive(dev->wlock);

	return;
}

static void sci_16550uart_write_bytes_from_isr(struct sci_device *dev,
				      struct sci_instance *ins, const char *buf,
				      int32_t bytes)
{
#define FIFO_MAX 16
	int sent = 0;
	char ch;
	bool add_cr = true;
	int fifo = 0;

	if (NULL == dev || NULL == buf || bytes == 0)
		return;

	while (!dev->reg->ulsr.thre)
		;

	do {
		fifo = 0;
		do {
			ch = buf[sent];
			if (ins->fd == STDOUT_FILENO || ins->fd == STDERR_FILENO) {
				if (ch == '\n' && add_cr) {
					dev->reg->urbr_utbr.val = '\r';
					fifo++;
					if (fifo == FIFO_MAX) {
						add_cr = false;
						break;
					}
				}
			}
			dev->reg->urbr_utbr.val = ch;
			fifo++;
			sent++;
			add_cr = true;
		} while (fifo < FIFO_MAX && sent < bytes);

		/* timeout, do polling wait */
		while (!dev->reg->ulsr.thre)
			;
	} while (sent < bytes);

	return;
}

static int sci_open(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct sci_device *dev = inode->i_private;
	struct sci_instance *ins;

	ins = malloc(sizeof(struct sci_instance));
	if (!ins)
		return -ENOMEM;
	memset(ins, 0, sizeof(struct sci_instance));
	ins->fd = -1;
	init_waitqueue_head(&ins->wait);
	list_add_tail(&ins->list, &dev->instance_list);

	if (dev->refcnt == 0) {
		dev->rx_buf = malloc(CONFIG_UART_RX_BUF_SIZE);
		if (!dev->rx_buf) {
			free(ins);
			return -ENOMEM;
		}
		sci_16550uart_reset(dev);
		sci_16550uart_set_mode(dev, 0);
		pinmux_select_setting(dev->active_state);
		xPortInterruptInstallISR(dev->irq, sci_isr, (uint32_t)dev);
	}

	filep->f_priv = ins;

	dev->refcnt++;

	return 0;
}

static int sci_close(struct file *filep)
{
	struct sci_instance *ins = filep->f_priv;
	struct inode *inode = filep->f_inode;
	struct sci_device *dev = inode->i_private;

	if (!list_empty(&ins->list))
		list_del(&ins->list);

	dev->refcnt--;

	if (dev->refcnt == 0) {
		xPortInterruptRemoveISR(dev->irq, sci_isr);
		free(dev->rx_buf);
		pinmux_select_setting(dev->default_state);
	}

	free(ins);

	return 0;
}

static ssize_t sci_read(struct file *filep, char *buffer, size_t buflen)
{
	struct inode *inode = filep->f_inode;
	struct sci_device *dev = inode->i_private;
	size_t index;

	for (index = 0; index < buflen; index++) {
		if (false == sci_16550uart_read(dev, buffer + index))
			if(buflen == 1)
				continue;
			else
				break;
	}
	
	return index;
}

static ssize_t sci_write(struct file *filep, const char *buffer, size_t buflen)
{
	struct inode *inode = filep->f_inode;
	struct sci_device *dev = inode->i_private;
	struct sci_instance *ins = filep->f_priv;

	if (ins->fd < 0) {
		ins->fd = fs_getfilefd(filep);
	}

	if (dev->gpio_tx != PINPAD_INVALID) {
		sci_16550uart_gpio_tx_write_bytes(dev, ins, buffer, buflen);
	} else {
		if (!uxInterruptNesting && !vTaskIsInCritical())
			sci_16550uart_write_bytes(dev, ins, buffer, buflen);
		else
			sci_16550uart_write_bytes_from_isr(dev, ins, buffer, buflen);
	}

	return buflen;
}

static int sci_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	struct inode *inode = filep->f_inode;
	struct sci_device *dev = inode->i_private;

	if (dev->gpio_tx != PINPAD_INVALID) {
		return -ENOTSUP;
	}

	switch (cmd) {
		case SCIIOC_SET_HIGH_SPEED:
			/*config high speed mode*/
			/*  high speed mode */
			dev->reg->dev_ctrl.fin_sel = 0x1;
			/* start to set UART Divisor Latch	*/
			dev->reg->ulcr.dlab = 0x1;
			dev->reg->urbr_utbr.val = 0x1 & 0xff;
			/* configuare baud rate as 3.375MHz */
			dev->reg->uier.val = (0x1 >> 8) & 0xff;
			/*finish seting UART Divisor Latch	*/
			dev->reg->ulcr.dlab = 0x0;
			break;

		case SCIIOC_SET_NORMAL_SPEED:
			/*config normal speed mode*/
			/* normal speed mode */
			dev->reg->dev_ctrl.fin_sel = 0x0;
			/* start to set UART Divisor Latch	*/
			dev->reg->ulcr.dlab = 0x1;
			dev->reg->urbr_utbr.val = 0x1 & 0xff;
			/* configuare baud rate as 115200Hz */
			dev->reg->uier.val = (0x1 >> 8) & 0xff;
			/*finish seting UART Divisor Latch	*/
			dev->reg->ulcr.dlab = 0x0;
			break;

		case SCIIOC_SET_BAUD_RATE_115200:
			dev->baud_rate = 115200;
			sci_16550uart_set_mode(dev, 0);
			break;

		case SCIIOC_SET_BAUD_RATE_57600:
			dev->baud_rate = 57600;
			sci_16550uart_set_mode(dev, 0);
			break;

		case SCIIOC_SET_BAUD_RATE_19200:
			dev->baud_rate = 19200;
			sci_16550uart_set_mode(dev, 0);
			break;

		case SCIIOC_SET_BAUD_RATE_9600:
			dev->baud_rate = 9600;
			sci_16550uart_set_mode(dev, 0);
			break;

		case SCIIOC_SET_BAUD_RATE_1125000:
			dev->baud_rate = 1125000;
			sci_16550uart_set_mode(dev, 0);
			break;

		case SCIIOC_SET_BAUD_RATE_921600:
			dev->baud_rate = 921600;
			sci_16550uart_set_mode(dev, 0);
			break;

		case SCIIOC_SET_BAUD_RATE_675000:
			dev->baud_rate = 675000;
			sci_16550uart_set_mode(dev, 0);
			break;

		case SCIIOC_SET_SETTING:
			sci_16550uart_setting(dev, (struct sci_setting *)arg);
			break;

		default:
			break;
	}

	return 0;
}

static int sci_poll(struct file *filep, poll_table *wait)
{
	struct inode *inode = filep->f_inode;
	struct sci_device *dev = inode->i_private;
	struct sci_instance *ins = filep->f_priv;
	int mask = 0;

	poll_wait(filep, &ins->wait, wait);

	if (dev->rx_wt != dev->rx_rd)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static const struct file_operations g_sciops = {
	.open = sci_open, /* open */
	.close = sci_close, /* close */
	.read = sci_read, /* read */
	.write = sci_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = sci_ioctl, /* ioctl */
	.poll = sci_poll /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int sci_driver_register(struct sci_device *sci_dev, const char *path, int id)
{
	uint32_t baud_rate = 0;

	if (sci_dev->gpio_tx != PINPAD_INVALID)
		baud_rate = UART_GPIO_TX_BAUDRATE;
	else
		baud_rate = UART_NORMAL_BAUDRATE;

	if (id == 0) {
		sci_dev->reg = (uart_reg_t *)&UART0;
		sci_dev->irq = (int32_t)&UART0_INTR;
		sci_dev->baud_rate = baud_rate;
	} else if (id == 1) {
		sci_dev->reg = (uart_reg_t *)&UART1;
		sci_dev->irq = (int32_t)&UART1_INTR;
		sci_dev->baud_rate = baud_rate;
	} else if (id == 2) {
		sci_dev->reg = (uart_reg_t *)&UART2;
		sci_dev->irq = (int32_t)&UART2_INTR;
		sci_dev->baud_rate = baud_rate;
	} else if (id == 3) {
		sci_dev->reg = (uart_reg_t *)&UART3;
		sci_dev->irq = (int32_t)&UART3_INTR;
		sci_dev->baud_rate = baud_rate;
	} else {
		free(sci_dev);
		return -ENODEV;
	}

	sci_dev->rx_wt = 0;
	sci_dev->rx_rd = 0;
	sci_dev->rlock = xSemaphoreCreateMutex();
	sci_dev->wlock = xSemaphoreCreateMutex();
	sci_dev->id = id;

	INIT_LIST_HEAD(&sci_dev->instance_list);
	init_completion(&sci_dev->completion);

	register_driver(path, &g_sciops, 0666, sci_dev);

	g_scidev[id] = sci_dev;

	return 0;
}

static int sci_driver_unregister(const char *path, int id)
{
	if (g_scidev[id]) {
		free(g_scidev[id]);
		unregister_driver(path);
	}

	return 0;
}

static int sci_module_probe(const char *node, int id)
{
	int np;
	int ret = 0;
	const char *path;
	struct sci_device *sci_dev;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return 0;

	sci_dev = malloc(sizeof(struct sci_device));
	if (!sci_dev)
		return -ENOMEM;

	hc_clk_enable(uart_gate_array[id]);

	memset(sci_dev, 0, sizeof(struct sci_device));

	sci_dev->active_state = fdt_get_property_pinmux(np, "active");
	sci_dev->default_state = fdt_get_property_pinmux(np, "default");

	if (!fdt_get_property_u_32_index(np, "tx-gpios", 0, (u32 *)&sci_dev->gpio_tx)) {
		gpio_set_output(sci_dev->gpio_tx, GPIO_ACTIVE_HIGH);
		gpio_configure(sci_dev->gpio_tx, GPIO_DIR_OUTPUT);
	} else {
		sci_dev->gpio_tx = PINPAD_INVALID;
	}

	ret = sci_driver_register(sci_dev, path, id);
	if (ret < 0) {
		hc_clk_disable(uart_gate_array[id]);
	}

	return 0;
}

static int sci_module_init(void)
{
	int rc = 0;

	rc |= sci_module_probe("/hcrtos/uart@0", 0);
	rc |= sci_module_probe("/hcrtos/uart@1", 1);
	rc |= sci_module_probe("/hcrtos/uart@2", 2);
	rc |= sci_module_probe("/hcrtos/uart@3", 3);

	return rc;
}

module_arch(uart, sci_module_init, NULL, 0)
