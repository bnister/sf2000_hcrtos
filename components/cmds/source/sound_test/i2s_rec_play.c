#include <kernel/delay.h>
#include "sound_test.h"
#include <hcuapi/pinmux.h>
#include <nuttx/i2c/i2c_master.h>

#define USE_BOARD_MIC 1
#define USE_EARPHONE_MIC 0
#define BOARD_C3100_V20 1

static char *ch0 = NULL;
static int i2si_source = SND_PCM_SOURCE_AUDPAD;
static int stop_i2si = 0;
int wm8960_i2s_rec_play(void);

static void i2s_rec_play_test_thread(void *arg)
{
	struct snd_pcm_params i2si_params = {0};
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

	int i2so_fd = 0;
	int i2si_fd = 0;

	wm8960_i2s_rec_play();

	i2si_fd = open("/dev/sndC0i2si", O_WRONLY);
	printf ("i2si fd %d\n", i2si_fd);

	i2si_params.access = access;
	i2si_params.format = format;
	i2si_params.sync_mode = AVSYNC_TYPE_FREERUN;
	i2si_params.align = align;
	i2si_params.rate = rate;
	i2si_params.channels = channels;
	i2si_params.period_size = period_size;
	i2si_params.periods = periods;
	i2si_params.bitdepth = bitdepth;
	i2si_params.start_threshold = 1;
	i2si_params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	i2si_params.pcm_dest = SND_PCM_DEST_BYPASS;

	ret = ioctl(i2si_fd, SND_IOCTL_HW_PARAMS, &i2si_params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
	}

	i2so_fd = open("/dev/sndC0i2so", O_WRONLY);
	printf ("i2so_fd %d\n", i2so_fd);

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
	ioctl(i2so_fd, SND_IOCTL_HW_PARAMS, &params);
	printf ("SND_IOCTL_HW_PARAMS done\n");

	ioctl(i2so_fd, SND_IOCTL_AVAIL_MIN, &poll_size);

	ioctl(i2si_fd, SND_IOCTL_START, 0);
	printf ("i2si_fd SND_IOCTL_START done\n");

	ioctl(i2so_fd, SND_IOCTL_START, 0);
	printf ("i2so_fd SND_IOCTL_START done\n");

	while(!stop_i2si) {
		usleep(1*1000*1000);
	}
	printf ("SND_IOCTL_HW_FREE \n");
	ioctl(i2so_fd, SND_IOCTL_DROP, 0);
	ioctl(i2so_fd, SND_IOCTL_HW_FREE, 0);
	close(i2so_fd);

	ioctl(i2si_fd, SND_IOCTL_DROP, 0);
	ioctl(i2si_fd, SND_IOCTL_HW_FREE, 0);
	close(i2si_fd);

	vTaskDelete(NULL);
}

int i2s_rec_play_test(int argc, char *argv[])
{
    int ret;

	if (argc > 1) {
		i2si_source = atoi(argv[1]);
	} else {
		i2si_source = SND_PCM_SOURCE_AUDPAD;
	}

	ret = xTaskCreate(i2s_rec_play_test_thread, "i2s_rec_play_test_thread",
		0x2000, NULL, portPRI_TASK_HIGH, NULL);
    if(ret != pdTRUE) {
        return -1;
    }

	return 0;
}

int stop_i2s_rec_play_test(int argc, char *argv[])
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

	//printf("reg_addr 0x%x, pdata 0x%x\n", reg_addr, pdata);

	return 0;
}

int wm8960_i2s_rec_play()
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

/* below is i2so spectrum test code */
#include <pthread.h>
#include <hcuapi/common.h>
#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
extern int *snd_spectrum_run(int *data, int ch, int bitdepth, int rate, int size, int *collumn_num);
extern void snd_spectrum_stop(void);
#endif
static bool stop_i2so_spectrum = 0;
static pthread_t i2so_spectrum_thread_id = 0;
static void *i2so_spectrum_thread(void *args)
{
	int snd_in_fd = -1;
	int collumn_num = 9;
	int ch = 2;
	struct kshm_info audio_read_hdl = {0};
	struct snd_hw_info hw_info = {0};
	uint32_t rec_buf_size = 0;
	void *tmp_buf = NULL;
	int tmp_buf_size = 0;
	int ret = 0;
	int *spectrum = NULL;

	snd_in_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(snd_in_fd < 0) {
		printf("error: can not open i2so dev \n");
		goto err;
	}

	rec_buf_size = 300 * 1024;
	ret = ioctl(snd_in_fd, SND_IOCTL_SET_RECORD, rec_buf_size);
	printf("ret0 %d\n", ret);
	ret = ioctl(snd_in_fd, SND_IOCTL_GET_HW_INFO, &hw_info);
	printf("ret1 %d\n", ret);
	ret |= ioctl(snd_in_fd, KSHM_HDL_ACCESS, &audio_read_hdl);
	printf("ret2 %d\n", ret);
	if (ret) {
		printf("set i2so rec failed %d\n", ret);
		ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
		goto err;
	}
	printf("i2so rec start\n");

	while (!stop_i2so_spectrum) {
		AvPktHd hdr = {0};
		while (kshm_read((kshm_handle_t)&audio_read_hdl, &hdr, sizeof(AvPktHd))
				!= sizeof(AvPktHd) && !stop_i2so_spectrum) {
			usleep(10*1000);
		}

		if (tmp_buf_size < hdr.size) {
			tmp_buf = realloc(tmp_buf, hdr.size);
			if (!tmp_buf) {
				printf("no memory\n");
				break;
			}
			tmp_buf_size = hdr.size;
		}
		while (kshm_read((kshm_handle_t)&audio_read_hdl, tmp_buf, hdr.size)
				!= hdr.size && !stop_i2so_spectrum) {
			usleep(20*1000);
		}
#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
		spectrum = snd_spectrum_run(tmp_buf, ch,
				(hw_info.pcm_params.bitdepth == 16) ? 16 : 24, hw_info.pcm_params.rate, hdr.size, &collumn_num);
		if (spectrum && collumn_num > 0) {
			printf("spectrum: ");
			for (int i = 0; i < collumn_num; i++) {
				printf(" %d", spectrum[i]);
			}
			printf("\n");
		}
#endif
	}

	ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
#ifdef BR2_PACKAGE_PREBUILTS_SPECTRUM
	snd_spectrum_stop();
#endif

err:
	printf("i2so rec exit\n");
	if (snd_in_fd >= 0) {
		close(snd_in_fd);
	}

	if (tmp_buf) {
		free(tmp_buf);
	}

	(void)args;
	return NULL;
}

static int i2so_spectrum_stop(int argc, char *argv[])
{
	stop_i2so_spectrum = 1;
	if (i2so_spectrum_thread_id)
		pthread_join(i2so_spectrum_thread_id, NULL);
	i2so_spectrum_thread_id = 0;

	(void)argc;
	(void)argv;
	return 0;
}

static int i2so_spectrum_start(int argc, char *argv[])
{
	pthread_attr_t attr;
	if (i2so_spectrum_thread_id) {
		i2so_spectrum_stop(0, 0);
	}

	stop_i2so_spectrum = 0;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x1000);
	if (pthread_create(&i2so_spectrum_thread_id, &attr,
				i2so_spectrum_thread, NULL)) {
		printf("create i2so spectrum thread failed\n");
	}

	(void)argc;
	(void)argv;
	return 0;
}

CONSOLE_CMD(i2so_rec_start, "NULL", i2so_spectrum_start, CONSOLE_CMD_MODE_SELF, "i2so_rec_start")
CONSOLE_CMD(i2so_rec_stop, "NULL", i2so_spectrum_stop, CONSOLE_CMD_MODE_SELF, "i2so_rec_stop")

