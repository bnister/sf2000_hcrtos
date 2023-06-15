#include "screen.h"
#include "setup.h"
#include <hcuapi/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "osd_com.h"
int sound_mode_vec[] = {STR_STANDARD, LINE_BREAK_STR, STR_MUSIC, LINE_BREAK_STR, STR_MOVIE, LINE_BREAK_STR, STR_SPORTS, LINE_BREAK_STR,
STR_USER, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};
extern mode_items sound_mode_items[2];



static int sound_mode_pre_text[2];

static void sound_mode_btnmatrix_event(lv_event_t* e);
static int change_soundMode_event_(int k);

int set_twotone(int mode, int bass, int treble);
int set_balance(int v);
void sound_mode_widget(lv_obj_t* btn);

extern lv_obj_t *new_widget_(lv_obj_t*, int title, int*,uint32_t index, int len, int w, int h);
extern void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
extern void label_set_text_color(lv_obj_t* label,const char* text, char* color);

void sound_mode_widget(lv_obj_t* btn){
    // static const char *title = "Sound Mode\0声音模式\0Sound Mode";

    lv_obj_t * obj = new_widget_(btn, STR_SOUND_MODE, sound_mode_vec,projector_get_some_sys_param(P_SOUND_MODE), 12,0,0);
    if(projector_get_some_sys_param(P_SOUND_MODE) ==  SND_TWOTONE_MODE_USER){
        for(int i=0; i<2; i++){
            sound_mode_pre_text[i] = projector_get_some_sys_param(sound_mode_items[i].index);
        }
    }

    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), sound_mode_btnmatrix_event, LV_EVENT_ALL, btn);

}

static void sound_mode_btnmatrix_event(lv_event_t* e){
    btnmatrix_event(e, change_soundMode_event_);
}

static int change_soundMode_event_(int k){
    lv_obj_t* obj;
    char *values[2] = {
        k==0 ? "0" : k==1 ? "5" : k==2 ? "6" : "-3" ,
         k==0 ? "0" : k==1 ? "5" : k==2 ? "5" : "-3" 
    };


    switch (k){
    case SND_TWOTONE_MODE_STANDARD:
    case SND_TWOTONE_MODE_MUSIC:
    case SND_TWOTONE_MODE_MOVIE:
    case SND_TWOTONE_MODE_SPORT:
        set_twotone(k, atoi(values[0]), atoi(values[1]));
         for(int i=0; i< 2; i++){
                obj = sound_mode_items[i].obj;
                if (!lv_obj_has_state(obj, LV_STATE_DISABLED)){
                    lv_obj_add_state(obj, LV_STATE_DISABLED);
                }
                lv_obj_set_style_text_color(obj, lv_color_make(125,125,125), 0);
              
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),0), lv_color_make(125,125,125), 0);
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),1), lv_color_make(125,125,125), 0);
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),2), lv_color_make(125,125,125), 0);
                lv_label_set_text(lv_obj_get_child(obj, 1), values[i]);
                //lv_dropdown_set_selected(lv_obj_get_child(lv_obj_get_child(obj, 1), 0), strtol(values[i], NULL, 10)+10);
            }
        break;
    case SND_TWOTONE_MODE_USER:
        set_twotone(k, sound_mode_pre_text[0], sound_mode_pre_text[1]);
        for (int i = 0; i < 2; i++) {
            
        obj = sound_mode_items[i].obj;
        
        if(lv_obj_has_state(obj, LV_STATE_DISABLED)){
            lv_obj_clear_state(obj, LV_STATE_DISABLED);
        }
        lv_obj_set_style_text_color(obj, lv_color_white(), 0);
        // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),0), lv_color_white(), 0);
        // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),1), lv_color_white(), 0);
        // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),2), lv_color_white(), 0);
        lv_label_set_text_fmt(lv_obj_get_child(obj, 1),"%d",sound_mode_pre_text[i]);
        //lv_dropdown_set_selected(lv_obj_get_child(lv_obj_get_child(obj, 1), 0),sound_mode_pre_text[i]+10);
    }
        break;   
    
    default:
        break;
    }
   
 return 0;
}

int set_twotone(int mode, int treble, int bass){
    
	int snd_fd = -1;

	struct snd_twotone tt = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("twotone open snd_fd %d failed\n", snd_fd);
		return -1;
	}
    tt.tt_mode = mode;
    tt.onoff = 1;

    if(bass >= -10 && bass <= 10){
        tt.bass_index = bass;
        projector_set_some_sys_param(P_BASS, bass);
    }
    if(treble >= -10 && treble <= 10){
        tt.treble_index = treble;
        projector_set_some_sys_param(P_TREBLE, treble);
    }
    projector_set_some_sys_param(P_SOUND_MODE, mode);
    projector_sys_param_save();

	ioctl(snd_fd, SND_IOCTL_SET_TWOTONE, &tt);
	close(snd_fd);
	return 0;
}

int set_balance(int v){
    int snd_fd = -1;

    struct snd_lr_balance lr = {0};

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("lr_balance open snd_fd %d failed\n", snd_fd);
		return -1;
	}

    lr.lr_balance_index = v;
    lr.onoff = 1;

    projector_set_some_sys_param(P_BALANCE, v);
    projector_sys_param_save();
    ioctl(snd_fd, SND_IOCTL_SET_LR_BALANCE, &lr);
	close(snd_fd);
	return 0;
}
