#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <debug.h>
#include <sys/unistd.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <nuttx/i2c/i2c_master.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <kernel/ld.h>
#include <hcuapi/pinmux.h>
#include <linux/jiffies.h>
#include <linux/irqreturn.h>
#include <linux/sched.h>
#include <freertos/semphr.h>
#include <kernel/drivers/hc_clk_gate.h>

#include <kernel/completion.h>
#include "i2c_reg_struct.h"

#define ISR_TDI			0x01
#define ISR_DNEE		0x02
#define ISR_DBIE		0x04
#define ISR_ARBLI		0x08
#define ISR_SS			0x10
#define ISR_SP			0x20
#define ISR_FF			0x40
#define ISR_FE			0x80
#define ISR1_FT			0x01

#define IER_TDI			0x01
#define IER_DNEE		0x02
#define IER_DBIE		0x04
#define IER_ARBLI		0x08
#define IER_SS			0x10
#define IER_SP			0x20
#define IER_FF			0x40
#define IER_FE			0x80
#define IER1_FT			0x01

#define HSR_FE			0x01
#define HSR_FF			0x02
#define HSR_FU			0x04
#define HSR_FO			0x08
#define HSR_FER			0x10
#define HSR_HB			0x20
#define HSR_DNE			0x40
#define HSR_DB			0x80

#define HCR_ST			0x01
#define HCR_CP_WRITE		0x00
#define HCR_CP_CUR_READ		0x04
#define HCR_CP_SEQ_READ		0x08
#define HCR_CP_STD_READ		0x0c
#define HCR_SR			0x10
#define HCR_EO			0x20
#define HCR_DNEE		0x40
#define HCR_HCE			0x80

#define I2C_SCB_TIME_OUT	100
#define CLEAR_FIFO		0x80
#define MASTER_MODE		0x80
#define BC_VAL			0x1f
#define I2C_RETRY		10
#define TIMES_OUT		500

#define LOW_SPEED		23600
#define CLOCK_1_843M		921500
#define CLOCK_12M		6000000

#define	STOP_ 

enum hc_i2c_state {
	HC_I2C_STAT_INIT_FAIL	= -2,
	HC_I2C_STAT_RW_ERR	= -1,
	HC_I2C_STAT_INIT	= 0,
	HC_I2C_STAT_RW		= 1,
	HC_I2C_STAT_RW_SUCCESS	= 2,
};

static void *ctrlreg[4] = {
	(void *)&I2C0CTRL,
	(void *)&I2C1CTRL,
	(void *)&I2C2CTRL,
	(void *)&I2C3CTRL,
};

static int i2c_irq_nb[4] = {
	(int)&I2C0_INTR,
	(int)&I2C1_INTR,
	(int)&I2C2_INTR,
	(int)&I2C3_INTR,
};

static int i2c_gate_array[4] = {
	I2C1_CLK,
	I2C2_CLK,
	I2C3_CLK,
	I2C4_CLK,
};

struct hc_i2cdev_s {
	struct		i2c_master_s dev;
	i2c_reg_t	*base;
	uint8_t		port;
	int		state;
	uint32_t	baudrate;
	struct		i2c_msg_s *msg;
	int		stop;
	int		eddc_flag; 
	int		ridx;
	int		widx;
	unsigned long	timeout;

	struct completion	completion;
	QueueHandle_t	 lock;
};

static void hc_i2c_master_init(struct hc_i2cdev_s *priv);

static int hc_i2c_wait_host_ready(struct hc_i2cdev_s *priv,
				  unsigned long timeout_jiffies)
{
	unsigned long deadline;
	int timeout = 0;
	i2c_reg_t *reg =(i2c_reg_t *) priv->base;

	deadline = jiffies + timeout_jiffies;

	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		if ((reg->hsr.host_busy) == 0)
			return 0;

		cond_resched();
	}

	i2cdebug("host ready time out\n");

	return -ETIMEDOUT;
}

static int hc_i2c_wait_dev_ready(struct hc_i2cdev_s *priv,
				 unsigned long timeout_jiffies)
{
	unsigned long deadline;
	int timeout = 0;

	i2c_reg_t *reg = (i2c_reg_t *)priv->base;
	deadline = jiffies + timeout_jiffies;

	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		if ((reg->hsr.val & (HSR_DB | HSR_DNE | HSR_HB)) == 0)
			return 0;
		else if ((reg->isr.val & (ISR_DNEE | ISR_DBIE | ISR_ARBLI)))
			return -1;

		cond_resched();
	}

	i2cdebug("dev ready time out\n");

	return -ETIMEDOUT;
}

static int hc_i2c_wait_trans_done(struct hc_i2cdev_s *priv,
				  unsigned long timeout_jiffies)
{
	unsigned long deadline;
	int timeout = 0;
	uint8_t status;

	i2c_reg_t *reg = (i2c_reg_t *)priv->base;
	deadline = jiffies + timeout_jiffies;

	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		status = reg->isr.val;

		if ((status & ISR_TDI) != 0)
			return 0;
		else if ((status & (ISR_DNEE | ISR_DBIE | ISR_ARBLI)) != 0) {
			return -1;
		}

		cond_resched();
	}

	i2cdebug("trans done time out\n");

	return -ETIMEDOUT;
}

static int hc_i2c_wait_fifo_not_full(struct hc_i2cdev_s *priv)	
{
	unsigned long deadline;
	int timeout = 0;
	unsigned long timeout_jiffies = priv->timeout;
	i2c_reg_t *reg = (i2c_reg_t *)priv->base;
	deadline = jiffies + timeout_jiffies;

	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		if (reg->hsr.fifo_full == 0)
			return 0;

		cond_resched();
	}

	i2cdebug("fifo full time out\n");

	return -ETIMEDOUT;
}

static int hc_i2c_wait_fifo_not_empty(struct hc_i2cdev_s *priv,
                                     unsigned long timeout_jiffies)
{
	unsigned long deadline;
	int timeout = 0;
	i2c_reg_t *reg = (i2c_reg_t *)priv->base;
	deadline = jiffies + timeout_jiffies;

	while (!timeout) {
		if (reg->hsr.fifo_empty == 0)
			return 0;

		if (time_after_eq(jiffies, deadline))
			timeout = 1; 
			cond_resched();
	}

	i2cdebug("fifo full time out\n");

	return -ETIMEDOUT;
}

static void hc_i2c_interrupt(uint32_t param)
{
	struct hc_i2cdev_s *priv = (struct hc_i2cdev_s *)param;
	uint8_t *rdata = priv->msg->buffer;
	uint8_t *wdata = priv->msg->buffer;
	int rlen = priv->msg->length;
	i2c_reg_t *reg = priv->base;
	reg->isr1.val = 0xff;

	if(priv->msg->flags & I2C_M_READ){
		while (priv->ridx < rlen) {
			if (reg->hsr.fifo_empty == 1)
				break;

			rdata[priv->ridx++] = reg->data_register;
		}

		if (priv->ridx == rlen)
			complete(&priv->completion);
		
		// return IRQ_HANDLED;
	}else{
		while (priv->widx) {
			if (reg->hsr.fifo_full == 1) {
				break;
			} 

			reg->data_register = wdata[priv->msg->length - priv->widx];
			priv->widx--;
		}

		if (priv->widx == 0) {
			complete(&priv->completion);                   
		}                               

		// return IRQ_HANDLED;
	}

}

static void hc_i2c_read_msg(struct hc_i2cdev_s *priv)
{
	int i, j;
	i2c_reg_t *reg = priv->base;
	int rlen = priv->msg->length;
	struct i2c_msg_s *msg = priv->msg;
	uint8_t *rdata = priv->msg->buffer;
	unsigned long tmout = 0;
	
	for (j = 10; j > 0; j--) {
		reg->hcr.val &= MASTER_MODE;
		reg->hsr.val = 0;
		reg->ier.val &= ~(IER_TDI| IER_DNEE | IER_DBIE |IER_ARBLI);
		reg->isr.val = (IER_TDI| IER_DNEE | IER_DBIE |IER_ARBLI);
		reg->sar.val = priv->msg->addr << 1;
		reg->scb_bc_12_5 = ((rlen >> 5) & 0xff);
		reg->fcr.val = (CLEAR_FIFO | (rlen & 0x1f));

		if ((msg->flags & I2C_M_NOSTART) == 0) {
			if ((msg->flags & I2C_M_NOSTOP) == 0) {
				if (rlen > 32) {
					priv->ridx = 0;
					init_completion(&priv->completion);
					reg->fifo_tregger.val = 8;
					reg->ier1.fifo_trigger_int_en = 1;
				}
				reg->hcr.val |= (HCR_HCE | HCR_DNEE |
						 HCR_CP_CUR_READ | HCR_ST);
			}
		} else if (priv->eddc_flag) {
			priv->ridx = 0;
			init_completion(&priv->completion);
			reg->fifo_tregger.val = 8;
			reg->ier1.fifo_trigger_int_en = 1;
			reg->hcr.val |= (HCR_HCE | HCR_DNEE | HCR_EO |	
					HCR_CP_SEQ_READ | HCR_ST);
		} else {
			if (rlen > 32) {
				priv->ridx = 0;
				init_completion(&priv->completion);
				reg->fifo_tregger.val = 8;

				reg->ier1.fifo_trigger_int_en = 1;
			}
			reg->hcr.val |= (HCR_HCE | HCR_DNEE |
					HCR_CP_SEQ_READ | HCR_ST);
		}

		if (hc_i2c_wait_dev_ready(priv, 1000) == 0) {
			break;
		}
		usleep(1000);
	}

	if (j == 0) {
		priv->state = HC_I2C_STAT_RW_ERR;
		return ;
	}


	if ((rlen > 32) || priv->eddc_flag) {
		tmout = (unsigned long)priv->msg->length * 8 * 1000 / priv->baudrate;
		tmout += msecs_to_jiffies(5);
		wait_for_completion_timeout(&priv->completion, tmout);
		reg->ier1.fifo_trigger_int_en = 0;

		while (priv->ridx < rlen) {
			if (hc_i2c_wait_fifo_not_empty(priv, 1000) != 0) {
				break;
			}

			rdata[priv->ridx++] = reg->data_register;
		}

		if (priv->ridx < rlen)
			priv->state = HC_I2C_STAT_RW_ERR;
		return ;
	}

	for (i = 0; i < rlen;i++) {
		if (hc_i2c_wait_fifo_not_empty(priv, 1000) != 0) {
	      		break;
		}

		rdata[i] = reg->data_register;
	}

	priv->state = HC_I2C_STAT_RW_SUCCESS;

	return ;
}

static void hc_i2c_write_msg(struct hc_i2cdev_s *priv)
{
	int j;
	int status = 0;
	i2c_reg_t *reg = priv->base;
	int wlen = priv->msg->length;
	int tmp_wlen = priv->msg->length; 
	struct i2c_msg_s *msg = priv->msg;
	uint8_t *wdata = priv->msg->buffer;
	unsigned long tmout = 0;

	if ((priv->eddc_flag == 0x01) && ((msg->flags & I2C_M_NOSTART) == 0)) {
		reg->eddc_addr = priv->msg->addr;
		reg->seg_pointer = wdata[0];
		return ;
	}

	if (msg->flags & I2C_M_NOSTOP) {
		reg->ssar.val = wdata[0];
		reg->scb_ssar_en.val = ((wlen-1) & 0x3);
		for (j = 0; j < ((wlen - 1) & 0x3); j++)
			*((&(reg->dev_offset_byte2)) + j) = wdata[j+1];
		return ;
	}

	for (j = 10; j > 0; j--) {
		wlen = priv->msg->length;
		reg->hcr.val &= MASTER_MODE;
		reg->hsr.val = 0;
		reg->ier.val &= ~(IER_TDI| IER_DNEE | IER_DBIE |IER_ARBLI);
		reg->isr.val = (IER_TDI| IER_DNEE | IER_DBIE |IER_ARBLI);
		reg->sar.val = priv->msg->addr << 1;
		reg->scb_bc_12_5 = ((wlen >> 5) & 0xff);
		reg->fcr.val = (CLEAR_FIFO | (wlen & 0x1f));
		reg->ssar.val = wdata[0];

		if (wlen <= 32) {
			while (wlen) {
				reg->data_register = wdata[tmp_wlen - wlen];
				wlen--;
				if (wlen == 0) {
					usleep(1000);
					reg->hcr.val = (HCR_HCE | HCR_DNEE | HCR_CP_WRITE | HCR_ST);
				}
			}
		} else {
			while (reg->hsr.fifo_full == 0) {
				reg->data_register = wdata[tmp_wlen - wlen];
				wlen--;
			}

			priv->widx = wlen;
			init_completion(&priv->completion);
			reg->fifo_tregger.val = 8;
        		reg->isr1.val = 0xff;                                  
			reg->ier1.fifo_trigger_int_en = 1;

			reg->hcr.val = (HCR_HCE | HCR_DNEE | HCR_CP_WRITE | HCR_ST);

			tmout = (unsigned long)priv->msg->length * 8 * 1000 / priv->baudrate;
			tmout += msecs_to_jiffies(5);
			wait_for_completion_timeout(&priv->completion, tmout);
			reg->ier1.fifo_trigger_int_en = 0;						
		}

		if (hc_i2c_wait_dev_ready(priv, 100) == 0){
			break;
		} else
			status = 0;
			
		usleep(1000);
	}

	if (j==0) {
		priv->state = HC_I2C_STAT_RW_ERR;
		return ;
	}

	if (hc_i2c_wait_trans_done(priv, 1000) != 0) {
		priv->state = HC_I2C_STAT_RW_ERR;
		return ;
	}

	priv->state = HC_I2C_STAT_RW_SUCCESS;

	return ;
}

static int hc_i2c_xfer_msg(struct hc_i2cdev_s *priv, struct i2c_msg_s *msg)
{
	int ret;

	priv->msg = msg;
	priv->state = HC_I2C_STAT_INIT;

	if ((ret = hc_i2c_wait_host_ready(priv, 1000)) != 0)
		goto err;

	priv->state = HC_I2C_STAT_RW;

	if (priv->msg->flags & I2C_M_READ)
		hc_i2c_read_msg(priv);
	else
		hc_i2c_write_msg(priv);

	ret = priv->state;

	if (ret < 0)
		goto err;

	return 0;
err:
	hc_i2c_master_init(priv);
	return  ret;
}

static int hc_i2c_timeout(struct i2c_master_s *dev, unsigned long timeout)
{
	struct hc_i2cdev_s *priv = (struct hc_i2cdev_s *) dev;
	priv->timeout = timeout;
	return 0;
}

static int hc_i2c_transfer(struct i2c_master_s *dev,
		struct i2c_msg_s *msgs,
		int count)
{
	int i, ret;
	struct hc_i2cdev_s *priv = (struct hc_i2cdev_s *) dev;

	if (count == 3)
		priv->eddc_flag = 0x01;
	else
		priv->eddc_flag = 0x00;

	xSemaphoreTake(priv->lock, portMAX_DELAY);
	for (i = 0; i < count; i++, msgs++) {
		if (i == 0) 
			msgs->flags &= ~I2C_M_NOSTART;
		else
			msgs->flags |= I2C_M_NOSTART;

		if (i == count -1) 
			msgs->flags &= ~I2C_M_NOSTOP;
		else 
			msgs->flags |= I2C_M_NOSTOP;
		
		ret = hc_i2c_xfer_msg(priv, msgs);

		if (ret < 0) {
			xSemaphoreGive(priv->lock);
			return ret;
		}
	}
	xSemaphoreGive(priv->lock);

	return count;	
}

static const struct i2c_ops_s hc_i2c_ops = {
	.transfer = hc_i2c_transfer,
	.timeout = hc_i2c_timeout,
};

static void hc_i2c_master_init(struct hc_i2cdev_s *priv)
{
	int irq = i2c_irq_nb[priv->port];
	uint32_t bps = priv->baudrate;
	i2c_reg_t *reg = priv->base;	

	if (irq < 0)
		return;

	if (bps <= 0) {
		priv->state = HC_I2C_STAT_INIT_FAIL; 	
		return ;
	}

	//reg->ier.val    &= IER_FE | IER_FF | IER_SP | IER_SS;
	//reg->ier1.val   = 0;
	//reg->isr.val    = ISR_TDI | ISR_DNEE | ISR_DBIE | ISR_ARBLI;	
	xPortInterruptInstallISR((uint32_t)irq, hc_i2c_interrupt, (uint32_t)priv);

	if (bps < LOW_SPEED) {
		reg->dev_control.fin_set = 0x1;

		reg->hpcc = CLOCK_1_843M / bps;
		reg->lpcc = CLOCK_1_843M / bps;
		reg->psur = CLOCK_1_843M / bps;
		reg->psdr = CLOCK_1_843M / bps;
		reg->rsur = CLOCK_1_843M / bps;
		reg->shdr = CLOCK_1_843M / bps;
	} else {
		reg->dev_control.fin_set = 0x0;

		reg->hpcc = CLOCK_12M / bps;
		reg->lpcc = CLOCK_12M / bps;
		reg->psur = CLOCK_12M / bps;
		reg->psdr = CLOCK_12M / bps;
		reg->rsur = CLOCK_12M / bps;
		reg->shdr = CLOCK_12M / bps;
	}

	reg->fcr.val = CLEAR_FIFO;
	reg->hcr.val = HCR_HCE;

	priv->state = HC_I2C_STAT_INIT;
	return ;
}

static void hc_i2c_slave_init(struct hc_i2cdev_s *priv)
{
	return ;
}

static void strapin_set(uint32_t clear_val, uint32_t set_val)
{
	return ;
}



static struct i2c_master_s *i2cbus_initialize(const char *node, int port)
{
	int np;
	struct hc_i2cdev_s *priv;
	const char *path;
	const char *mode;
	unsigned int baudrate;
	struct pinmux_setting *active_state;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return NULL;

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return NULL;
		
	priv = (struct hc_i2cdev_s *)malloc(sizeof(struct hc_i2cdev_s));
	if (!priv)
		return NULL;

	memset(priv, 0, sizeof(struct hc_i2cdev_s));
	priv->port = port;
	priv->base = (i2c_reg_t *)ctrlreg[port];

	hc_clk_enable(i2c_gate_array[port]);
	
	baudrate = 0;
	fdt_get_property_string_index(np, "mode", 0, &mode);
	fdt_get_property_u_32_index(np, "baudrate", 0, &baudrate);

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}

	if (!strcmp(mode, "slave")) {
		hc_i2c_slave_init(priv);
	} else {
		priv->lock = xSemaphoreCreateMutex();
		priv->baudrate = baudrate;
		hc_i2c_master_init(priv);
	}
		
	if (priv->state < 0)
		return NULL;

	priv->timeout = 1000;
	priv->dev.ops = &hc_i2c_ops;

	return &priv->dev;
}

static int i2cbus_uninitialize(struct i2c_master_s *dev)
{
	return 0;
}

static int i2c_probe(const char *node, int id)
{
	int ret = 0;
	struct i2c_master_s *i2c;

	i2c = i2cbus_initialize(node, id);
	if (i2c == NULL) {
		return 0;
	}

	i2c->simulate = 0;
	ret = i2c_register(i2c, id);
	if (ret < 0) {
		i2cerr("ERROR: Failed to register I2C0 driver:%d\n",\
				ret);

		i2cbus_uninitialize(i2c);
	}

	return ret;
}


static int i2c_init(void)
{
	int rc = 0;

	rc |= i2c_probe("/hcrtos/i2c@0", 0);
	rc |= i2c_probe("/hcrtos/i2c@1", 1);
	rc |= i2c_probe("/hcrtos/i2c@2", 2);
	rc |= i2c_probe("/hcrtos/i2c@3", 3);

	return rc;
}

module_driver(i2c, i2c_init, NULL, 0)
