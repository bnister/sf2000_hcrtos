#include <stdlib.h>
#include <string.h>
#include <kernel/soc/soc_common.h>
#include <kernel/drivers/snd.h>
#include <kernel/module.h>
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
#include <kernel/delay.h>
#include <kernel/lib/fdt_api.h>

#define USE_BOARD_MIC 1
#define USE_EARPHONE_MIC 0

struct dai_device {
	struct snd_capability cap;
	int fd;
};

#define SND_PCM_RATE_32K48K                                                    \
	(SND_PCM_RATE_8000 | SND_PCM_RATE_12000 | SND_PCM_RATE_16000 |         \
	 SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | SND_PCM_RATE_48000 |        \
	 SND_PCM_RATE_64000 | SND_PCM_RATE_96000 | SND_PCM_RATE_128000 |       \
	 SND_PCM_RATE_192000)

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

static int WM8960_Write_Reg(int fd, uint8_t reg_addr, uint16_t pdata)
{
	int ret = 0;
	uint8_t slave_addr = 0x34>>1;
	uint8_t data1;
	uint8_t data2;

	data1 = (reg_addr << 1) | ((uint8_t)((pdata >> 8) & 0x0001));
	data2 = (uint8_t)(pdata & 0x00ff);

	do {
		ret =  es_write_reg(fd, slave_addr, data1, data2);
		if (ret < 0)
			msleep(5);
	} while (ret < 0);

	return 0;
}

static int wm8960i_dai_hw_params(struct snd_soc_dai *dai, unsigned int rate,
			      snd_pcm_format_t fmt, unsigned int channels,
			      uint8_t align)
{
	uint8_t res = 0;
	int fd = 0;
	struct dai_device *dev = dai->priv;

	fd = open("dev/i2c3",O_RDWR);
	if(fd < 0){
		printf("i2c open error\n");
		return -1;
	}
	dev->fd = fd;
	//Reset Device
	res = WM8960_Write_Reg(fd, 0x0F, 0xFFFF);
	if(res != 0)
		return res;

	//Set Power Source
#if USE_BOARD_MIC
	res =  WM8960_Write_Reg(fd, 0x19, 0x00E8);
#elif USE_EARPHONE_MIC
	res =  WM8960_Write_Reg(fd, 0x19, 0x00D4);
#endif
	res += WM8960_Write_Reg(fd, 0x1A, 0x01F8);
	res += WM8960_Write_Reg(fd, 0x2F, 0x003C);

	if(res != 0)	{
		printf("Source set fail !!\r\n");
		printf("Error code: %d\r\n",res);
		return res;
	}

	//Configure clock
	//MCLK->div1->SYSCLK->DAC/ADC sample Freq = 25MHz(MCLK)/2*256 = 48.8kHz
	WM8960_Write_Reg(fd, 0x04, 0x0000);

	/*********Audio Interface*********/

	//I2S format 16 bits word length
	WM8960_Write_Reg(fd, 0x07, 0x0082);

	/*********PGA*********/

	//Input PGA
	WM8960_Write_Reg(fd, 0x00, 0x003F | 0x0100);//0x003F
	WM8960_Write_Reg(fd, 0x01, 0x003F | 0x0100);//0x003F

	//Input Signal Path
#if USE_BOARD_MIC
	WM8960_Write_Reg(fd, 0x20, 0x0008 | 0x0100);
	WM8960_Write_Reg(fd, 0x21, 0x0000);
#elif USE_EARPHONE_MIC
	WM8960_Write_Reg(fd, 0x20, 0x0000);
	WM8960_Write_Reg(fd, 0x21, 0x0008 | 0x0100);
#endif

	//Input Boost Mixer
	WM8960_Write_Reg(fd, 0x2B, 0x0000);
	WM8960_Write_Reg(fd, 0x2C, 0x0000);

	/*********ADC*********/

	//ADC Control //ADC High Pass Filter
	WM8960_Write_Reg(fd, 0x05, 0x000C);

	//ADC Digital Volume Control
	WM8960_Write_Reg(fd, 0x15, 0x00C3 | 0x0100);
	WM8960_Write_Reg(fd, 0x16, 0x00C3 | 0x0100);

#if USE_BOARD_MIC
	WM8960_Write_Reg(fd, 0x17, 0x01C4);
#elif USE_EARPHONE_MIC
	WM8960_Write_Reg(fd, 0x17, 0x01C8);
#endif

	/*********ALC Control*********/
	//Noise Gate Control
	WM8960_Write_Reg(fd, 0x14, 0x00F9);

	/*********OUTPUT SIGNAL PATH*********/

	//Digital Volume Control
	WM8960_Write_Reg(fd, 0x0A, 0x00FF | 0x0100);
	WM8960_Write_Reg(fd, 0x0B, 0x00FF | 0x0100);

	//DAC Soft-Mute Control
	WM8960_Write_Reg(fd, 0x05, 0x0004);
	WM8960_Write_Reg(fd, 0x06, 0x0000);

	//3D Stereo Enhancement Function
	WM8960_Write_Reg(fd, 0x10, 0x0000); //No 3D effect

	//Left and Right Output Mixer Mute and Volume Control
	//	WM8960_Write_Reg(0x22, 0x0180); //Left DAC and LINPUT3 to Left Output Mixer
	//	WM8960_Write_Reg(0x25, 0x0180); //Right DAC and RINPUT3 to Right Output Mixer
	//	WM8960_Write_Reg(0x2D, 0x0080); //Left Input Boost Mixer to Left Output Mixer
	//	WM8960_Write_Reg(0x2E, 0x0080); //Right Input Boost Mixer to Right Output Mixer

	/*********ANALOGUE OUTPUTS*********/

	//LOUT1/ROUT1 Volume Control
	//	WM8960_Write_Reg(0x02, 0x007F | 0x0100);  //Left Volume
	//	WM8960_Write_Reg(0x03, 0x007F | 0x0100);  //Right Volume

	/*********ENABLING THE OUTPUTS*********/

	//Analogue Output Control
	WM8960_Write_Reg(fd, 0x31, 0x00F7); //Enable Left and right speakers
	msleep(1000);
	printf("wm8960-for-i2si-dai wm8960i_dai_hw_params end\n");
	return 0;
}

static int wm8960i_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int wm8960i_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int wm8960i_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int wm8960i_dai_hw_free(struct snd_soc_dai *dai)
{
	struct dai_device *dev = dai->priv;

	close(dev->fd);

	return 0;
}

static struct snd_soc_dai_driver wm8960i_dai_driver = {
	.hw_params = wm8960i_dai_hw_params,
	.trigger = wm8960i_dai_trigger,
	.ioctl = wm8960i_dai_ioctl,
	.get_capability = wm8960i_dai_get_capability,
	.hw_free = wm8960i_dai_hw_free,
};

static struct snd_soc_dai wm8960_for_i2si0_dai = {
	.name = "wm8960-for-i2si0-dai",
	.driver = &wm8960i_dai_driver,
	.priv = &dai_dev,
};

static struct snd_soc_dai wm8960_for_i2si1_dai = {
	.name = "wm8960-for-i2si1-dai",
	.driver = &wm8960i_dai_driver,
	.priv = &dai_dev,
};

static struct snd_soc_dai wm8960_for_i2si2_dai = {
	.name = "wm8960-for-i2si2-dai",
	.driver = &wm8960i_dai_driver,
	.priv = &dai_dev,
};

int wm8960_for_i2si_dai_init(void)
{
	int np;
	const char *status;

	np = fdt_node_probe_by_path("/hcrtos/wm8960");
	if (np < 0) {
		return 0;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
	!strcmp(status, "disabled")) {
		return 0;
	}

	snd_soc_register_dai(&wm8960_for_i2si0_dai);

	snd_soc_register_dai(&wm8960_for_i2si1_dai);

	snd_soc_register_dai(&wm8960_for_i2si2_dai);
}

module_driver(wm8960_for_i2si_dai, wm8960_for_i2si_dai_init, NULL, 0)
