/**
 * win_media_list.h
 */

#ifndef __WIN_MEIDA_LIST_H__
#define __WIN_MEIDA_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

file_list_t *win_media_get_cur_list(void);
char *win_media_get_pre_file(file_list_t *file_list);
char *win_media_get_next_file(file_list_t *file_list);
char *win_media_get_cur_file_name(void);
void media_play_test(void);
uint32_t vkey_transfer(uint32_t vkey);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
