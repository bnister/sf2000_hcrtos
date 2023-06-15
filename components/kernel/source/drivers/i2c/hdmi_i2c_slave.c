#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/unistd.h>

#include <kernel/ld.h>
#include <nuttx/wqueue.h>
#include <kernel/lib/fdt_api.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <hcuapi/pinmux.h>
#include <hcuapi/i2c-slave.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <kernel/drivers/hc_clk_gate.h>

#include "i2c_reg_struct.h"

#define READ_BUF_INIT		0x01
#define WRITE_BUF_INIT		0x10

#define FIFO_TRIGGER_CLE	0xff	
#define FIFO_TRIGGER_EN		0x01

#define IER_DEV_BUSY		0x20
#define ISR_DEV_BUSY		0x04
#define HSR_ST			0x20

#define MS_TO_US(msec) ((msec) * 1000)
#define TIMEOUT MS_TO_US(10000)

int g_refs = 0;
static void *ctrlreg[4] = {
	(void *)&I2C0CTRL,
	(void *)&I2C1CTRL,
	(void *)&I2C2CTRL,
	(void *)&I2C3CTRL,
};

static void *irq_num[4] = {
	(void *)&I2C0_INTR,
	(void *)&I2C1_INTR,
	(void *)&I2C2_INTR,
	(void *)&I2C3_INTR,
};

enum transfer_mode {
	IDLE_MODE = 0,
	READ_MODE = 1,
	WRITE_MODE = 2,
	EDDC_MODE = 3,
};

static int i2c_gate_array[4] = {
	I2C1_CLK,
	I2C2_CLK,
	I2C3_CLK,
	I2C4_CLK,
};

struct hc_hdmi_i2cs_priv_s {
	const struct i2c_slaveops_s *ops;

	uint32_t base;
	uint32_t irq;
	enum transfer_mode status;

	int slave_addr;
	int addr_nbits;

	int eddc_addr;

	uint8_t *read_buffer;
	int read_buflen;
	int read_bufindex;

	const uint8_t *write_buffer;
	int write_buflen;
	int write_bufindex;
	int index;
	int fifo_data_num;

	int (*callback)(void *arg);
	void *callback_arg;

	int refs;
	int buf_flag;
	struct work_s work;
	QueueHandle_t	 lock;

	unsigned long			timeout_jiffies;
	struct timer_list		fifo_timeout;	
};


static int hc_hdmi_i2cs_read(struct i2c_slave_s *dev,
			     uint8_t *buffer, int buflen);
static int hc_hdmi_i2cs_write(struct i2c_slave_s *dev, 
			      const uint8_t *buffer, int buflen);
static int hc_hdmi_i2cs_setownaddress(struct i2c_slave_s *dev,
				      int addr, int nbits);
static int hc_hdmi_i2cs_setddcaddress(struct i2c_slave_s *dev, int eddc_addr);

static const struct i2c_slaveops_s hc_hdmi_i2cs_slaveops = {
	.setddcaddress		= hc_hdmi_i2cs_setddcaddress,
	.setownaddress		= hc_hdmi_i2cs_setownaddress,
	.write			= hc_hdmi_i2cs_write,
	.read			= hc_hdmi_i2cs_read,
	.registercallback	= NULL,
};

static int hc_hdmi_i2cs_setownaddress(struct i2c_slave_s *dev,
				      int addr, int nbits)
{
	struct hc_hdmi_i2cs_priv_s *priv = (struct hc_hdmi_i2cs_priv_s *)dev;

	i2c_reg_t *reg= (i2c_reg_t *)priv->base;
	
	if (nbits == 7) {
		priv->slave_addr = addr;		
		priv->addr_nbits = 7;
		reg->sar.slave_address = priv->slave_addr;
	}	

	return 0;
}

static int hc_hdmi_i2cs_setddcaddress(struct i2c_slave_s *dev, int eddc_addr)
{
	struct hc_hdmi_i2cs_priv_s *priv = (struct hc_hdmi_i2cs_priv_s *)dev;

	i2c_reg_t *reg= (i2c_reg_t *)priv->base;

	priv->eddc_addr = eddc_addr;	

	reg->eddc_addr = eddc_addr;	

	return 0;
}

static int hc_hdmi_i2cs_write(struct i2c_slave_s *dev, 
			      const uint8_t *buffer, int buflen)
{
	struct hc_hdmi_i2cs_priv_s *priv;

	priv = (struct hc_hdmi_i2cs_priv_s *)dev;
	i2c_reg_t *reg = (i2c_reg_t *)priv->base;

	xSemaphoreTake(priv->lock, portMAX_DELAY);

	reg->ier1.fifo_trigger_int_en = 0x00;

	priv->write_buffer = buffer;
	priv->write_buflen = buflen;

	priv->buf_flag |= WRITE_BUF_INIT;
	if (priv->buf_flag & READ_BUF_INIT)
		reg->ier1.fifo_trigger_int_en = 0x01;

	xSemaphoreGive(priv->lock);
	
	return 0;
}

static int hc_hdmi_i2cs_read(struct i2c_slave_s *dev,
			     uint8_t *buffer, int buflen)
{
	struct hc_hdmi_i2cs_priv_s *priv;

	priv = (struct hc_hdmi_i2cs_priv_s *)dev;
	i2c_reg_t *reg = (i2c_reg_t *)priv->base;

	xSemaphoreTake(priv->lock, portMAX_DELAY);

	reg->ier1.fifo_trigger_int_en = 0x00;
	
	priv->read_buffer = buffer;
	priv->read_buflen = buflen;

	priv->buf_flag |= READ_BUF_INIT;
	
	if (priv->buf_flag & WRITE_BUF_INIT) {
		reg->ier1.fifo_trigger_int_en = 0x01;
	}

	xSemaphoreGive(priv->lock);

	return 0;
}

static void hc_hdmi_i2cs_isr(uint32_t param)
{
	struct hc_hdmi_i2cs_priv_s *priv = (struct hc_hdmi_i2cs_priv_s *)param;

	i2c_reg_t *reg	= (i2c_reg_t *)priv->base;

	if (reg->isr.slave_selec_int == 1)
		reg->isr.slave_selec_int = 1;
	
	if (reg->isr1.fifo_trigger_int == 1)
		reg->isr1.fifo_trigger_int = 1;

	if (reg->isr.stop_int == 1) {
		reg->isr.stop_int = 1;
		if (reg->dev_control.slave_send_data_en == 1) {
			reg->dev_control.slave_send_data_en = 0;

			if (reg->isr1.fifo_trigger_int == 1)
				reg->isr1.fifo_trigger_int = 1;

			reg->fcr.fifo_flush = 1;
		}
		
		return;
	}

	if (reg->addr_got.val == 0xa0) {
		reg->fcr.fifo_flush = 1;

        while(reg->fcr.byte_count <= priv->fifo_data_num)
        {
            if(reg->addr_got.val != 0xa0)
                return;
        }

		while (reg->fcr.byte_count) {
			priv->index = reg->data_register;
		}

		reg->fcr.fifo_flush = 1;
		while (reg->hsr.fifo_full == 0)
			reg->data_register = priv->write_buffer[priv->index++];

		reg->isr1.fifo_trigger_int = 1;

		priv->fifo_data_num = 0;
	} else if (reg->addr_got.val == 0xa1) {
		if (reg->dev_control.slave_send_data_en == 0) {
			while (reg->hsr.fifo_full == 0)
				reg->data_register = priv->write_buffer[priv->index++];

			reg->dev_control.slave_send_data_en = 1;
		} else {
			while (reg->hsr.fifo_full == 0)
				reg->data_register = priv->write_buffer[priv->index++];

			if (reg->isr1.fifo_trigger_int == 1)
				reg->isr1.fifo_trigger_int = 1;
		}
	}


	return;
}

static int hc_hdmi_i2cs_init(struct hc_hdmi_i2cs_priv_s *priv)
{
	i2c_reg_t *reg	= (i2c_reg_t *)priv->base;

	reg->ier.val			= 0x00;
	reg->hcr.host_ctrl_en		= 0x00;	
	reg->sar.slave_address		= priv->slave_addr;
	reg->eddc_addr			= priv->eddc_addr;
	reg->dev_control.slave_ddc_en	= 0x01;
	reg->hpcc = 0x02;
	reg->lpcc = 0x02;

	xPortInterruptInstallISR(priv->irq, hc_hdmi_i2cs_isr, (uint32_t)priv);

	priv->fifo_data_num = 0;
	reg->ier.val = 0x00;
	reg->fifo_tregger.fifo_trigger_leve = 0x0f;
	reg->ier1.fifo_trigger_int_en = 0x01;
	reg->ier.slave_selec_int_en = 0x01;
	reg->ier.stop_int_en = 0x01;

	priv->status = IDLE_MODE; 
	priv->read_bufindex = 0;
	priv->write_bufindex = 0;
}

struct i2c_slave_s *hc_hdmi_i2cs_initialize(int port,void *param)
{
	int np;
	char name[128];
	struct hc_hdmi_i2cs_priv_s *priv = (struct hc_hdmi_i2cs_priv_s *)param;
	struct pinmux_setting *active_state;

	memset(name, 0, sizeof(name));
	sprintf(name, "/hcrtos/hdmi_i2cs@%d", port);
	np = fdt_node_probe_by_path(name);
	if (np < 0)
		return NULL;

	if (priv == NULL) {
		priv = malloc(sizeof(struct hc_hdmi_i2cs_priv_s));
		if (!priv)
			return NULL;
	}

	hc_clk_enable(i2c_gate_array[port]);

	memset(priv, 0, sizeof(struct hc_hdmi_i2cs_priv_s));

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}

	priv->base = (uint32_t)ctrlreg[port]; 
	priv->irq = (uint32_t)irq_num[port];

	fdt_get_property_u_32_index(np, "slave-addr", 0, &priv->slave_addr);

	fdt_get_property_u_32_index(np, "eddc-addr", 0, &priv->eddc_addr);
		
	priv->ops = &hc_hdmi_i2cs_slaveops;

	priv->lock = xSemaphoreCreateMutex();

	priv->refs = 0;

	hc_hdmi_i2cs_init(priv);

	priv->refs++;

  	return (struct i2c_slave_s *)priv;
}

int hc_hdmi_i2cs_deinit(void *param)
{
	struct hc_hdmi_i2cs_priv_s *priv = (struct hc_hdmi_i2cs_priv_s *)param;

	if ((priv == NULL))
		return -1;

	xPortInterruptRemoveISR(priv->irq, hc_hdmi_i2cs_isr);

	*((uint32_t *)0xb8800084) |= 0x1000;
	usleep(1000);
	*((uint32_t *)0xb8800084) &= (~0x1000);

	free(priv);
	priv = NULL;

	return 0;
}
