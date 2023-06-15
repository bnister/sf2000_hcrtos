#include <pthread.h>
#include <ffplayer.h>
#include <hcuapi/dis.h>
#include <sys/stat.h>
#include "media_player.h"

//media source 
media_handle_t *backstage_player_open(media_type_t type);
void backstage_player_close(media_handle_t *media_hld);
int backstage_player_play(media_handle_t *media_hld, const char *media_src);
int backstage_player_stop(media_handle_t *media_hld);

//for app user

int bs_player_open(file_list_t *m_cur_file_list);
int bs_player_close(void);

int backstage_player_task_start(int argc, char **argv);
int backstage_player_task_stop(int argc, char **argv);

void* app_get_bsplayer_glist(void);
// int bs_player_add_glist(file_list_t *m_cur_file_list,glist* player_glist);
