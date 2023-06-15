#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/delay.h>
#include <kernel/lib/console.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>

#include <hcuapi/iocbase.h>

// #include "pok.h"
//幻数
//定义命令
#define POK_READ _IOR(POK_IOCBASE, 2, int)

#define read_buf_len    128

static const char *device = "/dev/pok";

int pok_test(int argc, char * argv[])
{
    int ret = 0;
    int fd;
    // char read_buf[read_buf_len];
    int pok_status;

    // printf("In pok test\n");

    // ret = parse_opt(argc,argv);
	// if (ret)
	// 	return -1;

	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device");
		return -1;
	}

    while (1)
    {
        // ret = read(fd,read_buf,read_buf_len);
        // if (ret != 0)
        // {
        //     printf("can't read pok status\n");
        // }
        // else{
        //     printf("in app pok status is %c\n",read_buf[0]);
        // }
        
        ret = ioctl(fd,POK_READ,(uint32_t)&pok_status);
        if (!ret)
        {
            printf("in app pok status is %d\n",pok_status);
        }
        else{
            printf("can't get pok status\n");
        }        

		msleep(100);
    }

    close(fd);

    return ret;
}

CONSOLE_CMD(pok_test,NULL,pok_test,CONSOLE_CMD_MODE_SELF,"test pok function app")
