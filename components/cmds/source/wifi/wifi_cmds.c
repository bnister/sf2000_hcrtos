#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <kernel/lib/console.h>
#include <pthread.h>
#include <string.h>
#include <uapi/linux/wireless.h>

struct wpa_params{
	int argc;
	char **argv;
};


extern int wpa_supplicant_main(int argc, char *argv[]);
extern int wpa_cli_main(int argc, char *argv[]);
extern int hostapd_main(int argc, char *argv[]);

extern int hostapd_cli_main(int argc, char *argv[]);
extern int iwpriv_main(int argc, char *argv[]);
extern int iwconfig_main(int argc, char *argv[]);
extern int iwlist_main(int argc, char *argv[]);

static pthread_t wpa_supplicant_tid;
static pthread_t hostapd_tid;

static int wifi_entry(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return 0;
}

static void wpa_params_destroy(struct wpa_params *params)
{
	int i = 0;
	if(params){
		if(params->argv){
			for(i = 0; i < params->argc; i++)
				free(params->argv[i]);
			free(params->argv);
		}
		free(params);
	}
}

static struct wpa_params *wpa_params_create(int argc, char **argv)
{
	struct wpa_params *params = (struct wpa_params *)calloc(1, sizeof(struct wpa_params));
	int i = 0;
	if(!params){
		printf("%s:%d,Not enough memory", __func__, __LINE__);
		return NULL;
	}

	params->argc = argc;
	params->argv = (char **)calloc(argc + 1, sizeof(char *));
	if(!params->argv){
		printf("%s:%d,Not enough memory", __func__, __LINE__);
		goto fail;
	}
	for(i = 0; i < argc; i++){
		params->argv[i] = (char *)strdup(argv[i]);
		if(!params->argv[i]){
			printf("%s:%d,Not enough memory", __func__, __LINE__);
			goto fail;
		}
	}
	params->argv[argc] = NULL;

	return params;
fail:
	
	wpa_params_destroy(params);
	return NULL;
}


static void *wpa_supplicant_thread(void *args)
{
	struct wpa_params *params = (struct wpa_params *)args;
	wpa_supplicant_main(params->argc, params->argv);	
	wpa_params_destroy(params);
	printf("%s:exit\n", __func__);
	return NULL;
}

static void *hostapd_thread(void *args)
{
	struct wpa_params *params = (struct wpa_params *)args;
	hostapd_main(params->argc, params->argv);
	wpa_params_destroy(params);
	return NULL;
}


static int wpa_start_helper(int argc, char **argv, pthread_t *tid, void *(*entry) (void *))
{
	pthread_attr_t attr;
	int stack_size = 64*1024;
	int ret = 0;
	struct wpa_params *params;
	params = wpa_params_create(argc, argv);
	if(!params){
		return -1;
	}
	ret = pthread_attr_init(&attr);
	if(ret){
		printf("Init pthread_attr_t error.\n");
		wpa_params_destroy(params);
		return -1;
	}

	ret = pthread_attr_setstacksize(&attr, stack_size);
	if(ret != 0){
		printf("Set stack size error.\n");
		wpa_params_destroy(params);
		return -1;
	}

	ret = pthread_create(tid, &attr, entry, params);
	if(ret != 0){
		printf("Create wpa_supplicant error.\n");
		wpa_params_destroy(params);
		return -1;
	}

	printf("Create thread success.\n");
	return 0;
}

static int wpa_supplicant_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;
	return wpa_start_helper(argc, argv, &wpa_supplicant_tid, wpa_supplicant_thread);
}

static int hostapd_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;
	return wpa_start_helper(argc, argv, &hostapd_tid, hostapd_thread);
}

static int wpa_cli_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return wpa_cli_main(argc, argv);
}


static int hostapd_cli_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return hostapd_cli_main(argc, argv);
}

static int iwpriv_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return iwpriv_main(argc, argv);
}

static int iwconfig_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return iwconfig_main(argc, argv);
}

static int iwlist_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return iwlist_main(argc, argv);
}


static int log_level(int argc, char **argv)
{
extern int wpa_debug_level;
	if(argc < 2){
		printf("parameters error.\n");
		return -1;
	}
	wpa_debug_level = atoi(argv[1]);

	return 0;
}


static int get_best_channel(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	uint32_t val = 0;;
	struct iwreq iwr;
	int ioctl_sock = -1;
	memset(&iwr, 0, sizeof(iwr));
	ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (ioctl_sock < 0) {
		printf("socket[PF_INET,SOCK_DGRAM]");
		return -1;
	}

	strlcpy(iwr.ifr_name, "wlan0", IFNAMSIZ);
	iwr.u.data.pointer = &val;
	iwr.u.data.length = sizeof(val);
	if (ioctl(ioctl_sock, IW_PRIV_IOCTL_BEST_CHANNEL, &iwr) < 0) {
		printf("ioctl[IW_PRIV_IOCTL_BEST_CHANNEL]");
		close(ioctl_sock);
		return -1;
	}
	printf("%s:%d,2.4G: %lu, 5G: %lu\n", __func__, __LINE__, val & 0xFFFF, (val >> 16) & 0xFFFF);
	close(ioctl_sock);

	return 0;
}

CONSOLE_CMD(wifi, NULL, wifi_entry, CONSOLE_CMD_MODE_SELF, "enter wifi test utilities")
CONSOLE_CMD(wpa_supplicant, "wifi", wpa_supplicant_cmd, CONSOLE_CMD_MODE_SELF, "wpa_supplicant daemon")
CONSOLE_CMD(hostapd, "wifi", hostapd_cmd, CONSOLE_CMD_MODE_SELF, "hostapd daemon")
CONSOLE_CMD(wpa_cli, "wifi", wpa_cli_cmd, CONSOLE_CMD_MODE_SELF, "wpa_cli command")
CONSOLE_CMD(hostapd_cli, "wifi", hostapd_cli_cmd, CONSOLE_CMD_MODE_SELF, "hostpad_cli command")
CONSOLE_CMD(iwpriv, "wifi", iwpriv_cmd, CONSOLE_CMD_MODE_SELF, "iwpriv command")
CONSOLE_CMD(iwconfig, "wifi", iwconfig_cmd, CONSOLE_CMD_MODE_SELF, "iwconfig command")
CONSOLE_CMD(iwlist, "wifi", iwlist_cmd, CONSOLE_CMD_MODE_SELF, "iwconfig command")
CONSOLE_CMD(log_level, "wifi", log_level, CONSOLE_CMD_MODE_SELF, "set log level")
CONSOLE_CMD(get_best_channel, "wifi", get_best_channel, CONSOLE_CMD_MODE_SELF, "set log level")
