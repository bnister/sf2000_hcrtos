#ifndef __HCCAST_WEB_PACKAGE_H__
#define __HCCAST_WEB_PACKAGE_H__

#define HTTPD_WEB_FILE_NAME_MAX_LEN  64
#define HTTPD_WEB_FILE_MAX_NUM   64
#define HTTPD_WEB_PACKAGE_HEADER_LEN     (8 * 1024)

#define WEB_BIN_CHUNK_ID    (0x01FE0203)
#define WEB_CODE_MAX_LEN    (300 * 1024)

typedef struct
{
    unsigned char name[HTTPD_WEB_FILE_NAME_MAX_LEN];
    unsigned int offset;
    unsigned int size;
} httpd_web_file_st;

typedef struct
{
    unsigned char file_num;
    httpd_web_file_st file_list[HTTPD_WEB_FILE_NAME_MAX_LEN];
} httpd_web_package_header_st;

int httpd_web_load_page();
int httpd_web_free_page();

#endif
