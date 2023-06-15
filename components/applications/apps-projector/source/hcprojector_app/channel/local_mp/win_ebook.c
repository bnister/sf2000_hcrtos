#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_fspage.h"
#include "mp_ebook.h"
#include "screen.h"
#include <hcuapi/mmz.h>
#include "gb_2312.h"
#include "mp_ctrlbarpage.h"
#include "mp_playlist.h"
#include "backstage_player.h"

// #define ebook_malloc_size 4095
static uint32_t ebook_malloc_size = 2048;
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
//extern lv_obj_t * lv_page_num;

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

typedef struct
{
	char txt_path[258];
	UINT32	read_page_num;
}txt_ebook_data;


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
static txt_ebook_data txt_data;
static bool check_utf8=false;
//for ui func param
bool is_ebook_bgmusic= false;
lv_timer_t * bar_timer=NULL;


UINT32 ComAscStr2Uni(UINT8* Ascii_str,UINT16* Uni_str)
{
   UINT32 i=0;

   if((Ascii_str==NULL)||(Uni_str==NULL))
       return 0;
   while(Ascii_str[i])
   {
#if(SYS_CPU_ENDIAN==ENDIAN_LITTLE)
       Uni_str[i]=(UINT16)(Ascii_str[i]<<8);
#else
       Uni_str[i]=(UINT16)Ascii_str[i];
#endif
       i++;
   }

   Uni_str[i]=0;
   return i;
}
UINT16 *ComStr2UniStrExt(UINT16* uni, char* str, UINT16 maxcount)
{	
	UINT16 i;

	if (uni == NULL)
		return NULL;

	if (str == NULL || maxcount == 0)
	{
		uni[0]=(UINT16)'\0';
		return NULL;
	}

	for(i=0; (0!=str[i])&&(i<maxcount); i++)
#if (SYS_CPU_ENDIAN==ENDIAN_LITTLE)
       uni[i]=(UINT16)(str[i]<<8);
#else
       uni[i]=(UINT16)str[i];
#endif
	uni[i]=(UINT16)'\0';

	return uni;
}

#define u_hostendian_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = (UINT8)*uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|(*uni_str++&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|(*uni_str++&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}

bool IsUTF8(const void* pBuffer, long size)
{
    bool IsUTF8 = true;
	int error_time=0;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;
    while (start < end)
    {
    	//printf("%x\n",*start);
        if (*start < 0x80) // (10000000): 值小于0x80的为ASCII字符
        {
            start++;
        }
        else if (*start < (0xC0)) // (11000000): 值介于0x80与0xC0之间的为无效UTF-8字符
        {
        	error_time++;
			start++;
			if(error_time>3)
			{
	            IsUTF8 = false;
	            break;
			}
        }
        else if (*start < (0xE0)) // (11100000): 此范围内为2字节UTF-8字符
        {
            if (start >= end - 1)
            {
                break;
            }


            if ((start[1] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }


            start += 2;
        }
        else if (*start < (0xF0)) // (11110000): 此范围内为3字节UTF-8字符
        {
            if (start >= end - 2)
            {
                break;
            }

            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }

            start += 3;
        }
        else
        {
            IsUTF8 = false;
            break;
        }
    }


    return IsUTF8;
}


static int unicode_to_utf8(
	const UINT16 *src,
	int *srcLen,
	char *dst,
	int *dstLen)
{
	int result;
	int origlen = *srcLen;

	int srcLimit = *srcLen;
	int dstLimit = *dstLen;
	int srcCount = 0;
	int dstCount = 0;

	for (srcCount = 0; srcCount < srcLimit; srcCount++)
	{
		unsigned short  *UNICODE = (unsigned short *) & src[srcCount];
		unsigned char	utf8[4];
		unsigned char	*UTF8 = utf8;
		int utf8Len;
		int j;

		u_hostendian_to_utf8(UTF8, UNICODE);

		utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	result = ((dstCount > 0) ? 0 : -1);

	if (*srcLen < origlen)
	{
		return -1;
	}

	return result;
}

UINT32 ComUniStrLen(const UINT16* string)
{
	UINT32 i=0;

	if(string == NULL)
		return 0;
	
	while (string[i])
		i++;
	return i;
}

INT32 ComUniStr2UTF8(UINT16* Uni_str, UINT8* utf8,unsigned long utf8len)
{
	INT32 result;
	unsigned long unilen;


	unilen = ComUniStrLen(Uni_str) + 1;

	result = unicode_to_utf8(Uni_str, (int *) & unilen, utf8, (int *) & utf8len);

	return result;
}

void ebook_get_fullname(char *fullname, char *path, char *name)
{
    strcpy(fullname, path);
    strcat(fullname, "/");
    strcat(fullname, name);
}
void change_ebook_cur_page(lv_event_t *event,int vkey)
{
	#if 0
	char txt[64]={0};
	lv_obj_t * ta = lv_event_get_target(event);
    lv_obj_t * kb = lv_event_get_user_data(event);
	if(lv_obj_has_flag(lv_page_num,LV_OBJ_FLAG_HIDDEN))
		lv_obj_clear_flag(lv_page_num, LV_OBJ_FLAG_HIDDEN);
	sprintf(txt,"%d",vkey);
	printf("%s,%s\n",__func__,txt);
	lv_textarea_add_text(kb, txt);
	
	printf("%s,%d\n",__func__,vkey);
	#endif
}

void ebook_free_buff(void)
{
	txt_data.read_page_num = cur_page;
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
bool get_ebook_fp_state(void)
{
	if(fp_ebook)
		return true;
	return false;
}

void win_ebook_close(void)
{
	printf("%s\n",__func__);
	if(NULL == fp_ebook)
		return;

	check_utf8=false;
	ebook_free_buff();
	fclose(fp_ebook);
	fp_ebook = NULL;
}

void ebook_keyinput_page_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t * obj = lv_event_get_target(event);
    lv_obj_t * ta = lv_event_get_user_data(event); 
	const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
	sprintf(txt,"999");
	printf("%s,%s\n",__func__,txt);
	lv_textarea_add_text(ta, txt);
}

/*ebook page key ctrl in here
*  
*/
void ebook_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t * target=lv_event_get_target(event);
    if(code==LV_EVENT_PRESSED){
        lv_obj_clear_state(target,LV_STATE_PRESSED);
	}else if(code == LV_EVENT_KEY){
        uint32_t keypad_value = lv_indev_get_key(lv_indev_get_act());
		uint32_t vkey = key_convert2_vkey(keypad_value);
		switch (keypad_value){
			case LV_KEY_UP :            	
				if (m_play_bar_show==0)
			   		ctrl_bar_enter(ctrlbarbtn[0]);
				break;
			case LV_KEY_DOWN :
				if(m_play_bar_show==0)
					ctrl_bar_enter(ctrlbarbtn[1]);
				break;
			case LV_KEY_RIGHT :
				lv_group_focus_next(lv_group_get_default());
				break;
			case LV_KEY_LEFT :
				lv_group_focus_prev(lv_group_get_default());
				break;
			case LV_KEY_ESC :
            	if (m_play_bar_show){
                    show_play_bar(false);
                }else{
                    _ui_screen_change(ui_fspage,0,0);
                }
                break;
			case LV_KEY_ENTER:
				ctrl_bar_enter(target);
				break;
			case LV_KEY_NEXT:
                ctrl_bar_enter(ctrlbarbtn[3]);
                break;
            case LV_KEY_PREV:
                ctrl_bar_enter(ctrlbarbtn[2]);
                break;
			default :
				break;
		}
		//show bar or not
		//barbtn4->stop  mean Esc ctrlpage
		if(target==ctrlbarbtn[4])
			return ;
        else if (keypad_value!=LV_KEY_ESC&&keypad_value!=LV_KEY_UP&&keypad_value!=LV_KEY_DOWN){   
            show_play_bar(true);
            lv_timer_reset(bar_timer);
        }else if(vkey==V_KEY_STOP){
			_ui_screen_change(ui_fspage,0,0);
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
	txt_data.read_page_num = cur_page;
	sprintf(page_info,"%ld / %ld",cur_page,page_all);
	lv_label_set_text(ui_ebook_label_page,page_info);
}

void change_ebook_txt_info(int keypad_value)
{
	UINT32 lseek_num_tmp=0;
	int i;
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
	if(lseek_num*2>ebook_malloc_size){
		if(str_mul!=NULL){
			free(str_mul);
			str_mul=NULL;
			ebook_malloc_size=lseek_num*2;
			str_mul =	(UINT16 *)malloc(ebook_malloc_size*2);		
		}
		if(get_buff_mul!=NULL)
		{
			free(get_buff_mul);
			get_buff_mul=NULL;
			ebook_malloc_size=lseek_num*2;
			get_buff_mul =	(UINT16  *)malloc(ebook_malloc_size);
		}
	}
	memset(str_mul, 0, ebook_malloc_size*2);
	memset(get_buff_mul, 0, ebook_malloc_size);
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
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,lseek_num);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(cur_fp_type ==  UTF16_BE_TYPE)
	{
		printf("fffe\n");
		ComUniStrToMB(get_buff_mul);
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,lseek_num);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else
	{
		if(check_utf8)
		{
			lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
		}
		else
		{
			convert_gb2312_to_unicode((UINT8 *)get_buff_mul,lseek_num_tmp,str_mul,ebook_malloc_size);
			ComUniStrToMB((UINT16 *)str_mul);
			memset(get_buff_mul,0,ebook_malloc_size);
			ComUniStrCopyChar(  (UINT8 *)get_buff_mul,(UINT8 *)str_mul);
			memset(str_mul,0,ebook_malloc_size);
			utf16_to_utf8_t((UINT8 *)str_mul,get_buff_mul,lseek_num_tmp);
			lv_label_set_text(ui_ebook_label,(char *)str_mul);
		}
	}
	ebook_show_page_info();
}


void ebook_read_file(char *ebook_file_name)
{
	struct stat  e_sa;
	// char ebook_file_name[MAX_FILE_NAME]={0};
	char read_line[1024] = {0};
	// file_node_t *file_node = NULL;
	UINT32 read_tmp=0;
	int i=0;
	UINT32 page_size=0;
	UINT16 line_size = 0;
	
	/*1、free memory */
	// ebook_free_buff();
	win_ebook_close();

	/*2、open file */
	// file_node = file_mgr_get_file_node(m_cur_file_list, filelist_index);
	// ebook_get_fullname(ebook_file_name,m_cur_file_list->dir_path,file_node->name);
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
		get_buff_mul =	(UINT16  *)malloc(ebook_malloc_size);
		str_mul =	(UINT16 *)malloc(ebook_malloc_size*2);
	}
	// printf("%d,%x\n",__LINE__,str_mul);
	memset(str_mul, 0, ebook_malloc_size*2);
	memset(get_buff_mul, 0, ebook_malloc_size);
	cur_page = 1;
	lseek_num = 0;
	file_ebook_seek_size = 0;
	file_ebook_size = 0;
	fseek(fp_ebook,0,SEEK_SET);
	readtyped(fp_ebook,3);
	file_ebook_size = (long int)e_sa.st_size;
	file_tmp=file_ebook_size;
	file_ebook_seek_size = file_ebook_size;
	if(memcmp(ebook_file_name,txt_data.txt_path,strlen(ebook_file_name)) == 0)
	{
		cur_page = txt_data.read_page_num;
	}
	else
	{
		memset(&txt_data,0,sizeof(txt_ebook_data));
		strcpy(txt_data.txt_path,ebook_file_name);
		txt_data.read_page_num = cur_page;
	}
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
	ebook_line_info = malloc((read_tmp+1)*sizeof(line_data_t));
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
			page_size = ebook_line_info[read_tmp-1].line_byte;
			file_ebook_size -= ebook_line_info[read_tmp-1].line_byte;
		}
		else
		{
			fread((UINT8 *)get_buff_mul,ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte,1,fp_ebook);
			page_size = ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte;
			file_ebook_size-=ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte;
		}
	}
	
	else
	{
		fseek(fp_ebook,ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte,SEEK_SET);
		if(cur_page == page_all)
		{
			fread((UINT8 *)get_buff_mul,ebook_line_info[read_tmp-1].line_byte-ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte,1,fp_ebook);
			page_size = ebook_line_info[read_tmp-1].line_byte-ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte;
			file_ebook_size-=ebook_line_info[read_tmp-1].line_byte;
		}
		else
		{
			fread((UINT8 *)get_buff_mul,ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte-ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte,1,fp_ebook);
			page_size = ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte-ebook_line_info[((cur_page-1)*ebook_page_line)-1].line_byte;
			file_ebook_size-=ebook_line_info[((cur_page)*ebook_page_line)-1].line_byte;
		}
	}
	if(page_size*2>ebook_malloc_size){
		if(str_mul!=NULL){
			free(str_mul);
			str_mul=NULL;
			ebook_malloc_size=page_size*2;
			str_mul =	(UINT16 *)malloc(ebook_malloc_size*2);		
		}
		if(get_buff_mul!=NULL)
		{
			free(get_buff_mul);
			get_buff_mul=NULL;
			ebook_malloc_size=page_size*2;
			get_buff_mul =	(UINT16  *)malloc(ebook_malloc_size);
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
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,page_size);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(cur_fp_type ==  UTF16_BE_TYPE)
	{
		printf("fffe\n");
		ComUniStrToMB((UINT16 *)get_buff_mul);
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,page_size);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else
	{
		printf("other\n");
		
		if(IsUTF8(get_buff_mul,page_size))
		{
			check_utf8 = true;
			lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
		}
		else
		{
			convert_gb2312_to_unicode((UINT8 *)get_buff_mul,page_size,str_mul,ebook_malloc_size);
			ComUniStrToMB((UINT16 *)str_mul);
			memset(get_buff_mul,0,ebook_malloc_size);
			ComUniStrCopyChar(  (UINT8 *)get_buff_mul,(UINT8 *)str_mul);
			memset(str_mul,0,ebook_malloc_size);
			utf16_to_utf8_t((UINT8 *)str_mul,get_buff_mul,page_size);
			lv_label_set_text(ui_ebook_label,(char *)str_mul);
		}
	}
	read_tmp=0;
	file_ebook_seek_size = file_ebook_size;
	file_ebook_seek_size +=	page_size;
	ebook_show_page_info();
}
void ebook_open(void)
{
	ebook_group= lv_group_create();
	ebook_group->auto_focus_dis = 1;
    set_key_group(ebook_group);
	create_ebook_scr(ui_ebook_txt,ebook_keyinput_event_cb);
	char ebook_file_name[MAX_FILE_NAME]={0};
	file_node_t *file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
	ebook_get_fullname(ebook_file_name,m_cur_file_list->dir_path,file_node->name);
	lv_label_set_text(ui_playname,file_node->name);
	ebook_read_file(ebook_file_name);	//start to read file
	playlist_init();
	bar_timer=lv_timer_create(bar_show_timer_cb, 5000, NULL);
}

void ebook_close(void)
{
	lv_group_remove_all_objs(ebook_group);
    lv_group_del(ebook_group);

	lv_timer_pause(bar_timer);
    lv_timer_del(bar_timer);

	playlist_deinit();

	if(is_ebook_bgmusic==true){
		is_ebook_bgmusic=false;
		backstage_player_task_stop(0,NULL);
	}
    win_ebook_close();
	lv_obj_remove_event_cb(ui_ebook_label,ebook_keyinput_event_cb);
	lv_obj_clean(lv_scr_act());//del all child obj 
}

