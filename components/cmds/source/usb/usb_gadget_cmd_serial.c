#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <kernel/lib/console.h>
#include <linux/bitops.h>
#include <linux/bits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nuttx/fs/dirent.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>
#include <string.h>

#include <kernel/drivers/hcusb.h>

static void usbd_msg_help_info(void)
{
    // printf("\nMass Storage Gadget (or MSG) acts as a USB Mass Storage device\n");
    // printf("It supports multiple logical units (LUNs).\n");
    // printf("usage:\n");
    // printf("<1> print help information :\n");
    // printf("\tg_mass_storage -h\n");
    // printf("<2> shut down usb mass-storage device :\n");
    // printf("\tg_mass_storage -s\n");
    // printf("<3> setup block file or character file as logic unit:\n");
    // printf("\tg_mass_storage <path>\n");
    // printf("    examples:\n");
    // printf("\tg_mass_storage /dev/mmcblk0\n");
    // printf("\tg_mass_storage /dev/ram0\n");
    // printf("\tg_mass_storage /dev/ram0 /dev/mmcblk0\n");

    printf("usb device mode : serial\n");
}


int setup_usbd_serial(int argc, char **argv)
{
    char ch;
    int i;

    opterr = 0;
    optind = 0;

    while ((ch = getopt(argc, argv, "hHsS")) != EOF) {
        switch(ch) {
            case 'h' :
            case 'H' :
                usbd_msg_help_info();
                return 0;
            case 's' :
            case 'S' :
                hcusb_gadget_serial_deinit();
                return 0;
            default:
                break;
        }
    }

    // if(argc < 2){
    //     printf("[error] parameter error, please check for help information(cmd: g_mass_storage -h)\n");
    //     return -1;
    // }

    hcusb_set_mode(0, MUSB_PERIPHERAL);  // using usb0 as usb gadget
    hcusb_gadget_serial_init();
    return 0;
}


#if 0
int setup_usbd_serial_testing(int argc, char **argv)
{
    #define BUF_SIZE 128
    int fd;
    int i;
    char buf[BUF_SIZE] = {0};
    int rd_cnt;

    printf("===>(%s) testing start \n", __FUNCTION__);

    fd = open("/dev/ttyGS0", O_RDWR);
    if(fd < 0){
        printf("===>(%s) cannot open /dev/ttyGS0 \n", __FUNCTION__);
        return -1;
    }

    printf("===>(%s) send strings (size:%u) \n", __FUNCTION__, BUF_SIZE);

    for(i = 0; i < BUF_SIZE; i++)
        buf[i] = 'A' + (i % 58);

    write(fd, buf, BUF_SIZE);

    printf("===>(%s) send strings finsish \n", __FUNCTION__);

    printf("===>(%s) waiting to receive strings \n", __FUNCTION__);

    memset(buf, 0, BUF_SIZE);
    rd_cnt = read(fd, buf, BUF_SIZE);
    
    printf("===>(%s) receive strings (size:%u) \n", __FUNCTION__, rd_cnt);

    for(i = 0; i < rd_cnt; i++){
        if(i % 10 == 0)
            printf("%3.3d: ", i);
        printf("%c", buf[i]);
        if(i % 10 == 9)
            printf("\n");
    }
    printf("\n");

    printf("===>(%s) testing start finish \n", __FUNCTION__);
    
    close(fd);
    return 0;
}
#else

int setup_usbd_serial_testing(int argc, char **argv)
{
    #define BUF_SIZE 128
    int fd;
    int i;
    char buf[BUF_SIZE] = {0};
    int rd_cnt;
    // struct pollfd fds;

    printf("===>(%s) testing start \n", __FUNCTION__);
    fd = open("/dev/ttyGS0", O_RDWR);
    if(fd < 0){
        printf("===>(%s) cannot open /dev/ttyGS0 \n", __FUNCTION__);
        return -1;
    }

    // fds.fd = fd;
    // fds.events = POLLIN;

    while(1){
        // if(0 > poll(&fds, 1, -1)){
        //     printf("===>(%s) poll error \n", __FUNCTION__);
        //     goto exit;
        // }

        // memset(buf, 0, BUF_SIZE);
        // if(fds.revents & POLLIN){
            printf("wait ...\n");
            rd_cnt = read(fd, buf, BUF_SIZE);
            printf("revice %d\n", rd_cnt);
            write(fd, buf, rd_cnt);
            printf("send %u\n", rd_cnt);
        // }
    }

    printf("===>(%s) testing start finish \n", __FUNCTION__);

exit:
    close(fd);
    return 0;
}

#error "tony.su ......."

#endif
