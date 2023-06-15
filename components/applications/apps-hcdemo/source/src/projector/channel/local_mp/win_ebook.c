#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_ebook.h"
#include "../../screen.h"
#include <hcuapi/mmz.h>
#define EBOOK_MALLOC_SIZE 2048
#define utf8_char_len(c) ((((int)0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1)
#define utf8_to_u_hostendian(str, uni_str, err_flag) \
{\
	err_flag = 0;\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		err_flag = 1;\
	}\
}


extern lv_group_t * ebook_group;
extern lv_obj_t * ui_ebook_label;
extern lv_obj_t * ui_ebook_label_page;

extern file_list_t *m_cur_file_list;
extern file_list_t  m_file_list[MEDIA_TYPE_COUNT]; 

typedef struct line_data{
	UINT32 line_number;
	UINT32 line_byte;
}line_data_t;

typedef enum FP_TYPE
{
	UTF8_TYPE=1,
	UTF16_BE_TYPE,	
	UTF16_LE_TYPE,	
	TYPE_NULL,
}FP_TYPE_T;

static FILE *fp_ebook =NULL;
static UINT8  cur_fp_type=0;
//static line_data_t ebook_line_info[65536]={0};
static line_data_t *ebook_line_info= NULL;

static UINT16 ebook_page_line=15;
static UINT16 ebook_line_max_bytes=80;
static UINT32 lseek_num=0;
static long	file_ebook_size=0;
static long	file_ebook_seek_size=0;
static UINT32 cur_page=1;
static UINT32 page_all;
static UINT32 ebook_all_line = 0;
static long	file_tmp=0;
static UINT16 *get_buff_mul = NULL;
static UINT16 *str_mul = NULL;


void ebook_get_fullname(char *fullname, char *path, char *name)
{
    strcpy(fullname, path);
    strcat(fullname, "/");
    strcat(fullname, name);
}

void ebook_free_buff(void)
{
		if(get_buff_mul!=NULL)
		{
			free(get_buff_mul);
			get_buff_mul = NULL;
		}
		if(str_mul!=NULL)
		{
			free(str_mul);
			str_mul = NULL;
		}
		if(ebook_line_info != NULL)
		{
			free(ebook_line_info);
			ebook_line_info = NULL;
		}
}
void ebook_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if(code == LV_EVENT_KEY)
    {
        int keypad_value = lv_indev_get_key(lv_indev_get_act());
        if(keypad_value == LV_KEY_ESC)
        {
			ebook_free_buff();
            _ui_screen_change(ui_fspage, 0, 0);
        }
		else if(keypad_value == LV_KEY_DOWN)
		{
			change_ebook_txt_info(keypad_value);
		}
		else if(keypad_value == LV_KEY_UP)
		{
			change_ebook_txt_info(keypad_value);
		}
    }
}

/* Convert UTF-16 to UTF-8.  */
uint8_t *utf16_to_utf8_t(uint8_t *dest, const uint16_t *src, size_t size)
{
	uint32_t code_high = 0;

	while (size--) {
		uint32_t code = *src++;

		if (code_high) {
			if (code >= 0xDC00 && code <= 0xDFFF) {
				/* Surrogate pair.  */
				code = ((code_high - 0xD800) << 10) + (code - 0xDC00) + 0x10000;

				*dest++ = (code >> 18) | 0xF0;
				*dest++ = ((code >> 12) & 0x3F) | 0x80;
				*dest++ = ((code >> 6) & 0x3F) | 0x80;
				*dest++ = (code & 0x3F) | 0x80;
			} else {
				/* Error...  */
				*dest++ = '?';
				/* *src may be valid. Don't eat it.  */
				src--;
			}

			code_high = 0;
		} else {
			if (code <= 0x007F) {
				*dest++ = code;
			} else if (code <= 0x07FF) {
				*dest++ = (code >> 6) | 0xC0;
				*dest++ = (code & 0x3F) | 0x80;
			} else if (code >= 0xD800 && code <= 0xDBFF) {
				code_high = code;
				continue;
			} else if (code >= 0xDC00 && code <= 0xDFFF) {
				/* Error... */
				*dest++ = '?';
			} else if (code < 0x10000) {
				*dest++ = (code >> 12) | 0xE0;
				*dest++ = ((code >> 6) & 0x3F) | 0x80;
				*dest++ = (code & 0x3F) | 0x80;
			} else {
				*dest++ = (code >> 18) | 0xF0;
				*dest++ = ((code >> 12) & 0x3F) | 0x80;
				*dest++ = ((code >> 6) & 0x3F) | 0x80;
				*dest++ = (code & 0x3F) | 0x80;
			}
		}
	}

	return dest;
}


UINT32 ComUniStrToMB(UINT16* pwStr)
 {
 	if(pwStr == NULL)
		return 0;
	UINT32 i=0;
	while(pwStr[i])
	{
		pwStr[i]=(UINT16)(((pwStr[i]&0x00ff)<<8) | ((pwStr[i]&0xff00)>>8));
		i++;
	}
	return i;

 }

int ComUniStrCopyChar(UINT8 *dest, UINT8 *src)
{	
     unsigned int i;

	if((NULL == dest) || (NULL == src))
		return 0;
	
     for(i=0; !((src[i] == 0x0 && src[i+1] == 0x0)&&(i%2 == 0)) ;i++)
         dest[i] = src[i];
     if(i%2)
     {
        dest[i] = src[i];
        i++;
     }
     dest[i] = dest[i+1] = 0x0;
 
     return i/2;
}

void readtyped(FILE *fp_read,int n)
{
	unsigned char *buff = (unsigned char*)malloc(sizeof(unsigned char)*n);
	if(fp_read == NULL)
	{
		free(buff);
		buff	=	NULL;
		return;
	}
	fread(buff,sizeof(unsigned char),n,fp_read);
	ebook_page_line = 15;
	if(buff[0] == 0xef && buff[1] == 0xbb)
		cur_fp_type =  UTF8_TYPE;
	else if(buff[0] == 0xff && buff[1] == 0xfe)
	{
		cur_fp_type =  UTF16_LE_TYPE;
		ebook_page_line = 10;
	}
	else if(buff[0] == 0xfe && buff[1] == 0xff)
	{
		cur_fp_type =	UTF16_BE_TYPE;
		ebook_page_line = 10;
	}
	else
		cur_fp_type = TYPE_NULL;
	free(buff);
	buff = NULL;
}

UINT32  fgetws_ex(char *string, int n, FILE *fp)
{
    if(!string || !fp || feof(fp) || n <=0)
    {
    //    return 0;
    }
    int i = 0;
    while(i < n-1 && !feof(fp))
    {
    	fread(&string[i], sizeof(UINT16), 1, fp);
        if((string[i-2] == 0x0d &&string[i-1]==0x0&&string[i] == 0x0a &&string[i+1]==0x0) || (string[i] == '\0' && string[i+1]=='\n'))
        {
            i+=2;
            break;
        }
        i+=2;
    }
    string[i] = 0x0;
    return i;//string;
}

void ebook_show_page_info(void)
{
	char page_info[64]={0};
	sprintf(page_info,"%ld / %ld",cur_page,page_all);
	lv_label_set_text(ui_ebook_label_page,page_info);
}

static void change_ebook_txt_info(int keypad_value)
{
	UINT32 lseek_num_tmp=0;
	if(keypad_value == LV_KEY_DOWN)
	{
		if(cur_page == page_all)
			return;
		lseek_num_tmp = 0;
		cur_page++;
		if(cur_page == page_all)
		{
			lseek_num_tmp = file_ebook_size;
			lseek_num = lseek_num_tmp;
		}
		else
		{
			lseek_num_tmp = ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte-ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte;
			lseek_num = lseek_num_tmp;
		}
	}
	else if(keypad_value == LV_KEY_UP)
	{
		if(cur_page==1)
		{
			return;
		}
		cur_page--;
		lseek_num_tmp = 0;
		if(cur_page == 1)
			lseek_num_tmp = ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte;
		else
			lseek_num_tmp = ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte - ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte;
		lseek_num = lseek_num_tmp;
		file_ebook_size = file_ebook_seek_size+lseek_num_tmp;
		fseek(fp_ebook,file_tmp-file_ebook_size,SEEK_SET);
		if(cur_page == 1)
		{
			if(cur_fp_type ==  UTF8_TYPE || cur_fp_type ==  UTF16_LE_TYPE || cur_fp_type ==  UTF16_BE_TYPE)
				fseek(fp_ebook,2,SEEK_SET);
		}
	}
	memset(str_mul, 0, EBOOK_MALLOC_SIZE*2);
	memset(get_buff_mul, 0, EBOOK_MALLOC_SIZE);
	fread((UINT8 *)get_buff_mul,lseek_num,1,fp_ebook);
	file_ebook_seek_size = file_ebook_size;
	file_ebook_size -= lseek_num;
	if(cur_fp_type ==  UTF8_TYPE)
	{
		printf("UTF8\n");
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(cur_fp_type ==  UTF16_LE_TYPE)
	{
		printf("feff\n");
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,EBOOK_MALLOC_SIZE);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,lseek_num);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(cur_fp_type ==  UTF16_BE_TYPE)
	{
		printf("fffe\n");
		ComUniStrToMB(get_buff_mul);
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,EBOOK_MALLOC_SIZE);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,lseek_num);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else
	{
		printf("other\n");
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	ebook_show_page_info();
}


void ebook_read_file(void)
{
	struct stat  e_sa;
	char ebook_file_name[512]={0};
	char read_line[1024] = {0};
	file_node_t *file_node = NULL;
	UINT32 read_tmp=0;
	int i=0;
	UINT16 line_size = 0;
	file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
	ebook_get_fullname(ebook_file_name,m_cur_file_list->dir_path,file_node->name);
	fp_ebook = fopen(ebook_file_name,"rb");
	if(fp_ebook == NULL)
	{
		printf("%s,%d,open %s file failed\n",__func__,__LINE__,ebook_file_name);
		return;
	}
	if(stat(ebook_file_name,&e_sa) == -1)
	{
		printf("stat failed\n");
		return;
	}
	if(get_buff_mul == NULL ||str_mul == NULL)
	{
		get_buff_mul =	(UINT16  *)malloc(EBOOK_MALLOC_SIZE);
		str_mul =	(UINT16 *)malloc(EBOOK_MALLOC_SIZE*2);
	}
	printf("%d,%x\n",__LINE__,str_mul);
	memset(str_mul, 0, EBOOK_MALLOC_SIZE*2);
	memset(get_buff_mul, 0, EBOOK_MALLOC_SIZE);
	cur_page = 1;
	lseek_num = 0;
	file_ebook_seek_size = 0;
	file_ebook_size = 0;
	fseek(fp_ebook,0,SEEK_SET);
	readtyped(fp_ebook,3);
	file_ebook_size = (long int)e_sa.st_size;
	file_tmp=file_ebook_size;
	file_ebook_seek_size = file_ebook_size;
	
	if(cur_fp_type ==  UTF16_LE_TYPE || cur_fp_type ==  UTF16_BE_TYPE)
	{
		while((line_size = fgetws_ex(read_line,1024,fp_ebook)) >0)
		{
			//line_size = my_unistrlen(read_line);
			if(line_size>ebook_line_max_bytes)
			{
				read_tmp += line_size/ebook_line_max_bytes;
				if(line_size%ebook_line_max_bytes)
				{
					read_tmp ++;	
				}
			}
			else
			{
				read_tmp++;
			}
			memset(read_line,0,1024);
		}
	}
	else
	{
		while(fgets(read_line,1024,fp_ebook))
		{
			line_size = strlen(read_line);
			if(line_size>ebook_line_max_bytes)
			{
				read_tmp += line_size/ebook_line_max_bytes;
				if(line_size%ebook_line_max_bytes)
				{
					read_tmp ++;
				}
			}
			else
			{
				read_tmp++;
			}
			memset(read_line,0,sizeof(read_line));
		}
	}
	ebook_line_info = mmz_malloc(0,read_tmp);
	printf("%d,size:%ld\n",__LINE__,read_tmp);
	read_tmp = 0;
	fseek(fp_ebook,0,SEEK_SET);
	if(cur_fp_type ==  UTF16_LE_TYPE || cur_fp_type ==  UTF16_BE_TYPE)
	{
		while((line_size = fgetws_ex(read_line,1024,fp_ebook)) >0)
		{
			//line_size = my_unistrlen(read_line);
			if(line_size>ebook_line_max_bytes)
			{
				for(i=0;i<line_size/ebook_line_max_bytes;i++)
				{
					ebook_line_info[read_tmp+i].line_number = read_tmp+i;
					lseek_num += ebook_line_max_bytes;
					ebook_line_info[read_tmp+i].line_byte = lseek_num;
				}
				read_tmp += line_size/ebook_line_max_bytes;
				if(line_size%ebook_line_max_bytes)
				{
					ebook_line_info[read_tmp].line_number = read_tmp;
					lseek_num += line_size%ebook_line_max_bytes;
					ebook_line_info[read_tmp].line_byte = lseek_num;
					read_tmp ++;	
				}
			}
			else
			{
				ebook_line_info[read_tmp].line_number = read_tmp;
				lseek_num += line_size;
				ebook_line_info[read_tmp].line_byte = lseek_num;
				read_tmp++;
			}
			memset(read_line,0,1024);
		}
	}
	else
	{
		//line_size = strlen(read_line);
		while(fgets(read_line,1024,fp_ebook))
		{
			line_size = strlen(read_line);
			if(line_size>ebook_line_max_bytes)
			{
				for(i=0;i<line_size/ebook_line_max_bytes;i++)
				{
					ebook_line_info[read_tmp+i].line_number = read_tmp+i;
					lseek_num += ebook_line_max_bytes;
					ebook_line_info[read_tmp+i].line_byte = lseek_num;
				}
				read_tmp += line_size/ebook_line_max_bytes;
				if(line_size%ebook_line_max_bytes)
				{
					ebook_line_info[read_tmp].line_number = read_tmp;
					lseek_num += line_size%ebook_line_max_bytes;
					ebook_line_info[read_tmp].line_byte = lseek_num;
					read_tmp ++;
					
				}
			}
			else
			{
				ebook_line_info[read_tmp].line_number = read_tmp;
				lseek_num += line_size;
				ebook_line_info[read_tmp].line_byte = lseek_num;
				read_tmp++;
			}
			memset(read_line,0,sizeof(read_line));
		}
	}
	ebook_all_line = read_tmp;
	page_all = read_tmp/ebook_page_line;
	if(read_tmp % ebook_page_line)
		page_all++;
	if(cur_fp_type ==  UTF8_TYPE || cur_fp_type ==  UTF16_LE_TYPE || cur_fp_type ==  UTF16_BE_TYPE)
		fseek(fp_ebook,2,SEEK_SET);
	else 
		fseek(fp_ebook,0,SEEK_SET);
	if(cur_page == 1)
	{
		//fseek(fp_ebook,0,SEEK_SET);
		if(read_tmp < ebook_page_line)
		{
			fread((UINT8 *)get_buff_mul,ebook_line_info[read_tmp-1].line_byte,1,fp_ebook);
			file_ebook_size -= ebook_line_info[read_tmp-1].line_byte;
		}
		else
		{
			fread((UINT8 *)get_buff_mul,ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte,1,fp_ebook);
			file_ebook_size-=ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte;
		}
	}
	
	else
	{
		fseek(fp_ebook,ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte,SEEK_SET);
		if(cur_page == page_all)
		{
			fread((UINT8 *)get_buff_mul,ebook_line_info[read_tmp-1].line_byte-ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte,1,fp_ebook);
			file_ebook_size-=ebook_line_info[read_tmp-1].line_byte;
		}
		else
		{
			fread((UINT8 *)get_buff_mul,ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte-ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte,1,fp_ebook);
			file_ebook_size-=ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte;
		}
	}
	if(cur_fp_type ==  UTF8_TYPE)
	{
		printf("UTF8\n");
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(cur_fp_type ==  UTF16_LE_TYPE)
	{
		printf("feff\n");
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,EBOOK_MALLOC_SIZE);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(cur_fp_type ==  UTF16_BE_TYPE)
	{
		printf("fffe\n");
		ComUniStrToMB((UINT16 *)get_buff_mul);
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,EBOOK_MALLOC_SIZE);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else
	{
		printf("other\n");
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	read_tmp=0;
	file_ebook_seek_size = file_ebook_size;
	ebook_show_page_info();
}

void ebook_open(void)
{
    ebook_group= lv_group_create();
    set_key_group(ebook_group);
    lv_group_add_obj(ebook_group,ui_ebook_label);
    lv_obj_add_event_cb(ui_ebook_label,ebook_keyinput_event_cb,LV_EVENT_KEY, sub_group);
	ebook_read_file();
}

void ebook_close(void)
{
	lv_group_remove_all_objs(ebook_group);
    lv_group_del(ebook_group);
    //test
    lv_obj_remove_event_cb(ui_ebook_label,ebook_keyinput_event_cb);
}

