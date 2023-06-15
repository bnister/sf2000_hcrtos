#include <stdlib.h>
#include <string.h>
#include <kernel/soc/soc_common.h>
#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <nuttx/i2c/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <nuttx/fs/fs.h>
#include <kernel/elog.h>
#include <kernel/ld.h>
#include <kernel/delay.h>
#include <kernel/lib/fdt_api.h>

struct dai_device {
	struct snd_capability cap;
	const char *i2c_dev;
};

/*
 * Some devices do not support rates lower than 44100, remove those
 * low rates from the capability.
 */
#define SND_PCM_RATE_32K48K                                                    \
	(SND_PCM_RATE_8000 | SND_PCM_RATE_12000 | SND_PCM_RATE_16000 |         \
	 SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | SND_PCM_RATE_48000 |        \
	 SND_PCM_RATE_64000 | SND_PCM_RATE_96000 | SND_PCM_RATE_128000)

#define SND_PCM_RATE_44_1K                                                     \
	(SND_PCM_RATE_5512 | SND_PCM_RATE_11025 | SND_PCM_RATE_22050 | \
	SND_PCM_RATE_44100 | SND_PCM_RATE_88200 | SND_PCM_RATE_176400)

static struct dai_device dai_dev = {
	.cap =
		{
			.rates = (SND_PCM_RATE_32K48K | SND_PCM_RATE_44_1K),
			.formats =
				(SND_PCM_FMTBIT_S8 | SND_PCM_FMTBIT_U8 |
				 SND_PCM_FMTBIT_S16_LE | SND_PCM_FMTBIT_U16_LE |
				 SND_PCM_FMTBIT_S24_LE | SND_PCM_FMTBIT_U24_LE),
		},
};

#define SND_IN 1
#define SND_OUT 2
static int snd_direction = 0;

static int es_write_reg(int fd, uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
{
	int ret = 0;
	uint8_t wdata[2] = {0,0};
	wdata[0] = reg_addr;
	wdata[1] = data;
	struct i2c_transfer_s xfer;
	struct i2c_msg_s i2c_msg = {0};
	xfer.msgv = &i2c_msg;
	xfer.msgv->addr = slave_addr;
	xfer.msgv->flags = 0x0;
	xfer.msgv->buffer = wdata;
	xfer.msgv->length = 2;
	xfer.msgc         = 1;

	ret = ioctl(fd,I2CIOC_TRANSFER,&xfer);
	if (ret < 0) {
		printf ("i2c write failed\n");
	}
	return ret;
}

static int cjc8990_Write_Reg(int fd, uint8_t reg_addr, uint8_t pdata)
{
	uint8_t slave_addr = 0x1B;
	return es_write_reg(fd, slave_addr, reg_addr, pdata);
}

static int es_read_reg(int fd, uint8_t slave_addr, uint8_t *data)
{
	int ret = 0;

	struct i2c_transfer_s xfer;
	struct i2c_msg_s i2c_msg[2] = {0};

	i2c_msg[0].addr = slave_addr;
	i2c_msg[0].buffer = &data[0];
	i2c_msg[0].flags = 0;
	i2c_msg[0].length = 1;

	i2c_msg[1].addr = slave_addr;
	i2c_msg[1].buffer = &data[1];
	i2c_msg[1].flags = 1;
	i2c_msg[1].length = 1;

	xfer.msgv = i2c_msg;
	xfer.msgc = 2;

	ret = ioctl(fd,I2CIOC_TRANSFER,&xfer);
	if (ret < 0) {
		printf ("i2c read failed\n");
	}

	return ret;
}

static int cjc8990_read_Reg(int fd, uint8_t *pdata)
{
	uint8_t slave_addr = 0x1B;
	return es_read_reg(fd, slave_addr, pdata);
}

static uint8_t read_data[2] = {0};

static int cjc8990_i2c_config_is_right(int fd, uint8_t chip_addr, uint8_t val)
{
	int ret;
	/* cjc8990 spec */
	if (chip_addr % 2 != 0) {
		chip_addr -= 1;
	}

	read_data[0] = chip_addr;
	ret = cjc8990_read_Reg(fd, &read_data[0]);

	if (read_data[1] != val || ret < 0) {
		printf("cjc8990_Write_Reg 0x%x error\n", chip_addr);
	} else {
		printf("chip_addr 0x%x: val %d is right\n", chip_addr, val);
	}

	return ret;
}

static int cjc8990_dai_hw_params(struct snd_soc_dai *dai, unsigned int rate,
			      snd_pcm_format_t fmt, unsigned int channels,
			      uint8_t align)
{
	int res = 0;
	int fd = 0;
	uint8_t val = 0;
	struct dai_device *dai_dev = (struct dai_device *)dai->priv;

	log_i("cjc8990_dai_hw_params, %d, %d, %d\n", (int)rate, (int)fmt, (int)channels);
	fd = open(dai_dev->i2c_dev,O_RDWR);
	if(fd < 0){
		printf("cjc8990 open %s error\n", dai_dev->i2c_dev);
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	log_i("dai->name %s\n", dai->name);
	if (!strcmp("cjc8990-for-i2so-dai", dai->name)) {
		log_i("cfg snd out\n");
		snd_direction |= SND_OUT;
	} else if (!strcmp("cjc8990-for-i2si-dai", dai->name)) {
		log_i("cfg snd in\n");
		snd_direction |= SND_IN;
	}

	//power off adc && dac
	cjc8990_Write_Reg(fd, 0x32, 0x00);
	cjc8990_Write_Reg(fd, 0x34, 0x00);

	if (snd_direction & SND_IN) {
		res = cjc8990_Write_Reg(fd, 0x01, 0x17);
		if (res < 0) {
			printf("cjc8990 i2c config error\n");
			close(fd);
			return -1;
		}
	}

	if (snd_direction & SND_OUT) {
		res +=  cjc8990_Write_Reg(fd, 0x05, 0x79);
		res +=  cjc8990_Write_Reg(fd, 0x07, 0x79);
	}

	res +=  cjc8990_Write_Reg(fd, 0x0A, 0x00);

	/* R7, bit7, 0 = BCLK not inverted; bit 6, 0 = Enable Slave Mode; 
	 * bit5, 1 = swap left and right DAC data; bit4, LRCLK polarity or pcm A/B select;
	 * bit3-2 Word Length 11: 32bit 10: 24bit 01: 20bit  00:16bit
	 * bit1-0 Format Select 11: pcm mode 10: i2s mode 01: Left justified
	 */
	val = 2;//i2s mode
	if (fmt == SND_PCM_FORMAT_S16_LE) {
		val |= (0 << 2);
	} else if (fmt == SND_PCM_FORMAT_S24_LE) {
		val |= (2 << 2);
	} else if (fmt == SND_PCM_FORMAT_S32_LE) {
		val |= (3 << 2);
	}
	res += cjc8990_Write_Reg(fd, 0x0E, val);

	res +=  cjc8990_Write_Reg(fd, 0x10, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x15, 0xFF);

	res +=  cjc8990_Write_Reg(fd, 0x17, 0xFF);

	res +=  cjc8990_Write_Reg(fd, 0x18, 0x0F);

	res +=  cjc8990_Write_Reg(fd, 0x1A, 0x0F);

	res +=  cjc8990_Write_Reg(fd, 0x20, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x22, 0x7B);

	res +=  cjc8990_Write_Reg(fd, 0x24, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x26, 0x32);

	res +=  cjc8990_Write_Reg(fd, 0x28, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x2B, 0xC3);

	res +=  cjc8990_Write_Reg(fd, 0x2E, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x30, 0x04);

	res +=  cjc8990_Write_Reg(fd, 0x36, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x3E, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x40, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x42, 0x0A);

	res +=  cjc8990_Write_Reg(fd, 0x44, 0x0A);

	res +=  cjc8990_Write_Reg(fd, 0x47, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x48, 0x80);

	res +=  cjc8990_Write_Reg(fd, 0x4B, 0x00);

	res +=  cjc8990_Write_Reg(fd, 0x86, 0x00);

	if (snd_direction & SND_IN) {
		/* R25 Pwr Mgmt (1), ADC*/
		log_d("power on adc && vmid\n");
		res += cjc8990_Write_Reg(fd, 0x33, 0x68);
		cjc8990_i2c_config_is_right(fd, 0x33, 0x68);
	} else {
		log_d("only power on vmid & vref\n");
		cjc8990_Write_Reg(fd, 0x33, 0x40);
	}

	if (snd_direction & SND_OUT) {
		/* R26 Pwr Mgmt (2), DAC*/
		log_d("power on dac\n");
		res += cjc8990_Write_Reg(fd, 0x35, 0xE0);
		cjc8990_i2c_config_is_right(fd, 0x35, 0xE0);
	}

	if (res < 0) {
		log_e("cjc8990 i2c config error\n");
		close(fd);
		return -1;
	}

	close(fd);
	log_i("cjc8990_dai_hw_params done, snd_direction %d\n", snd_direction);
	return 0;
}

static int cjc8990_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int cjc8990_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int cjc8990_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int cjc8990_dai_hw_free(struct snd_soc_dai *dai)
{
	int fd;
	struct dai_device *dai_dev = (struct dai_device *)dai->priv;

	fd = open(dai_dev->i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("cjc8990 open %s error\n", dai_dev->i2c_dev);
		return -1;
	}
	ioctl(fd, I2CIOC_TIMEOUT, 100);

	printf("free dai->name %s\n", dai->name);
	if (!strcmp("cjc8990-for-i2so-dai", dai->name)) {
		snd_direction &= ~SND_OUT;
		/* power down dac */
		cjc8990_Write_Reg(fd, 0x34, 0x00);//R25
		cjc8990_Write_Reg(fd, 0x05, 0x00);//L, mute
		cjc8990_Write_Reg(fd, 0x07, 0x00);//R, mute
		cjc8990_Write_Reg(fd, 0x15, 0x00);
		cjc8990_Write_Reg(fd, 0x17, 0x00);

		printf("power down dac\n");
	} else if (!strcmp("cjc8990-for-i2si-dai", dai->name)) {
		snd_direction &= ~SND_IN;
		/* power down adc */
		cjc8990_Write_Reg(fd, 0x33, 0x00);//R25
		cjc8990_Write_Reg(fd, 0x01, 0x80);//L, mute
		if (snd_direction & SND_OUT) {
			cjc8990_i2c_config_is_right(fd, 0x35, 0xE0);
		}
		printf("power down adc\n");
	}

	if (snd_direction == 0) {
		cjc8990_Write_Reg(fd, 0x32, 0x00);//R25
	}

	close (fd);
	return 0;

}

static struct snd_soc_dai_driver cjc8990_dai_driver = {
	.hw_params = cjc8990_dai_hw_params,
	.trigger = cjc8990_dai_trigger,
	.ioctl = cjc8990_dai_ioctl,
	.get_capability = cjc8990_dai_get_capability,
	.hw_free = cjc8990_dai_hw_free,
};

static struct snd_soc_dai cjc8990_for_i2so_dai = {
	.name = "cjc8990-for-i2so-dai",
	.driver = &cjc8990_dai_driver,
	.priv = &dai_dev,
};

static struct snd_soc_dai cjc8990_for_i2si_dai = {
	.name = "cjc8990-for-i2si-dai",
	.driver = &cjc8990_dai_driver,
	.priv = &dai_dev,
};

int cjc8990_dai_init(void)
{
	int np;
	const char *status;

	np = fdt_node_probe_by_path("/hcrtos/cjc8990");
	if (np < 0) {
		return 0;
	}

	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &dai_dev.i2c_dev)) {
		return 0;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		!strcmp(status, "disabled")) {
		return 0;
	}

	snd_soc_register_dai(&cjc8990_for_i2so_dai);
	snd_soc_register_dai(&cjc8990_for_i2si_dai);
	return 0;
}

#if 0
static int reg_set(int argc, char *argv[])
{
	int fd, res;
	uint8_t addr, val;

	(void)argc;
	(void)argv;
	if (argc < 3) {
		return 0;
	}

	fd = open("dev/i2c0",O_RDWR);
	if (fd < 0) {
		printf("i2c0 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	addr = strtoul(argv[1], NULL, 16);
	val = strtoul(argv[2], NULL, 16);
	printf("i2c0 fd %d, set reg 0x%x to 0x%x\n",fd, addr, val);
	res = cjc8990_Write_Reg(fd, addr, val);
	printf("reg res %d\n", res);
	cjc8990_i2c_config_is_right(fd, addr, val);

	close(fd);
	return 0;
}

CONSOLE_CMD(reg, NULL, reg_set, CONSOLE_CMD_MODE_SELF,
	"reg addr val")

#include <hcuapi/gpio.h>
static int gpio_set(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	if (argc < 3) {
		return 0;
	}
	printf("set gpio %d to %d\n", atoi(argv[1]), atoi(argv[2]));
	gpio_configure(atoi(argv[1]), GPIO_DIR_OUTPUT);
	gpio_set_output(atoi(argv[1]), atoi(argv[2]));

	return 0;
}

CONSOLE_CMD(gpio, NULL, gpio_set, CONSOLE_CMD_MODE_SELF,
	"gpio pin val")
#endif

