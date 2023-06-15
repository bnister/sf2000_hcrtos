#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/delay.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define IPERF_PORT 5001
#define IPERF_BUFSZ (32 * 1024)

#define IPERF_MODE_STOP 0
#define IPERF_MODE_SERVER 1
#define IPERF_MODE_CLIENT 2

#define IPERF_TCP 0
#define IPERF_UDP 1

typedef struct {
	int mode;
	int proto;
	char *host;
	int port;
} iperf_param_t;

static iperf_param_t param = { IPERF_MODE_STOP, IPERF_TCP, NULL, IPERF_PORT };

static void iperf_udp_server(void *data)
{
	vTaskDelete(NULL);
}

static void iperf_udp_client(void *data)
{
	int i;
	int sockfd;
	int ret;
	struct sockaddr_in addr;

	uint8_t *send_buf;
	int sentlen;

	send_buf = (uint8_t *)malloc(IPERF_BUFSZ);
	if (!send_buf)
		return;

	for (i = 0; i < IPERF_BUFSZ; i++)
		send_buf[i] = i & 0xff;

	while (param.mode != IPERF_MODE_STOP) {
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0) {
			log_d("create socket failed!\n");
			msleep(1000);
			continue;
		}
		addr.sin_family = AF_INET;
		addr.sin_port = htons(param.port);
		addr.sin_addr.s_addr = inet_addr((char *)param.host);

		{
			int flag = 1;

			setsockopt(sockfd,
				   IPPROTO_UDP, /* set option at TCP level */
				   0x01, /* name of option */
				   (void *)&flag, /* the cast is historical */
				   sizeof(int)); /* length of option value */
		}

		sentlen = 0;
		while (param.mode != IPERF_MODE_STOP) {

			ret = sendto(sockfd, send_buf, 128, 0,
				     (const struct sockaddr *)&addr,
				     sizeof(struct sockaddr_in));
			if (ret > 0) {
				sentlen += ret;
			}

			if (ret < 0)
				break;
		}
		close(sockfd);
		msleep(1000);
		log_d("disconnected!\n");
	}
	if (send_buf)
		free(send_buf);

	vTaskDelete(NULL);
}

static void iperf_tcp_server(void *data)
{
	uint8_t *recv_data;
	socklen_t sin_size;
	int sock = -1, connected, bytes_received, recvlen;
	struct sockaddr_in server_addr, client_addr;

	recv_data = (uint8_t *)malloc(IPERF_BUFSZ);
	if (recv_data == NULL) {
		log_d("No memory\n");
		goto __err_return;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		log_d("Socket error\n");
		goto __err_return;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(param.port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(server_addr.sin_zero), 0x0, sizeof(server_addr.sin_zero));

	if (bind(sock, (struct sockaddr *)&server_addr,
		 sizeof(struct sockaddr)) == -1) {
		log_d("Unable to bind\n");
		goto __err_return;
	}

	if (listen(sock, 5) == -1) {
		log_d("Listen error\n");
		goto __err_return;
	}

	while (param.mode != IPERF_MODE_STOP) {
		sin_size = sizeof(struct sockaddr_in);

		connected = accept(sock, (struct sockaddr *)&client_addr,
				   &sin_size);

		log_d("new client connected from (%s, %d)\n",
		      inet_ntoa(client_addr.sin_addr),
		      ntohs(client_addr.sin_port));

		{
			int flag = 1;

			setsockopt(connected,
				   IPPROTO_TCP, /* set option at TCP level */
				   TCP_NODELAY, /* name of option */
				   (void *)&flag, /* the cast is historical */
				   sizeof(int)); /* length of option value */
		}

		recvlen = 0;
		while (param.mode != IPERF_MODE_STOP) {
			bytes_received =
				recv(connected, recv_data, IPERF_BUFSZ, 0);
			if (bytes_received <= 0)
				break;

			recvlen += bytes_received;
		}

		if (connected >= 0)
			close(connected);
		connected = -1;
	}

__err_return:
	if (sock >= 0)
		close(sock);
	if (recv_data)
		free(recv_data);

	vTaskDelete(NULL);
}

static void iperf_tcp_client(void *data)
{
	int i;
	int sock;
	int ret;

	uint8_t *send_buf;
	int sentlen;
	struct sockaddr_in addr;

	send_buf = (uint8_t *)malloc(IPERF_BUFSZ);
	if (!send_buf)
		return;

	for (i = 0; i < IPERF_BUFSZ; i++)
		send_buf[i] = i & 0xff;

	while (param.mode != IPERF_MODE_STOP) {
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0) {
			log_d("create socket failed!\n");
			msleep(1000);
			continue;
		}

		addr.sin_family = PF_INET;
		addr.sin_port = htons(param.port);
		addr.sin_addr.s_addr = inet_addr((char *)param.host);

		ret = connect(sock, (const struct sockaddr *)&addr,
			      sizeof(addr));
		if (ret == -1) {
			log_d("Connect failed!\n");
			close(sock);

			msleep(1000);
			continue;
		}

		log_d("Connect to iperf server successful!\n");

		{
			int flag = 1;

			setsockopt(sock,
				   IPPROTO_TCP, /* set option at TCP level */
				   TCP_NODELAY, /* name of option */
				   (void *)&flag, /* the cast is historical */
				   sizeof(int)); /* length of option value */
		}

		sentlen = 0;

		while (param.mode != IPERF_MODE_STOP) {
			ret = send(sock, send_buf, IPERF_BUFSZ, 0);
			if (ret > 0) {
				sentlen += ret;
			}

			if (ret < 0)
				break;
		}

		close(sock);

		log_d("disconnected!\n");
	}

	if (send_buf)
		free(send_buf);

	vTaskDelete(NULL);
}

static void iperf_start_task(int mode)
{
	BaseType_t ret = pdFAIL;

	if (mode == IPERF_MODE_SERVER) {
		if (param.proto == IPERF_UDP) {
			ret = xTaskCreate(iperf_udp_server, "iperf_udp_s", 512,
					  NULL, portPRI_TASK_NORMAL, NULL);
		} else {
			ret = xTaskCreate(iperf_tcp_server, "iperf_tcp_s", 512,
					  NULL, portPRI_TASK_NORMAL, NULL);
		}
	} else if (mode == IPERF_MODE_CLIENT) {
		if (param.proto == IPERF_UDP) {
			ret = xTaskCreate(iperf_udp_client, "iperf_udp_c", 512,
					  NULL, portPRI_TASK_NORMAL, NULL);

		} else {
			ret = xTaskCreate(iperf_tcp_client, "iperf_tcp_c", 512,
					  NULL, portPRI_TASK_NORMAL, NULL);
		}
	} else {
		log_d("unknown mode\n");
	}

	if (ret == pdPASS)
		param.mode = mode;
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -h, --help               print this message\n"
	     "  -c, --client    <host>   run in client mode, connecting to <host>\n"
	     "  -s, --server             run in server mode\n"
	     "  -u, --udp                use UDP rather than TCP\n"
	     "  -p, --port      #        server port to listen on/connect to\n"
	     "  -q, --quit               quit\n");
}

static int iperf_entry(int argc, char **argv)
{
	int ch;

	opterr = 0;
	optind = 0;

	int mode = IPERF_MODE_STOP;
	param.proto = IPERF_TCP;

	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	while (1) {
		static const struct option lopts[] = {
			{ "client",  1, 0, 'c' },
			{ "server",  0, 0, 's' },
			{ "port",    1, 0, 'p' },
			{ "udp",     0, 0, 'u' },
			{ "quit",    0, 0, 'q' },
			{ "help",    0, 0, 'h' },
			{ NULL, 0, 0, 0 },
		};

		ch = getopt_long(argc, argv, "c:p:suqh", lopts, NULL);

		if (ch == -1)
			break;

		switch (ch) {
		case 'h':
			print_usage(argv[0]);
			break;
		case 's':
			mode = IPERF_MODE_SERVER;
			break;
		case 'c':
			mode = IPERF_MODE_CLIENT;
			if (param.host != NULL) {
				free(param.host);
				param.host = NULL;
			}
			param.host = strdup(optarg);
			log_d("%s", param.host);
			break;
		case 'p':
			param.port = atoi(optarg);
			log_d("%d", param.port);
			break;
		case 'u':
			param.proto = IPERF_UDP;
			break;
		case 'q':
			param.mode = IPERF_MODE_STOP;
			/*wait socket close*/
			msleep(2000);
			if (param.host != NULL) {
				free(param.host);
				param.host = NULL;
			}
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (param.mode == IPERF_MODE_STOP && mode != IPERF_MODE_STOP)
		iperf_start_task(mode);

	return 0;
}

static void lwipiperf_print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -h, --help               print this message\n"
	     "  -c, --client    <host>   run in client mode, connecting to <host>\n"
	     "  -s, --server             run in server mode\n"
	     "  -p, --port      #        server port to listen on/connect to\n"
	     "  -q, --quit               quit\n"
	     "note: only support TCP mode\n");
}

void lwiperf_example_init(void);
void *lwiperf_example_server_init(uint16_t port);
void *lwiperf_example_client_init(char *server_ip, uint16_t port);
void lwiperf_example_deinit(void *lwiperf_session);

static int lwipiperf_entry(int argc, char **argv)
{
	int ch;
	char *server_ip = NULL;
	uint16_t port = 0;
	static void *lwipiperf_session = NULL;

	opterr = 0;
	optind = 0;

	static int mode = IPERF_MODE_STOP;

	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	while (1) {
		static const struct option lopts[] = {
			{ "client",  1, 0, 'c' },
			{ "server",  0, 0, 's' },
			{ "port",    1, 0, 'p' },
			{ "quit",    0, 0, 'q' },
			{ "help",    0, 0, 'h' },
			{ NULL, 0, 0, 0 },
		};

		ch = getopt_long(argc, argv, "c:p:sqh", lopts, NULL);

		if (ch == -1)
			break;

		switch (ch) {
		case 'h':
			lwipiperf_print_usage(argv[0]);
			break;
		case 's':
			mode = IPERF_MODE_SERVER;
			break;
		case 'c':
			mode = IPERF_MODE_CLIENT;
			server_ip = optarg;
			log_d("%s", param.host);
			break;
		case 'p':
			port = atoi(optarg);
			log_d("%d", param.port);
			break;
		case 'q':
			mode = IPERF_MODE_STOP;
			break;
		default:
			lwipiperf_print_usage(argv[0]);
			return -1;
		}
	}
	
	if (mode == IPERF_MODE_STOP){
		lwiperf_example_deinit(lwipiperf_session);
		lwipiperf_session = NULL;
	}else if (mode == IPERF_MODE_SERVER){
		if(lwipiperf_session){
			lwiperf_example_deinit(lwipiperf_session);
			lwipiperf_session = NULL;
		}
		lwipiperf_session = lwiperf_example_server_init(port);
	}else if (mode == IPERF_MODE_CLIENT){
		if(lwipiperf_session){
			lwiperf_example_deinit(lwipiperf_session);
			lwipiperf_session = NULL;
		}
		lwipiperf_session = lwiperf_example_client_init(server_ip, port);
	}

	return 0;
}

CONSOLE_CMD(iperf, "net", iperf_entry, CONSOLE_CMD_MODE_SELF, "iperf")
CONSOLE_CMD(lwipiperf, "net", lwipiperf_entry, CONSOLE_CMD_MODE_SELF, "lwip iperf")
