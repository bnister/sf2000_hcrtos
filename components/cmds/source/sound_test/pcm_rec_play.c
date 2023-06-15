#include <kernel/delay.h>
#include "sound_test.h"
#include <hcuapi/pinmux.h>
#include <nuttx/i2c/i2c_master.h>

#define USE_BOARD_MIC 1
#define USE_EARPHONE_MIC 0
#define BOARD_C3100_V20 1

static char *ch0 = NULL;
static int pcmi_source = SND_PCM_SOURCE_AUDPAD;
static int stop_pcmi = 0;
int wm8960_pcm_rec_play(void);

static void pcm_rec_play_test_thread(void *arg)
{
	struct snd_pcm_params pcmi_params = {0};
	int channels = 2;

	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	int align = SND_PCM_ALIGN_RIGHT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	int period_size = 1536;

	int read_size = period_size;
	int ret = 0;
	snd_pcm_uframes_t poll_size = period_size;
	struct snd_pcm_params params = {0};

	int pcmo_fd = 0;
	int pcmi_fd = 0;

	wm8960_pcm_rec_play();

	pcmi_fd = open("/dev/sndC0pcmi", O_WRONLY);
	printf ("pcmi fd %d\n", pcmi_fd);

	pcmi_params.access = access;
	pcmi_params.format = format;
	pcmi_params.sync_mode = AVSYNC_TYPE_FREERUN;
	pcmi_params.align = align;
	pcmi_params.rate = rate;
	pcmi_params.channels = channels;
	pcmi_params.period_size = period_size;
	pcmi_params.periods = periods;
	pcmi_params.bitdepth = bitdepth;
	pcmi_params.start_threshold = 1;
	pcmi_params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	pcmi_params.pcm_dest = SND_PCM_DEST_BYPASS;

	ret = ioctl(pcmi_fd, SND_IOCTL_HW_PARAMS, &pcmi_params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
	}

	pcmo_fd = open("/dev/sndC0pcmo", O_WRONLY);
	printf ("pcmo_fd %d\n", pcmo_fd);

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = align;
	params.rate = rate;

	params.channels = channels;
	params.period_size = read_size;
	params.periods = periods;
	params.bitdepth = bitdepth;
	params.start_threshold = 2;
	ioctl(pcmo_fd, SND_IOCTL_HW_PARAMS, &params);
	printf ("SND_IOCTL_HW_PARAMS done\n");

	ioctl(pcmo_fd, SND_IOCTL_AVAIL_MIN, &poll_size);

	ioctl(pcmi_fd, SND_IOCTL_START, 0);
	printf ("pcmi_fd SND_IOCTL_START done\n");

	ioctl(pcmo_fd, SND_IOCTL_START, 0);
	printf ("pcmo_fd SND_IOCTL_START done\n");

	while(!stop_pcmi) {
		msleep(1000);
	}
	printf ("SND_IOCTL_HW_FREE \n");
	ioctl(pcmo_fd, SND_IOCTL_DROP, 0);
	ioctl(pcmo_fd, SND_IOCTL_HW_FREE, 0);
	close(pcmo_fd);

	ioctl(pcmi_fd, SND_IOCTL_DROP, 0);
	ioctl(pcmi_fd, SND_IOCTL_HW_FREE, 0);
	close(pcmi_fd);

	vTaskDelete(NULL);
}

int pcm_rec_play_test(int argc, char *argv[])
{
    int ret;

	if (argc > 1) {
		pcmi_source = atoi(argv[1]);
	} else {
		pcmi_source = SND_PCM_SOURCE_AUDPAD;
	}

	ret = xTaskCreate(pcm_rec_play_test_thread, "pcm_rec_play_test_thread",
		0x2000, NULL, portPRI_TASK_HIGH, NULL);
    if(ret != pdTRUE) {
        return -1;
    }

	return 0;
}

int stop_pcm_rec_play_test(int argc, char *argv[])
{
	stop_pcmi = 1;
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

	//printf("reg_addr 0x%x, pdata 0x%x\n", reg_addr, pdata);

	return 0;
}

int wm8960_pcm_rec_play()
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
	res =  WM8960_Write_Reg(fd, 0x19, 0x01E8);
#elif USE_EARPHONE_MIC
	res =  WM8960_Write_Reg(fd, 0x19, 0x00D4);
#endif
	res += WM8960_Write_Reg(fd, 0x1A, 0x01Ff);
	//res += WM8960_Write_Reg(fd, 0x1A, (uint16_t)0x01ff);//(1 << 8) | (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3));
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
	WM8960_Write_Reg(fd, 0x07, 0x0083);

	//Configure ADC/DAC
	WM8960_Write_Reg(fd, 0x05, (uint16_t)0x0002);
	//I2S format 32 bits word length
	//WM8960_Write_Reg(fd, 0x07, 0x008E);

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
	WM8960_Write_Reg(fd, 0x02, (uint16_t)0x016F);//0x006F | 0x0100);  //LOUT1 Volume Set
	WM8960_Write_Reg(fd, 0x03, (uint16_t)0x016F);//0x006F | 0x0100);  //ROUT1 Volume Set

	//Configure SPK_RP and SPK_RN
	WM8960_Write_Reg(fd, 0x28, (uint16_t)0x017F);//0x007F | 0x0100); //Left Speaker Volume
	WM8960_Write_Reg(fd, 0x29, (uint16_t)0x017F);//0x007F | 0x0100); //Right Speaker Volume
	//Input Boost Mixer
	WM8960_Write_Reg(fd, 0x2B, 0x0000);
	WM8960_Write_Reg(fd, 0x2C, 0x0000);
	WM8960_Write_Reg(fd, 0x31, (uint16_t)0x00F7); //Enable Class D Speaker Outputs
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
	WM8960_Write_Reg(fd, 0x1a, (uint16_t)0x01FF);//self
	WM8960_Write_Reg(fd, 0x22, (uint16_t)0x0180);//1<<8 | 1<<7);
	WM8960_Write_Reg(fd, 0x25, (uint16_t)0x0180);//1<<8 | 1<<7);
	WM8960_Write_Reg(fd, 0x18, (uint16_t)0x0040);//1<<6 | 0<<5);
	WM8960_Write_Reg(fd, 0x17, (uint16_t)0x01C3);
	WM8960_Write_Reg(fd, 0x30, (uint16_t)0x0009);//0x000D,0x0005
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

