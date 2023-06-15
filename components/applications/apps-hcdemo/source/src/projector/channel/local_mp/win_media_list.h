/**
 * win_media_list.h
 */

#ifndef __WIN_MEIDA_LIST_H__
#define __WIN_MEIDA_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "file_mgr.h"

file_list_t *win_media_get_cur_list(void);
char *win_media_get_pre_file(file_list_t *file_list);
char *win_media_get_next_file(file_list_t *file_list);
char *win_media_get_cur_file_name(void);
void media_play_test(void);
uint32_t vkey_transfer(uint32_t vkey);

bool win_media_is_root_dir(file_list_t *file_list);
void win_get_parent_dirname(char *parentpath, char *path);
uint16_t win_get_file_idx_fullname(file_list_t *file_list, char *file_name);


#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
