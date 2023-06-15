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

static void usbd_ncm_help_info(void)
{
    printf("\nThis driver implements USB CDC NCM subclass standard. NCM is\n");
    printf("an advanced protocol for Ethernet encapsulation, allows grouping\n");
    printf("of several ethernet frames into one USB transfer and differentn\n");
    printf("alignment possibilities\n");
    printf("<1> print help information :\n");
    printf("\tg_ncm -h\n");
    printf("<2> shut down usb NCM device :\n");
    printf("\tg_ncm -s\n");
    printf("\tg_ncm -s -p 1\n");
    printf("\tg_ncm -s -p 0\n");
    printf("<3> -p == Specify the usb port(usb#0 or usb#1), default usb#0 :\n");
    printf("<4> setup NCM functions:\n");
    printf("    examples:\n");
    printf("\tg_ncm  ## using usb#0\n");
    printf("\tg_ncm -p 0   ## using usb#0\n");
    printf("\tg_ncm -p 1   ## using usb#1\n");
}


int setup_usbd_ncm(int argc, char **argv)
{
    char ch;
    int i;
    const char *udc_name = NULL;
    int usb_port = 0;

    opterr = 0;
    optind = 0;

    while ((ch = getopt(argc, argv, "hHsSp:P:")) != EOF) {
        switch(ch) {
            case 'h' :
            case 'H' :
                usbd_ncm_help_info();
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
                printf("==> set usb#%u as NCM gadget\n", usb_port);
                break;
            case 's' :
            case 'S' :
                hcusb_set_mode(usb_port, MUSB_HOST);
                hcusb_gadget_ncm_deinit();
                return 0;
            default:
                break;
        }
    }

    hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
    if(!udc_name)
        hcusb_gadget_ncm_init();
    else 
        hcusb_gadget_ncm_specified_init(udc_name);
    return 0;
}
