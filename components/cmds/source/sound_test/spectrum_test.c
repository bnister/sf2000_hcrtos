#include <kernel/delay.h>
#include "sound_test.h"
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <hcuapi/dumpstack.h>
#include <hcuapi/kshm.h>
#include <hcuapi/common.h>
#include <nuttx/i2c/i2c_master.h>

static int wm8960_i2s_spectrum_rec_play(void);
#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
extern int *snd_spectrum_run(int *data, int ch, int bitdepth, int rate, int size, int *collumn_num);
extern void snd_spectrum_stop(void);
#endif
/* below is i2si rec test code */
static int i2si_source = SND_PCM_SOURCE_AUDPAD;
static int i2si_rate = 44100;
static int i2si_format = 16;
static int i2si_channles = 2;
static bool stop_i2si_spectrum = 0;
static bool stop_i2so_spectrum = 0;
static void *buf = NULL;
static pthread_t i2so_spectrum_thread_id = 0;
static pthread_t i2si_spectrum_thread_id = 0;

static void* i2si_spectrum_thread(void *args)
{
	struct snd_pcm_params params;
	int read_size = 1536;
	void *buf = NULL;
	int ret = 0;
	int collumn_num = 9;
	int *spectrum = NULL;
	struct pollfd pfd = {0};
	int snd_in_fd = 0;
	struct snd_xfer xfer = {0};

	wm8960_i2s_spectrum_rec_play();

	snd_in_fd = open("/dev/sndC0i2si", O_WRONLY);
	if (snd_in_fd < 0) {
		printf("open i2si dev failed\n");
		goto err;
	}

	buf = malloc (read_size);
	if (!buf) {
		printf("malloc rec buf failed\n");
		goto err;
	}
	memset(buf, 0, read_size);

	switch(i2si_format) {
		case 16:
			params.format = SND_PCM_FORMAT_S16_LE;
			params.bitdepth = 16;
			break;
		case 24:
			params.format = SND_PCM_FORMAT_S24_LE;
			params.bitdepth = 32;
			break;
		default:
			printf("unsupport i2si_format %d\n", i2si_format);
			goto err;
	}
	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.rate = i2si_rate;
	params.channels = i2si_channles;
	params.period_size = read_size;
	params.periods = 16;
	params.start_threshold = 1;
	params.pcm_source = i2si_source;
	ret = ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret) {
		printf("i2si hw config failed, ret %d\n", ret);
		goto err;
	}

	ret = ioctl(snd_in_fd, SND_IOCTL_START, 0);
	if (ret) {
		printf("i2si start failed, ret %d\n", ret);
		ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
		goto err;
	}

	xfer.data = buf;
	xfer.frames = read_size / (params.channels * params.bitdepth / 8);
	do {
		ret = ioctl(snd_in_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			pfd.fd = snd_in_fd;
			pfd.events = POLLIN | POLLRDNORM;
			poll(&pfd, 1, 100);
			continue;
		}
#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
		spectrum = snd_spectrum_run(buf, i2si_channles, i2si_format, i2si_rate, read_size, &collumn_num);
		if (spectrum && collumn_num > 0) {
			printf("spectrum: ");
			for (int i = 0; i < collumn_num; i++) {
				printf(" %d", spectrum[i]);
			}
			printf("\n");
		}
#endif
	} while (!stop_i2si_spectrum);
	printf ("i2si spectrum exit\n");

	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
	snd_spectrum_stop();
#endif

err:
	if (snd_in_fd)
		close(snd_in_fd);
	if (buf)
		free(buf);

	return NULL;
}

static void* i2so_spectrum_thread(void *args)
{
	int snd_in_fd;
	uint32_t buf_size;
	int recoder_size;
	int ds = 0;
	int ret = 0;
	int recoder_time = 0;
	int ch = 2;
	int collumn_num = 9;
	int *spectrum =NULL;

	struct kshm_info audio_read_hdl = {0};
	uint32_t aread_size = 0;

	snd_in_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(snd_in_fd < 0) {
		printf("error: can not open i2so dev\n");
		return NULL;
	}

	buf_size = 300 * 1024;
	recoder_size = 0;

	ret = ioctl(snd_in_fd, SND_IOCTL_SET_RECORD, buf_size);
	if (ret < 0) {
		printf("error: can not set i2so recode\n");
	}

	ret = ioctl(snd_in_fd, KSHM_HDL_ACCESS, &audio_read_hdl);
	if (ret < 0) {
		printf("error: can not set kshm hdl acess\n");
	}

	printf("start spectrum test\n");

	while (!stop_i2so_spectrum) {
		AvPktHd hdr = {0};
		while (kshm_read((void *)&audio_read_hdl, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd)) {
			msleep(5);
		}

		if (recoder_size < hdr.size) {
			recoder_size = hdr.size;
			if(buf) {
				buf = realloc(buf, recoder_size);
			} else {
				buf = malloc(recoder_size);
			}
		}
		while (kshm_read((void *)&audio_read_hdl, buf, hdr.size) != hdr.size) {
			msleep(20);
		}
#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
		spectrum = snd_spectrum_run(buf, ch, 24, 48000, hdr.size, &collumn_num);
		if (spectrum && collumn_num > 0) {
			printf("spectrum: ");
			for (int i = 0; i < collumn_num; i++) {
				printf(" %d", spectrum[i]);
			}
			printf("\n");
		}
#endif
	}

	printf("i2so spectrum exit !\n");

	ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
	snd_spectrum_stop();
	close(snd_in_fd);
 	if (buf) {
		free(buf);
		buf = NULL;
	}
	return NULL;

}

int stop_i2si_spectrum_test(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	stop_i2si_spectrum = 1;
	if (i2si_spectrum_thread_id)
		pthread_join(i2si_spectrum_thread_id, NULL);
	i2si_spectrum_thread_id = 0;

	return 0;
}

int stop_i2so_spectrum_test(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	stop_i2so_spectrum = 1;
	if (i2so_spectrum_thread_id)
		pthread_join(i2so_spectrum_thread_id, NULL);
	i2so_spectrum_thread_id = 0;

	return 0;
}

int i2si_spectrum_test(int argc, char *argv[])
{
	if (pthread_create(&i2si_spectrum_thread_id, NULL,
		i2si_spectrum_thread, NULL)) {
		printf("create i2si spectrum thread failed\n");
	}
	(void)argc;
	(void)argv;
	return 0;
}

int i2so_spectrum_test(int argc, char *argv[])
{
	if (pthread_create(&i2so_spectrum_thread_id, NULL,
		i2so_spectrum_thread, NULL)) {
		printf("create i2so spectrum thread failed\n");
	}
	(void)argc;
	(void)argv;
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

static int wm8960_i2s_spectrum_rec_play()
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
	WM8960_Write_Reg(fd, 0x07, 0x0082);

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

