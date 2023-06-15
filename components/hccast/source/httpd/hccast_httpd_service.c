/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include <hccast_scene.h>
#include <hccast_air.h>

#include "hccast_web_interface.h"
#include "hccast_web_package.h"
#include "hccast_web_page.h"

#include "hccast_wifi_mgr.h"
#include "hccast_httpd.h"
#include <hccast_log.h>

#if 0
#define HOST_IP "192.168.1.101"
#define PAGE_INDEX "index.html"
#define PAGE_WIFIAP_LIST "wifiap_list.html"
#define PAGE_MEDIACONTROL "media_control.html"
#define PAGE_SETTING "setting.html"
#define PAGE_WIFIAP_SELECT "wifiap_select.html"
#define PAGE_WIFIAP_SUBMIT "wifiap_submit.html"
#define PAGE_WIFIAP_DISCONNECT_SUBMIT "wifiap_disconnect_submit.html"
#define PAGE_WIFIAP_FORGET_SUBMIT "wifiap_forget_submit.html"
#define PAGE_NETUPG_SUBMIT "netupg_submit.html"
#define PAGE_MIRRORMODE_SUBMIT "mirrormode_submit.html"
#define PAGE_IPERF_SUBMIT "iperf_start.html"
#endif

#define ISspace(x) isspace((int)(x))

#define ONLINE_DEFAULT_SERVER "www.hccast.com"
#define SERVER_STRING "Server: hccastsmartdisplay/1.0.0\r\n"

//pfn_tinyhttp_callback p_tinyhttp_callback = NULL;
//static int g_scan_time_out = -1;
//static unsigned char upg_server[SERVER_LEN] = { 0 };
static unsigned int g_content_length = 0;
pthread_t g_httpd_tid = 0;
unsigned char *g_web_page_buf = g_web_page;
httpd_web_package_header_st *g_web_page_header = (httpd_web_package_header_st *)g_web_page;
hccast_httpd_callback httpd_callback = NULL;

#define STR_LEN 255
#define PATH_LEN 512

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return (i);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[BUF_LEN];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    perror(sc);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path, const char *method,
                 const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A';
    buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)
        while ((numchars > 0) &&
               strcmp("\n", buf)) /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
    else   /* POST */
    {
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }
        if (content_length == -1)
        {
            bad_request(client);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    if (pipe(cgi_output) < 0)
    {
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0)
    {
        cannot_execute(client);
        return;
    }

    if ((pid = fork()) < 0)
    {
        cannot_execute(client);
        return;
    }
    if (pid == 0)   /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], 1);
        dup2(cgi_input[0], 0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0)
        {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else     /* POST */
        {
            sprintf(length_env, "CONTENT_LENGTH=%d",
                    content_length);
            putenv(length_env);
        }
        execl(path, path, NULL);
    }
    else     /* parent */
    {
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++)
            {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/

#define HTTP_RESPONSE_OK "HTTP/1.0 200 OK\r\n\
%s\
Content-Type: text/html\r\n\
Access-Control-Allow-Origin: *\r\n\
Connection: close\r\n\
Content-Length: 0\r\n\
\r\n"

#define HTTP_RESPONSE_SERVER_FAIL "HTTP/1.0 500 Internal Server Error\r\n\
%s\
Content-Type: text/html\r\n\
Access-Control-Allow-Origin: *\r\n\
\r\n"


int web_response_server_str(int client, char *str_hint)
{
    char buf[BUF_LEN] = {0};

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "%s", SERVER_STRING);
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "Access-Control-Allow-Origin: *\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "Content-Length: %d\r\n", strlen(str_hint));
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    
    sprintf(buf, str_hint);
    send(client, buf, strlen(buf), 0);

    return 0;
}


void web_response_ok(int client, const char *filename)
{
    char buf[BUF_LEN] = {0};

    snprintf(buf, BUF_LEN, HTTP_RESPONSE_OK, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
}

void http_response_server_fail(int client, const char *filename)
{
    char buf[BUF_LEN] = {0};

    snprintf(buf, BUF_LEN, HTTP_RESPONSE_SERVER_FAIL, SERVER_STRING);
    send(client, buf, strlen(buf), 0);

}

void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename; /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

void web_server_error(int client, int error_code)
{
    char buf[BUF_LEN];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "Server Error,invalid settings.\r\n");
    send(client, buf, strlen(buf), 0);
    hccast_log(LL_INFO, "%s\n", __FUNCTION__);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

static void page_failed(int client, char *str_hint)
{
    char buf[BUF_LEN];

    web_response_ok(client, NULL);

    sprintf(buf, "<HTML><TITLE>Page Failed</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY style=\"background-color:white;\">\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<p style=\"font-family:arial;font-size:40px;text-align:left;\">%s</p>\r\n", str_hint);
    send(client, buf, strlen(buf), 0);

}

void page_hint(int client, char *str_hint)
{
    char buf[BUF_LEN];

    http_response_server_fail(client, NULL);

    sprintf(buf, "<HTML><TITLE>SyncScreen Hint</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY style=\"background-color:white;\">\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<p style=\"font-family:arial;font-size:40px;text-align:left;\">%s</p>\r\n", str_hint);
    send(client, buf, strlen(buf), 0);

}

void web_headers_ex(int client, const char *filename, unsigned char *content_type)
{
    char buf[BUF_LEN];

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: %s\r\n", content_type);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Access-Control-Allow-Origin: *\r\n");
    send(client, buf, strlen(buf), 0);
}

static void http_get_content_type(unsigned char *name, unsigned char *type)
{
    unsigned char *str = NULL;

    str = strstr(name, ".");
    if (!strcmp(str, ".html"))
    {
        strcpy(type, "text/html");
    }
    else if (!strcmp(str, ".ico"))
    {
        strcpy(type, "image/x-icon");
    }
    else if (!strcmp(str, ".css"))
    {
        strcpy(type, "text/css");
    }
    else if (!strcmp(str, ".js"))
    {
        strcpy(type, "application/javascript");
    }
}

static void http_response_page(int client, httpd_web_file_st *page)
{
    unsigned char content_type[64] = {0};
    char buf[BUF_LEN];

    http_get_content_type(page->name, content_type);
    web_headers_ex(client, NULL, content_type);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    send(client, g_web_page_buf + page->offset, page->size, 0);
}

unsigned int httpd_get_content_length(void)
{
    return g_content_length;
}

#if 0
void tinyhttp_callback_notify(unsigned int param1, unsigned int param2)
{
    if (p_tinyhttp_callback != NULL)
    {
        p_tinyhttp_callback(param1, param2);
    }
}
#endif

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[BUF_LEN];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    int numchars = 1;
    char buf[BUF_LEN] = { 0 };
    char path[PATH_LEN] = { 0 };
    char param[PATH_LEN] = { 0 };
    char *str1 = NULL, *str2 = NULL, *tmp = NULL;
    unsigned char i = 0;
    int ret = 0;
    int cur_scene_play = 0;

    g_content_length = 0;

    buf[0] = 'A';
    buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))   /* read & discard headers */
    {
        numchars = get_line(client, buf, sizeof(buf));
        //hccast_log(LL_DEBUG,"%s\n", buf);
        if (numchars > 0 && strstr(buf, "Content-Length:"))
        {
            char *length_start = buf + strlen("Content-Length:");
            g_content_length = atoi(length_start);
        }
    }

    if (!g_web_page_buf)
    {
        not_found(client);
        return;
    }

    tmp = (char*)filename;
    if (filename[1] == 'u' && filename[2] == 'p' && filename[3] == 'g')
    {
        str1 = (char*)&filename[1];
    }
    else
    {
        while (tmp = strstr(tmp, "/"))
        {
            tmp++;
            str1 = tmp;
        }
    }
    str2 = strstr(str1, "?");
    if (str2)
    {
        strncpy(path, str1, str2 - str1);
        str2++;
        strcat(param, str2);
    }
    else
    {
        strcat(path, str1);
    }

    if (httpd_callback)
    {
        cur_scene_play = httpd_callback(HCCAST_HTTPD_GET_CUR_SCENE_PLAY, NULL, NULL);
        if (cur_scene_play)
        {
            if(web_interface_request_blacklist_check(client, path, param))
            {
                hccast_log(LL_WARNING, "http not allow to access server!\n");
                return;
            }    
        }
    }

    hccast_log(LL_INFO, "Path: %s, Param: %s\n", path, param);
    for (i = 0; i < g_web_page_header->file_num; i++)
    {
        if (!strcmp(path, (char*)g_web_page_header->file_list[i].name))
        {
            hccast_log(LL_INFO, "HTTP response %s\n", g_web_page_header->file_list[i].name);
            http_response_page(client, &g_web_page_header->file_list[i]);
            goto out;
        }
    }

    ret = web_interface_request(client, path, param);
    if (0 != ret)
    {
        if (ret == 0xFF)
            not_found(client);
        else
            web_server_error(client, ret);
    }

#if 0
    if (get_scanning_flag())
    {
        set_scanning_flag(0);
#ifdef HQCAST_SUPPORT
        extern void api_mira_start();
        api_mira_start();
#endif
    }
#endif

out:
    return;
}

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(void* arg)
{
    int client = (int)arg;
    char buf[BUF_LEN] = { 0 };
    size_t numchars;
    char method[STR_LEN] = { 0 };
    char url[STR_LEN] = { 0 };
    char path[PATH_LEN] = { 0 };
    size_t i, j;

    numchars = get_line(client, buf, sizeof(buf));
    i = 0;
    j = 0;
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[i];
        i++;
    }

    j = i;
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return;
    }

    i = 0;
    while (ISspace(buf[j]) && (j < numchars))
    {
        j++;
    }
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < numchars))
    {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    //hccast_log(LL_DEBUG,"URL: %s \n", url);

    if (!strcmp(url, "/cgi-bin/home.cgi") || !strcmp(url, "/home.cgi") || \
        strstr(url, "tbScancodeApproach") || strstr(url, "?extra_params"))
    {
        //some browser default add home.cgi
        url[1] = '\0';
    }

    sprintf(path, "%s", url);

    if (path[strlen(path) - 1] == '/')
    {


        int connect = httpd_callback(HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS, NULL, NULL);
        if (!connect)
        {
#ifdef WEB_CSTM_P1
            strcat(path, "wlan.html");
#else
            strcat(path, "guide.html");
#endif
        }
        else
        {
            strcat(path, "index.html");
        }
    }

    serve_file(client, path);

    close(client);
    pthread_detach(pthread_self());
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int hccast_httpd_startup(unsigned short *port)
{
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
    {
        error_die("socket");
        return httpd;
    }
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    int on = 1;
    if ((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
        //if ((setsockopt(httpd, SOL_SOCKET,SO_REUSEPORT, &on, sizeof(on))) < 0)
    {
        error_die("setsockopt failed");
    }

    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");

    if (*port == 0)   /* if dynamically allocating a port */
    {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) ==
            -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }

    if (listen(httpd, 5) < 0)
        error_die("listen");
    return (httpd);
}

/**********************************************************************/

static int g_httpd_is_start = 0;

int hccast_httpd_main(void)
{
    int ret = -1;
    int server_sock = -1;
    unsigned short port = 80;
    int client_sock = -1;
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);
    pthread_t newthread;

    server_sock = hccast_httpd_startup(&port);
    hccast_log(LL_NOTICE, "httpd running on port %d\n", port);
    if (server_sock == -1)
    {
        hccast_log(LL_ERROR, "httpd startup failed, server_sock=%d\n", server_sock);
        return 0;
    }

    struct timeval tv = { 0, 500 * 1000};
    fd_set rfd;

    while (g_httpd_is_start)
    {
        FD_ZERO(&rfd);
        FD_SET(server_sock, &rfd);
        tv.tv_usec = 500 * 1000;

        ret = select(server_sock + 1, &rfd, NULL, NULL, &tv);
        if (ret < 0)
        {
            if (EINTR == errno)
                continue;

            perror("httpd select failed!\n");
            break;
        }
        else if (ret == 0)
        {
            continue;
        }
        else if (FD_ISSET(server_sock, &rfd))
        {
            client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
            if (client_sock == -1)
            {
                error_die("accept");
            }

            /* accept_request(client_sock); */
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, 0x3000);
            if (pthread_create(&newthread, &attr, (void*)accept_request, (void*)client_sock) != 0)
            {
                perror("pthread_create");
            }
        }
    }

    hccast_log(LL_INFO, "httpd end!\n");

    g_httpd_is_start = 0;
    close(server_sock);
    //pthread_detach(pthread_self());

    return (0);
}

int hccast_httpd_service_is_start()
{
    return g_httpd_is_start;
}

int hccast_httpd_service_start()
{
    g_httpd_is_start = 1;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    if (pthread_create(&g_httpd_tid, &attr, (void*)hccast_httpd_main, NULL) != 0)
    {
        perror("httpd_main create failed!");
        return -1;
    }

    return 0;
}

int hccast_httpd_service_stop()
{
    if (g_httpd_is_start)
    {
        g_httpd_is_start = 0;
        pthread_join(g_httpd_tid, NULL);
    }

    return 0;
}


int hccast_httpd_service_init(hccast_httpd_callback func)
{
    httpd_callback = func;
}

int hccast_httpd_service_uninit()
{
    httpd_callback = NULL;
}
