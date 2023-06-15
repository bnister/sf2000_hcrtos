/**
 * file_mgr.c
 */

#include <dirent.h>
#include <sys/stat.h>
#include "com_api.h"
#include "glist.h"
#include "file_mgr.h"
#ifdef __linux__
#include <ctype.h>
#else
#include <linux/ctype.h>
#endif
#define _GNU_SOURCE

static file_type_t file_mgr_filter_file(media_type_t media_type, char *file_name);

/////////////////////////////////////////////////////////////////////////////
// compare_file_list_items
/////////////////////////////////////////////////////////////////////////////
/*
 *  Sort by Name.
 *   a < b  return <0; 
 *   a==b  return 0; 
 *   a > b  return > 0
 */
int fs_compare_func(void *a, void *b, void *user_data)
{
    file_node_t *pnode =NULL;
    char *pnm1,*pnm2;
    uint8_t c1, c2;
    pnode = (file_node_t *)a;
    pnm1 = pnode->name;
    pnode = (file_node_t *)b;
    pnm2 = pnode->name;
    do {
        c1 = tolower(*pnm1++);
        c2 = tolower(*pnm2++);
    } while (c1 && c1 == c2);
    return c1 - c2;
}

int compare_file_list_items(void *p1, void *p2, void *sort_mode)
{
	file_node_t *pnode =NULL;
    char *pnm1,*pnm2;
    //uint8_t c1, c2;
    pnode = (file_node_t *)p1;
    pnm1 = pnode->name;
    pnode = (file_node_t *)p2;
    pnm2 = pnode->name;
    while ((*pnm1 != '\0') && (*pnm2 != '\0'))
	{
		if (*pnm1 != *pnm2) 
		{
			break;
		}
		pnm1++;
		pnm2++;
	}
    return (int)((unsigned char)(*pnm1)) - (int)((unsigned char)(*pnm2));

}

//only free data,have to set data NULL
static void file_mgr_free_data(void *data, void *user_data)
{
	(void)user_data;
	if (data)
		free(data);
}

static int strcmp_c(const char *s1, const char *s2)
{
	int i;
	char c1, c2;

	for (i = 0, c1 = *s1, c2 = *s2; (c1 != '\0') && (c2 != '\0'); i++)
	{
		c1 = *(s1 + i);
		c2 = *(s2 + i);
		if ((c1 >= 'A') && (c1 <='Z')) c1 += 'a' - 'A';
		if ((c2 >= 'A') && (c2 <='Z')) c2 += 'a' - 'A';
		if (c1 != c2) break;
	}

	return (int)c1 - (int)c2;
}

static file_type_t file_mgr_filter_file(media_type_t media_type, char *file_name)
{
	int i;
	int j;
	char *ext_name = NULL;
	file_type_t file_type = FILE_OTHER;

	for (i = 0, j = -1; file_name[i] != '\0'; i++){
		if (file_name[i] == '.') j = i;
	}

	if (j == -1){
		return FILE_OTHER;
	}else{
		j++;
		ext_name = file_name + j;
	}

	if (MEDIA_TYPE_VIDEO == media_type){
		if (!strcmp_c(ext_name, "MPG") 		||
			!strcmp_c(ext_name, "MPEG") 	||
			!strcmp_c(ext_name, "DAT") 		||
			!strcmp_c(ext_name, "VOB") 		||
			!strcmp_c(ext_name, "AVI") 		||
			!strcmp_c(ext_name, "TS") 		||
			!strcmp_c(ext_name, "TRP") 		||
			!strcmp_c(ext_name, "TP") 		||
			!strcmp_c(ext_name, "M2T") 		||
			!strcmp_c(ext_name, "M2TS") 	||
			!strcmp_c(ext_name, "MTS") 		||
			!strcmp_c(ext_name, "MP4") 		||
			!strcmp_c(ext_name, "MKV") 		||
			!strcmp_c(ext_name, "MOV") 		||
			!strcmp_c(ext_name, "3GP") 		||
			!strcmp_c(ext_name, "FLV") 		||
			!strcmp_c(ext_name, "RMVB") 	||
			!strcmp_c(ext_name, "RM") 		||
			!strcmp_c(ext_name, "RAM") 		||
			!strcmp_c(ext_name, "WEBM") 	||
			!strcmp_c(ext_name, "H264") 	||
			!strcmp_c(ext_name, "ES") 		||
			!strcmp_c(ext_name, "WMV") 		||
			!strcmp_c(ext_name, "DIVX") 	||
			!strcmp_c(ext_name, "ASF") 		||
			!strcmp_c(ext_name, "M4V")		||
			!strcmp_c(ext_name, "F4V")
			)
			file_type = FILE_VIDEO;
	} else if (MEDIA_TYPE_MUSIC == media_type){
		if (!strcmp_c(ext_name, "MP3") 		||
			!strcmp_c(ext_name, "MP2") 		||
			!strcmp_c(ext_name, "MP1") 		||
			!strcmp_c(ext_name, "MPA") 		||
			!strcmp_c(ext_name, "OGG") 		||
			!strcmp_c(ext_name, "AAC") 		||
			!strcmp_c(ext_name, "AC3") 		||
			!strcmp_c(ext_name, "PCM") 		||
			!strcmp_c(ext_name, "WAV") 		||
			!strcmp_c(ext_name, "WMA") 		||
			!strcmp_c(ext_name, "FLAC") 	||
			!strcmp_c(ext_name, "MKA") 		||
			!strcmp_c(ext_name, "M4A") 	||	
			!strcmp_c(ext_name, "APE") 	
			)
			file_type = FILE_MUSIC;
	} else if (MEDIA_TYPE_PHOTO == media_type){
		if (!strcmp_c(ext_name, "JPG") 		||
			!strcmp_c(ext_name, "JPE")		||
			!strcmp_c(ext_name, "JPEG")		||
			!strcmp_c(ext_name, "BMP") 		||
			!strcmp_c(ext_name, "GIF") 		||
			!strcmp_c(ext_name, "PNG")	|| 
			!strcmp_c(ext_name, "TGA") 	||
			!strcmp_c(ext_name, "ICO") 	||
			!strcmp_c(ext_name, "WEBP") 	||
			!strcmp_c(ext_name, "TIF") 
			)
			file_type = FILE_IMAGE;
	}else if (MEDIA_TYPE_TXT == media_type){
		if (!strcmp_c(ext_name, "TXT") 		||
			!strcmp_c(ext_name, "LRC")	
			)
			file_type = FILE_TXT;
	}


	return file_type;
}

void file_mgr_get_fullname(char *fullname,char *path, char *name)
{
    strcpy(fullname, path);
    strcat(fullname, "/");
    strcat(fullname, name);
}

file_node_t * file_mgr_get_file_node(file_list_t *file_list, int index)
{
	glist *plist = NULL;
	plist = (glist *)file_list->list;
	file_node_t *file_node = NULL;

	if (index > (file_list->dir_count + file_list->file_count))
		return (void *)API_FAILURE;

	file_node = glist_nth_data(plist, index);
	if (NULL != file_node){
		return file_node;
	} else {
		printf("can not find the file node via the index:%d\n", index);
		return NULL;
	}
}

static glist* subtitle_glist=NULL;
/**
 * @description: add a subtitile file filter ,support file include .ass .ssa .srt .txt .sub .smi
 * @param {char*} file_name
 * @return {*} true is subtitle file ,false is not 
 * @author: Yanisin
 */
bool file_mgr_subtitle_filter(char* file_name)
{
	int i;
	int j;
	char *ext_name = NULL;

	for (i = 0, j = -1; file_name[i] != '\0'; i++){
		if (file_name[i] == '.') j = i;
	}

	if (j == -1){
		return false;
	}else{
		j++;
		ext_name = file_name + j;
	}
	if( strcasestr(ext_name,"ass")||
		strcasestr(ext_name,"ssa")||
		strcasestr(ext_name,"srt")||
		strcasestr(ext_name,"txt")||
		strcasestr(ext_name,"sub")||
		strcasestr(ext_name,"smi")||
		strcasestr(ext_name,"idx")||
		strcasestr(ext_name,"vtt")||
		strcasestr(ext_name,"lrc")
	){
		return true;
	}else{
		return false;
	}
}
/**
 * @description: selcet a ext to filter
 * @param {char*} file_name
 * @param {char*} ext optional ext
 * @return {*}
 * @author: Yanisin
 */
bool file_mgr_optional_filter(char* file_name,char* ext)
{
	int i;
	int j;
	char *ext_name = NULL;

	for (i = 0, j = -1; file_name[i] != '\0'; i++){
		if (file_name[i] == '.') j = i;
	}

	if (j == -1){
		return false;
	}else{
		j++;
		ext_name = file_name + j;
	}
	if( strcasestr(ext_name,ext)){
		return true;
	}else{
		return false;
	}
}
bool file_mgr_subtitile_list_create(file_list_t *src_list,char * name)
{
	if(src_list->media_type==MEDIA_TYPE_VIDEO||src_list->media_type==MEDIA_TYPE_MUSIC){
		if(file_mgr_subtitle_filter(name)){
			char *subtitle_file=strdup(name);
			subtitle_glist=glist_append(subtitle_glist,subtitle_file);
			return true;
		}
		return false;
	}else{
		return false;
	}
}



#define LimitFile_Num 1000
int file_mgr_create_list(file_list_t *file_list, char *path)
{
	DIR *dirp;
	struct dirent *entry;
	file_node_t *file_node = NULL;
	file_type_t file_type;
	ASSERT_API(file_list);
	// int i=0;
	int len;
	if ((dirp = opendir(path)) == NULL) {
		return API_FAILURE;
	}

	//step 1: free current file list's node
	if (file_list->list){
		glist_free_full(file_list->list, file_mgr_free_data);
		file_list->dir_count = 0;
		file_list->file_count = 0;
		file_list->list = NULL;
	}
	strcpy(file_list->dir_path, path);
	if(subtitle_glist){
		glist_free_full(subtitle_glist, file_mgr_free_data);
		subtitle_glist=NULL;
	}
	
	printf("%s start\n",__func__);
	//step 2: create the new file list via scan the dir
	while (1) {
		entry = readdir(dirp);
		if (!entry)
			break;

		//printf("entry->d_name %s, entry->d_type %d\n", entry->d_name, entry->d_type);

		if(!strcmp(entry->d_name, ".")){
			//skip the upper dir
			continue;
		}
		if(!strcmp(entry->d_name, "..")){
			//skip the upper dir
			continue;
		}

		len = strlen(entry->d_name);
		if (entry->d_type == 4){
			//dir
			file_node = (file_node_t*)malloc(sizeof(file_node_t) + len + 1);
			memset(file_node,0,sizeof(file_node_t));
			file_node->type = FILE_DIR;
			file_list->dir_count ++;
		} else {
			file_type = file_mgr_filter_file(file_list->media_type, entry->d_name);

//			file_type = FILE_VIDEO;
			if (FILE_OTHER == file_type){
				if(file_mgr_subtitile_list_create(file_list,entry->d_name)==true){
					printf(">>! %s, add in subtutle list\n",entry->d_name);
				}
				continue;

			}

			file_node = (file_node_t*)malloc(sizeof(file_node_t) + len + 1);
			memset(file_node,0,sizeof(file_node_t));
			file_node->type = file_type;
			file_list->file_count ++;
		}
		strcpy(file_node->name, entry->d_name);
		file_list->list = glist_prepend(file_list->list, (void *)file_node);

		//add 1000 files to limit
		if(file_list->dir_count+ file_list->file_count ==LimitFile_Num)
			break;
	}
	
	//step 3: post process the list, for example, sort the file list. to be done
	printf("%s end\n",__func__);
	file_list->list = glist_sort(file_list->list,fs_compare_func,NULL); // sort by Name
	
    file_node = (file_node_t*)malloc(sizeof(file_node_t) + 3 + 1);
	memset(file_node,0,sizeof(file_node_t));
    file_node->type = FILE_DIR;
    file_list->dir_count ++;
    strcpy(file_node->name, "..");
	// insert ".." as list head after sorted.
    file_list->list = glist_prepend(file_list->list, (void *)file_node);

	if(dirp!=NULL)
	{
		closedir(dirp);
		dirp=NULL;
	}
		
	printf("current dir: %s, file: %d, dir: %d\n", path, file_list->file_count, file_list->dir_count);
	return API_SUCCESS;
}

int file_mgr_create_list_without_dir(file_list_t *file_list, char *path)
{
	DIR *dirp;
	struct dirent *entry;
	file_node_t *file_node = NULL;
	file_type_t file_type;
	ASSERT_API(file_list);
	// int i=0;
	int len;
	if ((dirp = opendir(path)) == NULL) {
		return API_FAILURE;
	}

	//step 1: free current file list's node
	if (file_list->list){
		glist_free_full(file_list->list, file_mgr_free_data);
		file_list->dir_count = 0;
		file_list->file_count = 0;
		file_list->list = NULL;
	}
	strcpy(file_list->dir_path, path);

	printf("%s start\n",__func__);
	//step 2: create the new file list via scan the dir
	while (1) {
		entry = readdir(dirp);
		if (!entry)
			break;

		//printf("entry->d_name %s, entry->d_type %d\n", entry->d_name, entry->d_type);

		if(!strcmp(entry->d_name, ".")){
			//skip the upper dir
			continue;
		}
		if(!strcmp(entry->d_name, "..")){
			//skip the upper dir
			continue;
		}

		len = strlen(entry->d_name);
		if (entry->d_type == 4){
			//dir
			// file_node = (file_node_t*)malloc(sizeof(file_node_t) + len + 1);
			// file_node->type = FILE_DIR;
			// file_list->dir_count ++;
			// do not thing
		} else {
			file_type = file_mgr_filter_file(file_list->media_type, entry->d_name);

//			file_type = FILE_VIDEO;
			if (FILE_OTHER == file_type)
				continue;

			file_node = (file_node_t*)malloc(sizeof(file_node_t) + len + 1);
			memset(file_node,0,sizeof(file_node_t));
			file_node->type = file_type;
			// file_node->size = ;
			// file_node->attr = ;
			file_list->file_count ++;
			
			strcpy(file_node->name, entry->d_name);
			file_list->list = glist_prepend(file_list->list, (void *)file_node);

			//add 1000 files to limit
			if(file_list->dir_count+ file_list->file_count ==LimitFile_Num)
				break;
		}
		// strcpy(file_node->name, entry->d_name);
		// file_list->list = glist_prepend(file_list->list, (void *)file_node);

		// //add 1000 files to limit
		// if(file_list->dir_count+ file_list->file_count ==LimitFile_Num)
		// 	break;
	}
	
	//step 3: post process the list, for example, sort the file list. to be done
	printf("%s end\n",__func__);
	file_list->list = glist_sort(file_list->list,fs_compare_func,NULL); // sort by Name
	if(dirp!=NULL)
	{
		closedir(dirp);
		dirp=NULL;
	}
		
	printf("current dir: %s, file: %d, dir: %d\n", path, file_list->file_count, file_list->dir_count);
#if 0
//////////////////////////////////////////////////	
	glist *list_tmp;
	file_node_t *node_tmp;
	list_tmp = file_list->list;
	while (list_tmp){
		node_tmp = (file_node_t*)(list_tmp->data);
		printf("node list: %s\n", node_tmp->name);
		list_tmp = list_tmp->next;
	}
////////////////////////////////////////////////
#endif
	return API_SUCCESS;
}

#if 0
//When usb/uda device is pulled out, should del list and clean file_list
int file_mgr_del_list(file_list_t *file_list)
{
	ASSERT_API(file_list);
	glist_free_full(file_list, file_mgr_free_data);
	memset((void*)file_list, 0, sizeof(file_list_t));
}
#endif

/*free mem list */
int file_mgr_free_list(file_list_t *file_list)
{
	if(file_list==NULL)
		return 0;		
	if (file_list->list){
		glist_free_full(file_list->list, file_mgr_free_data);
		file_list->dir_count = 0;
		file_list->file_count = 0;
		file_list->list = NULL;
		memset(file_list,0,sizeof(file_list_t));
		printf("free media_list struct  memory\n");
	}
	return 0;
}
int file_mgr_glist_free(void * list)
{
	if(list==NULL)
		return 0;
	else{ 
		glist_free_full(list,file_mgr_free_data);// glist just free glist data&glist have to set NULL 
		return 0;
	}
}

void* file_mgr_subtitile_list_get()
{
	return subtitle_glist;
}
void file_mgr_subtitle_list_free()
{
	file_mgr_glist_free(subtitle_glist);
	subtitle_glist=NULL;
}

/**
 * @description: remove the externsion in end of the file ,such as hello_world.cpp-> hello_world
 * @param {char} *str_out	output string 
 * @param {char} *str_in	input string
 * @return {*}
 * @author: Yanisin
 */
int file_mgr_rm_extension(char *str_out, char *str_in)
{
    int len = 0;
    int i = 0;
    len = strlen(str_in);
    for(i = len-1; i >= 0; i--){
        if('.' == str_in[i]){
            strncpy(str_out, str_in, i);
            break;
        }
    }
 
    return 0;
}
