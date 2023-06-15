
//#define SUPPORT_BLUETOOTH 1

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/input-event-codes.h>

#include "app_config.h"
#include "screen.h"
#include "factory_setting.h"
#include "setup.h"
#include "mul_lang_text.h"
#include "osd_com.h"
#include "com_api.h"
#include "hcstring_id.h"
#include "app_config.h"
#include "../../channel/main_page/main_page.h"
extern lv_font_t *select_middle_font[3];

#ifdef BLUETOOTH_SUPPORT

#ifdef LVGL_RESOLUTION_240P_SUPPORT
#define BT_LIST_FONT SiYuanHeiTi_Light_3500_12_1b
#else
#define BT_LIST_FONT SIYUANHEITI_LIGHT_3000_28_1B
#endif

#include <bluetooth.h>



//#ifdef SUPPORT_BLUETOOTH
#define bt_dev_len  5

typedef enum wait_type_{
    SCAN_WAIT,
    CONN_WAIT,
    POWER_ON_WAIT
} wait_type;





typedef enum conn_type_{
    CONN_TYPE_IS_CONNECTEDE,
    CONN_TYPE_CONNECT,
    CONN_TYPE_POWER_ON
} conn_type; //bluetooth_is_connected(),bluetooth_connect(), power_on()都有可能触发BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED事件

enum bt_list_refresh_event{
    BT_LIST_EVENT_ADD_CONN,
    BT_LIST_EVENT_REMOVE_CONN,
    BT_LIST_EVENT_CREATE_NEW_BTN,
    BT_LIST_EVENT_MOVE_LOC,
    BT_LIST_EVENT_SWAP_LOC,
};

typedef struct{
    int id;
    void* param1;
    void* param2;
} bt_refresh_event_param;

bt_refresh_event_param bt_refresh_param;

extern lv_timer_t *timer_setting;
extern lv_font_t* select_font_normal[3];

extern lv_obj_t* slave_scr_obj;
extern lv_obj_t *tab_btns;
extern char* bt_v;
extern SCREEN_TYPE_E cur_scr;

static int sel_id = -1;
static int connected_bt_id = -2;
static bool active_disconn = false;
static wait_type bt_wait_type = -1;
static conn_type bt_conn_type = -1;

struct bluetooth_slave_dev devs_info[bt_dev_len]={0};
struct bluetooth_slave_dev *devs_info_t[bt_dev_len]={NULL, NULL, NULL, NULL,NULL};


lv_obj_t *bluetooth_obj = NULL;
lv_obj_t *wait_anim = NULL;

lv_obj_t* bt_list_obj = NULL;
lv_obj_t* my_dev = NULL;
lv_obj_t* other_dev = NULL;
static bool is_connected = false;
static bool reset_timer = true;//重设置timer_setting

static int found_bt_num=0;
static lv_timer_t * wait_anim_timer = NULL;


static bt_scan_status scan_status = BT_SCAN_STATUS_DEFAULT;
static bt_connect_status_e connet_status = BT_CONNECT_STATUS_DEFAULT;
static bt_connect_status_e mute_connet_status = BT_CONNECT_STATUS_DEFAULT;
static  lv_obj_t* saved_bt_widget;
static lv_obj_t *prev_obj = NULL;

LV_FONT_DECLARE(font_china_22)
LV_IMG_DECLARE(MAINMENU_IMG_BT)

static bool str_is_black(char *str);
static void get_bt_mac(char *name,unsigned char* mac);
static void bluetooth_wait(wait_type type);
static void event_cb(lv_event_t * e);
int bt_event1(unsigned long event, unsigned long param);
 
static void remove_bt_dev(int i);
static bool bt_mac_cmp(unsigned char* mac1, unsigned char* mac2);
static void my_bt_dev_event_handle(lv_event_t *e);
static bool bt_mac_invalid(unsigned char* mac);
static void bt_list_change_loc(lv_obj_t *prev, lv_obj_t *next);
static bool bt_has_my_dev();
static   void add_connected(int i);
static void remove_connected(int i);

lv_obj_t* create_bt_list_obj(lv_obj_t *parent, int w, int h);
lv_obj_t* create_list_obj1(lv_obj_t *parent, int w, int h);
static lv_obj_t* create_list_bt_obj(lv_obj_t *parent, int w, int h, lv_obj_t *btn);
static lv_obj_t* create_list_sub_bt_text_obj1(lv_obj_t *parent, int w, int h, int str1);
static lv_obj_t* create_list_sub_obj(lv_obj_t *parent, char *str);
static lv_obj_t* create_list_sub_btn_obj(lv_obj_t *parent);
static lv_obj_t* create_list_sub_btn_obj1(lv_obj_t *parent, int str1, int str2);
static lv_obj_t* create_list_sub_btn_obj2(lv_obj_t *parent, char* str1, int str2);

static void create_list_bt_sub_obj(lv_obj_t *parent, char *str);
static lv_obj_t* create_list_bt_sub_btn_obj(lv_obj_t *parent, list_sub_param, int, int str2);

static void remove_list_sub_obj(lv_obj_t *parent, int id);
static void remove_list_sub_objs(lv_obj_t *parent, int start, int end);
static void hidden_on_list_sub_objs(lv_obj_t *parent, int start, int end);
static void hidden_off_list_sub_obj(lv_obj_t *parent, int start, int end);
static void bt_list_refresh_event_handle(lv_event_t *e);

extern lv_obj_t* create_page_(lv_obj_t* parent, choose_item * data, int len);
extern void set_remote_control_disable(bool b);

bool app_bt_is_connected(){
    return is_connected;
}

void bt_screen_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SCREEN_LOADED){
        if(projector_get_some_sys_param(P_BT_SETTING) && is_connected){
            add_connected(3);
        }
    }
}

static void add_connected(int i){
    lv_obj_t *label = lv_obj_get_child(bt_list_obj, i);
    set_label_text2(lv_obj_get_child(label, 1), STR_BT_CONN, FONT_NORMAL);
}

static void remove_connected(int i){
    lv_obj_t *label = lv_obj_get_child(bt_list_obj, i);
     set_label_text2(lv_obj_get_child(label, 1), STR_BT_DISCONN, FONT_NORMAL);
}

static void remove_bt_dev(int i){
    printf("delete dev id %d\n", i);
    for(int j=i; j+1<found_bt_num; j++){
        memcpy(&devs_info[j], &devs_info[j+1], sizeof(struct bluetooth_slave_dev));
    }
    found_bt_num-=1;
}

static int contain_bt_dev(char *name){
    for(int i=0; i<found_bt_num; i++){
        if(strcmp(name, devs_info[i].name) == 0){
            return i;
        }
    }
    return -1;
}

static bool bt_mac_cmp(unsigned char* mac1, unsigned char* mac2){
    for(int i=0; i<6; i++){
        if(mac1[i] != mac2[i]){
            return false;
        }
    }
    return true;
}

static void my_bt_dev_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
     
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            int sel = lv_obj_has_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED) ? 0 : 1;
            if(sel == 0 ){
                if(is_connected){
                    //connet_status = BT_CONNECT_STATUS_DISCONNECTING;
                    //bluetooth_disconnect();
                }               
                lv_obj_del(lv_obj_get_child(bt_list_obj, 3));
                struct bluetooth_slave_dev temp;
                memset(&temp, 0, sizeof(struct bluetooth_slave_dev));
                projector_set_bt_dev(&temp);
                bluetooth_del_all_device();
                is_connected = false;
                sel_id=1;                    
            }else{
                if(is_connected){
                    connet_status = BT_CONNECT_STATUS_DISCONNECTING;
                    bluetooth_disconnect();
                    sel_id=3;
                }else{
                    active_disconn = true;
                    bt_conn_type = CONN_TYPE_CONNECT;
                    char* mac = projector_get_bt_mac();
                    if(bluetooth_connect(mac)==0){
                        if(bt_mac_invalid(mac)){
                            bluetooth_stop_scan();
                            // bluetooth_disconnect();
                            printf("invalid mac address\n");    
                            sel_id = 3;                       
                        }else{
                            connet_status = BT_CONNECT_STATUS_CONNECTING;
                            api_set_bt_connet_status(BT_CONNECT_STATUS_CONNECTING);
                            bluetooth_wait(CONN_WAIT);
                            reset_timer = false;                            
                        }
                    }else{
                        bt_conn_type = -1;
                        sel_id = 3;
                        create_message_box(get_some_language_str("BT Connection Failed\0蓝牙连接失败\0BT Connection Failed",projector_get_some_sys_param(P_OSD_LANGUAGE))); 
                    }
                }
               
            }
            if(sel_id>-1){
                lv_group_focus_obj(bt_list_obj);
            }
            lv_obj_del(target);
            slave_scr_obj = NULL;

        }else if(key == LV_KEY_DOWN){
            lv_obj_clear_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED);
            lv_obj_add_state(lv_obj_get_child(target, 1), LV_STATE_CHECKED);
        }else if(key == LV_KEY_UP){
            lv_obj_clear_state(lv_obj_get_child(target, 1), LV_STATE_CHECKED);
            lv_obj_add_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED);           
        }else if(key == LV_KEY_ESC){
            sel_id = 3;
            lv_group_focus_obj(bt_list_obj);
            lv_obj_del(target);
            slave_scr_obj = NULL;
        }
    }else if(code == LV_EVENT_FOCUSED){
        lv_obj_add_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED);  
    }
}


static void bt_setting_event_handle1(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        reset_timer = true;      
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        
        if(key == LV_KEY_ENTER){
            if(sel_id == 0){
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON){
                #if PROJECTER_C2_D3000_VERSION
                    bluetooth_disconnect();
                #else
                    bluetooth_poweroff();
                #endif
                    found_bt_num = 0;
                    is_connected = false;
                    api_set_bt_connet_status(BT_CONNECT_STATUS_DEFAULT);
                    api_set_i2so_gpio_mute(false);//api_set_i2so_gpio_mute_auto();
                    connet_status = BT_CONNECT_STATUS_DEFAULT;
                    scan_status = BT_SCAN_STATUS_DEFAULT;
                    
                    remove_list_sub_objs(bt_list_obj, lv_obj_get_index(other_dev)+1, lv_obj_get_child_cnt(bt_list_obj));
                    if(3 != lv_obj_get_index(other_dev)){
                       remove_connected(3);
                    }
                   
                    hidden_on_list_sub_objs(bt_list_obj, 1, lv_obj_get_index(other_dev)+1);
                  
                    projector_set_some_sys_param(P_BT_SETTING, BLUETOOTH_OFF);
                    set_label_text2(lv_obj_get_child(lv_obj_get_child(bt_list_obj, 0), 1), STR_OFF, FONT_NORMAL);
                }else{
                    api_set_bt_connet_status(BT_CONNECT_STATUS_DEFAULT);
                    
                    if(bluetooth_poweron() == 0){
                        projector_set_some_sys_param(P_BT_SETTING, BLUETOOTH_ON);
                        printf("Device exists\n");
                        hidden_off_list_sub_obj(bt_list_obj, 1, lv_obj_get_index(other_dev)+1);
                        set_label_text2(lv_obj_get_child(lv_obj_get_child(bt_list_obj, 0), 1), STR_ON, FONT_NORMAL);
					#if PROJECTER_C2_D3000_VERSION
						bluetooth_scan();
					#endif
                        scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
                        connet_status = BT_CONNECT_STATUS_CONNECTING;
                        bt_conn_type = CONN_TYPE_POWER_ON;
                        active_disconn = true;
                        printf("scan bluettoth!");
                        bluetooth_wait(POWER_ON_WAIT);
                        reset_timer = false;
                    }else{
                        set_label_text2(lv_obj_get_child(lv_obj_get_child(bt_list_obj, 0), 1), STR_NO_BT, FONT_NORMAL);
                    }
                }
            }else if(sel_id == 1){
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON){
                    remove_list_sub_objs(bt_list_obj, lv_obj_get_index(other_dev)+1, lv_obj_get_child_cnt(bt_list_obj));
                    found_bt_num = 0;      
                    if(bluetooth_scan() == 0){
                        scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
                        active_disconn = true;
                       
                        printf("scan bluettoth!");
                        bluetooth_wait(SCAN_WAIT);
                        reset_timer = false;
                    }
                }
                
            }else if(sel_id>2){
                if(connet_status == BT_CONNECT_STATUS_DISCONNECTING){
                    return;
                }
                if(sel_id == 3){
                    if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_OFF){
                        printf("bt off\n");
                        return;
                    } 
                    saved_bt_widget = create_list_obj1(setup_slave_root, 18, 10);
                    lv_obj_set_style_bg_color(saved_bt_widget, lv_palette_darken(LV_PALETTE_GREY, 1),0);
                    lv_obj_set_style_bg_opa(saved_bt_widget, LV_OPA_70, 0);   
                    lv_obj_t *sub_obj;                  
                    if(is_connected){
                        sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50, STR_BT_DELETE, FONT_NORMAL);
                        lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                        sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50, STR_BT_MAKE_DISCONN, FONT_NORMAL);
                        lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                    }else{
                        sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50, STR_BT_DELETE, FONT_NORMAL);
                        lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                        sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50,STR_BT_MAKE_CONN, FONT_NORMAL);
                        lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                    }
                    connected_bt_id = sel_id;
                    lv_obj_add_event_cb(saved_bt_widget, my_bt_dev_event_handle, LV_EVENT_ALL, 0);
                    lv_group_focus_obj(saved_bt_widget); 
                    slave_scr_obj = saved_bt_widget;    
                    if(timer_setting){
                        lv_timer_reset(timer_setting);
                        lv_timer_resume(timer_setting);
                    }              
                    return;
                }

                char *text = lv_label_get_text(lv_obj_get_child(lv_obj_get_child(bt_list_obj, sel_id), 0));
                unsigned char mac[6] = {0};
               
                get_bt_mac(text, mac);

                if(bt_mac_invalid(mac)){
                    printf("invalid mac address!\n");
                    remove_bt_dev(sel_id-(int)lv_obj_get_index(other_dev)-1); 
                    remove_list_sub_obj(bt_list_obj, sel_id); 
                    sel_id = 1;
                    lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
                    return;
                }
                
                active_disconn = true;
                bt_conn_type = CONN_TYPE_CONNECT;
                
                if(bluetooth_connect(mac)==0){
                    for(int i=0; i<6; i++){
                        printf("%02x,", mac[i]);
                    }
                    printf("\n");
                   connected_bt_id = sel_id;
                    connet_status = BT_CONNECT_STATUS_CONNECTING;
                    // api_set_bt_connet_status(BT_CONNECT_STATUS_CONNECTING);
                    bluetooth_wait(CONN_WAIT);
                    reset_timer = false;
                }else{
                    bt_conn_type = -1;
                }

            }
        }else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            if(sel_id>1){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_DOT);
            }           
            sel_id = sel_id+1;
            while (sel_id<(int)lv_obj_get_child_cnt(target) && (lv_obj_get_child(target, sel_id)->class_p == &lv_list_text_class ||
                    lv_obj_has_flag(lv_obj_get_child(target, sel_id), LV_OBJ_FLAG_HIDDEN))){
                sel_id +=1;
            }
  
            if(sel_id == (int)lv_obj_get_child_cnt(target)){
                lv_group_focus_obj(tab_btns);
                return;
            }   
            if(sel_id>1){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_SCROLL_CIRCULAR);
            }                      
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
           
        }else if(key == LV_KEY_UP || key == LV_KEY_LEFT){

            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            if(sel_id>1){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_DOT);
            } 
            sel_id-=1;
            while (sel_id>-1 && (lv_obj_get_child(target, sel_id)->class_p == &lv_list_text_class ||
            lv_obj_has_flag(lv_obj_get_child(target, sel_id), LV_OBJ_FLAG_HIDDEN))){
                sel_id -=1;
            }
            if(sel_id == -1){
               lv_group_focus_obj(tab_btns);
                return;
            }
            if(sel_id>1){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_SCROLL_CIRCULAR);
            }     
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
    

        }else if(key == LV_KEY_HOME){
            turn_to_main_scr();
            return;

        }else if(key == LV_KEY_ESC){
            turn_to_main_scr();
            return;
        }
        if(timer_setting && reset_timer){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    }else if(code == LV_EVENT_FOCUSED){
        if(act_key_code == KEY_UP){
            sel_id = (int)lv_obj_get_child_cnt(target)-1;
            while (sel_id > -1 && (lv_obj_get_child(target, sel_id)->class_p == &lv_list_text_class ||
                lv_obj_has_flag(lv_obj_get_child(target, sel_id), LV_OBJ_FLAG_HIDDEN))){
                sel_id-=1;
            }
            if(sel_id>1){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_SCROLL_CIRCULAR);               
            }
        }else if(act_key_code == KEY_DOWN || act_key_code == KEY_OK){
            if(sel_id == -1){//没有修改默认值时置为0
                sel_id = 0;
            }
        }
        if(sel_id>-1)
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
    }else if(code == LV_EVENT_DEFOCUSED){
        if(sel_id<(int)lv_obj_get_child_cnt(target)){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        }
        sel_id = -1;


    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
      
    }
}





static bool str_is_black(char *str){
    for(int i=0; i<strlen(str); i++){
        int c = str[i];
        printf("%02x\n", c);
        if( c != 1 && !isspace(c)){
            return false;
        }
    }
    return true;
}

static bool bt_mac_invalid(unsigned char* mac){
    for(int i=0; i<6; i++){
        printf("%2x ", mac[i]);
        if(mac[i] != 0){
            return false;
        }
    }
    printf("\n");
    return true;
}

static bool bt_dev_contained(char *mac){
    for(int i=0; i<found_bt_num; i++){
        if(strncmp(mac, devs_info[i].mac, 6) == 0){
            return true;
        }
    }
    return false;
}

static void bt_list_change_loc(lv_obj_t *prev, lv_obj_t *next){
    int begin_id = lv_obj_get_index(prev);
    int end_id = lv_obj_get_index(next);
    for(int i=end_id; i>=0 && i<lv_obj_get_child_cnt(bt_list_obj) && i>begin_id; i--){
        lv_obj_swap(lv_obj_get_child(bt_list_obj, i), lv_obj_get_child(bt_list_obj, i-1));
    }
}

static bool bt_has_my_dev(){
    return lv_obj_get_index(my_dev)+1 != lv_obj_get_index(other_dev);
}

int bt_event1(unsigned long event, unsigned long param){
    control_msg_t ctl_msg = {0};
     switch (event){
        case BLUETOOTH_EVENT_SLAVE_DEV_SCANNED:
            printf("BT_AD6956F_EVENT_SLAVE_DEV_SCANNED\n");
            scan_status=BT_SCAN_STATUS_GET_DATA_SEARCHED;
            if(param==0)break;
            if(found_bt_num>=5){
                break;
            }
           
            devs_info_t[found_bt_num]=(struct bluetooth_slave_dev*)param;
            if(strlen(devs_info_t[found_bt_num]->name) == 0 || str_is_black(devs_info_t[found_bt_num]->name)){
                break;
            }
            if(!wait_anim_timer){
                break;
            }
            if(bt_dev_contained(devs_info_t[found_bt_num]->mac)){
                break;
            }
            if(strncmp(projector_get_bt_mac(), devs_info_t[found_bt_num]->mac, 6) == 0){
                break;
            }
            memcpy(devs_info+found_bt_num, devs_info_t[found_bt_num],sizeof(struct bluetooth_slave_dev));
            printf("dev %d: %s\n",found_bt_num, devs_info[found_bt_num].name);
            if(bt_list_obj){
                bt_refresh_param.id = BT_LIST_EVENT_CREATE_NEW_BTN;
                bt_refresh_param.param1 = (void*)devs_info[found_bt_num].name;
                bt_refresh_param.param2 = 0;
                lv_event_send(bt_list_obj, LV_EVENT_REFRESH, &bt_refresh_param);
                //create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_STR, STR_BT_DISCONN);
            }
               found_bt_num += 1;
            scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED\n");
            scan_status = BT_SCAN_STATUS_GET_DATA_FINISHED;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED 1\n");
            is_connected = false;
            api_set_bt_connet_status(BT_CONNECT_STATUS_DISCONNECTED);
            api_set_i2so_gpio_mute(false);//api_set_i2so_gpio_mute_auto();
            if(bt_list_obj){
                printf("bluetooth_obj exit\n");
                if(lv_obj_get_child(bt_list_obj, 3) != other_dev){
                    bt_refresh_param.id = BT_LIST_EVENT_REMOVE_CONN;
                    bt_refresh_param.param1 = (void*)3;
                    lv_event_send(bt_list_obj, LV_EVENT_REFRESH, &bt_refresh_param);
                }
                if(connet_status == BT_CONNECT_STATUS_CONNECTED && !active_disconn){
                    create_message_box(get_some_language_str("BT Disconnected\0蓝牙已断开\0BT Disconnected", projector_get_some_sys_param(P_OSD_LANGUAGE)));
                }                    
            }
            if(bt_conn_type == CONN_TYPE_POWER_ON){
                create_message_box(get_some_language_str("BT Connection Failed\0蓝牙连接失败\0BT Connection Failed",
                projector_get_some_sys_param(P_OSD_LANGUAGE)));
            }
            ctl_msg.msg_type = MSG_TYPE_BT_DISCONNECTED;
            api_control_send_msg(&ctl_msg);
            connet_status = BT_CONNECT_STATUS_DISCONNECTED;
            break;
        //case BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED: 
        case BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO:
            active_disconn = false;
            is_connected = true;
            
            ctl_msg.msg_type = MSG_TYPE_BT_CONNECTED;
            api_control_send_msg(&ctl_msg);
            printf("BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED 1\n");

            if(bt_conn_type == CONN_TYPE_CONNECT){
                if(connected_bt_id>(int)lv_obj_get_index(other_dev)){

                    if (bt_has_my_dev()){
                        struct bluetooth_slave_dev temp;
                        memcpy(&temp, projector_get_bt_dev(), sizeof(struct bluetooth_slave_dev));

                        projector_set_bt_dev(&devs_info[connected_bt_id-(int)lv_obj_get_index(other_dev)-1]);
                        printf("%d is: %s\n", connected_bt_id-(int)lv_obj_get_index(other_dev)-1, devs_info[connected_bt_id-(int)lv_obj_get_index(other_dev)-1].name);
                        memcpy(&devs_info[connected_bt_id-(int)lv_obj_get_index(other_dev)-1], &temp, sizeof(struct bluetooth_slave_dev));                       

                    }else{
                        projector_set_bt_dev(&devs_info[connected_bt_id-(int)lv_obj_get_index(other_dev)-1]);
                        remove_bt_dev(connected_bt_id-(int)lv_obj_get_index(other_dev)-1);
                    }
                    if(bt_has_my_dev()){
                        if(bt_list_obj){
                            bt_refresh_param.id = BT_LIST_EVENT_SWAP_LOC;
                            bt_refresh_param.param1 = (void*)3;
                            bt_refresh_param.param2 = (void*)connected_bt_id;
                            lv_event_send(bt_list_obj, LV_EVENT_REFRESH, &bt_refresh_param);
                        }
                        //lv_obj_swap(lv_obj_get_child(bt_list_obj, 3), lv_obj_get_child(bt_list_obj, connected_bt_id));
                    }else{
                        if(bt_list_obj){
                            bt_refresh_param.id = BT_LIST_EVENT_MOVE_LOC;
                            bt_refresh_param.param2 = (void*)3;
                            bt_refresh_param.param1 = (void*)connected_bt_id;
                            lv_event_send(bt_list_obj, LV_EVENT_REFRESH, &bt_refresh_param);                          
                        }
                       //lv_obj_move_to_index(lv_obj_get_child(bt_list_obj, connected_bt_id), 3);
                    }
                }
                sel_id = 3;
            }else if(bt_conn_type == CONN_TYPE_POWER_ON){
                if(!bt_has_my_dev()){
                    projector_set_bt_dev((struct bluetooth_slave_dev *)param);
                    if(bt_list_obj){
                        bt_refresh_param.id = BT_LIST_EVENT_CREATE_NEW_BTN;
                        bt_refresh_param.param1 = (void*)projector_get_bt_name();
                        bt_refresh_param.param2 = (void*)3;
                        lv_event_send(bt_list_obj, LV_EVENT_REFRESH, &bt_refresh_param);
                    }
                }
               if(wait_anim_timer && wait_anim){
                    sel_id = 1;           
                    lv_group_focus_obj(bt_list_obj);
                    if(wait_anim_timer){
                        lv_timer_pause(wait_anim_timer);
                        lv_timer_del(wait_anim_timer);
                        wait_anim_timer = NULL;
                    }
                    if(wait_anim){
                        lv_obj_add_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
                    } 
               }
            }
			api_set_bt_connet_status(BT_CONNECT_STATUS_CONNECTED);
            api_set_i2so_gpio_mute(true);//api_set_i2so_gpio_mute_auto();
            //if(bt_conn_type == CONN_TYPE_CONNECT || bt_conn_type == CONN_TYPE_POWER_ON)
            {
                printf("connect_state: %d\n", connet_status);
                if(bt_list_obj){
                    bt_refresh_param.id = BT_LIST_EVENT_ADD_CONN;
                    bt_refresh_param.param1 = (void*)3;
                    lv_event_send(bt_list_obj, LV_EVENT_REFRESH, &bt_refresh_param);
                }
                create_message_box(get_some_language_str("BT Connected\0蓝牙已连接\0BT Connected", projector_get_some_sys_param(P_OSD_LANGUAGE)));
            }
            bt_conn_type = -1;
            connected_bt_id = -1;
            
            connet_status = BT_CONNECT_STATUS_CONNECTED;
            break;
       

        }
    return 0;
}
    



static lv_obj_t* create_list_bt_sub_btn_obj(lv_obj_t *parent,list_sub_param param1,int type1,  int str2){
    
    if(type1 == LIST_PARAM_TYPE_INT){
        return create_list_sub_btn_obj1(parent, param1.str_id, str2);
    }else if(type1 == LIST_PARAM_TYPE_STR){
        return create_list_sub_btn_obj2(parent, param1.str, str2);
    }
    
}






static lv_obj_t* create_list_sub_obj(lv_obj_t *parent, char *str){
    lv_obj_t *list_btn;
    list_btn = lv_list_add_text(parent, str);


    lv_obj_set_size(list_btn,100,17);
    lv_obj_set_style_pad_top(list_btn, 5, 0);
    lv_obj_set_style_text_align(list_btn, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_set_style_bg_color(list_btn, lv_color_make(101,101,177), 0);
    lv_obj_set_style_border_width(list_btn, 3, 0);
    lv_obj_set_style_border_color(list_btn, lv_color_make(101,101,177), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_btn, lv_color_black(), LV_STATE_CHECKED);
 
    return list_btn;
}





static lv_obj_t* create_list_sub_btn_obj(lv_obj_t *parent){
    lv_obj_t *list_btn;
    list_btn = lv_list_add_btn(parent, NULL, " ");
    lv_group_remove_obj(list_btn);

    lv_obj_set_size(list_btn,LV_PCT(100),LV_PCT(11));
    //lv_obj_set_style_pad_top(list_btn, 5, 0);
    //lv_obj_set_style_text_align(list_btn, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_border_side(list_btn, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_border_width(list_btn, 2, 0);
    lv_obj_set_style_border_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_100, LV_STATE_CHECKED);

    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);

    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_btn, lv_color_black(), LV_STATE_CHECKED);

    lv_obj_t* label = lv_obj_get_child(list_btn, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_grow(label, 9);   

    label = lv_label_create(list_btn);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_flex_grow(label, 5);  
    lv_obj_set_width(label, LV_SIZE_CONTENT);   
    return list_btn;
}

static lv_obj_t* create_list_sub_btn_obj1(lv_obj_t *parent, int str1, int str2){

    lv_obj_t *list_btn = create_list_sub_btn_obj(parent);


    lv_obj_t *label = lv_obj_get_child(list_btn, 0);
    if(str1>=0){  
        set_label_text2(label, str1, FONT_NORMAL);     
    }else{
        lv_label_set_text(label, " ");
    }

    label = lv_obj_get_child(list_btn, 1);
    if(str2>=0){       
        set_label_text2(label, str2, FONT_NORMAL);
    }else{
        lv_label_set_text(label, " ");
    }

    return list_btn;
}

static lv_obj_t* create_list_sub_btn_obj2(lv_obj_t *parent, char* str1, int str2){
    lv_obj_t *list_btn = create_list_sub_btn_obj(parent);

    lv_obj_t *label = lv_obj_get_child(list_btn, 0);
    lv_label_set_text(label, str1);
    lv_obj_set_style_text_font(label,&LISTFONT_3000, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    
    label = lv_obj_get_child(list_btn, 1);

    if(str2>=0){      
         set_label_text2(label, str2, FONT_NORMAL);
    }else{
        lv_label_set_text(label, " ");
    }

    return list_btn;
}

static void remove_list_sub_obj(lv_obj_t *parent, int id){  
    if(id >= lv_obj_get_child_cnt(parent)){
        return;
    }
    lv_obj_del(lv_obj_get_child(parent, id));

}

static void remove_list_sub_objs(lv_obj_t *parent, int start, int end){
    if(start>=lv_obj_get_child_cnt(parent)){
        return;
    }
    for(int i=end-1; i>=start; i--){
        remove_list_sub_obj(parent, i);
    }
}

static void hidden_off_list_sub_obj(lv_obj_t *parent, int start, int end){
    if(start>=lv_obj_get_child_cnt(parent)){
        return;
    }
    for(int i=end-1; i>=start; i--){
        lv_obj_clear_flag(lv_obj_get_child(parent, i), LV_OBJ_FLAG_HIDDEN);
    }
}

static void hidden_on_list_sub_objs(lv_obj_t *parent, int start, int end){
    if(start>=lv_obj_get_child_cnt(parent)){
        return;
    }
    for(int i=end-1; i>=start; i--){
        lv_obj_add_flag(lv_obj_get_child(parent, i), LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t* create_bt_list_obj(lv_obj_t *parent, int w, int h){
    lv_obj_t *obj = lv_list_create(parent);
    lv_obj_set_style_radius(obj, 0, 0);
    set_pad_and_border_and_outline(obj);
    lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_scroll_to_view(obj, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(obj, lv_color_make(101,101,177), 0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(lv_group_get_default(), obj);
    lv_group_focus_obj( obj);
    return obj;
}




lv_obj_t* create_list_bt_obj1(lv_obj_t *parent, int w, int h){
    lv_obj_t* obj = create_list_obj1(parent, w, h);
    lv_obj_add_event_cb(obj, bt_setting_event_handle1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(obj, bt_list_refresh_event_handle, LV_EVENT_REFRESH, NULL);
    return obj;
}

static lv_obj_t* create_list_sub_bt_text_obj1(lv_obj_t *parent, int w, int h, int str1){
    lv_obj_t *obj = create_list_sub_text_obj1(parent, w, h, str1, FONT_NORMAL);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT,0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_set_style_pad_left(obj, 20, 0);
    return obj;
}



static void get_bt_mac(char *name,unsigned char* mac){
    for(int i=0; i<4; i++){
        if(strcmp(name, devs_info[i].name) == 0){
            strncpy((char*)mac, (char*)devs_info[i].mac, 6);
        }
    }
}

static bool del_wait = false;
static uint total_wait_time = 0;
static void bluetooth_wait_timer_handle(lv_timer_t *timer_){
    static uint8_t i = 0;
    
   
    int radius1 = lv_disp_get_hor_res(lv_disp_get_default())/100*1.6;
    int radius2 = lv_disp_get_hor_res(lv_disp_get_default())/50;
    total_wait_time += 500;
    if(timer_setting){
        lv_timer_pause(timer_setting);
    }
    
    if( (total_wait_time > 20000 ||
        bt_wait_type == POWER_ON_WAIT && (scan_status == BT_SCAN_STATUS_GET_DATA_FINISHED || connet_status == BT_CONNECT_STATUS_CONNECTED)) ||
        (bt_wait_type == SCAN_WAIT && (scan_status == BT_SCAN_STATUS_GET_DATA_FINISHED)) ||
    (bt_wait_type == CONN_WAIT && connet_status == BT_CONNECT_STATUS_CONNECTED) ){
        i=0;
        printf("total_wait_timer: %d", total_wait_time);
        if(total_wait_time > 20000){
            is_connected = false;
            sel_id=1;
            bluetooth_stop_scan();
            if(scan_status == BT_SCAN_STATUS_GET_DATA_SEARCHING || scan_status == BT_SCAN_STATUS_GET_DATA_SEARCHED){
                printf("scan_status: %d\n", scan_status); 
            }
            if(connet_status == BT_CONNECT_STATUS_CONNECTING){
                printf("connet_status: %d\n", connet_status);
                create_message_box(get_some_language_str("BT Connection Failed\0蓝牙连接失败\0BT Connection Failed", projector_get_some_sys_param(P_OSD_LANGUAGE)));
                if(connected_bt_id>(int)lv_obj_get_index(other_dev)){
                    remove_bt_dev(connected_bt_id-(int)lv_obj_get_index(other_dev)-1);
                    remove_list_sub_obj(bt_list_obj, connected_bt_id); 
                    connected_bt_id = -1;
                  
                }
           }
        }

        total_wait_time = 0;
        printf("reset timer\n");
        bt_conn_type = -1;
        if(timer_setting){
            lv_timer_resume(timer_setting);
            lv_timer_reset(timer_setting); 
        }   
        scan_status = BT_SCAN_STATUS_DEFAULT;
        if(connet_status != BT_CONNECT_STATUS_CONNECTED){
            connet_status = BT_CONNECT_STATUS_DISCONNECTED;
        }
        set_remote_control_disable(false);
        lv_group_focus_obj(bt_list_obj);
        if(wait_anim_timer){
            lv_timer_pause(wait_anim_timer);
            lv_timer_del(wait_anim_timer);
            wait_anim_timer = NULL;
        }
        if(wait_anim){
            lv_obj_add_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    
    if(wait_anim){
        lv_obj_set_style_radius(lv_obj_get_child(wait_anim, i), radius1, 0);
        lv_obj_set_size(lv_obj_get_child(wait_anim, i), LV_PCT(20), LV_PCT(40));
        lv_obj_set_style_bg_color(lv_obj_get_child(wait_anim, i), lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
    

        i = (i+1)%3;
        lv_obj_set_style_radius(lv_obj_get_child(wait_anim, i), radius2, 0);
        lv_obj_set_size(lv_obj_get_child(wait_anim, i), LV_PCT(26), LV_PCT(50));
        lv_obj_set_style_bg_color(lv_obj_get_child(wait_anim, i), lv_palette_main(LV_PALETTE_BLUE), 0);        
    }

    if(bt_wait_type == CONN_WAIT && connet_status != BT_CONNECT_STATUS_CONNECTED){
        connet_status = BT_CONNECT_STATUS_CONNECTING;
    }else if(bt_wait_type == SCAN_WAIT){
        scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
   }else if(bt_wait_type == POWER_ON_WAIT){

   }
}

void del_bt_wait_anim(){
    if(wait_anim_timer){
        lv_timer_pause(wait_anim_timer);
        lv_timer_del(wait_anim_timer);
        wait_anim_timer = NULL;
    }          
    if(wait_anim ){
        lv_obj_del(wait_anim);
        wait_anim = NULL;
    }
    
    if(bt_wait_type == SCAN_WAIT && cur_scr != SCREEN_SETUP){
        bluetooth_stop_scan(); 
    }

}

static void bluetooth_wait_handle(lv_event_t *e){
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC && bt_wait_type == SCAN_WAIT && scan_status != BT_SCAN_STATUS_GET_DATA_SEARCHED){
            printf("stop scan\n");
            bluetooth_stop_scan();  
            scan_status = BT_SCAN_STATUS_DEFAULT;
            sel_id = 1;
            total_wait_time = 0;           
            lv_group_focus_obj(bt_list_obj);
            if(wait_anim_timer){
                lv_timer_pause(wait_anim_timer);
                lv_timer_del(wait_anim_timer);
                wait_anim_timer = NULL;
            }
            if(wait_anim){
                lv_obj_add_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
            } 
        }
    }
}

void BT_first_power_on(){
    api_set_bt_connet_status(BT_CONNECT_STATUS_DEFAULT);
    if(projector_get_some_sys_param(P_BT_SETTING)){
        bt_conn_type = CONN_TYPE_POWER_ON;
        if(bluetooth_poweron() == 0){
            printf("Device exist\n");
            unsigned char *mac = projector_get_bt_mac();
            printf("bt name is : %s\n", projector_get_bt_name());            
            for(int i=0; i<6; i++){
                printf("%02x,", mac[i]);
            }            
            #if PROJECTER_C2_D3000_VERSION
            //bt_conn_type = CONN_TYPE_CONNECT;
            if(bluetooth_connect(mac) == 0){
                if(bt_mac_invalid(mac)){
                    bluetooth_stop_scan();
                    // bluetooth_disconnect();
                    printf("invalid mac address\n");
                    return;
                }
                    printf("mac\n");
                    connet_status = BT_CONNECT_STATUS_CONNECTING;
            }else{
                bluetooth_stop_scan();
                // bluetooth_disconnect();
                bt_conn_type = -1;
            }
            #else
                connet_status = BT_CONNECT_STATUS_CONNECTING;
            #endif
        }else{
            bt_conn_type = -1;
        }
    }else{
    #if PROJECTER_C2_D3000_VERSION
        bluetooth_poweron();
    #else
        bluetooth_poweroff();
    #endif
        // bluetooth_disconnect();
        hidden_on_list_sub_objs(bt_list_obj, 1, lv_obj_get_index(other_dev)+1);
        api_set_i2so_gpio_mute(false);//api_set_i2so_gpio_mute_auto();
    }
}



static void bluetooth_wait(wait_type type){
    if(wait_anim){
        lv_obj_clear_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
    }else{
        wait_anim = lv_obj_create(other_dev);
        lv_obj_set_size(wait_anim, LV_PCT(16), LV_PCT(100));
        lv_obj_align(wait_anim, LV_ALIGN_TOP_MID, 0,0);
        lv_obj_set_style_bg_opa(wait_anim, LV_OPA_0, 0);
        lv_obj_set_style_pad_all(wait_anim, 0, 0);
        lv_obj_set_style_border_width(wait_anim, 0, 0);
        lv_obj_set_style_outline_width(wait_anim, 0, 0);
        lv_obj_set_scrollbar_mode(wait_anim, LV_SCROLLBAR_MODE_OFF);
        // prev_obj = lv_group_get_focused(lv_group_get_default());
        lv_group_add_obj(lv_group_get_default(), wait_anim);
        
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
    }
    lv_group_focus_obj(wait_anim);
    bt_wait_type = type;
    
    if (wait_anim_timer == NULL)
    {
        wait_anim_timer = lv_timer_create(bluetooth_wait_timer_handle, 500, (void*)wait_anim);
        lv_timer_reset(wait_anim_timer);       
    }else{
        lv_timer_resume(wait_anim_timer);
        lv_timer_reset(wait_anim_timer);
    }
    

 
    if(timer_setting){
        lv_timer_pause(timer_setting);
        printf("pause timer\n");
    }
        
    if(bt_wait_type == CONN_WAIT){
        set_remote_control_disable(true);
    }
}



static void bt_list_refresh_event_handle(lv_event_t *e){
    lv_obj_t* obj = lv_event_get_target(e);
    bt_refresh_event_param *param = (bt_refresh_event_param*)lv_event_get_param(e);

    switch (param->id){
        case BT_LIST_EVENT_ADD_CONN:{
            int id = (int)(param->param1);
            add_connected(id);            
        }
            break;
        case BT_LIST_EVENT_REMOVE_CONN:{
            int id = (int)(param->param1);
            remove_connected(id);
        }
            break;
        case BT_LIST_EVENT_CREATE_NEW_BTN:{
            list_sub_param pa;
            pa.str = (char*)(param->param1);
            lv_obj_t * obj = create_list_bt_sub_btn_obj(bt_list_obj, pa, LIST_PARAM_TYPE_STR, STR_BT_DISCONN);
            int loc = (int)(param->param2);
            if(loc){
                lv_obj_move_to_index(obj, loc);
            }            
        }
            break;
        case BT_LIST_EVENT_MOVE_LOC:{
            int loc1 = (int)(param->param1);
            int loc2 = (int)(param->param2);
            lv_obj_move_to_index(lv_obj_get_child(bt_list_obj, loc1), loc2);            
        }

            break;
        case BT_LIST_EVENT_SWAP_LOC:{
            int loc1 = (int)(param->param1);
            int loc2 = (int)(param->param2);           
            lv_obj_swap(lv_obj_get_child(bt_list_obj, loc1), lv_obj_get_child(bt_list_obj, loc2));            
        }
            break;
    }

}

void bt_init(){
    const char *devpath=NULL;
    int np = fdt_node_probe_by_path("/hcrtos/bluetooth");
    if(np>0)
    {
        if(!fdt_get_property_string_index(np, "devpath", 0, &devpath))
        {
            if(bluetooth_init(devpath, bt_event1) == 0){
                printf("%s %d bluetooth_init ok\n",__FUNCTION__,__LINE__);
            }else{
                printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
            }
        }
    }
    else
        printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
}

lv_obj_t* create_bt_page(lv_obj_t* parent){
    lv_obj_t *obj = create_page_(parent, NULL, 0);
    //pthread_mutex_init(&bt_mutex)
    
    bt_list_obj = create_list_bt_obj1(obj, 82, 100);
    lv_obj_set_style_pad_all(bt_list_obj, 0, 0);
    int bt_model_str2 = projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON ?
       STR_ON : STR_OFF;
   
    list_sub_param param;
    param.str_id = STR_BT_SETTING;
    create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_INT,bt_model_str2);
    param.str_id = STR_SEARCH_BT;
    create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_INT,BLANK_SPACE_STR);
    my_dev = create_list_sub_bt_text_obj1(bt_list_obj,100,11, STR_BT_MY_DEV);
    char* mac = projector_get_bt_mac();
    if(!bt_mac_invalid(mac)){
        param.str = projector_get_bt_name();
        create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_STR, STR_BT_DISCONN);
    }
    other_dev = create_list_sub_bt_text_obj1(bt_list_obj,100,11, STR_BT_OTHER_DEV);

    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    set_label_text2(label, STR_BT_DEVICE, FONT_NORMAL);
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_BT);
    lv_obj_center(icon);

    bt_init();
}

#endif



