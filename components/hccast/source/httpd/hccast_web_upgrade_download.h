#ifndef _HCCAST_WEB_UPGRADE_DOWNLOAD_H
#define _HCCAST_WEB_UPGRADE_DOWNLOAD_H


typedef struct
{
    int status_code;//HTTP/1.1 '200' OK
    long content_length;//Content-Length
}hccast_http_res_header_t;




void hccast_web_uprade_download_start(char *url);
int hccast_web_get_upgrade_status();
void hccast_web_set_upgrade_status(int status);


#endif
