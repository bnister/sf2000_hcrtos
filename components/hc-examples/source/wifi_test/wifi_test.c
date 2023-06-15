#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <netdb.h>

#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include "console.h"
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/vidmp.h>

#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_dhcpd.h>

int wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out)
{
    switch (event)
    {
        case HCCAST_WIFI_SCAN:
        {
            printf("%s:%d: event: HCCAST_WIFI_SCAN\n", __func__, __LINE__);

            break;
        }
        case HCCAST_WIFI_SCAN_RESULT:
        {
            printf("%s:%d: event: HCCAST_WIFI_SCAN_RESULT\n", __func__, __LINE__);

            break;
        }
        case HCCAST_WIFI_CONNECT:
        {
            printf("%s:%d: event: HCCAST_WIFI_CONNECT\n", __func__, __LINE__);

            break;
        }

        case HCCAST_WIFI_CONNECT_SSID:
        {
            printf("%s:%d: event: HCCAST_WIFI_CONNECT_SSID, ssid: %s\n", __func__, __LINE__, (char*)out);
            break;
        }

        case HCCAST_WIFI_CONNECT_RESULT:
        {
            hccast_udhcp_result_t* result = (hccast_udhcp_result_t *) out;

            printf("%s:%d: event: HCCAST_WIFI_CONNECT_RESULT\n", __func__, __LINE__);

            if (result)
            {
                printf("state: %d", result->stat);
                if (result->stat)
                {
                    printf("ifname: %s", result->ifname);
                    printf("ip addr: %s", result->ip);
                    printf("mask addr: %s", result->mask);
                    printf("gw addr: %s", result->gw);
                    printf("dns: %s", result->dns);
                }
            }

            break;
        }

        case HCCAST_WIFI_DISCONNECT:
        {
            printf("%s:%d: event: HCCAST_WIFI_DISCONNECT\n", __func__, __LINE__);

            break;
        }

        case HCCAST_WIFI_HOSTAP_OFFER:
        {
            printf("%s:%d: event: HCCAST_WIFI_HOSTAP_OFFER\n", __func__, __LINE__);
        }

        default:
            break;
    }

    return 0;
}

int wifi_scan(int argc , char *argv[])
{
    hccast_wifi_scan_result_t scan_res = {0};
    int ret = 0;
    int i = 0;

    ret = hccast_wifi_mgr_scan(&scan_res);
    if(ret != 0){
        printf("%s:%d: hccast_wifi_mgr_scan failed, ret=%d\n", __func__, __LINE__, ret);
        return -1;
    }

    if(scan_res.ap_num > 0){
        printf("ssid \t\t encryptMode \t keyIdx \t pwd \t numCharInPwd \t quality \t special_ap\n");
        for (i = 0; i < scan_res.ap_num; i++){
            //fiter some bad ap.
            if(strlen(scan_res.apinfo[i].ssid) > 0){
                printf("[%d]: %s \t %d \t %d \t %s \t %d \t\t %d \t\t %d\n",
                i, scan_res.apinfo[i].ssid, scan_res.apinfo[i].encryptMode,
                scan_res.apinfo[i].keyIdx, scan_res.apinfo[i].pwd, scan_res.apinfo[i].numCharInPwd,
                scan_res.apinfo[i].quality, scan_res.apinfo[i].special_ap);
            }
        }
    }else{
        printf("%s:%d:err: scan_res.ap_num is %d\n", __func__, __LINE__, scan_res.ap_num);
    }

    return 0;
}

int wifi_connect(int argc , char *argv[])
{
    hccast_wifi_scan_result_t scan_res = {0};
    hccast_wifi_ap_info_t *ap_info = NULL;
    char ssid[WIFI_MAX_SSID_LEN] = {0};
    char pass_word[WIFI_MAX_PWD_LEN] = {0};
	int opt;
	int ret = 0;
    int i = 0;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "s:p:")) != EOF) {
		switch(opt) {
			case 's':
                snprintf(ssid, WIFI_MAX_SSID_LEN, "%s", optarg);
				break;
			case 'p':
                snprintf(pass_word, WIFI_MAX_PWD_LEN, "%s", optarg);
				break;
			default:
				break;
		}
	}

    printf("%s:%d: ssid=%s, pass_word=%s\n", __func__, __LINE__, ssid, pass_word);
    if((strlen(ssid) == 0) || (strlen(pass_word) == 0) ||
        (strlen(ssid) > WIFI_MAX_SSID_LEN) || (strlen(pass_word) > WIFI_MAX_PWD_LEN)){
        printf("%s:%d: ssid or pass_word is invalid, ssid=%s, pass_word=%s\n", __func__, __LINE__, ssid, pass_word);
        return -1;
    }

    /* wifi scan, if ssid is exist? */
    ret = hccast_wifi_mgr_scan(&scan_res);
    if(ret != 0){
        printf("%s:%d: hccast_wifi_mgr_scan failed, ret=%d\n", __func__, __LINE__, ret);
        return -1;
    }

    if(scan_res.ap_num > 0){
        for (i = 0; i < scan_res.ap_num; i++){
            if(strcmp(ssid, scan_res.apinfo[i].ssid) == 0){
                ap_info = &scan_res.apinfo[i];
                break;
            }
        }

        if( i >= scan_res.ap_num){
            printf("%s:%d:err: ssid(%s) is exist\n", __func__, __LINE__, ssid);
            return -1;
        }
    }else{
        printf("%s:%d:err: wifi scan failed, scan_res.ap_num is %d\n", __func__, __LINE__, scan_res.ap_num);
        return -1;
    }

    /* wifi connect */
    strcpy(ap_info->pwd, pass_word);
    ap_info->keyIdx = 1;

    hccast_wifi_mgr_connect(ap_info);
    if (hccast_wifi_mgr_get_connect_status())
    {
        //hccast_wifi_mgr_udhcpc_start();
        ret = system("udhcpc -i wlan0 -q &");
        if(ret <= 0){
            printf("%s:%d:err: udhcpc start failed, ret=%d\n", __func__, __LINE__, ret);
        }
    }
	else
	{
		hccast_wifi_mgr_disconnect();
	}

    return 0;
}

int wifi_get_status(int argc , char *argv[])
{
    int connect = 0;

    connect = hccast_wifi_mgr_get_connect_status();

    printf("wifi staus: %s\n", (connect ? "connect" : "disconnect"));

    return 0;
}

int wifi_get_connect_ssid(int argc , char *argv[])
{
    char cur_ssid[64] = {0};

    hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
    printf("%s:%d: cur_ssid=%s\n", __func__, __LINE__, cur_ssid);

    return 0;
}

int wifi_test_init(void)
{
    hccast_wifi_mgr_init(wifi_mgr_callback_func);

    return 0;
}

int wifi_test_exit(void)
{
    hccast_wifi_mgr_uninit();

    return 0;
}

static struct termios stored_settings;
static void exit_console(int signo)
{
    (void)signo;
    wifi_test_exit();
    tcsetattr(0 , TCSANOW , &stored_settings);
    exit(0);
}

int main(int argc, char *argv[])
{
	struct termios new_settings;

	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	signal(SIGTERM, exit_console);
	signal(SIGINT, exit_console);
	signal(SIGSEGV, exit_console);
	signal(SIGBUS, exit_console);

    wifi_test_init();

	console_init("wifi_test:");
	console_register_cmd(NULL, "scan", wifi_scan, CONSOLE_CMD_MODE_SELF, "scan");
	console_register_cmd(NULL, "connect", wifi_connect, CONSOLE_CMD_MODE_SELF, "connect -s ssid -p password");
	console_register_cmd(NULL, "get_status", wifi_get_status, CONSOLE_CMD_MODE_SELF, "get_status");
	console_register_cmd(NULL, "get_connect_ssid", wifi_get_connect_ssid, CONSOLE_CMD_MODE_SELF, "get_connect_ssid");

	console_start();
	exit_console(0);
	(void)argc;
	(void)argv;
	return 0;
}



