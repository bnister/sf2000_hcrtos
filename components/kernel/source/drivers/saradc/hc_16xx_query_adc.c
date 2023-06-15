#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <hcuapi/efuse.h>
#include <linux/jiffies.h>
#include "adc_16xx_reg_struct.h"
#include "hc_16xx_key_adc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <nuttx/fs/fs.h>
#include <linux/slab.h>

struct adc_priv_16xx
{
	int 			ch_id;
	struct 			device *dev;
	void 			__iomem *base;
	uint32_t		def_key_value;
	uint8_t 		efuse_def_val;
};

static int queryadc_open(struct file *filep){
	return 0;
}

static int queryadc_close(struct file *filep){
	return 0;
}

static uint8_t sar_dout = 0;
static ssize_t queryadc_read(struct file *filep, char *buffer, size_t buflen)
{
	struct inode *inode = filep->f_inode;
	struct adc_priv_16xx *priv = inode->i_private;
	adc_reg_t *reg = (adc_reg_t *)priv->base;

	if (priv->efuse_def_val == 0)
		sar_dout = reg->read_data[priv->ch_id].ch;
	else
		sar_dout = reg->read_data[priv->ch_id].ch * 237 /
			   priv->def_key_value;
	*buffer = sar_dout;

	return buflen;
}

static const struct file_operations query_fops = {
	.open  = queryadc_open, /* open */
	.close = queryadc_close, /* close */
	.read  = queryadc_read, /* read */
};

static uint32_t hc_get_def_val_from_efuse(void)
{
	struct hc_efuse_bit_map bitmap;

	int fd = open("/dev/efuse", O_RDWR);
	if (fd < 0) {
		printf("can't find /dev/efuse\n");
		return 0;
	}

	memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
	ioctl(fd, EFUSE_DUMP, (uint32_t)&bitmap);

	return bitmap.customer.content0;
}

static void hc_set_key_adc_def_val(struct adc_priv_16xx *priv)
{
	int id;

	adc_reg_t *reg = (adc_reg_t *)priv->base;

	if (priv->ch_id > 3)
		id = priv->ch_id + 2;
	else 
		id = priv->ch_id;

	reg->saradc_en.sar_en = 0x01;
	usleep(10000);
	reg->def_val[priv->ch_id].ch = reg->read_data[id].ch;
	reg->def_val[priv->ch_id].ch = reg->read_data[id].ch;
	usleep(10000);
//	priv->deviation = reg->def_val[priv->ch_id].ch;

	reg->saradc_en.sar_en = 0x01;

	return;
}

static void hc_key_adc_init(struct adc_priv_16xx *priv)
{
	adc_reg_t *reg = (adc_reg_t *)priv->base;

	reg->ctrl_reg.touch_panel_mode_sel	= 0x00;
	reg->count_end_thr[priv->ch_id].ch	= 0x06;
	reg->ave_thr[priv->ch_id].ch		= 0x02;
	reg->cmp_def_val[priv->ch_id].ch	= 0x0a;
	reg->count_thr[priv->ch_id].ch		= 0x06;
	reg->ctrl_reg.val			|= 0x01 << (16 + priv->ch_id);

	reg->enable_ctl.out_wait_end_int_en	= 0x01;
	reg->ctrl_reg.new_arc_mode_sel		= 0x01;
	reg->old_read_ch			&= 0x00ffffff;
	reg->saradc_ctl.saradc_pwd		= 0x00;

	reg->def_val[priv->ch_id].ch = priv->def_key_value;
	if (priv->def_key_value == 0) {
		priv->efuse_def_val = 0;
		hc_set_key_adc_def_val(priv);
	} else
		priv->efuse_def_val = 1;

	return;
}

static int hc_16xx_queryadc_probe(char *node, int id)
{
	int np;
	int ret = -EINVAL;
	const char * path;
	struct adc_priv_16xx *priv;

	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		return 0;
	}

	if (fdt_get_property_string_index(np, "devpath", 0, &path)) {
		return -ENOENT;
	}

	priv = kzalloc(sizeof(struct adc_priv_16xx), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->base = (void *)&ADCCTRL;
	priv->ch_id = id;

	priv->def_key_value =  hc_get_def_val_from_efuse() ;
	hc_key_adc_init(priv);

	register_driver(path, &query_fops, 0666, priv);

	return 0;

err:	
	kfree(priv);

	return ret;
}

static int hc_16xx_queryadc_init(void)
{
    int ret = 0;

    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc0", 0);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc1", 1);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc2", 2);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc3", 3);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc4", 4);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc5", 5);

    return ret;
}

module_driver(saradc, hc_16xx_queryadc_init, NULL, 0)
