
#include "screen.h"
#include "setup.h"
#include <hcuapi/dis.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <hcuapi/fb.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "osd_com.h"

static int picture_mode_pre_text[4] = {50, 50, 50, 5};
int picture_mode_vec[] = {STR_STANDARD, LINE_BREAK_STR, STR_DYNAMIC, LINE_BREAK_STR, STR_MILD, LINE_BREAK_STR, STR_USER, LINE_BREAK_STR,
BLANK_SPACE_STR, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};

extern mode_items picture_mode_items[4];

static int change_pictureMode_event_(int k);
static void change_pictureMode_event(lv_event_t* e);

void picture_mode_widget( lv_obj_t*);
void set_enhance1(int value, uint8_t op);

extern lv_obj_t *new_widget_(lv_obj_t*, int title, int*,uint32_t index, int len, int w, int h);
extern void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
extern void label_set_text_color(lv_obj_t* label,const char* text, char* color);


void picture_mode_widget(lv_obj_t* btn){

    lv_obj_t * obj = new_widget_(btn, STR_PICTURE_MODE, picture_mode_vec, projector_get_some_sys_param(P_PICTURE_MODE), 12, 0, 0);
    if(projector_get_some_sys_param(P_PICTURE_MODE) == PICTURE_MODE_USER){
        for(int i=0; i<4; i++){
            picture_mode_pre_text[i] = projector_get_some_sys_param(picture_mode_items[i].index);
        }
    }
    
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), change_pictureMode_event, LV_EVENT_ALL, btn);
}

int change_pictureMode_event_(int k){
    projector_set_some_sys_param(P_PICTURE_MODE, k);
    
    uint8_t ops[4] = {P_CONTRAST, P_BRIGHTNESS, P_COLOR, P_SHARPNESS};
    
    
    char* values[4] = { k==0 ? "50" : k==1 ? "60" : "45",
                        k==0 ? "50" : k==1 ? "49" : "50",
                        k==0 ? "50" : k==1 ? "60" : "45",
                        k==0 ? "5" : k==1 ? "5" : "4"};
    lv_obj_t *obj;
    switch (k) {
        case PICTURE_MODE_STANDARD:
        case PICTURE_MODE_DYNAMIC:
        case PICTURE_MODE_MILD:
            for(int i=0; i< 4; i++){
                set_enhance1(strtol(values[i], NULL, 10), ops[i]);
                obj = picture_mode_items[i].obj;
                if (!lv_obj_has_state(obj, LV_STATE_DISABLED)){
                    lv_obj_add_state(obj, LV_STATE_DISABLED);
                }
                lv_obj_set_style_text_color(obj, lv_color_make(125,125,125), 0);
                //lv_label_set_text(lv_obj_get_child(obj, 0), lv_label_get_text(lv_obj_get_child(obj, 0)));
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),0), lv_color_make(125,125,125), 0);
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),1), lv_color_make(125,125,125), 0);
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),2), lv_color_make(125,125,125), 0);
                lv_label_set_text(lv_obj_get_child(obj, 1), values[i]);
                //lv_dropdown_set_selected(lv_obj_get_child(lv_obj_get_child(obj, 1), 0), strtol(values[i], NULL, 10));
            }
            printf("\n");
            break;
        case PICTURE_MODE_USER:
            for (int i = 0; i < 4; i++) {
                set_enhance1(picture_mode_pre_text[i], ops[i]);
                obj = picture_mode_items[i].obj;
                if(lv_obj_has_state(obj, LV_STATE_DISABLED)){
                    lv_obj_clear_state(obj, LV_STATE_DISABLED);
                }
                lv_obj_set_style_text_color(obj, lv_color_white(), 0);
                //lv_label_set_text(lv_obj_get_child(obj, 0), lv_label_get_text(lv_obj_get_child(obj, 0)));
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),0), lv_color_white(), 0);
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),1), lv_color_white(), 0);
                // lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(obj, 1),2), lv_color_white(), 0);
                lv_label_set_text_fmt(lv_obj_get_child(obj, 1),"%d", picture_mode_pre_text[i]);
                //lv_dropdown_set_selected(lv_obj_get_child(lv_obj_get_child(obj, 1), 0),picture_mode_pre_text[i]);
            }
            printf("\n");
            break ;
        default:
            break;
    }

    return 0;
}

void change_pictureMode_event(lv_event_t* e){
    btnmatrix_event(e, change_pictureMode_event_);
}

void set_venhance1( uint16_t enhance_type, uint16_t grade){
    struct dis_video_enhance vhance = { 0 };
    int fd = open("/dev/dis" , O_RDWR);
    if( fd < 0){
        printf("open /dev/dis failed, ret=%d\n", fd);

        return;
    }

    vhance.distype = DIS_TYPE_HD;
    vhance.enhance.enhance_type = enhance_type;
    vhance.enhance.grade = grade;

    
    int ret = ioctl(fd, DIS_SET_VIDEO_ENHANCE , &vhance);
    if(ret != 0){
        printf("%s:%d: warning: HCFBIOSET_VENHANCE failed\n", __func__, __LINE__);
        close(fd);
        return;
    }
    close(fd);
}

void set_enhance1(int value, uint8_t op){
    hcfb_enhance_t eh = {0};
    int fd = -1;
    int ret = 0;

    fd = open("/dev/fb0" , O_RDWR);
    if( fd < 0){
        printf("open /dev/fb0 failed, ret=%d\n", fd);
        return;
    }

    ret = ioctl(fd, HCFBIOGET_ENHANCE, &eh);
    if( ret != 0 ){
        printf("%s:%d: warning: HCFBIOGET_ENHANCE failed\n", __func__, __LINE__);
        close(fd);
        fd = -1;
        return;
    }
    uint32_t enhance_type = 0;
    switch (op) {
        case P_BRIGHTNESS:
            eh.brightness = value;
            enhance_type = DIS_VIDEO_ENHANCE_BRIGHTNESS;
            projector_set_some_sys_param(P_BRIGHTNESS, value);
            break;

        case P_CONTRAST:
            eh.contrast = value<10 ? 10 : value;
            enhance_type = DIS_VIDEO_ENHANCE_CONTRAST;
            projector_set_some_sys_param(P_CONTRAST, value);
            break;

        case P_COLOR:
            eh.saturation = value;
            enhance_type = DIS_VIDEO_ENHANCE_SATURATION;
            projector_set_some_sys_param(P_COLOR, value);
            break;

        case P_HUE:
            eh.hue = value;
            enhance_type = DIS_VIDEO_ENHANCE_HUE;
            projector_set_some_sys_param(P_HUE, value);
            break;

        case P_SHARPNESS:
            eh.sharpness = value;
            enhance_type = DIS_VIDEO_ENHANCE_SHARPNESS;
            projector_set_some_sys_param(P_SHARPNESS, value);
            break;

        default:
            break;
    }

    projector_sys_param_save();
    ret = ioctl(fd, HCFBIOSET_ENHANCE, &eh);
    if( ret != 0 ){
        printf("%s:%d: warning: HCFBIOSET_ENHANCE failed\n", __func__, __LINE__);
        close(fd);
        fd = -1;
        return;
    }


    set_venhance1(enhance_type, value);

    close(fd);
    fd = -1;
}




