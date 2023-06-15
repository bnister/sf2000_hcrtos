#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <hccast_log.h>
#include <errno.h>


#include "hccast_wifi_mgr.h"
#include "hccast_httpd.h"
#include "hccast_web_upgrade_download.h"

extern hccast_httpd_callback httpd_callback;

static int upgrade_download_status = 0;//0-idel, 1-download file, 2-burnflash.
static int user_abort = 0;
static pthread_mutex_t g_upgrade_mutex = PTHREAD_MUTEX_INITIALIZER;



int hccast_web_get_upgrade_status()
{
    int status = 0;
    pthread_mutex_lock(&g_upgrade_mutex);
    status = upgrade_download_status;
    pthread_mutex_unlock(&g_upgrade_mutex);
    return status;
}

void hccast_web_set_upgrade_status(int status)
{
    pthread_mutex_lock(&g_upgrade_mutex);
    upgrade_download_status = status;
    pthread_mutex_unlock(&g_upgrade_mutex);
}

void hccast_web_set_user_abort(int flag)
{
    user_abort = flag;
}

hccast_http_res_header_t hccast_web_upgrade_parse_header(const char *response)
{
    hccast_http_res_header_t resp = {0};

    char *pos = strstr(response, "HTTP/");
    if (pos)
        sscanf(pos, "%*s %d", &resp.status_code);

    pos = strstr(response, "Content-Length:");
    if (pos)
        sscanf(pos, "%*s %ld", &resp.content_length);

    return resp;
}

void hccast_web_upgrade_parse_url(const char *url, char *host, int *port, char *server_ip)
{
    int j = 0;
    int start = 0;
    *port = 80;
    char *patterns[] = {"http://", NULL};

    for (int i = 0; patterns[i]; i++)
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    //1.parse Host server name.
    for (int i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';

    //2.if has port num,parse it.
    char *pos = strstr(host, ":");
    if (pos)
    {
        sscanf(pos, ":%d", port);
        memcpy(server_ip,host,strlen(host));
        char* tmp = strstr(server_ip, ":");
        if(tmp)
        {
            *tmp = '\0';
        }
    }
    else
    {
        memcpy(server_ip,host,strlen(host));
    }
}

void hccast_web_upgrade_get_ip_addr(char *host_name, char *ip_addr)
{

    struct hostent *host = gethostbyname(host_name);
    if (!host)
    {
        ip_addr = NULL;
        return;
    }

    for (int i = 0; host->h_addr_list[i]; i++)
    {
        strcpy(ip_addr, inet_ntoa( * (struct in_addr*) host->h_addr_list[i]));
        break;
    }
}

void hccast_web_download_upgrade_file(int client_socket,  long content_length)
{
    int ret = 0;
    long left_len = content_length;
    int downloap_len = 0;
    int progress = 0;
    int ui_progress = 0;

    char *buf = (char *) malloc(content_length);
    if(buf == NULL)
    {
        hccast_log(LL_ERROR,"malloc upgrade buf fail\n");
        hccast_web_set_upgrade_status(0);
        return;

    }
    memset(buf, 0, content_length);
    //httpd_callback(HCCAST_HTTPD_GET_UPGRADE_FILE_BEGING, NULL, NULL);
    hccast_log(LL_INFO,"buf: %p\n",buf);

    while (left_len)
    {
        if(user_abort)
        {
            hccast_log(LL_WARNING,"user abort\n");
            httpd_callback(HCCAST_HTTPD_MSG_USER_UPGRADE_ABORT, NULL, NULL);
            hccast_web_set_upgrade_status(0);
            free(buf);
            return ;
        }

        ret = recv(client_socket, buf+downloap_len, left_len, 0);
        if(ret > 0)
        {
            left_len -= ret;
            downloap_len += ret;
            progress = downloap_len*100/content_length;
            if(progress != ui_progress)
            {
                ui_progress = progress;
                httpd_callback(HCCAST_HTTPD_SHOW_PROGRESS, (void*)ui_progress, NULL);
            }
        }
        else if(ret <= 0) //it mean server connect close.
        {
            httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
            hccast_web_set_upgrade_status(0);
            free(buf);
            return ;
        }

    }
    hccast_log(LL_NOTICE,"hccast_web_download successful len: %d\n",downloap_len);
    //callback to upper.
    hccast_web_set_upgrade_status(2);
    hccast_web_upgrade_info_st upgrade_info;
    upgrade_info.buf = buf;
    upgrade_info.length = content_length;
    httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_FILE_SUC, (void*)&upgrade_info, NULL);

    if(buf)
    {
        free(buf);
    }

}

static void *hccast_web_download_thread(void *args)
{
    char* url = (char*)args;
    char host[128] = {0};
    char server_name[128] = {0};
    char ip_addr[16] = {0};
    int port = 80;
    char header[2048] = {0};
    int client_socket = -1;
	int retry = 2;
	int res = 0;
	
	httpd_callback(HCCAST_HTTPD_GET_UPGRADE_FILE_BEGING, NULL, NULL);
	usleep(500*1000);
    hccast_web_upgrade_parse_url(url, host, &port, server_name);
    hccast_log(LL_NOTICE,"Host: %s\n",host);
    hccast_log(LL_NOTICE,"server_name: %s\n",server_name);
    hccast_log(LL_NOTICE,"port: %d\n",port);

    hccast_web_upgrade_get_ip_addr(server_name, ip_addr);
    if (strlen(ip_addr) == 0)
    {
        hccast_log(LL_WARNING,"can not get remote server ip.\n");
        hccast_web_set_upgrade_status(0);
		httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
        return ;
    }

    hccast_log(LL_NOTICE,"remote ip: %s\n",ip_addr);

    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
            ,url, host);

RETRY:
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
        hccast_log(LL_WARNING,"create socket fail\n");
        hccast_web_set_upgrade_status(0);
		httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
        return;
    }


    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res < 0)
    {
    	if(retry)
    	{		
			hccast_log(LL_NOTICE,"connect retry\n");
			retry --;
			close(client_socket);
			client_socket = -1;
			usleep(1000*1000);
			goto RETRY;
		}
        //hccast_log(LL_WARNING,"connect socket fail\n");
        perror("connect connect fail:");
        hccast_web_set_upgrade_status(0);
        close(client_socket);
		httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
        return;
    }
    hccast_log(LL_NOTICE,"connect remote OK\n");

    //request HTTP GET.
    if(send(client_socket, header, strlen(header),0) < 0)
    {
		hccast_log(LL_WARNING,"send socket fail\n");
		close(client_socket);
		httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
		return ;
	}
	hccast_log(LL_NOTICE,"send len [%d] OK\n",strlen(header));
#if 1
    //need fiter http header, that we can get upgrade file.
    int mem_size = 4096;
    int length = 0;
    int len = 0;
	char* buf = NULL;
	char * response = NULL;
    buf = (char *) calloc(mem_size * sizeof(char),1);
	if(buf == NULL)
	{
		hccast_log(LL_WARNING,"%s %d calloc fail\n",__func__,__LINE__);
		httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
		goto FAIL;
	}
    response = (char *) calloc(mem_size * sizeof(char),1);
	if(response == NULL)
	{
		hccast_log(LL_WARNING,"%s %d calloc fail\n",__func__,__LINE__);
		httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
		goto FAIL;
	}
	hccast_log(LL_NOTICE,"buf [%x], response:[%x]OK\n",buf,response);
    while(1)
    {
        len = recv(client_socket, buf, 1,0);
        if(len <= 0)
        {
            hccast_log(LL_WARNING,"read fail\n");
            hccast_web_set_upgrade_status(0);
            free(buf);
            free(response);
            close(client_socket);
			httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
            return;
        }
        else
        {
            if (length + len > mem_size)
            {
                mem_size *= 2;
                char * temp = (char *) realloc(response, sizeof(char) * mem_size);
                if (temp == NULL)
                {
                    hccast_log(LL_WARNING,"realloc fail\n");
                }
                response = temp;
            }

            buf[len] = '\0';
            strcat(response, buf);


            int flag = 0;
            for (int i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);

            if (flag == 4)
                break;

            length += len;
        }
    }
#endif
	hccast_log(LL_NOTICE,"HEAD length: %d\n", length);
    hccast_http_res_header_t resp = hccast_web_upgrade_parse_header(response);
    hccast_log(LL_NOTICE,"\tHTTP status_code: %d\n", resp.status_code);
    hccast_log(LL_NOTICE,"\tHTTP file size: %ld\n", resp.content_length);

    //look weather ok or not.
    if(resp.status_code == 200)
    {
        hccast_web_download_upgrade_file(client_socket,resp.content_length);
    }
    else
    {
        httpd_callback(HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD, NULL, NULL);
        hccast_web_set_upgrade_status(0);
    }
FAIL:
    free(buf);
    free(response);
    close(client_socket);
}


void hccast_web_uprade_download_start(char *url)
{
    pthread_t pid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x3000);

    hccast_web_set_upgrade_status(1);
    user_abort = 0;
    hccast_log(LL_NOTICE,"beging to hccast_web_uprade_download_start\n");
    if(pthread_create(&pid,&attr,hccast_web_download_thread,(void*)url) < 0)
    {
        hccast_log(LL_WARNING,"Create hccast_web_uprade_download_start error.\n");
    }
}


//download url data(no http header) to buffer.
//return the length of download url data. return <=0, error happen
int hccast_upgrade_download_config(char *url, char *buffer, int buffer_len)
{
    char host[128] = {0};
    char server_name[128] = {0};
    char ip_addr[16] = {0};
    int port = 80;
    char header[2048] = {0};
    int client_socket = -1;
    int ret;
    int downloap_len = 0;


    hccast_web_upgrade_parse_url(url, host, &port, server_name);
    hccast_log(LL_INFO,"Host: %s\n",host);
    hccast_log(LL_INFO,"server_name: %s\n",server_name);
    hccast_log(LL_INFO,"port: %d\n",port);

    hccast_web_upgrade_get_ip_addr(server_name, ip_addr);
    if (strlen(ip_addr) == 0)
    {
        hccast_log(LL_WARNING,"can not get remote server ip.\n");
        return -1;
    }

    hccast_log(LL_INFO,"remote ip: %s\n",ip_addr);

    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
            ,url, host);

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
        hccast_log(LL_WARNING,"create socket fail\n");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    int res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res < 0)
    {
        hccast_log(LL_WARNING,"connect socket fail\n");
        close(client_socket);
        return -1;
    }
    hccast_log(LL_INFO,"connect remote OK\n");

    //request HTTP GET.
    write(client_socket, header, strlen(header));

#if 1
    //need fiter http header, that we can get upgrade file.
    int mem_size = 4096;
    int length = 0;
    int len = 0;
    char *buf = (char *) malloc(mem_size * sizeof(char));
    char *response = (char *) malloc(mem_size * sizeof(char));

    while(1)
    {
        len = read(client_socket, buf, 1);
        if(len <= 0)
        {
            free(buf);
            free(response);
            close(client_socket);
            return -1;
        }
        else
        {
            if (length + len > mem_size)
            {
                mem_size *= 2;
                char * temp = (char *) realloc(response, sizeof(char) * mem_size);
                if (temp == NULL)
                {
                    hccast_log(LL_WARNING,"realloc fail\n");
                }
                response = temp;
            }

            buf[len] = '\0';
            strcat(response, buf);


            int flag = 0;
            for (int i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);

            if (flag == 4)
                break;

            length += len;
        }
    }
#endif


    hccast_http_res_header_t resp = hccast_web_upgrade_parse_header(response);
    hccast_log(LL_NOTICE,"\tHTTP status_code: %d\n", resp.status_code);
    hccast_log(LL_NOTICE,"\tHTTP file size: %ld\n", resp.content_length);


    free(buf);
    free(response);

    uint32_t content_length = (uint32_t)resp.content_length;
    //look weather ok or not.
    downloap_len = 0;

    uint32_t left_len;
    //set non blcok mode
    if (0 == content_length)
    {
        int flags = fcntl(client_socket,F_GETFL,0);
        fcntl(client_socket,F_SETFL,flags|O_NONBLOCK);
        left_len = buffer_len;
    } 
    else
    {
        left_len = content_length ? (content_length > buffer_len ? buffer_len : content_length) : buffer_len;        
    }

    buf = buffer;
    if(resp.status_code == 200)
    {
        //continue read the url content to buffer.
        
        while (left_len)
        {
            ret = read(client_socket, buf+downloap_len, left_len);
            if(ret > 0)
            {
                left_len -= ret;
                downloap_len += ret;
            }
            else if(ret <= 0) //it mean server connect close.
            {
                close(client_socket);
                return downloap_len;
            }

        }
        hccast_log(LL_NOTICE,"hccast_web_download successful!\n");

    }
    else
    {
        close(client_socket);
        return 0;
    }
    close(client_socket);

    return downloap_len;

}

