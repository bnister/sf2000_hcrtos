/**
 * file_mgr.c
 */

#include <dirent.h>
#include <sys/stat.h>
#include "com_api.h"
#include "glist.h"
#include "file_mgr.h"

static file_type_t file_mgr_filter_file(media_type_t media_type, char *file_name);

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
			!strcmp_c(ext_name, "WMV") 		
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
			!strcmp_c(ext_name, "FLAC") 	
			)
			file_type = FILE_MUSIC;
	} else if (MEDIA_TYPE_PHOTO == media_type){
		if (!strcmp_c(ext_name, "JPG") 		||
			!strcmp_c(ext_name, "JPEG")		||
			!strcmp_c(ext_name, "BMP") 		||
			!strcmp_c(ext_name, "GIF") 		||
			!strcmp_c(ext_name, "PNG") 
			)
			file_type = FILE_IMAGE;
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

int file_mgr_create_list(file_list_t *file_list, char *path)
{
	DIR *dirp;
	struct dirent *entry;
	file_node_t *file_node = NULL;
	file_type_t file_type;
	ASSERT_API(file_list);

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

	//step 2: add ".." used for upper directory	
	file_node = (file_node_t*)malloc(sizeof(file_node_t) + 3 + 1);
	file_node->type = FILE_DIR;
	file_list->dir_count ++;
	strcpy(file_node->name, "..");
	file_list->list = glist_append(file_list->list, (void *)file_node);

	//step 3: create the new file list via scan the dir
	while (1) {
		entry = readdir(dirp);
		if (!entry)
			break;

		//printf("entry->d_name %s, entry->d_type %d\n", entry->d_name, entry->d_type);

		if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")){
			//skip the upper directory and current directory
			continue;
		}

		len = strlen(entry->d_name);
		if (entry->d_type == 4){
			//dir
			file_node = (file_node_t*)malloc(sizeof(file_node_t) + len + 1);
			file_node->type = FILE_DIR;
			file_list->dir_count ++;
		} else {
			file_type = file_mgr_filter_file(file_list->media_type, entry->d_name);

//			file_type = FILE_VIDEO;
			if (FILE_OTHER == file_type)
				continue;

			file_node = (file_node_t*)malloc(sizeof(file_node_t) + len + 1);
			file_node->type = file_type;
			// file_node->size = ;
			// file_node->attr = ;
			file_list->file_count ++;
		}
		strcpy(file_node->name, entry->d_name);
		file_list->list = glist_append(file_list->list, (void *)file_node);
	}

	//step 4: post process the list, for example, sort the file list. to be done
	//

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

    if (dirp)
        closedir(dirp);

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
