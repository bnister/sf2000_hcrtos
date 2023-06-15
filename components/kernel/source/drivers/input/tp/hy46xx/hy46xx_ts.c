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
#include <nuttx/fs/fs.h>

#include <errno.h>
#include <kernel/drivers/spi.h>
#include <nuttx/fs/fs.h>
#include <hcuapi/spidev.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <kernel/lib/fdt_api.h>

#include "hy46xx_ts.h"
#include <hcuapi/gpio.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <hcuapi/i2c-master.h>
#include <freertos/semphr.h>

static int hy46xx_i2c_Read(struct hy46xx_ts_data *data, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret;
	int fd;
	
	fd = open(data->i2c_devpath,O_RDWR);
	if(fd < 0)
		return -1;
#if 1
	if (writelen > 0) {
		struct i2c_transfer_s xfer_read;
		struct i2c_msg_s i2c_msg_read[2] = { 0 };

		i2c_msg_read[0].addr = (uint8_t)data->addr;
		i2c_msg_read[0].flags = 0x0;
		i2c_msg_read[0].buffer = writebuf;
		i2c_msg_read[0].length = writelen;

		i2c_msg_read[1].addr = (uint8_t)data->addr;
		i2c_msg_read[1].flags = 0x1;
		i2c_msg_read[1].buffer = readbuf;
		i2c_msg_read[1].length = readlen;

		xfer_read.msgv = i2c_msg_read;
		xfer_read.msgc = 2;

		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_read);

		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n", __func__);
	} else {
		struct i2c_transfer_s xfer_write;
		struct i2c_msg_s i2c_msg_write;

		i2c_msg_write.addr = (uint8_t)data->addr;
		i2c_msg_write.buffer = readbuf;
		i2c_msg_write.length = readlen;

		xfer_write.msgv = &i2c_msg_write;
		xfer_write.msgc = 1;

		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_write);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
#endif
	close(fd);

	return ret;
}

static int hy46xx_read_Touchdata(struct hy46xx_ts_data *data)
{
	struct ts_event *event = &data->event;
	unsigned char  buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	int i = 0;
	unsigned char pointid = HY_MAX_ID;

	for(i = 0; i < POINT_READ_BUF; i++)
	{
		buf[i] = 0xff;
	}
	memset(event, 0, sizeof(struct ts_event));
	buf[0] = 0;
	ret = hy46xx_i2c_Read(data, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&data->client->dev, "%s read touchdata failed.\n",
			__func__);
		return ret;
	}
	#if 0
	for(i = 0; i < 9; i++)
	{
		pr_info("buf[%d]= 0x%x  ", i,buf[i]);
	}
	#endif
	event->touch_point = 0;
	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {
		pointid = (buf[HY_TOUCH_ID_POS + HY_TOUCH_STEP * i]) >> 4;
		if (pointid >= HY_MAX_ID)
			break;
		else{
			event->touch_point++;
			event->au16_x[i] =(unsigned short) (buf[HY_TOUCH_X_H_POS + HY_TOUCH_STEP * i] & 0x0F) << 8 | (s16) buf[HY_TOUCH_X_L_POS + HY_TOUCH_STEP * i];
			event->au16_y[i] =(unsigned short) (buf[HY_TOUCH_Y_H_POS + HY_TOUCH_STEP * i] & 0x0F) <<8 | (s16) buf[HY_TOUCH_Y_L_POS + HY_TOUCH_STEP * i];
			event->au8_touch_event[i] =buf[HY_TOUCH_EVENT_POS + HY_TOUCH_STEP * i] >> 6;
			event->au8_finger_id[i] =(buf[HY_TOUCH_ID_POS + HY_TOUCH_STEP * i]) >> 4;
			}
		#if 0
		pr_info("id=%d event=%d x=%d y=%d\n", event->au8_finger_id[i],event->au8_touch_event[i], event->au16_x[i], event->au16_y[i]);
		#endif
	}

	event->pressure = 0x05;

	return 0;
}

void hy46xx_report_value(struct hy46xx_ts_data *data)
{
	struct ts_event *event = &data->event; 
	int i; 
	int uppoint = 0;
	for (i = 0; i < event->touch_point; i++)
	{
		if((event->au16_x[i] < RESOLUTION_X) && (event->au16_y[i] < RESOLUTION_Y))
		{
			input_mt_slot(data->input_dev, event->au8_finger_id[i]);			
			if (event->au8_touch_event[i]== 0 || event->au8_touch_event[i] == 2)
			{								
				input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,true);
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,event->pressure);
				input_report_abs(data->input_dev, ABS_MT_POSITION_X,event->au16_x[i]);
				input_report_abs(data->input_dev, ABS_MT_POSITION_Y,event->au16_y[i]);
			}
			else
			{
				uppoint++;
				input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,false);
			}
			
		}
		else
		{
			input_mt_slot(data->input_dev, event->au8_finger_id[i]);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,false);
		}
#ifdef LINUX_OS
		input_mt_sync(data->input_dev);
#endif

		input_mt_sync_frame(data->input_dev);
		input_sync(data->input_dev);
	}

#if 0
	if(event->touch_point == uppoint)
		input_report_key(data->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(data->input_dev, BTN_TOUCH, 1);

	input_sync(data->input_dev);
#endif

}

static void hy46xx_work(void *param)
{
	struct hy46xx_ts_data *hy46xx_ts = (struct hy46xx_ts_data*)param;
	
	hy46xx_read_Touchdata(hy46xx_ts);
	hy46xx_report_value(hy46xx_ts);
	
	return;
}
 
static void hy46xx_irq(uint32_t param)
{
	struct hy46xx_ts_data *hy46xx_ts = (struct hy46xx_ts_data*)param;

	if (work_available(&hy46xx_ts->work)) {
		work_queue(HPWORK, &hy46xx_ts->work, hy46xx_work, (void *)hy46xx_ts, 0);
	}
}

void hy46xx_reset_tp(struct hy46xx_ts_data * param)
{
	gpio_set_output(param->reset, 0);
	usleep(10000);
	gpio_set_output(param->reset, 1);
	usleep(5000); 
}

static int hc_hy46xx_ts_probe(void)
{
	int ret,np;
	gpio_pinset_t irq_pinset = GPIO_DIR_INPUT | GPIO_IRQ_FALLING;
	struct hy46xx_ts_data *hy46xx_ts;
	struct input_dev		*input_dev;

	hy46xx_ts =  kzalloc(sizeof(*hy46xx_ts),GFP_KERNEL);
	hy46xx_ts->x_max = RESOLUTION_X;
	hy46xx_ts->y_max = RESOLUTION_Y;


	np = fdt_get_node_offset_by_path("/hcrtos/hy46xx_ts");
	if (np < 0) {
		printf("no find hy46xx_ts\n");
		ret = -ENOMEM;
	}

	if (fdt_get_property_u_32_index(np, "reset-gpio", 0, (u32 *)&hy46xx_ts->reset))
		return -1;
	if (fdt_get_property_u_32_index(np, "irq-gpio", 0, (u32 *)&hy46xx_ts->irq))
		return -1;
	if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&hy46xx_ts->addr))
		return -1;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &hy46xx_ts->i2c_devpath))
		return -1;

	gpio_configure(hy46xx_ts->irq,irq_pinset);
	ret = gpio_irq_request(hy46xx_ts->irq,hy46xx_irq,(uint32_t)hy46xx_ts);
	if (ret < 0) {
		printf("ERROR: gpio_irq_request() failed: %d\n", ret);
		goto err_probe;
	}
	gpio_irq_disable(hy46xx_ts->irq);

	gpio_configure(hy46xx_ts->reset,GPIO_DIR_OUTPUT);

	hy46xx_reset_tp(hy46xx_ts);
	usleep(200000);

	input_dev = input_allocate_device();

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);

	input_mt_init_slots(input_dev, CFG_MAX_TOUCH_POINTS, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0XFF, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,  0, (RESOLUTION_X-1), 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,  0, (RESOLUTION_Y-1), 0, 0);

	hy46xx_ts->input_dev = input_dev;
	ret = input_register_device(input_dev);


	gpio_irq_enable(hy46xx_ts->irq);

	return 0;

err_probe:
	return ret;
}

static int hc_hy46xx_ts_init(void)
{
	int ret = 0;

	ret = hc_hy46xx_ts_probe();

	return ret;
}

module_driver(hc_hy46xx_driver,hc_hy46xx_ts_init,NULL,1)
