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

    printf("usb device mode : hid\n");
}


int setup_usbd_hid(int argc, char **argv)
{
    char ch;
    int i;
    const char *udc_name = NULL;
    int usb_port = 0;

    opterr = 0;
    optind = 0;

    elog_set_filter_tag_lvl("usbd-core", ELOG_LVL_DEBUG);
    elog_set_filter_tag_lvl("usbd-udc", ELOG_LVL_DEBUG);
    elog_set_filter_tag_lvl("usbd", ELOG_LVL_DEBUG);
    elog_set_filter_tag_lvl("hidg", ELOG_LVL_DEBUG);

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
                hcusb_gadget_hidg_deinit();
                return 0;
            default:
                break;
        }
    }

    hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
    if(!udc_name)
        hcusb_gadget_hidg_init();
    else 
        hcusb_gadget_hidg_specified_init(udc_name);
    return 0;
}


#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_LEN 512

struct options {
	const char    *opt;
	unsigned char val;
};

static struct options kmod[] = {
	{.opt = "--left-ctrl",		.val = 0x01},
	{.opt = "--right-ctrl",		.val = 0x10},
	{.opt = "--left-shift",		.val = 0x02},
	{.opt = "--right-shift",	.val = 0x20},
	{.opt = "--left-alt",		.val = 0x04},
	{.opt = "--right-alt",		.val = 0x40},
	{.opt = "--left-meta",		.val = 0x08},
	{.opt = "--right-meta",		.val = 0x80},
	{.opt = NULL}
};

static struct options kval[] = {
	{.opt = "--return",	.val = 0x28},
	{.opt = "--esc",	.val = 0x29},
	{.opt = "--bckspc",	.val = 0x2a},
	{.opt = "--tab",	.val = 0x2b},
	{.opt = "--spacebar",	.val = 0x2c},
	{.opt = "--caps-lock",	.val = 0x39},
	{.opt = "--f1",		.val = 0x3a},
	{.opt = "--f2",		.val = 0x3b},
	{.opt = "--f3",		.val = 0x3c},
	{.opt = "--f4",		.val = 0x3d},
	{.opt = "--f5",		.val = 0x3e},
	{.opt = "--f6",		.val = 0x3f},
	{.opt = "--f7",		.val = 0x40},
	{.opt = "--f8",		.val = 0x41},
	{.opt = "--f9",		.val = 0x42},
	{.opt = "--f10",	.val = 0x43},
	{.opt = "--f11",	.val = 0x44},
	{.opt = "--f12",	.val = 0x45},
	{.opt = "--insert",	.val = 0x49},
	{.opt = "--home",	.val = 0x4a},
	{.opt = "--pageup",	.val = 0x4b},
	{.opt = "--del",	.val = 0x4c},
	{.opt = "--end",	.val = 0x4d},
	{.opt = "--pagedown",	.val = 0x4e},
	{.opt = "--right",	.val = 0x4f},
	{.opt = "--left",	.val = 0x50},
	{.opt = "--down",	.val = 0x51},
	{.opt = "--kp-enter",	.val = 0x58},
	{.opt = "--up",		.val = 0x52},
	{.opt = "--num-lock",	.val = 0x53},
	{.opt = NULL}
};

int keyboard_fill_report(char report[8], char buf[BUF_LEN], int *hold)
{
	char *tok = strtok(buf, " ");
	int key = 0;
	int i = 0;

	for (; tok != NULL; tok = strtok(NULL, " ")) {

		if (strcmp(tok, "--quit") == 0)
			return -1;

		if (strcmp(tok, "--hold") == 0) {
			*hold = 1;
			continue;
		}

		if (key < 6) {
			for (i = 0; kval[i].opt != NULL; i++)
				if (strcmp(tok, kval[i].opt) == 0) {
					report[2 + key++] = kval[i].val;
					break;
				}
			if (kval[i].opt != NULL)
				continue;
		}

		if (key < 6)
			if (islower(tok[0])) {
				report[2 + key++] = (tok[0] - ('a' - 0x04));
				continue;
			}

		for (i = 0; kmod[i].opt != NULL; i++)
			if (strcmp(tok, kmod[i].opt) == 0) {
				report[0] = report[0] | kmod[i].val;
				break;
			}
		if (kmod[i].opt != NULL)
			continue;

		if (key < 6)
			fprintf(stderr, "unknown option: %s\n", tok);
	}
	return 8;
}

static struct options mmod[] = {
	{.opt = "--b1", .val = 0x01},
	{.opt = "--b2", .val = 0x02},
	{.opt = "--b3", .val = 0x04},
	{.opt = NULL}
};

int mouse_fill_report(char report[8], char buf[BUF_LEN], int *hold)
{
	char *tok = strtok(buf, " ");
	int mvt = 0;
	int i = 0;
	for (; tok != NULL; tok = strtok(NULL, " ")) {

		if (strcmp(tok, "--quit") == 0)
			return -1;

		if (strcmp(tok, "--hold") == 0) {
			*hold = 1;
			continue;
		}

		for (i = 0; mmod[i].opt != NULL; i++)
			if (strcmp(tok, mmod[i].opt) == 0) {
				report[0] = report[0] | mmod[i].val;
				break;
			}
		if (mmod[i].opt != NULL)
			continue;

		if (!(tok[0] == '-' && tok[1] == '-') && mvt < 2) {
			errno = 0;
			report[1 + mvt++] = (char)strtol(tok, NULL, 0);
			if (errno != 0) {
				fprintf(stderr, "Bad value:'%s'\n", tok);
				report[1 + mvt--] = 0;
			}
			continue;
		}

		fprintf(stderr, "unknown option: %s\n", tok);
	}
	return 3;
}

static struct options jmod[] = {
	{.opt = "--b1",		.val = 0x10},
	{.opt = "--b2",		.val = 0x20},
	{.opt = "--b3",		.val = 0x40},
	{.opt = "--b4",		.val = 0x80},
	{.opt = "--hat1",	.val = 0x00},
	{.opt = "--hat2",	.val = 0x01},
	{.opt = "--hat3",	.val = 0x02},
	{.opt = "--hat4",	.val = 0x03},
	{.opt = "--hatneutral",	.val = 0x04},
	{.opt = NULL}
};

int joystick_fill_report(char report[8], char buf[BUF_LEN], int *hold)
{
	char *tok = strtok(buf, " ");
	int mvt = 0;
	int i = 0;

	*hold = 1;

	/* set default hat position: neutral */
	report[3] = 0x04;

	for (; tok != NULL; tok = strtok(NULL, " ")) {

		if (strcmp(tok, "--quit") == 0)
			return -1;

		for (i = 0; jmod[i].opt != NULL; i++)
			if (strcmp(tok, jmod[i].opt) == 0) {
				report[3] = (report[3] & 0xF0) | jmod[i].val;
				break;
			}
		if (jmod[i].opt != NULL)
			continue;

		if (!(tok[0] == '-' && tok[1] == '-') && mvt < 3) {
			errno = 0;
			report[mvt++] = (char)strtol(tok, NULL, 0);
			if (errno != 0) {
				fprintf(stderr, "Bad value:'%s'\n", tok);
				report[mvt--] = 0;
			}
			continue;
		}

		fprintf(stderr, "unknown option: %s\n", tok);
	}
	return 4;
}

void print_options(char c)
{
	int i = 0;

	if (c == 'k') {
		printf("	keyboard options:\n"
		       "		--hold\n");
		for (i = 0; kmod[i].opt != NULL; i++)
			printf("\t\t%s\n", kmod[i].opt);
		printf("\n	keyboard values:\n"
		       "		[a-z] or\n");
		for (i = 0; kval[i].opt != NULL; i++)
			printf("\t\t%-8s%s", kval[i].opt, i % 2 ? "\n" : "");
		printf("\n");
	} else if (c == 'm') {
		printf("	mouse options:\n"
		       "		--hold\n");
		for (i = 0; mmod[i].opt != NULL; i++)
			printf("\t\t%s\n", mmod[i].opt);
		printf("\n	mouse values:\n"
		       "		Two signed numbers\n"
		       "--quit to close\n");
	} else {
		printf("	joystick options:\n");
		for (i = 0; jmod[i].opt != NULL; i++)
			printf("\t\t%s\n", jmod[i].opt);
		printf("\n	joystick values:\n"
		       "		three signed numbers\n"
		       "--quit to close\n");
	}
}

static void dump_report(char *report, int len)
{
    int i;
    printf("\t==> send report(len:%d): ", len);
    for(i = 0; i < len; i++)
        printf(" %2.2x", (unsigned char)report[i]);
    printf("\n");
}

static void dump_raw_buffer(char *buffer, int len)
{
    int i;
    printf("\t==> raw(len:%d): ", len);
    for(i = 0; i < len; i++)
        printf("%c", (unsigned char)buffer[i]);
    printf("\n");
}

char *mouse_demo[] = {
    "55 55",
    "55 -55",
    "-55 55",
    "-55 -55",
};

int hid_gadget_test(int argc, char **argv)
{
	const char *filename = NULL;
	int fd = 0;
	char buf[BUF_LEN];
	int cmd_len;
	char report[8];
	int to_send = 8;
	int hold = 0;
	fd_set rfds;
	int retval, i, loops;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s devname mouse|keyboard|joystick\n",
			argv[0]);
		return 1;
	}

	if (argv[2][0] != 'k' && argv[2][0] != 'm' && argv[2][0] != 'j')
	  return 2;

	filename = argv[1];

	if ((fd = open(filename, O_RDWR, 0666)) == -1) {
        printf("cannot open %s\n", filename);
		perror(filename);
		return 3;
	}

	print_options(argv[2][0]);

    for(loops = 0; loops < 100; loops++) {
        
        if(argv[2][0] == 'k'){
            buf[0] = 'a' + (loops % 26);
            buf[1] = '\0';
            hold = 0;
            printf("--> send to host: 0x%2.2x(%c) \n", buf[0], buf[0]);
        }else if(argv[2][0] == 'm'){
            int index = loops % 4;
            memset(buf, 0, BUF_LEN);
            memcpy(buf, mouse_demo[index], strlen(mouse_demo[index]) + 1);
            dump_raw_buffer(buf, strlen(mouse_demo[index]) + 1);
        }else {
            printf("not support : %s\n", argv[2]);
            break;
        }

        memset(report, 0x0, sizeof(report));
        if (argv[2][0] == 'k')
            to_send = keyboard_fill_report(report, buf, &hold);
        else if (argv[2][0] == 'm')
            to_send = mouse_fill_report(report, buf, &hold);
        else
            to_send = joystick_fill_report(report, buf, &hold);


        if (to_send == -1)
            break;

        dump_report(report, to_send);
        if (write(fd, report, to_send) != to_send) {
            perror(filename);
            return 5;
        }
        if (!hold) {
            memset(report, 0x0, sizeof(report));
            dump_report(report, to_send);            
            if (write(fd, report, to_send) != to_send) {
                perror(filename);
                return 6;
            }
        }

        sleep(1);
    }
    printf("==> finish ....\n");

	close(fd);
	return 0;
}
