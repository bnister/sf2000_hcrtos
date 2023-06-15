#define LOG_TAG "curlcmd"
#define ELOG_OUTPUT_LVL ELOG_LVL_VERBOSE

#include <stdio.h>
#include <string.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <getopt.h>
#include <curl/curl.h>
#include <kernel/elog.h>

#define DEFAULT_TMO_MS 60000L
#define DEFAULT_CONNECTION_TMO_MS 60000L
#define DEFAULT_MAX_URL 256

static int g_curl_init_flag = 0;
static bool g_curl_task_run = false;
static char g_curl_url[DEFAULT_MAX_URL];
static struct Curl_easy *g_curl_handle = NULL;

static int cmd_curl_initialize(void)
{
	CURLcode err = 0;

	g_curl_init_flag = 0;

	err = curl_global_init(CURL_GLOBAL_ALL);

	if (CURLE_OK == err) {
		g_curl_init_flag = 1;
		log_d("curl init ok.");
	} else {
		log_e("curl_global_init error: %s", curl_easy_strerror(err));
		return -1;
	}

	return err;
}

static size_t cmd_curl_write_data_callback(char *ptr, size_t size, size_t nmemb,
					   void *userdata)
{
	log_d("user data = %d", (int)userdata);

	elog_hexdump("data", 16, (uint8_t *)ptr, size * nmemb);

	return size * nmemb;
}

static void cmd_curl_http_request(void *p)
{
	struct Curl_easy *curl_handle = NULL;
	CURLcode res = 0;
	char *url = (char *)p;

	if (url == NULL)
		goto exit;

	log_d("URL: %s", url);
	/*Now init one curl session */
	curl_handle = curl_easy_init();
	g_curl_handle = curl_handle;

	/* Now set option for this session */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
			 cmd_curl_write_data_callback);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,
			 (void *)1); /* Just add some user data for test */

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER,
			 0L); /* Don't verify */

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST,
			 0L); /* Don't verify */

	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS,
			 DEFAULT_CONNECTION_TMO_MS);

	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS,
			 DEFAULT_TMO_MS); /* Read operation timeout*/

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	res = curl_easy_perform(curl_handle);

	if (CURLE_OK != res) {
		log_e("curl_easy_perform fail: %s", curl_easy_strerror(res));
	}

exit:
	if (curl_handle)
		curl_easy_cleanup(curl_handle);
	g_curl_handle = NULL;
	g_curl_task_run = false;
	log_i("Curl thread exit");
	vTaskDelete(NULL);
	return;
}

static void cmd_curl_create_thread(char *url)
{
	if (g_curl_task_run == true) {
		log_i("Already have ont curl thread running.");
		return;
	}

	xTaskCreate(cmd_curl_http_request, "curl", configTASK_STACK_DEPTH,
		    (void *)url, portPRI_TASK_NORMAL, NULL);
	g_curl_task_run = true;
}

static int console_curl_help(char *progname)
{
	log_d("%s https://xxxxx ", progname);
	log_d("%s https://xxxxx ", progname);
}

static int console_curl_cmd(int argc, char *argv[])
{
	char ch;

	/* Clear opt global variables
	 * opterr
	 * optind
	 */
	opterr = 0;
	optind = 0;

	while ((ch = getopt(argc, argv, "hs")) != EOF) {
		switch (ch) {
		case 's':
			if (g_curl_handle) {
				curl_easy_setopt(g_curl_handle,
						 CURLOPT_TIMEOUT_MS, 1);
			}
			return 0;
		default:
			console_curl_help(argv[0]);
			return -1;
		}
	}

	if ((optind + 1) != argc) {
		log_e("Arguments are not correct, please curl -h to get help");
		return -1;
	}

	if (g_curl_init_flag == 0)
		cmd_curl_initialize();

	log_d("URL: %s", argv[optind]);
	strcpy(g_curl_url, argv[optind]);

	cmd_curl_create_thread(g_curl_url);

	return 0;
}

CONSOLE_CMD(network, NULL, NULL, CONSOLE_CMD_MODE_SELF, "network cmds entry")
CONSOLE_CMD(curl, "network", console_curl_cmd, CONSOLE_CMD_MODE_SELF, "HTTP test command")
