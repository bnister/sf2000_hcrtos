#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <bluetooth.h>
#include "../../screen.h"
#include "../../factory_setting.h"
#include "../setup.h"
#include <bluetooth.h>
#define bt_dev_len  5

typedef enum wait_type_{
    SCAN_WAIT,
    CONN_WAIT
} wait_type;

extern lv_timer_t *timer_setting;
extern lv_font_t select_font[3];
extern lv_obj_t* salve_scr_obj;

static int sel_id = 0;
static int connected_bt_id = -1;
static bool master_disconn = false;
static wait_type bt_wait_type;

struct bluetooth_slave_dev devs_info[bt_dev_len]={0};
struct bluetooth_slave_dev *devs_info_t[bt_dev_len]={NULL, NULL, NULL, NULL,NULL};
int connected_bt_index = 0;

lv_obj_t *bluetooth_obj = NULL;
lv_obj_t *wait_anim = NULL;


char* first = "Off\0关\0Off";
char* second = "On\0开\0On";
static int found_bt_num=0;
static lv_timer_t * wait_anim_timer = NULL;


static bt_scan_status scan_status = BT_SCAN_STATUS_DEFAULT;
static bt_connect_status_e connet_status = BT_CONNECT_STATUS_DEFAULT;
static bt_connect_status_e mute_connet_status = BT_CONNECT_STATUS_DEFAULT;

static bool str_is_black(char *str);
static void get_bt_mac(char *name,unsigned char* mac);
static void bluetooth_wait(wait_type type);
static void event_cb(lv_event_t * e);
int bt_event(unsigned long event, unsigned long param);
static void create_message_box(char* str);
static void remove_bt_dev(char* name);
static bool bt_mac_cmp(unsigned char* mac1, unsigned char* mac2);
static bool bt_mac_invalid(unsigned char* mac);
void add_connected(lv_obj_t *label);
void remove_connected(lv_obj_t *label);

lv_obj_t* create_list_obj(lv_obj_t *parent, int w, int h, lv_obj_t *btn);

bt_connect_status_e bt_get_connet_state(void)
{
    return mute_connet_status;
}

void bt_set_connet_state(bt_connect_status_e val)
{
    mute_connet_status = val;
}

void add_connected(lv_obj_t *label){
    char temp_str[140];
    char *str = lv_label_get_text(label);
    memset(temp_str, 0, 140);
    strcpy(temp_str, str);
    strcat(temp_str, get_some_language_str(" Connected\0 已连接\0  Connected", projector_get_some_sys_param(P_OSD_LANGUAGE)));
    lv_label_set_text(label, temp_str);
}

void remove_connected(lv_obj_t *label){
    char *str = lv_label_get_text(label);
    int i=0;
    long  size = strlen(str);
    for(long j = size-1; j>=0; j--){
        if(str[j] == ' '){
            break;
        }
        i++;
    }
    char temp_str[140];
    memset(temp_str, 0, 140);
    strncpy(temp_str, str, size-i-1);
    lv_label_set_text(label, temp_str);
}

static void remove_bt_dev(char* name){
    for(int i=0; i<found_bt_num; i++){
        if(strcmp(name, devs_info[i].name) == 0){
            for(int j=i; j<found_bt_num-1; j++){
                memcpy(&devs_info[i], &devs_info[i+1], sizeof(struct bluetooth_slave_dev));
            }
            printf("delete dev %s\n", name);
            connected_bt_id=-1;
            found_bt_num-=1;
            if(bluetooth_obj){
                remove_list_sub_obj(bluetooth_obj, 2+i);
            }
        }
    }
}

static bool bt_mac_cmp(unsigned char* mac1, unsigned char* mac2){
    for(int i=0; i<6; i++){
        if(mac1[i] != mac2[i]){
            return false;
        }
    }
    return true;
}


static void bt_setting_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *btn = lv_event_get_user_data(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(sel_id == 0){
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON){
                    connected_bt_id = -1;
                    found_bt_num = 0;
                    second = "On\0打开\0sur";
                    lv_label_set_text(lv_obj_get_child(bluetooth_obj, 1), get_some_language_str(second, P_OSD_LANGUAGE));
                    bluetooth_poweroff();
                    bt_set_connet_state(BT_CONNECT_STATUS_DISCONNECTED);

                    remove_list_sub_objs(bluetooth_obj, 2, lv_obj_get_child_cnt(bluetooth_obj));


                    lv_obj_t *lab = lv_obj_get_child(btn, 1);
                    language_choose_add_label(lab, first, 0);
                    set_label_text(lab, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
                    projector_set_some_sys_param(P_BT_SETTING, BLUETOOTH_OFF);
                }
            }else if(sel_id == 1){
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_OFF){
                    second = "Reconnecting...\0重连接...\0Reconnecting...";

                     if(bluetooth_poweron() == 0){
                        lv_obj_t *lab = lv_obj_get_child(btn, 1);
                        language_choose_add_label(lab, "On\0打开\0On", 0);
                        set_label_text(lab, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
                        projector_set_some_sys_param(P_BT_SETTING, BLUETOOTH_ON);
                        printf("Device exists\n");

 
                       

                     }else{
                        second = "BT Failed To Open\0蓝牙打开失败\0BT Failed To Open";
                        printf("Device not exists\n");
                     }
                    lv_label_set_text(lv_obj_get_child(bluetooth_obj, 1), get_some_language_str(second, projector_get_some_sys_param(P_OSD_LANGUAGE)));


                    unsigned char *mac = projector_get_bt_mac();
                    if(!bt_mac_invalid(mac) && bluetooth_connect(mac) == 0){
                        printf("mac\n");
                        connet_status = BT_CONNECT_STATUS_CONNECTING;
                        bluetooth_wait(CONN_WAIT);
                    }
                }else{
                    second = "Device searching...\0搜索设备...\0Recherche d’appareils";
                    lv_label_set_text(lv_obj_get_child(bluetooth_obj, 1), get_some_language_str(second, projector_get_some_sys_param(P_OSD_LANGUAGE)));
                    remove_list_sub_objs(bluetooth_obj, 2, lv_obj_get_child_cnt(bluetooth_obj));
                    found_bt_num = 0;      
                    printf("%s\n", second);
                    if(bluetooth_scan() == 0){
                        scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
                        printf("scan bluettoth!");
                        bluetooth_wait(SCAN_WAIT);
                    }
                }
                
            }else{
                if(sel_id == connected_bt_id+2){
                    printf("haha BT Connected\n");
                    create_message_box(get_some_language_str("BT Connected\0蓝牙已连接\0BT Connected",projector_get_some_sys_param(P_OSD_LANGUAGE)));
                    return;
                }
                char *text = lv_label_get_text(lv_obj_get_child(bluetooth_obj, sel_id));
                unsigned char mac[6] = {0};
               
                get_bt_mac(text, mac);

                if(bt_mac_invalid(mac)){
                    printf("invalid mac address\n");
                    return;
                }

                if(bluetooth_is_connected()==0 ){
                    master_disconn = true;
                 }

                if(bluetooth_connect(mac)==0){
                    for(int i=0; i<6; i++){
                        printf("%02x,", mac[i]);
                    }
                    printf("\n");
                    connet_status = BT_CONNECT_STATUS_CONNECTING;
                    bluetooth_wait(CONN_WAIT);

                }
            }
        }else if(key == LV_KEY_DOWN){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id = (sel_id+1)%lv_obj_get_child_cnt(bluetooth_obj);
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            if(sel_id == 0){
                lv_obj_scroll_to_view(lv_obj_get_child(target, 0), LV_ANIM_ON);
            }
            lv_timer_reset(timer_setting);
        }else if(key == LV_KEY_UP){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id = --sel_id < 0 ? lv_obj_get_child_cnt(bluetooth_obj)-1 : sel_id;
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            if(sel_id == lv_obj_get_child_cnt(bluetooth_obj)-1){
                lv_obj_scroll_to_view(lv_obj_get_child(target, sel_id), LV_ANIM_ON);
            }
            lv_timer_reset(timer_setting);
        }else if(key == LV_KEY_HOME){
            bluetooth_obj = NULL;
            lv_obj_del(target->parent);
            turn_to_setup_scr();

        }else if(key == LV_KEY_ESC){
            bluetooth_obj = NULL;
            turn_to_main_scr();
            return;
        }
    }
}


void msg_timer_handle(lv_timer_t *timer_){
    lv_obj_t *obj = (lv_obj_t*)timer_->user_data;
    printf("del msgbox\n");
    lv_msgbox_close(obj);
}

static void create_message_box(char* str){
    printf("msgbox\n");
    lv_obj_t *obj = lv_msgbox_create(slave_scr, NULL, str, NULL, false);
    lv_obj_set_size(obj, LV_PCT(20), LV_PCT(15));
     lv_obj_set_style_text_font(obj, &select_font[projector_get_some_sys_param(P_OSD_LANGUAGE)], 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_center(obj);
    lv_obj_t *con = lv_msgbox_get_content(obj);
    lv_obj_set_style_bg_color(con, lv_color_white(), 0);
    lv_obj_set_style_bg_color(obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_90, 0);
    lv_timer_t *timer = lv_timer_create(msg_timer_handle, 2000, obj);
    lv_timer_set_repeat_count(timer, 1);
    lv_timer_reset(timer);
}

static bool str_is_black(char *str){
    for(int i=0; i<strlen(str); i++){
        unsigned char c = str[i];
        if(!isspace(c)){
            return false;
        }
    }
    return true;
}

static bool bt_mac_invalid(unsigned char* mac){
    for(int i=0; i<6; i++){
        if(mac[i] != 0){
            return false;
        }
    }
    return true;
}

int bt_event(unsigned long event, unsigned long param){
    switch (event){
        case BLUETOOTH_EVENT_SLAVE_DEV_SCANNED:
            printf("BT_AD6956F_EVENT_SLAVE_DEV_SCANNED\n");
            scan_status=BT_SCAN_STATUS_GET_DATA_SEARCHED;
            if(param==0)break;
            if(found_bt_num>=4){
                break;
            }

            devs_info_t[found_bt_num]=(struct bluetooth_slave_dev*)param;
            if(strlen(devs_info_t[found_bt_num]->name) == 0 || str_is_black(devs_info_t[found_bt_num]->name)){
                break;
            }
            if(!wait_anim_timer){
                break;
            }
            memcpy(devs_info+found_bt_num, devs_info_t[found_bt_num],sizeof(struct bluetooth_slave_dev));
            printf("dev %d: %s",found_bt_num, devs_info[found_bt_num].name);
            create_list_sub_obj(bluetooth_obj, devs_info[found_bt_num].name);
            found_bt_num += 1;
            
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED\n");
            if(found_bt_num>0){
                second = "Disconnected/Search BT...\0断开连接/搜索蓝牙...\0Déconcerté/Rechercher BT...";
            }else{
                second = "No Device/Search BT...\0无设备/搜索蓝牙...\0No Device/Search BT...";
            }
            lv_label_set_text(lv_obj_get_child(bluetooth_obj, 1), get_some_language_str(second, projector_get_some_sys_param(P_OSD_LANGUAGE)));
            scan_status=BT_SCAN_STATUS_GET_DATA_FINISHED;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED 1\n");

            app_set_i2so_gpio_mute(0);
            bluetooth_set_gpio_mutu(0);
            bt_set_connet_state(BT_CONNECT_STATUS_DISCONNECTED);
            second = "Disconnected/Search BT...\0断开连接/搜索蓝牙...\0Déconcerté/Rechercher BT...";
            if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_OFF){
                second = "On\0开\0On";
            }
            if(bluetooth_obj){
                printf("bluetooth_obj exit\n");
                lv_label_set_text(lv_obj_get_child(bluetooth_obj, 1), get_some_language_str(second,projector_get_some_sys_param(P_OSD_LANGUAGE)));
                remove_connected(lv_obj_get_child(bluetooth_obj, connected_bt_id+2));//you wen ti
            }

            if(!master_disconn){
                create_message_box(get_some_language_str("BT Disconnected\0蓝牙已关闭\0BT Disconnected", projector_get_some_sys_param(P_OSD_LANGUAGE)));
            }

            if(connet_status == BT_CONNECT_STATUS_CONNECTED && !master_disconn){
                unsigned char *mac = projector_get_bt_mac();
                printf("ready reconnected\n");
                if(!bt_mac_invalid(mac) && bluetooth_connect(mac)==0){
                    printf("reconneted\n");
                    connet_status = BT_CONNECT_STATUS_CONNECTING;
                    bluetooth_wait(CONN_WAIT);
                }
            }else{
                printf("connet status is: %d\n", connet_status);
                connet_status = BT_CONNECT_STATUS_DISCONNECTED;
            }
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED: 
            master_disconn = false;
            printf("BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED 1\n");

            if(connet_status == BT_CONNECT_STATUS_CONNECTING ){
                second =" Connected/Search BT\0 已连接/搜索蓝牙\0Connected/Search BT";
                if(sel_id>1 && bluetooth_obj){
                    projector_set_bt_dev(&devs_info[sel_id-2]);
                    printf("%d is: %s\n", sel_id-2, devs_info[sel_id-2].name);
                    add_connected(lv_obj_get_child(bluetooth_obj, sel_id));
                    connected_bt_id = sel_id-2;
                    //lv_label_set_text(lv_obj_get_child(bluetooth_obj, 1), get_some_language_str(second, projector_get_some_sys_param(P_OSD_LANGUAGE)));
                   
                }else{
                    memcpy(devs_info+found_bt_num, projector_get_bt_dev(), sizeof(struct bluetooth_slave_dev));
                    printf("found_bt_name: %d, bt_name: %s", found_bt_num, devs_info[found_bt_num].name);
                    if(bluetooth_obj){
                        create_list_sub_obj(bluetooth_obj, devs_info[0].name);
                        add_connected(lv_obj_get_child(bluetooth_obj, 2));
                    }
                    
                    
                    found_bt_num++;
                    connected_bt_id=0;
                }
            }
            app_set_i2so_gpio_mute(1);
            bluetooth_set_gpio_mutu(1);
			bt_set_connet_state(BT_CONNECT_STATUS_CONNECTED);
            
            
            if(connet_status != BT_CONNECT_STATUS_CONNECTED){
                printf("connect_state: %d\n", connet_status);
                create_message_box(get_some_language_str("BT Connected\0蓝牙已连接\0BT Connected", projector_get_some_sys_param(P_OSD_LANGUAGE)));
            }

            connet_status = BT_CONNECT_STATUS_CONNECTED;
            break;
    }
    return 0;
}




void create_list_sub_obj(lv_obj_t *parent, char *str){
    lv_obj_t *list_btn;
    list_btn = lv_list_add_text(parent, str);
    if(lv_obj_get_child_cnt(bluetooth_obj)>2){
        lv_obj_set_style_text_font(list_btn, &select_font[0], 0);
    }else{
        lv_obj_set_style_text_font(list_btn, &select_font[projector_get_some_sys_param(P_OSD_LANGUAGE)], 0);
    }

    lv_obj_set_size(list_btn,LV_PCT(100),LV_PCT(17));
    lv_obj_set_style_pad_top(list_btn, 5, 0);
    lv_obj_set_style_text_align(list_btn, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_set_style_bg_color(list_btn, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_border_width(list_btn, 1, 0);
    lv_obj_set_style_border_color(list_btn, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
}

void remove_list_sub_obj(lv_obj_t *parent, int id){  
    printf("delete count: %d, sel_id: %d\n", lv_obj_get_child_cnt(bluetooth_obj), sel_id);
    if(id >= lv_obj_get_child_cnt(parent)){
        return;
    }
    lv_obj_del(lv_obj_get_child(parent, id));
    if(id == sel_id){
        sel_id = 1;
        lv_obj_add_state(lv_obj_get_child(parent, sel_id), LV_STATE_CHECKED);
    }
}

void remove_list_sub_objs(lv_obj_t *parent, int start, int end){
    if(start>=lv_obj_get_child_cnt(bluetooth_obj)){
        return;
    }
    for(int i=end-1; i>=start; i--){
        remove_list_sub_obj(parent, i);
    }
}

lv_obj_t* create_list_obj(lv_obj_t *parent, int w, int h, lv_obj_t *btn){
    lv_obj_t *obj = lv_list_create(parent);
    lv_obj_set_style_radius(obj, 0, 0);
    set_pad_and_border_and_outline(obj);
    lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_scroll_to_view(obj, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(lv_group_get_default(), obj);
    lv_group_focus_obj( obj);
    lv_obj_add_event_cb(obj, bt_setting_event_handle, LV_EVENT_ALL, btn);
    return obj;
}



static void get_bt_mac(char *name,unsigned char* mac){
    for(int i=0; i<4; i++){
        if(strcmp(name, devs_info[i].name) == 0){
            strncpy((char*)mac, (char*)devs_info[i].mac, 6);
        }
    }
}

static void bluetooth_wait_timer_handle(lv_timer_t *timer_){
    static uint8_t i = 0;
    static uint total_time = 0;
   
    int radius1 = lv_disp_get_hor_res(lv_disp_get_default())/100*1.6;
    int radius2 = lv_disp_get_hor_res(lv_disp_get_default())/50;
    total_time += 500;
    lv_timer_pause(timer_setting);
    if(total_time > 15000 || (bt_wait_type == SCAN_WAIT && scan_status == BT_SCAN_STATUS_GET_DATA_FINISHED) ||
    (bt_wait_type == CONN_WAIT && connet_status == BT_CONNECT_STATUS_CONNECTED) ){
        i=0;
        printf("total_timer: %d", total_time);
        if(total_time > 15000){
            if(scan_status == BT_SCAN_STATUS_GET_DATA_SEARCHING || scan_status == BT_SCAN_STATUS_GET_DATA_SEARCHED){
                bluetooth_stop_scan();
                bt_set_connet_state(BT_CONNECT_STATUS_DISCONNECTED);
                if(found_bt_num>0){
                    second = "Disconnected/Search BT...\0断开连接/搜索蓝牙...\0Déconcerté/Rechercher BT...";
                }else{
                    second = "No Device/Search BT...\0无设备/搜索蓝牙...\0No Device/Rechercher BT...";
                }
            }
           if(connet_status == BT_CONNECT_STATUS_CONNECTING){
                create_message_box(get_some_language_str("BT connection failed\0蓝牙连接失败\0BT connection failed", projector_get_some_sys_param(P_OSD_LANGUAGE)));
                second = "Disconnected/Search BT...\0断开连接/搜索蓝牙...\0Déconcerté/Rechercher BT...";
                remove_bt_dev(devs_info[connected_bt_id].name);
                connet_status = BT_CONNECT_STATUS_DEFAULT;
           }
        }

        total_time = 0;
        if(bluetooth_obj){
            printf("reset timer\n");
            lv_timer_resume(timer_setting);
            lv_timer_reset(timer_setting); 
            lv_obj_clear_state(lv_obj_get_child(bluetooth_obj, sel_id), LV_STATE_CHECKED);
            sel_id = connected_bt_id+2;
            lv_obj_add_state(lv_obj_get_child(bluetooth_obj, sel_id), LV_STATE_CHECKED);
            lv_label_set_text(lv_obj_get_child(bluetooth_obj, 1), get_some_language_str(second, projector_get_some_sys_param(P_OSD_LANGUAGE)));
        }
        if(wait_anim_timer){
            lv_timer_del(wait_anim_timer);
            wait_anim_timer = NULL;
        }
        if(wait_anim){
            lv_obj_del(wait_anim);
            wait_anim = NULL;
        }
        scan_status = BT_SCAN_STATUS_DEFAULT;
        del_bt_wait_anim();
        return;
    }


    lv_obj_set_style_radius(lv_obj_get_child(wait_anim, i), radius1, 0);
    lv_obj_set_size(lv_obj_get_child(wait_anim, i), LV_PCT(20), LV_PCT(40));
    lv_obj_set_style_bg_color(lv_obj_get_child(wait_anim, i), lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
  

    i = (i+1)%3;
    lv_obj_set_style_radius(lv_obj_get_child(wait_anim, i), radius2, 0);
    lv_obj_set_size(lv_obj_get_child(wait_anim, i), LV_PCT(26), LV_PCT(50));
    lv_obj_set_style_bg_color(lv_obj_get_child(wait_anim, i), lv_palette_main(LV_PALETTE_BLUE), 0);

   connet_status = BT_CONNECT_STATUS_CONNECTING;
   scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
}

void del_bt_wait_anim(){
    if(wait_anim_timer){
        lv_timer_del(wait_anim_timer);
        wait_anim_timer = NULL;
    }
    if(wait_anim){
        lv_obj_del(wait_anim);
        wait_anim = NULL;
    }

}

static void bluetooth_wait_handle(lv_event_t *e){
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            if(wait_anim_timer){
                lv_timer_del(wait_anim_timer);
                wait_anim_timer = NULL;
                
            }
            lv_obj_del(target);
            wait_anim = NULL;
            scan_status = BT_SCAN_STATUS_DEFAULT;
        }
       
    }
}

void BT_first_power_on(){
    if(projector_get_some_sys_param(P_BT_SETTING)){
        if(bluetooth_poweron() == 0){
            printf("Device exist\n");
        }
        printf("bt name is : %s\n", projector_get_bt_name());
        unsigned char *mac = projector_get_bt_mac();
        for(int i=0; i<6; i++){
            printf("%02x,", mac[i]);
        }
        printf("\n");
        if(bluetooth_is_connected()==0){
            memcpy(devs_info+0, projector_get_bt_dev(), sizeof(struct bluetooth_slave_dev));
            printf("bt %d name: %s\n",found_bt_num, devs_info[0].name);
            found_bt_num += 1;
            connected_bt_id = 0;
        }else{
            if(bt_mac_invalid(mac)){
                return;
            }
            if(bluetooth_connect(projector_get_bt_mac()) == 0){
                connet_status = BT_CONNECT_STATUS_CONNECTING;
                bluetooth_wait(CONN_WAIT);
            }
        }
        
    }
}

static void bluetooth_wait(wait_type type){
    wait_anim = lv_obj_create(slave_scr);
    lv_obj_set_size(wait_anim, LV_PCT(16), LV_PCT(14));
    lv_obj_center(wait_anim);
    lv_obj_set_style_bg_opa(wait_anim, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(wait_anim, 0, 0);
    lv_obj_set_style_border_width(wait_anim, 0, 0);
    lv_obj_set_style_outline_width(wait_anim, 0, 0);
    lv_obj_set_scrollbar_mode(wait_anim, LV_SCROLLBAR_MODE_OFF);
    lv_group_add_obj(setup_g, wait_anim);
    lv_group_focus_obj(wait_anim);
    lv_obj_add_event_cb(wait_anim, bluetooth_wait_handle, LV_EVENT_ALL, 0);


    lv_obj_t *ball = lv_obj_create(wait_anim);
    lv_obj_set_scrollbar_mode(ball, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(ball, LV_ALIGN_LEFT_MID,0, 0);
    int radius = lv_disp_get_hor_res(lv_disp_get_default())/100*1.6;
    lv_obj_set_style_radius(ball, radius, 0);
    lv_obj_set_size(ball, LV_PCT(20), LV_PCT(40));

    lv_obj_set_style_bg_color(ball, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);

    ball = lv_obj_create(wait_anim);
    lv_obj_set_scrollbar_mode(ball, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(ball, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ball, radius, 0);
    lv_obj_set_size(ball, LV_PCT(20), LV_PCT(40));
    lv_obj_set_style_bg_color(ball, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);

    ball = lv_obj_create(wait_anim);
    lv_obj_set_scrollbar_mode(ball, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(ball, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(ball, radius, 0);
    lv_obj_set_size(ball, LV_PCT(20), LV_PCT(40));
    lv_obj_set_style_bg_color(ball, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);   
    bt_wait_type = type;
    wait_anim_timer = lv_timer_create(bluetooth_wait_timer_handle, 500, (void*)wait_anim);
    lv_timer_reset(wait_anim_timer);
    if(bluetooth_obj){
        lv_timer_pause(timer_setting);
        printf("pause timer\n");
    }
    
}




void bt_setting_widget1(lv_obj_t *btn){
    scan_status = BT_SCAN_STATUS_DEFAULT;
    static const char *title = "BT Setting\0蓝牙设置\0BT Setting";
    lv_obj_t* btnm = create_new_widget(33, 50);
    lv_obj_set_style_opa(btnm, LV_OPA_100, 0);
    create_widget_head(btnm, title, 15);

    bluetooth_obj = create_list_obj(btnm, 100, 72,btn);
    if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_OFF){
        second = "On\0开\0On";
    }
    create_list_sub_obj(bluetooth_obj, get_some_language_str(first, projector_get_some_sys_param(P_OSD_LANGUAGE)));
    create_list_sub_obj(bluetooth_obj, get_some_language_str(second, projector_get_some_sys_param(P_OSD_LANGUAGE)));
    for(int i=0; i<found_bt_num; i++){
        create_list_sub_obj(bluetooth_obj, devs_info[i].name);
    }
    if(projector_get_some_sys_param(P_BT_SETTING)){
        sel_id = connected_bt_id + 2;
        if(bluetooth_is_connected() != 0){
            unsigned char *mac = projector_get_bt_mac();
            if(!bt_mac_invalid(mac) && bluetooth_connect(mac) == 0){
                connet_status = BT_CONNECT_STATUS_CONNECTING;
                sel_id = connected_bt_id+2;
                bluetooth_wait(CONN_WAIT);
            }
        }
    }else{
        sel_id=0;
    }
    printf("connected_bt_id: %d\n", connected_bt_id);
    lv_obj_add_state(lv_obj_get_child(bluetooth_obj, sel_id), LV_STATE_CHECKED);
    if(sel_id>1){
        add_connected(lv_obj_get_child(bluetooth_obj, sel_id));
    }
    create_widget_foot(btnm, 14, btn);
}
