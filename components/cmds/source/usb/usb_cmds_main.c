#include "usb_cmds_main.h"
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>

int usb_debug_speed_cmds(int argc, char **argv)
{
#define TEST_TOTAL_SIZE (160 * 1024 * 1024)    
#define BUFFER_SIZE (64 * 1024)    
	char *buf;
	int rc;
	int cnt = 0;
	struct timeval tv1, tv2;
    int cost_ms;
    FILE *fp;


    if(argc != 2){
        printf("=---> Error command\n");
        return -1;
    }


	fp = fopen(argv[1], "w+");
	if (fp == NULL) {
        printf("Error: Cannot create %s\n", argv[1]);
        return -1;
	}
    printf("=---> Create file(%s) successfully...\n", argv[1]); 


    buf = malloc(BUFFER_SIZE);    
	if (buf == NULL) {
        printf("Error: Cannot malloc 64KB buffer\n");
        return -1;
	}
    memset(buf, 0xa5, BUFFER_SIZE);




    printf("=---> Try to write file(%s)...\n", argv[1]);
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = fwrite(buf, BUFFER_SIZE, 1, fp);
        
        cnt += rc;
        //printf("=---> Write file(%s) ... (offset: %d)\n", argv[1], cnt);


        if(cnt >= TEST_TOTAL_SIZE / BUFFER_SIZE)
            break;
        
	}
	gettimeofday(&tv2, NULL);
    printf("=---> Write file(%s) successfully (offset: %d)...\n", argv[1], cnt);
    cnt *= (BUFFER_SIZE / 1024);


    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);


	printf("total bytes %d KB\n", cnt);
	printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("duration: %d ms\n", cost_ms);
    printf("speed: %d MB/s, %d KB/s\n",  
            (cnt / 1000) / cost_ms, 
            cnt / cost_ms);


	fclose(fp);
	free(buf);


	return 0;
}

int usb_debug_speed_test_cmds(int argc, char **argv)
{
#define TEST_TOTAL_SIZE (160 * 1024 * 1024)    
#define BUFFER_SIZE (64 * 1024)    
	char *buf;
	int rc;
	int cnt = 0;
	struct timeval tv1, tv2;
    int cost_ms;
    int fp;


    if(argc != 2){
        printf("=---> Error command\n");
        return -1;
    }


	fp = open(argv[1], O_RDWR | O_CREAT | O_TRUNC);
	if (fp < 0) {
        printf("Error: Cannot create %s\n", argv[1]);
        return -1;
	}
    printf("=---> Create file(%s) successfully...\n", argv[1]); 


    buf = malloc(BUFFER_SIZE);    
	if (buf == NULL) {
        printf("Error: Cannot malloc 64KB buffer\n");
        return -1;
	}
    memset(buf, 0xa5, BUFFER_SIZE);




    printf("=---> Try to write file(%s)...\n", argv[1]);
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = write(fp,buf, BUFFER_SIZE);
        if (rc != BUFFER_SIZE) {
			printf("[%s][%d] ******write error rc = %d\n",__FUNCTION__,__LINE__,rc);
			break;
		}
        cnt += rc;
        //printf("=---> Write file(%s) ... (offset: %d)\n", argv[1], cnt);


        if(cnt >= TEST_TOTAL_SIZE)
            break;
        
	}
	gettimeofday(&tv2, NULL);
    printf("=---> Write file(%s) successfully (offset: %d)...\n", argv[1], cnt);
    //cnt *= (BUFFER_SIZE / 1024);
	cnt /= 1024;

    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);


	printf("total bytes %d KB\n", cnt);
	printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("duration: %d ms\n", cost_ms);
    printf("write speed: %d MB/s, %d KB/s\n",  
            (cnt / 1024) / (cost_ms/1000), 
            cnt / (cost_ms/1000));
	cnt = 0;
	gettimeofday(&tv1, NULL);
	lseek(fp, 0, SEEK_SET);
	
	for (;;) {
		rc = read(fp,buf, BUFFER_SIZE);
        if (rc != BUFFER_SIZE) {
			printf("[%s][%d] ******read error rc = %d\n",__FUNCTION__,__LINE__,rc);
			break;
		}
        cnt += rc;
        //printf("=---> read file(%s) ... (offset: %d)\n", argv[1], cnt);


        if(cnt >= TEST_TOTAL_SIZE)
            break;
        
	}
	gettimeofday(&tv2, NULL);
	cnt /= 1024;

    cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);


	printf("total bytes %d KB\n", cnt);
	printf("tv1 %lld %ld\n", tv1.tv_sec, tv1.tv_usec);
	printf("tv2 %lld %ld\n", tv2.tv_sec, tv2.tv_usec);
	printf("duration: %d ms\n", cost_ms);
    printf("read speed: %d MB/s, %d KB/s\n",  
            (cnt / 1024) / (cost_ms/1000), 
            cnt / (cost_ms/1000));
	
	close(fp);
	
	free(buf);


	return 0;
}
int usb_get_cap_cmds(int argc, char **argv)
{
	struct stat file_stat;
	int ret = -1;
	if(argc != 2){
        printf("=---> Error command\n");
        return -1;
    }
	ret = stat(argv[1],&file_stat);
	if( ret<0 ) {
		printf("[%s][%d] err\n",__FUNCTION__,__LINE__);
	}
	else {
		printf("size == %lld\n",(long long)file_stat.st_size);
	}
	return 0;
}
CONSOLE_CMD(usb, NULL, NULL, CONSOLE_CMD_MODE_SELF, "usb configuration")

CONSOLE_CMD(g_mass_storage, "usb", setup_usbd_mass_storage, CONSOLE_CMD_MODE_SELF, "setup USB as mass-storage device")
CONSOLE_CMD(g_serial, "usb", setup_usbd_serial, CONSOLE_CMD_MODE_SELF, "setup USB as serial console")
CONSOLE_CMD(g_serial_test, "usb", setup_usbd_serial_testing, CONSOLE_CMD_MODE_SELF, "setup USB as serial console test demo")
CONSOLE_CMD(g_ncm, "usb", setup_usbd_ncm, CONSOLE_CMD_MODE_SELF, "setup USB as NCM device demo")
CONSOLE_CMD(g_zero, "usb", setup_usbd_zero, CONSOLE_CMD_MODE_SELF, "setup USB as zero device demo")
CONSOLE_CMD(g_hid, "usb", setup_usbd_hid, CONSOLE_CMD_MODE_SELF, "setup USB as HID device demo")
CONSOLE_CMD(hid_gadget_test, "usb", hid_gadget_test, CONSOLE_CMD_MODE_SELF, "hid gadget test")
CONSOLE_CMD(hid, "usb", hid_test_main, CONSOLE_CMD_MODE_SELF, "usb hid: get input event from USB keyboard")
CONSOLE_CMD(hid_kbd_demo, "usb", hid_kbd_demo, CONSOLE_CMD_MODE_SELF, "usb hid: get input event from USB keyboard, and send these data to usb hid gadget")
CONSOLE_CMD(hid_mouse_demo, "usb", hid_mouse_demo, CONSOLE_CMD_MODE_SELF, "usb hid: get input event from USB Mouse, and send these data to usb hid gadget")
CONSOLE_CMD(hid_dev_demo, "usb", hid_dev_demo_main, CONSOLE_CMD_MODE_SELF, "usb hid dev demo")

#ifdef CONFIG_CMDS_LIBUSB_EXAMPLES
CONSOLE_CMD(hello, "usb", libusb_helloworld_demo, CONSOLE_CMD_MODE_SELF, "libusb examples: hello world demo")
CONSOLE_CMD(testlibusb, "usb", testlibusb, CONSOLE_CMD_MODE_SELF, "libusb examples: test libusb")
CONSOLE_CMD(xusb, "usb", xusb, CONSOLE_CMD_MODE_SELF, "libusb examples: xusb")
CONSOLE_CMD(hotplug, "usb", hotplug, CONSOLE_CMD_MODE_SELF, "libusb examples: hotplug")
#endif
CONSOLE_CMD(g_usb_wr, "usb", usb_debug_speed_cmds, CONSOLE_CMD_MODE_SELF, "usb rw test")
CONSOLE_CMD(g_usb_wr2, "usb", usb_debug_speed_test_cmds, CONSOLE_CMD_MODE_SELF, "usb rw test2")
CONSOLE_CMD(g_get_cap, "usb", usb_get_cap_cmds, CONSOLE_CMD_MODE_SELF, "usb/sd get cap")


#include <kernel/drivers/hcusb.h>

// int usb_debug_cmds(int argc, char **argv)
// {
//     printf("--> usb#0 debug otg test\n");
// 	console_run_cmd("nsh mkrd -m 0 -s 512 20480 ");
// 	console_run_cmd("nsh mkfatfs /dev/ram0");
// 	console_run_cmd("usb g_mass_storage -p 0 /dev/ram0");
//     return 0;
// }

// CONSOLE_CMD(otg, "usb", usb_debug_cmds, CONSOLE_CMD_MODE_SELF, "usb debug test for OTG switch")


int usb_debug_host_cmds(int argc, char **argv)
{
    printf("--> usb#0 debug otg test : host\n");
	// console_run_cmd("usb g_mass_storage -p 0 -s");
    hcusb_set_mode(0, MUSB_HOST);
    hcusb_set_mode(1, MUSB_HOST);
    return 0;
}

CONSOLE_CMD(host, "usb", usb_debug_host_cmds, CONSOLE_CMD_MODE_SELF, "usb debug test for switch to host")



int usb_debug_gadget_cmds(int argc, char **argv)
{
    printf("--> usb#0 debug otg test: gadget\n");
	// console_run_cmd("usb g_mass_storage -p 0 /dev/ram0");
    hcusb_set_mode(0, MUSB_PERIPHERAL);
    hcusb_set_mode(1, MUSB_PERIPHERAL);
    return 0;
}

CONSOLE_CMD(gadget, "usb", usb_debug_gadget_cmds, CONSOLE_CMD_MODE_SELF, "usb debug test for OTG switch")


#ifdef CONFIG_CMDS_USB_EYE_PATTERN

int usb_1_debug_eye_test(int argc, char **argv)
{
    printf("--> usb#1 eye test\n");
    *((uint8_t*)0xB8850001) = 0x23;
    *((uint8_t*)0xB8850380) = 0x01;
    usleep(10);
    *((uint8_t*)0xB8850001) = 0x20;
    usleep(1000);
    *((uint8_t*)0xB8850380) = 0x00;
    usleep(1);
    *((uint8_t*)0xB8800087) = 0x20;
    usleep(1);
    *((uint8_t*)0xB8800087) = 0x00;
    usleep(1);
    *((uint8_t*)0xB8850380) = 0x40; // ok, set usb1 iddig = 1;
    *((uint8_t*)0xB8850001) = 0x60;
    // rick pattern 
    usleep(100000); //delay 100ms for usb init
    //config usb phy 

    *((uint8_t*)0xB8845000) = 0x1f;usleep(1); // [2:0] always enable pre-emphasis
    *((uint8_t*)0xB8845002) = 0x64;usleep(1);// [6:4] HS eye tuning
    *((uint8_t*)0xB8845003) = 0xfc;usleep(1);//[6:2] odt, default:0xd4,0xfc:fastest rise time; 0xc0:slowest rise time. bit7 default is 1
    *((uint8_t*)0xB8845005) = 0xbc;usleep(1);// [4:2] TX HS pre_emphasize stength


    *((uint8_t*)0xB885000F) = 0x01;usleep(1);
    *((uint16_t*)0xB8850020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8850020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8850020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8850020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8850020) = 0xAA00;usleep(1);
    *((uint16_t*)0xB8850020) = 0xAAAA;usleep(1);
    *((uint16_t*)0xB8850020) = 0xAAAA;usleep(1);
    *((uint16_t*)0xB8850020) = 0xAAAA;usleep(1);
    *((uint16_t*)0xB8850020) = 0xEEAA;usleep(1);
    *((uint16_t*)0xB8850020) = 0xEEEE;usleep(1);
    *((uint16_t*)0xB8850020) = 0xEEEE;usleep(1);
    *((uint16_t*)0xB8850020) = 0xEEEE;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFEEE;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8850020) = 0x7FFF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xDFBF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xF7EF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFDFB;usleep(1);
    *((uint16_t*)0xB8850020) = 0x7EFC;usleep(1);
    *((uint16_t*)0xB8850020) = 0xDFBF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xF7EF;usleep(1);
    *((uint16_t*)0xB8850020) = 0xFDFB;usleep(1);
    *((uint8_t*)0xB8850020) = 0x7e;usleep(1);

    *((uint8_t*)0xB885000F) = 0x08;
    usleep(1);
    *((uint8_t*)0xB8850012) = 0x02;
    usleep(1);
    printf("--> usb#1 eye test exit .....\n");
    return 0;
}

CONSOLE_CMD(eye1, "usb", usb_1_debug_eye_test, CONSOLE_CMD_MODE_SELF, "usb#1 eye patern for oscilloscope")


int usb_0_debug_eye_test(int argc, char **argv)
{
    printf("--> usb#0 eye test\n");
    *((uint8_t*)0xB8844001) = 0x23;
    *((uint8_t*)0xB8844380) = 0x01;
    usleep(10);
    *((uint8_t*)0xB8844001) = 0x20;
    usleep(1000);
    *((uint8_t*)0xB8844380) = 0x00;
    usleep(1);
    *((uint8_t*)0xB8800083) = 0x10;
    usleep(1);
    *((uint8_t*)0xB8800083) = 0x00;
    usleep(1);
    *((uint8_t*)0xb8845020) = 0xc0;
    *((uint8_t*)0xb8845021) = 0x10;
    *((uint8_t*)0xB8844380) = 0x00; //ok    otg_dis = 0, use default iddig = 1, Èç¹û²åÉÏ×ª½ÓÏßÀ­µÍid£¬Ôòfail
    *((uint8_t*)0xB8844001) = 0x60;
    // rick pattern 
    usleep(100000); //delay 100ms for usb init
    //config usb phy 
    /*
    *((uint8_t*)0xB8845100) = 0x1f;usleep(1); // [2:0] always enable pre-emphasis
    *((uint8_t*)0xB8845102) = 0x64;usleep(1);// [6:4] HS eye tuning 400/362.5/350/387.5 /412.5/425/475/450
    *((uint8_t*)0xB8845103) = 0xfc;usleep(1);//[6:2] odt, default:0xd4,0xfc:fastest rise time; 0xc0:slowest rise time. bit7 default is 1
    *((uint8_t*)0xB8845105) = 0xbc;usleep(1);// [4:2] TX HS pre_emphasize stength 111 is the strongest
    */

    *((uint8_t*)0xB884400F) = 0x01;usleep(1);
    *((uint16_t*)0xB8844020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8844020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8844020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8844020) = 0x0000;usleep(1);
    *((uint16_t*)0xB8844020) = 0xAA00;usleep(1);
    *((uint16_t*)0xB8844020) = 0xAAAA;usleep(1);
    *((uint16_t*)0xB8844020) = 0xAAAA;usleep(1);
    *((uint16_t*)0xB8844020) = 0xAAAA;usleep(1);
    *((uint16_t*)0xB8844020) = 0xEEAA;usleep(1);
    *((uint16_t*)0xB8844020) = 0xEEEE;usleep(1);
    *((uint16_t*)0xB8844020) = 0xEEEE;usleep(1);
    *((uint16_t*)0xB8844020) = 0xEEEE;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFEEE;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFFFF;usleep(1);
    *((uint16_t*)0xB8844020) = 0x7FFF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xDFBF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xF7EF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFDFB;usleep(1);
    *((uint16_t*)0xB8844020) = 0x7EFC;usleep(1);
    *((uint16_t*)0xB8844020) = 0xDFBF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xF7EF;usleep(1);
    *((uint16_t*)0xB8844020) = 0xFDFB;usleep(1);
    *((uint8_t*)0xB8844020) = 0x7e;usleep(1);

    *((uint8_t*)0xB884400F) = 0x08; // set test mode = 3
    usleep(1);
    *((uint8_t*)0xB8844012) = 0x02;// set tx packet ready
    usleep(100);
    printf("--> usb#0 eye test exit ...\n"); 
    return 0;
}

CONSOLE_CMD(eye0, "usb", usb_0_debug_eye_test, CONSOLE_CMD_MODE_SELF, "usb#0 eye patern for oscilloscope")

#endif  /* CONFIG_CMDS_USB_EYE_PATTERN */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
