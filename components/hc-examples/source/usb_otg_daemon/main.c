#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define INTERVAL_MS	1
#define ADC_PATH "/dev/check_adc0"
#define USB_MODE_PATH  "/sys/devices/platform/soc/18844000.usb/musb-hdrc.0.auto/mode"

#define HOST_MODE "host"
#define GADGET_MODE "peripheral"
#define OTG_MODE "otg"

typedef enum usb_mode {
	USB_MODE_INVAIL = 0,
	USB_MODE_HOST = 1,
	USB_MODE_GADGET = 2,
}usb_mode_t;



static void set_usb_mode(int fd, usb_mode_t usb_mode)
{
	unsigned char buf[32] = {0};

	if(fd <= 0)
		return;

	// printf("==> set usb mode : %u (1:host, 2:gadget)\n", usb_mode);
	
	switch(usb_mode){
		case USB_MODE_HOST:
			memcpy(&buf[0], HOST_MODE, sizeof(HOST_MODE));
			write(fd, &buf[0], sizeof(HOST_MODE));
			printf(" ==> %s %u\n", __FUNCTION__, __LINE__);
			break;
		case USB_MODE_GADGET:
			memcpy(&buf[0], GADGET_MODE, sizeof(GADGET_MODE));
			write(fd, &buf[0], sizeof(GADGET_MODE));
			break;
		default:
			break;
	}
}


static usb_mode_t get_usb_otg_line_state(int fd)
{
	unsigned char adc_val;
	if(fd <= 0)
		return USB_MODE_INVAIL;

	read(fd, &adc_val, 1);
	
	// printf("===>(get_usb_otg_line_state) otg value : %u\n", adc_val);

	if(adc_val > 200)
		return USB_MODE_GADGET;
	else if(adc_val > 5)
		return USB_MODE_HOST;
	else
		return USB_MODE_INVAIL;
}


int main(int argc, char *argv[])
{
	int fd_adc, fd_usb_mode;
	usb_mode_t cur_otg, otg;

	fd_adc = open(ADC_PATH, O_RDONLY);
	if(fd_adc < 0){
		printf("Error: cannot open %s\n", ADC_PATH);
		return -1;
	}

	fd_usb_mode = open(USB_MODE_PATH, O_RDWR);
	if(fd_usb_mode < 0){
		printf("Error: cannot open %s\n", USB_MODE_PATH);
		return -1;
	}

	printf("USB-OTG-Daemon: daemon process for USB OTG switch\n");
	fflush(stdout);	
	otg = get_usb_otg_line_state(fd_adc);
	cur_otg = otg;

	set_usb_mode(fd_usb_mode, otg);

	while(1){
		sleep(INTERVAL_MS);

		otg = get_usb_otg_line_state(fd_adc);
		if(otg == cur_otg)
			continue;

		set_usb_mode(fd_usb_mode, otg);
		cur_otg = otg;
	}

	close(fd_adc);
	close(fd_usb_mode);

	return 0;
}
