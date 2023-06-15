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

#include <kernel/elog.h>

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

    printf("usb device mode : zero\n");
}


int setup_usbd_zero(int argc, char **argv)
{
    char ch;
    int i;
    const char *udc_name = NULL;
    int usb_port = 0;

    opterr = 0;
    optind = 0;

    elog_set_filter_tag_lvl("usbd-ss", ELOG_LVL_ALL);
    elog_set_filter_tag_lvl("usbd-lb", ELOG_LVL_ALL);
    elog_set_filter_tag_lvl("usbd-zero", ELOG_LVL_ALL);
    elog_set_filter_tag_lvl("usbd-core", ELOG_LVL_ALL);
    elog_set_filter_tag_lvl("usbd", ELOG_LVL_ALL);

    while ((ch = getopt(argc, argv, "hHsSp:P:")) != EOF) {
        switch(ch) {
            case 'h' :
            case 'H' :
                usbd_msg_help_info();
                return 0;
            case 'p' :
            case 'P' :
                usb_port = atoi(optarg);
                udc_name = get_udc_name(usb_port);
                if(udc_name == NULL){
                    printf("[error] parameter(-p {usb_port}) error,"
                        "please check for help information(cmd: g_mass_storage -h)\n");
                    return -1;
                }
                printf("==> set usb#%u as zero demo gadget\n", usb_port);
                break;
            case 's' :
            case 'S' :
                hcusb_gadget_zero_deinit();
                return 0;
            default:
                break;
        }
    }

    hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
    if(!udc_name)
        hcusb_gadget_zero_init();
    else 
        hcusb_gadget_zero_specified_init(udc_name);
    return 0;
}
