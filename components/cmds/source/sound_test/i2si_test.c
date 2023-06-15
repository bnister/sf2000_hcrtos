#include <kernel/delay.h>
#include "sound_test.h"
#include <hcuapi/pinmux.h>
#include <nuttx/i2c/i2c_master.h>

#define USE_BOARD_MIC 1
#define USE_EARPHONE_MIC 0
#define BOARD_C3100_V20 1

static char *ch0 = NULL;
#if BOARD_C3100_V20
static char *ch1 = NULL;
#endif
static int i2si_source = SND_PCM_SOURCE_AUDPAD;
static int stop_i2si = 0;

static void i2si_test_thread(void *arg)
{
	struct snd_pcm_params params;
	int i,j = 0;
	int channels = 2;

	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	//int bitdepth = 32;
	//snd_pcm_format_t format = SND_PCM_FORMAT_S24_LE;
	int align = SND_PCM_ALIGN_RIGHT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	int period_size = 1536;

	int total_read_times = 600;
	int read_size = period_size;
	int record_size = period_size * total_read_times;

	int read_times = 0;
	char *buf = NULL;
	int ret = 0;
	snd_pcm_uframes_t poll_size = period_size;
	if (!BOARD_C3100_V20) {
		pinmux_configure(PINPAD_R08, 0xa); //i2si   A3200   fs
		pinmux_configure(PINPAD_R09, 0xa); //datai
		pinmux_configure(PINPAD_R10, 0xa); //mclk
		pinmux_configure(PINPAD_R01, 0xa); //bck
	}

	struct pollfd pfd;
	int snd_in_fd = 0;
	struct snd_xfer xfer = {0};

	buf = malloc (read_size);
	memset(buf, 0, read_size);

	if(!ch0)
		ch0 = malloc (record_size);
	memset(ch0, 0, record_size);
	printf ("ch0 %p\n", ch0);

	if (BOARD_C3100_V20) {
		if(!ch1)
			ch1 = malloc (record_size  / 2);
		memset(ch1, 0, record_size / 2);
		printf ("ch1 %p\n", ch1);
	}

	snd_in_fd = open("/dev/sndC0i2si", O_WRONLY);
	printf ("i2si fd %d\n", snd_in_fd);

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = snd_in_fd;
	pfd.events = POLLIN | POLLRDNORM;

	params.access = access;
	params.format = format;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = align;
	params.rate = rate;
	params.channels = channels;
	params.period_size = period_size;
	params.periods = periods;
	params.bitdepth = bitdepth;
	params.start_threshold = 1;
	params.pcm_source = i2si_source;

	ret = ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
	}
	//static sysio_reg_t *SYSIO = &MSYSIO0;
	//SYSIO->strap_pin_ctrl_register1.ejtag_unvalid_sel = 1;
	printf ("start record %d\n", ret);
	ret = ioctl(snd_in_fd, SND_IOCTL_START, 0);

	xfer.data = buf;
	xfer.frames = period_size / (channels * params.bitdepth / 8);
	do {
		ret = ioctl(snd_in_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			poll(&pfd, 1, 100);
			continue;
		}
		memcpy(ch0 + period_size * read_times, xfer.data, period_size);
		read_times++;
	} while (read_times < total_read_times);
	printf ("record1 done, dump data to check\n");
	if (BOARD_C3100_V20) {
		/*cjc8990 only has one channel ,i2si used interleaved access to recoder data */
		/*should abandon another channel data*/
		for (i = 0;i < (record_size / 2);i++) {
			if(i % 2 == 0) {
				ch1[i] = ch0[j];
				j = j + 1;
			} else {
				ch1[i] = ch0[j];
				j = j + 3;
			}
		}
	}
	printf ("deal with data done\n");
	//asm volatile ("nop;.word 0x1000ffff;nop;");

	while(!stop_i2si) {
		usleep(1*1000*1000);
	}
	printf ("SND_IOCTL_HW_FREE \n");
	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
	close(snd_in_fd);

	if (ch0) {
		free(ch0);
		ch0 = NULL;
	}
	if (ch1) {
		free(ch1);
		ch1 = NULL;
	}

	free(buf);

	vTaskDelete(NULL);
}

int i2si_test(int argc, char *argv[])
{
    int ret;

	if (argc > 1) {
		i2si_source = atoi(argv[1]);
	} else {
		i2si_source = SND_PCM_SOURCE_AUDPAD;
	}

	ret = xTaskCreate(i2si_test_thread, "i2si_test_thread",
		0x2000, NULL, portPRI_TASK_HIGH, NULL);
    if(ret != pdTRUE) {
        return -1;
    }

	return 0;
}

int stop_i2si_test(int argc, char *argv[])
{
	stop_i2si = 1;
	return 0;
}

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

	printf("reg_addr 0x%x, pdata 0x%x\n", reg_addr, pdata);

	return 0;
}

int wm8960_test(int argc, char *argv[])
{
	uint8_t res = 0;
	int fd = 0;
	//struct dai_device *dev = dai->priv;
	printf("i2si wm8960 test start\n");
	fd = open("dev/i2c3",O_RDWR);
	if(fd < 0){
		printf("i2c open error\n");
		return -1;
	}
	//dev->fd = fd;
	//Reset Device
	res = WM8960_Write_Reg(fd, 0x0F, 0xFFFF);
	if(res != 0)
	return res;
	else
	printf("WM8960 reset completed !!\r\n");

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
	//WM8960_Write_Reg(fd, 0x07, 0x0082);

	//I2S format 32 bits word length
	WM8960_Write_Reg(fd, 0x07, 0x008E);

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
	printf("i2si wm8960 test end\n");
	return 0;

}

