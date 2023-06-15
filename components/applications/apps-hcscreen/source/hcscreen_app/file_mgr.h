/**
 * file_mgr.h
 */

#ifndef __FILE_MGR_H__
#define __FILE_MGR_H__

#include "media_player.h"
#include "com_api.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILE_NUMBER	(2048)

typedef struct {
    char dir_path[MAX_FILE_NAME];
    media_type_t media_type;
    uint16_t    dir_count;
    uint16_t    file_count;
    uint16_t    item_index;
    void       *list;
}file_list_t;

typedef enum
{ 
    FILE_DIR = 0,
    FILE_VIDEO,
    FILE_MUSIC,
    FILE_IMAGE,
    FILE_OTHER,

    FILE_ALL,
}file_type_t; //valid types arranged be alphabet


typedef struct {
    file_type_t type;
    uint32_t    size;
    uint8_t     attr;
    char     name[0];
}file_node_t;


file_node_t *file_mgr_get_file_node(file_list_t *file_list, int index);
int file_mgr_create_list(file_list_t *file_list, char *path);
void file_mgr_get_fullname(char *fullname,char *path, char *name);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif	
