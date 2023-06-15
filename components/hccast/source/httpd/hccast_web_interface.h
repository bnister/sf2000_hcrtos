#ifndef __HCCAST_WEB_INTERFACE_H__
#define __HCCAST_WEB_INTERFACE_H__

#define BUF_LEN 512//1024
#define STR_LEN 255
#define PATH_LEN 512

#define IP_LEN  18
#define SERVER_LEN  128

typedef struct
{
    unsigned char *request;
    int (*process_func)(int, char *);
} web_interface_st;

typedef struct
{
    unsigned char *request;
    unsigned char *param_key;
    int resp_type;//0 -- string, 1 -- page.
} web_request_blacklist_st;


void web_response_ok(int client, const char *filename);
void web_server_error(int client, int error_code);
void web_headers_ex(int client, const char *filename, unsigned char *content_type);
int web_interface_request(int client, char *request, char *param);
int web_interface_request_blacklist_check(int client, char *request, char *param);
int web_response_server_str(int client, char *str_hint);
void page_hint(int client, char *str_hint);


#endif
