#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"
#include <pthread.h>

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"
#include <hcuapi/vidmp.h>
#include <hcuapi/common.h>
#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
// #include "local_mp_ui_lld.h"
#include "mp_mainpage.h"
#include "mp_subpage.h"
#include "mp_fspage.h"
#include "mp_ctrlbarpage.h"
#include "mp_ebook.h"





static bool stop_thumbnail = 0;
static pthread_t thumbnail_thread_id = 0;
static media_handle_t *m_cur_media_hld2=NULL;


#if 1
static void * thumbnail_thread(void *args)
{
    //test to show a pic in lvgl obj
    if (!m_cur_media_hld2)
        m_cur_media_hld2 = media_open(MEDIA_TYPE_VIDEO);
    media_play_for_thumbnail(m_cur_media_hld2, MOUNT_ROOT_DIR"/sda2/Video/24_timed-text2 MP4_P4_A2x2_688x320 star.war.mp4");
    printf(">> ! %s to get thumbnail start \n",__func__);
    while(1){
        api_sleep_ms(100);
    }
}


 int thumbnail_pthread_stop(void)
{
	stop_thumbnail = 1;
	if (thumbnail_thread_id)
		pthread_join(thumbnail_thread_id, NULL);
	thumbnail_thread_id = 0;
        printf("\n>>> %s\n\n", __func__);

	return 0;
}

 int thumbnail_pthread_start()
{
        pthread_attr_t attr;
        if (thumbnail_thread_id) {
               thumbnail_pthread_stop();
        }

        stop_thumbnail = 0;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x2000);
        if (pthread_create(&thumbnail_thread_id, &attr,
                                thumbnail_thread, NULL)) {
            printf("create thread failed\n");
        }
		printf("\n>>> %s\n\n", __func__);
        pthread_attr_destroy(&attr);

        return 0;
}

static lv_img_dsc_t thumbnail_img_dsc = {0};
#endif
int thumpnail_data_handle()
{
	AvPktHd hdr = { 0 };
    media_handle_t *media_hld=m_cur_media_hld2;
    if (hcplayer_read_transcoded_picture(media_hld->player, &hdr, sizeof(AvPktHd))
        != sizeof(AvPktHd)) {
        printf("read header err\n");
    } else {
        void *data = malloc(hdr.size);
        if (data) {
            if (hcplayer_read_transcoded_picture(media_hld->player , data , hdr.size) != hdr.size) {
                printf ("read data err\n");
            } else {
                printf("get a picture, size %d\n", hdr.size);   //file size
                thumbnail_img_dsc.header.h=85;
                thumbnail_img_dsc.header.w=150;
                thumbnail_img_dsc.header.cf=LV_IMG_CF_TRUE_COLOR;
                thumbnail_img_dsc.data_size=85*150*LV_IMG_PX_SIZE_ALPHA_BYTE;
                thumbnail_img_dsc.data= (uint8_t *)data;
                // lv_img_set_src(ui_fsimg0,&thumbnail_img_dsc);
                #if 0
                if (g_vtranscode_path) {
                    FILE *rec = fopen(g_vtranscode_path, "wb");
                    if (rec) {
                        fwrite(data, hdr.size, 1, rec);
                        fclose(rec);
                        printf("write pic success\n");
                    } else {
                        printf("write pic failed\n");
                    }
                }
                #endif
            }

            // free(data);
        } else {
            printf(">> ! no memory\n");
        }
    }
    return 0;
}



#if 0
void  backstage_player_task(void *pvParameters)
{
    file_list_t * bs_music_list=get_bs_musiclist_t();
    glist* player_glist=app_get_bsplayer_glist();
    bs_player_open_form_glist(bs_music_list,player_glist);
    while(1){
        api_sleep_ms(100);
    }
   
}
static TaskHandle_t bs_player_task_hdl=NULL;
//use a task to start backstage music player
int backstage_player_task_start(int argc, char **argv)
{
	// start projector main task.
	xTaskCreate(backstage_player_task, (const char *)"backstage_player", 0x2000/*configTASK_STACK_DEPTH*/,
		    NULL, portPRI_TASK_NORMAL, &bs_player_task_hdl);
		    return 0;
}

int backstage_player_task_stop(int argc, char **argv)
{
	// start projector main task.
	if (bs_player_task_hdl != NULL){
        bs_player_close();
        vTaskDelete(bs_player_task_hdl);
        bs_player_task_hdl=NULL;
    }
		
    return 0;
}
#endif

