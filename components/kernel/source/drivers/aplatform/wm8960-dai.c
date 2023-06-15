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
	int fd;
};

/*
 * Some devices do not support rates lower than 44100, remove those
 * low rates from the capability.
 */
#define SND_PCM_RATE_32K48K                                                    \
	(/*SND_PCM_RATE_8000 | SND_PCM_RATE_12000 | SND_PCM_RATE_16000 |         \
	 SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | */SND_PCM_RATE_48000 /*|        \
	 SND_PCM_RATE_64000 | SND_PCM_RATE_96000 | SND_PCM_RATE_128000 |       \
	 SND_PCM_RATE_192000*/)

#define SND_PCM_RATE_44_1K                                                     \
	(/*SND_PCM_RATE_5512 | SND_PCM_RATE_11025 | SND_PCM_RATE_22050 | \
	*/SND_PCM_RATE_44100 /*| SND_PCM_RATE_88200 | SND_PCM_RATE_176400*/)

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
	uint8_t slave_addr = 0x34 >> 1;
	uint8_t data1;
	uint8_t data2;

	data1 = (reg_addr << 1) | ((uint8_t)((pdata >> 8) & 0x0001));
	data2 = (uint8_t)(pdata & 0x00ff);

	do {
		ret =  es_write_reg(fd, slave_addr, data1, data2);
		if (ret < 0)
			msleep(5);
	} while (ret < 0);

	printf("reg_addr 0x%x, pdata 0x%x\n", reg_addr, pdata);

	return 0;
}

static int wm8960_dai_hw_params(struct snd_soc_dai *dai, unsigned int rate,
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
	res = WM8960_Write_Reg(fd, 0x0f, (uint16_t)0x0000);
	printf("WM8960 reset completed !!\r\n");

	msleep(1);
	//Set Power Source
	res =  WM8960_Write_Reg(fd, 0x19, (uint16_t)0x01c0);// (1 << 8) | (1 << 7) | (1 << 6));
	res += WM8960_Write_Reg(fd, 0x1A, (uint16_t)0x01ff);//(1 << 8) | (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3));
	res += WM8960_Write_Reg(fd, 0x2F, (uint16_t)0x000c);//(1 << 3) | (1 << 2));
	if(res != 0)  {
		printf("Source set fail !!\r\n");
		printf("Error code: %d\r\n",res);
	//return res;
	}

	//Configure clock
	//MCLK->div1->SYSCLK->DAC/ADC sample Freq = 25MHz(MCLK)/2*256 = 48.8kHz
	WM8960_Write_Reg(fd, 0x04, (uint16_t)0x0000);

	//Configure ADC/DAC
	WM8960_Write_Reg(fd, 0x05, (uint16_t)0x0002);

	//Configure audio interface
	//I2S format 24 bits word length
	//WM8960_Write_Reg(0x07, (uint16_t)0x000a);
	//DSP format 24 bits word length
	WM8960_Write_Reg(fd, 0x07, (uint16_t)0x008f);

	//Configure HP_L and HP_R OUTPUTS
	WM8960_Write_Reg(fd, 0x02, (uint16_t)0x016F);//0x006F | 0x0100);  //LOUT1 Volume Set
	WM8960_Write_Reg(fd, 0x03, (uint16_t)0x016F);//0x006F | 0x0100);  //ROUT1 Volume Set

	//Configure SPK_RP and SPK_RN
	WM8960_Write_Reg(fd, 0x28, (uint16_t)0x017F);//0x007F | 0x0100); //Left Speaker Volume
	WM8960_Write_Reg(fd, 0x29, (uint16_t)0x017F);//0x007F | 0x0100); //Right Speaker Volume

	//Enable the OUTPUTS
	WM8960_Write_Reg(fd, 0x31, (uint16_t)0x00F7); //Enable Class D Speaker Outputs

	//Configure DAC volume
	msleep(100);
	WM8960_Write_Reg(fd, 0x0a, (uint16_t)0x01FF);//0x00FF | 0x0100);
	WM8960_Write_Reg(fd, 0x0b, (uint16_t)0x01FF);//0x00FF | 0x0100);

	WM8960_Write_Reg(fd, 0x1a, (uint16_t)0x01FF);//self

	//3D
	//  WM8960_Write_Reg(0x10, 0x001F);

	//Configure MIXER
	WM8960_Write_Reg(fd, 0x22, (uint16_t)0x0180);//1<<8 | 1<<7);
	WM8960_Write_Reg(fd, 0x25, (uint16_t)0x0180);//1<<8 | 1<<7);

	//Jack Detect
	WM8960_Write_Reg(fd, 0x18, (uint16_t)0x0040);//1<<6 | 0<<5);
	WM8960_Write_Reg(fd, 0x17, (uint16_t)0x01C3);
	WM8960_Write_Reg(fd, 0x30, (uint16_t)0x0009);//0x000D,0x0005

	return 0;
}

static int wm8960_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int wm8960_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int wm8960_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int wm8960_dai_hw_free(struct snd_soc_dai *dai)
{
	printf("wm8960_dai_hw_free``\n");
	struct dai_device *dev = dai->priv;

	close(dev->fd);
	printf("wm8960_dai_hw_free out``\n");

	return 0;
}

static struct snd_soc_dai_driver wm8960_dai_driver = {
	.hw_params = wm8960_dai_hw_params,
	.trigger = wm8960_dai_trigger,
	.ioctl = wm8960_dai_ioctl,
	.get_capability = wm8960_dai_get_capability,
	.hw_free = wm8960_dai_hw_free,
};

static struct snd_soc_dai wm8960_dai = {
	.name = "wm8960-dai",
	.driver = &wm8960_dai_driver,
	.priv = &dai_dev,
};

int wm8960_dai_init(void)
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

	return snd_soc_register_dai(&wm8960_dai);
}

