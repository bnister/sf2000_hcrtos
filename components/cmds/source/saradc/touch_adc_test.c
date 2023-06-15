#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input.h>
#include <kernel/lib/console.h>

/* I'm not sure it's event0 */
static const char *device = "/dev/input/event0";


#define SET_CLK_200_HZ _IOW(SARADC_IOCBASE, 1, int)
#define SET_CLK_250_HZ _IOW(SARADC_IOCBASE, 2, int)
#define SET_CLK_300_HZ _IOW(SARADC_IOCBASE, 3, int)
#define SET_CLK_350_HZ _IOW(SARADC_IOCBASE, 4, int)
#define SET_CLK_400_HZ _IOW(SARADC_IOCBASE, 5, int)

#define GET_CURRENT_CLK_HZ _IOR(SARADC_IOCBASE, 0, int)

int touch_adc_test(int argc, char * argv[])
{
    int ret = 0;
    int fd;
    int x = 0, y = 0;
	struct pollfd pfd;
	struct input_event t;

    int i = 0;

    int current_hz = 0;

    printf("In touch adc test\n");

    // ret = parse_opt(argc,argv);
	// if (ret)
	// 	return -1;

	fd = open(device, O_RDONLY);
	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

	if (fd < 0) {
		printf("can't open device");
		return -1;
	}
    
    ret = ioctl(fd,GET_CURRENT_CLK_HZ,(uint32_t)&current_hz);
    printf("current is %d hz\n",current_hz);

    while (1)
    {
        i++;

        if (poll(&pfd, 1, -1) <= 0)
			continue;

		if (read(fd, &t, sizeof(t)) != sizeof(t))
			continue;

		// printf("type:%d, code:%d, value:%ld\n", t.type, t.code, t.value);
		if (t.type == EV_ABS&& t.code == ABS_X) {
                x = t.value;
			}
        if (t.type == EV_ABS && t.code == ABS_Y) {
                y = t.value;
			}
        if (t.type == EV_SYN)
        {
            printf("(%4d %4d) in app \n",x,y);
        }

        if(i == 5000) {
            ret = ioctl(fd,SET_CLK_200_HZ,0);
            ret = ioctl(fd,GET_CURRENT_CLK_HZ,(uint32_t)&current_hz);
            printf("current is %d hz\n",current_hz);
        }
        if(i == 15000) {
            ret = ioctl(fd,SET_CLK_250_HZ,0);
            ret = ioctl(fd,GET_CURRENT_CLK_HZ,(uint32_t)&current_hz);
            printf("current is %d hz\n",current_hz);
        }
        if(i == 20000) {
            ret = ioctl(fd,SET_CLK_300_HZ,0);
            ret = ioctl(fd,GET_CURRENT_CLK_HZ,(uint32_t)&current_hz);
            printf("current is %d hz\n",current_hz);
        }
        if(i == 25000) {
            ret = ioctl(fd,SET_CLK_350_HZ,0);
            ret = ioctl(fd,GET_CURRENT_CLK_HZ,(uint32_t)&current_hz);
            printf("current is %d hz\n",current_hz);
        }
        if(i == 30000) {
            ret = ioctl(fd,SET_CLK_400_HZ,0);
            ret = ioctl(fd,GET_CURRENT_CLK_HZ,(uint32_t)&current_hz);
            printf("current is %d hz\n",current_hz);
            i = 0;
        }

    }
    close(fd);

    return ret;
}

CONSOLE_CMD(touch_adc_test,"adc_test",touch_adc_test,CONSOLE_CMD_MODE_SELF,"test touch-adc function app")