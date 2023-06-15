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
	SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | SND_PCM_RATE_48000 | SND_PCM_RATE_96000)

#define SND_PCM_RATE_44_1K                                                     \
	(SND_PCM_RATE_11025 | SND_PCM_RATE_22050 | SND_PCM_RATE_44100 | SND_PCM_RATE_88200)

static struct dai_device dai_dev = {
	.cap =
		{
			.rates = (SND_PCM_RATE_32K48K | SND_PCM_RATE_44_1K),
			.formats = (SND_PCM_FMTBIT_S16_LE | SND_PCM_FMTBIT_S24_LE | SND_PCM_FMTBIT_S32_LE),
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
	struct i2c_transfer_s xfer = {0};
	struct i2c_msg_s i2c_msg = {0};

	i2c_msg.addr = slave_addr;
	i2c_msg.flags = 0x0;
	i2c_msg.buffer = wdata;
	i2c_msg.length = 2;

	xfer.msgc = 1;
	xfer.msgv = &i2c_msg;

	ret = ioctl(fd,I2CIOC_TRANSFER,&xfer);
	if (ret < 0) {
		printf ("i2c write failed\n");
	}
	return ret;
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

static int cjc8988_Write_Reg(int fd, uint8_t reg_addr, uint8_t pdata)
{
	uint8_t slave_addr = 0x1A;
	return es_write_reg(fd, slave_addr, reg_addr, pdata);
}

static int cjc8988_read_Reg(int fd, uint8_t *pdata)
{
	uint8_t slave_addr = 0x1A;
	return es_read_reg(fd, slave_addr, pdata);
}

static uint8_t read_data[2] = {0};

static int cjc8988_i2c_config_is_right(int fd, uint8_t chip_addr, uint8_t val)
{
	int ret;
	/* cjc8998 spec */
	if (chip_addr % 2 != 0) {
		chip_addr -= 1;
	}

	read_data[0] = chip_addr;
	ret = cjc8988_read_Reg(fd, &read_data[0]);

	if (read_data[1] != val || ret < 0) {
		printf("cjc8988_Write_Reg 0x%x error\n", chip_addr);
	} else {
		printf("chip_addr 0x%x: val %d is right\n", chip_addr, val);
	}

	return ret;
}

static int cjc8988_dai_hw_params(struct snd_soc_dai *dai, unsigned int rate,
			      snd_pcm_format_t fmt, unsigned int channels,
			      uint8_t align)
{
	int res = 0;
	int fd = 0;
	int8_t val;
	struct dai_device *dai_dev = (struct dai_device *)dai->priv;

	log_i("cjc8988_dai_hw_params, %d, %d, %d\n", (int)rate, (int)fmt, (int)channels);
	fd = open(dai_dev->i2c_dev, O_RDWR);
	if (fd < 0) {
		log_e("cjc8988 i2c1 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	log_i("dai->name %s\n", dai->name);
	if (!strcmp("cjc8988-for-i2so-dai", dai->name)) {
		log_d("cfg snd out\n");
		snd_direction |= SND_OUT;
	} else if (!strcmp("cjc8988-for-i2si-dai", dai->name)) {
		log_d("cfg snd in\n");
		snd_direction |= SND_IN;
	}

	cjc8988_Write_Reg(fd, 0x32, 0x00);//power off adc
	cjc8988_Write_Reg(fd, 0x34, 0x00);//power off dac

	/* R0 & R1, bit7: 1, mute; bit0-5: vol, 000000 = -17.25dB,
	 * 0.75dB steps up to 111111 = +30dB */
	if (snd_direction & SND_IN) {
		//printf("set adc analogue volume\n");
		res += cjc8988_Write_Reg(fd, 0x01, 0x17);//L, 0dB
		res += cjc8988_Write_Reg(fd, 0x03, 0x17);//R, 0dB
		cjc8988_i2c_config_is_right(fd, 0x03, 0x17);
	}

	/* R2 & R3, bit0-6: Analogue vol, 1111111 = +6dB, 80 steps to 0110000 = -67dB,
	 * 0110000 to 0000000 = Analogue MUTE*/
	if (snd_direction & SND_OUT) {
		//printf("set dac analogue volume\n");
		res += cjc8988_Write_Reg(fd, 0x05, 0x79);//L, 0dB
		res += cjc8988_Write_Reg(fd, 0x07, 0x79);//R, 0dB
		cjc8988_i2c_config_is_right(fd, 0x07, 0x79);
	}

	/* R5, ADC & DAC control */
	res += cjc8988_Write_Reg(fd, 0x0A, 0x00);

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
	res += cjc8988_Write_Reg(fd, 0x0E, val);

	/* R8, only used when cjc8988 works on master mode */
	res += cjc8988_Write_Reg(fd, 0x10, 0x00);

	/* R10, bit 7:0, 0000 0000 = Digital Mute(-127.5), ... 0.5dB steps up to 1111 1111 = 0dB*/
	res += cjc8988_Write_Reg(fd, 0x15, 0xFF);
	/* R11, bit 7:0, 0000 0000 = Digital Mute(-127.5), ... 0.5dB steps up to 1111 1111 = 0dB*/
	res += cjc8988_Write_Reg(fd, 0x17, 0xFF);
	cjc8988_i2c_config_is_right(fd, 0x17, 0xFF);

	/* R12, BASS */ 
	res += cjc8988_Write_Reg(fd, 0x18, 0x0F);
	/* R13, bit6: Treble Filter, 0 = High Cutoff (8kHz at 48kHz sampling), 1 = Low Cutoff (4kHz at 48kHz sampling);
	 * bit0-3: Treble Intensity, 0000 or 0001 = +9dB, 1.5dB steps, 1111 = Disable */
	res += cjc8988_Write_Reg(fd, 0x1A, 0x0F);//DAC power select

	/* R16 */
	res += cjc8988_Write_Reg(fd, 0x20, 0x00);

	/* R20 */
	res += cjc8988_Write_Reg(fd, 0x28, 0x00);

	/* R21, bit 7:0, Left ADC volume 0000 0000 = Digital Mute(-97.5dB), 0.5dB steps up to 1111 1111 = +30dB */
	res +=  cjc8988_Write_Reg(fd, 0x2B, 0xC3);//0dB
	/* R22, bit 7:0, Right ADC volume 0000 0000 = Digital Mute(-97.5dB), 0.5dB steps up to 1111 1111 = +30dB*/
	res += cjc8988_Write_Reg(fd, 0x2D, 0xC3);//0dB

	/* R23, DAC mono mix, bit5:4, 00: stereo, 01: DACL, 01: DACR 11: mono ((L+R)/2) into DACL and DACR*/
	res += cjc8988_Write_Reg(fd, 0x2E, 0x00);//stereo

	/* R24 */
	res += cjc8988_Write_Reg(fd, 0x31, 0x80);//bit 7: Enable Common Mode Feedback

	/* R27 VREF to analogue output resistance */
	res += cjc8988_Write_Reg(fd, 0x36, 0x00);

	/* R31 ADC input Mode, bit7:6, 00: Stereo, 01: Analogue Mono Mix (using left ADC), 10: Analogue Mono Mix (using right ADC), 11: Digital Mono Mix*/
	res += cjc8988_Write_Reg(fd, 0x3E, 0x00);

	/* R32 ADC SignalPath Control (Left)
	 * bit7:6, 00 = LINPUT1, 01 = LINPUT2; bit5:4, Left Channel Microphone Gain Boost, 00 = Boost off
	 */
	res += cjc8988_Write_Reg(fd, 0x40, 0x40);
	/* R33 ADC SignalPath Control (Right)) */
	res += cjc8988_Write_Reg(fd, 0x42, 0x40);

	/* R34 Left DAC to Left Mixer, bit2:0, 000 = LINPUT1, 001 = LINPUT2, 010 = Reserved */
	/* bit6:4, LI2LOVOL */
	res += cjc8988_Write_Reg(fd, 0x45, 0x52);

	/* R35 Right DAC to Left Mixer*/
	res += cjc8988_Write_Reg(fd, 0x46, 0x50);

	/* R36 Left DAC to Right Mixer,  bit2:0, 000 = LINPUT1, 001 = LINPUT2, 010 = Reserved  */
	res += cjc8988_Write_Reg(fd, 0x48, 0x52);

	/* R37 Right DAC to Right Mixer*/
	res += cjc8988_Write_Reg(fd, 0x4B, 0x50);

	/* R40 LOUT2VOL*/
	res += cjc8988_Write_Reg(fd, 0x51, 0x79);
	/* R41 OUT2VOL*/
	res += cjc8988_Write_Reg(fd, 0x53, 0x79);

	/* R67 DACMIX BIAS*/
	res += cjc8988_Write_Reg(fd, 0x86, 0x08);//bit 3, low bias current (results in lower performance and power consumption)

	if (snd_direction & SND_IN) {
		/* R25 Pwr Mgmt (1), ADC*/
		log_d("power on adc && vmid\n");
		res += cjc8988_Write_Reg(fd, 0x33, 0x7C);
		cjc8988_i2c_config_is_right(fd, 0x33, 0x7C);
	} else {
		printf("only power on vmid\n");
		cjc8988_Write_Reg(fd, 0x33, 0x00);
	}

	if (snd_direction & SND_OUT) {
		/* R26 Pwr Mgmt (2), DAC*/
		log_d("power on dac\n");
		msleep(10);
		res += cjc8988_Write_Reg(fd, 0x35, 0xF8);
		cjc8988_i2c_config_is_right(fd, 0x35, 0xF8);
	}

	if (res < 0) {
		log_d("cjc8988 i2c config error\n");
		close (fd);	
		return -1;
	}

	close (fd);
	log_i("cjc8988_dai_hw_params done, snd_direction %d\n", snd_direction);
	return 0;
}

static int cjc8988_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int cjc8988_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int cjc8988_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int cjc8988_dai_hw_free(struct snd_soc_dai *dai)
{
	int fd;
	struct dai_device *dai_dev = (struct dai_device *)dai->priv;

	fd = open(dai_dev->i2c_dev, O_RDWR);
	if (fd < 0) {
		log_e("cjc8988 i2c1 open error\n");
		return -1;
	}
	ioctl(fd,I2CIOC_TIMEOUT,100);

	log_i("free dai->name %s\n", dai->name);
	if (!strcmp("cjc8988-for-i2so-dai", dai->name)) {
		snd_direction &= ~SND_OUT;
		/* power down dac */
		cjc8988_Write_Reg(fd, 0x34, 0x00);//R25
		cjc8988_Write_Reg(fd, 0x05, 0x00);//L, mute
		cjc8988_Write_Reg(fd, 0x07, 0x00);//R, mute

		printf("power down dac\n");
	} else if (!strcmp("cjc8988-for-i2si-dai", dai->name)) {
		snd_direction &= ~SND_IN;
		/* power down adc */
		cjc8988_Write_Reg(fd, 0x33, 0x00);//R25
		cjc8988_Write_Reg(fd, 0x01, 0x80);//L, mute
		cjc8988_Write_Reg(fd, 0x03, 0x80);//R, mute
		printf("power down adc\n");
	}

	if (snd_direction == 0) {
		cjc8988_Write_Reg(fd, 0x32, 0x00);//R25
	}

	close (fd);
	return 0;
}

static struct snd_soc_dai_driver cjc8988_dai_driver = {
	.hw_params = cjc8988_dai_hw_params,
	.trigger = cjc8988_dai_trigger,
	.ioctl = cjc8988_dai_ioctl,
	.get_capability = cjc8988_dai_get_capability,
	.hw_free = cjc8988_dai_hw_free,
};

static struct snd_soc_dai cjc8988_for_i2so_dai = {
	.name = "cjc8988-for-i2so-dai",
	.driver = &cjc8988_dai_driver,
	.priv = &dai_dev,
};

static struct snd_soc_dai cjc8988_for_i2si_dai = {
	.name = "cjc8988-for-i2si-dai",
	.driver = &cjc8988_dai_driver,
	.priv = &dai_dev,
};

int cjc8988_dai_init(void)
{
	int np;
	const char *status;

	np = fdt_node_probe_by_path("/hcrtos/cjc8988");
	if (np < 0) {
		return 0;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		!strcmp(status, "disabled")) {
		return 0;
	}

	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &(dai_dev.i2c_dev))) {
		return 0;
	}

	snd_soc_register_dai(&cjc8988_for_i2so_dai);
	snd_soc_register_dai(&cjc8988_for_i2si_dai);
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

	fd = open("dev/i2c1",O_RDWR);
	if (fd < 0) {
		printf("cjc8988 i2c1 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	addr = strtoul(argv[1], NULL, 16);
	val = strtoul(argv[2], NULL, 16);
	printf("cjc8988 i2c1 fd %d, set reg 0x%x to 0x%x\n",fd, addr, val);
	res = cjc8988_Write_Reg(fd, addr, val);
	printf("reg res %d\n", res);
	cjc8988_i2c_config_is_right(fd, addr, val);

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
