#ifndef __MP_EBOOK_H_
#define __MP_EBOOK_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef char			INT8;
typedef unsigned char	UINT8;

typedef short			INT16;
typedef unsigned short	UINT16;

typedef long			INT32;
typedef unsigned long	UINT32;

typedef unsigned long long UINT64;
typedef long long INT64;

typedef signed int INT;
typedef unsigned int UINT;


void ebook_open(void);
void ebook_close(void);
void ebook_keyinput_event_cb(lv_event_t *event);
static void change_ebook_txt_info(int keypad_value);
void ebook_get_fullname(char *fullname, char *path, char *name);
void readtyped(FILE *fp_read,int n);
UINT32  fgetws_ex(char *string, int n, FILE *fp);
void ebook_show_page_info(void);
static void change_ebook_txt_info(int keypad_value);
void ebook_read_file(void);



#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif

