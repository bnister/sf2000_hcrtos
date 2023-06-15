#include "app_config.h"
#include <stdio.h>



#ifdef WIFI_SUPPORT
#include "wifi.h"
#include<stdlib.h>
#include<unistd.h>


#ifdef __HCRTOS__
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#endif
#include <pthread.h>
#include "setup.h"

#include "factory_setting.h"

#include "mul_lang_text.h"
#include "com_api.h"
#include "osd_com.h"
#include "network_api.h"
#include "../../app_config.h"
#include "hcstring_id.h"
#include "../main_page/main_page.h"

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define WIFI_SCR_HOR_PAD 0
    #define WIFI_SETTING_HOR_PAD 0
    #define WIFI_LIST_HOR_PAD 0
    #define WIFI_SUB_LIST_HOR_PAD 0
    #define WIFI_SUB_LIST_FLEX_GROUP0 3
    #define WIFI_SUB_LIST_FLEX_GROUP1 41
    #define WIFI_SUB_LIST_FLEX_GROUP2 22
    #define WIFI_SUB_LIST_FLEX_GROUP3 2
    #define WIFI_SETTING_WIDTH_PCT 28
    #define WIFI_SETTING_HEIGHT_PCT 70
    #define WIFI_IP_MSG_WIDGET_WIDTH_PCT 70
    #define WIFI_IP_MSG_WIDGET_HEIGHT_PCT 82
    #define WIFI_IP_MSG_SUB_LIST_PAD_VER 2
    #define WIFI_IP_MSG_PAD_HOR 4
    #define WIFI_LIST_FONT SiYuanHeiTi_Light_3500_12_1b
    #define WIFI_CONNE_WIDGET_HEIGHT_PCT 70
    #define WIFI_CONNE_WIDGET_WIDTH_PCT 60
    #define WIFI_CONNE_WIDGET_ALIGN_TOP 1
    #define WIFI_CONNE_WIDGET_ALIGN_TOP0 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP1 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP2 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP3 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP4 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP5 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP6 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP7 0
    #define STR_WIFI_CONNECT_ERR_MSG_BOX_W 50
    #define STR_WIFI_SAVED_CONNECT_MSG_BOX_W 50
    #define STR_WIFI_CONNECT_ERR_MSG_BOX_H 35
    #define STR_WIFI_SAVED_CONNECT_MSG_BOX_H 20
    #define WIFI_RECONN_MSG_BOX_W 45
    #define WIFI_RECONN_MSG_BOX_H 35
    #define STR_WIFI_ADD_HIDDEN_NET_WIDGET_W 62
    #define STR_WIFI_ADD_HIDDEN_NET_WIDGET_H 50      
    #define STR_WIFI_ADD_HIDDEN_DROP_OBJ_W 75
    #define STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_LINE_SPACE 3
    #define STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_PAD 3
    #define WIFI_ONOFF_H 90
    #define WIFI_NEW_PWD_WIDGET_W 65
    #define WIFI_NEW_PWD_WIDGET_H 40
    #define WIFI_SUB_TEXT_LIST_V_PCT 10 //LV_SIZE_CONTEXT
    #define SEARCHING_LABEL_HOR_PCT 30
    #define WIFI_CONN_WIDGET_SET_H 0
#else
    #define WIFI_SCR_HOR_PAD lv_disp_get_hor_res(lv_disp_get_default())/24
    #define WIFI_SETTING_HOR_PAD -1 //默认
    #define WIFI_LIST_HOR_PAD -1 //默认
    #define WIFI_SUB_LIST_HOR_PAD -1//默认
    #define WIFI_SUB_LIST_FLEX_GROUP0 1
    #define WIFI_SUB_LIST_FLEX_GROUP1 20
    #define WIFI_SUB_LIST_FLEX_GROUP2 6
    #define WIFI_SUB_LIST_FLEX_GROUP3 1
    #define WIFI_SETTING_WIDTH_PCT 30
    #define WIFI_SETTING_HEIGHT_PCT 52
    #define WIFI_IP_MSG_WIDGET_WIDTH_PCT 40
    #define WIFI_IP_MSG_WIDGET_HEIGHT_PCT 70
    #define WIFI_IP_MSG_SUB_LIST_PAD_VER -1 //默认
    #define WIFI_IP_MSG_PAD_HOR -1//默认
    #define WIFI_LIST_FONT SiYuanHeiTi_Nor_7000_28_1b
    #define WIFI_CONNE_WIDGET_HEIGHT_PCT 52
    #define WIFI_CONNE_WIDGET_WIDTH_PCT 30
    #define WIFI_CONNE_WIDGET_ALIGN_TOP 3
    #define WIFI_CONNE_WIDGET_ALIGN_TOP0 7
    #define WIFI_CONNE_WIDGET_ALIGN_TOP1 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP2 7
    #define WIFI_CONNE_WIDGET_ALIGN_TOP3 0
    #define WIFI_CONNE_WIDGET_ALIGN_TOP4 7
    #define WIFI_CONNE_WIDGET_ALIGN_TOP5 3
    #define WIFI_CONNE_WIDGET_ALIGN_TOP6 7
    #define WIFI_CONNE_WIDGET_ALIGN_TOP7 6
    #define STR_WIFI_CONNECT_ERR_MSG_BOX_W 28 //默认
    #define STR_WIFI_SAVED_CONNECT_MSG_BOX_W 28 //默认
    #define STR_WIFI_CONNECT_ERR_MSG_BOX_H 0 //默认
    #define STR_WIFI_SAVED_CONNECT_MSG_BOX_H 0
    #define WIFI_RECONN_MSG_BOX_W 0 // 默认
    #define WIFI_RECONN_MSG_BOX_H 0 //默认
    #define STR_WIFI_ADD_HIDDEN_NET_WIDGET_W 30 //默认
    #define STR_WIFI_ADD_HIDDEN_NET_WIDGET_H 35 //默认
    #define STR_WIFI_ADD_HIDDEN_DROP_OBJ_W 65
    #define STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_LINE_SPACE -1 //默认
    #define STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_PAD -1 //默认
    #define WIFI_ONOFF_H -1//默认
    #define WIFI_NEW_PWD_WIDGET_W 40
    #define WIFI_NEW_PWD_WIDGET_H 28
    #define WIFI_SUB_TEXT_LIST_V_PCT 10
    #define SEARCHING_LABEL_HOR_PCT 23
    #define WIFI_CONN_WIDGET_SET_H 1
#endif

lv_obj_t *wifi_scr;
lv_group_t *wifi_g;

LV_FONT_DECLARE(font_china_22);

LV_IMG_DECLARE(wifi4);
LV_IMG_DECLARE(wifi3);
LV_IMG_DECLARE(wifi2);
LV_IMG_DECLARE(wifi1);
LV_IMG_DECLARE(wifi0);
LV_IMG_DECLARE(wifi_no);
LV_IMG_DECLARE(wifi_icon);
LV_IMG_DECLARE(refresh_icon);
LV_IMG_DECLARE(pwd_icon);
LV_IMG_DECLARE(no_pwd_icon);
LV_IMG_DECLARE(wifi_add_icon);
LV_IMG_DECLARE(return_main_icon);
LV_IMG_DECLARE(prompt_icon);

wifi_scan_type scan_type = WIFI_SCAN_SEARCH;
wifi_conn_type conn_type = WIFI_CONN_NORMAL;
int saved_wifi_sig_strength_max_id = -1;
lv_obj_t* wifi_lists;
lv_obj_t *wifi_onoff;
lv_obj_t *wifi_search;
lv_obj_t *wifi_add;
lv_obj_t *cur_wifi_setting_obj;
lv_obj_t *wifi_return;
lv_obj_t *wifi_save_obj;
lv_obj_t *wifi_nearby_obj;
static lv_obj_t* msg_box, *reconn_msg_box, *prompt_box;
static lv_obj_t* cur_scr_focused_obj;//

static int cur_wifi_id = -1;//wifi列表当前选中的id
static int cur_wifi_conning_id = -1;//wifi列表中当前正在连接的节点的id
static wifi_scan_node* cur_wifi_conning_node_p=NULL;//指向wifi列表中当前正在连接的节点
static lv_obj_t *show_ip_widget;
static lv_obj_t *wifi_conn_widget;
static int wifi_widget_x=0;
static int wifi_widget_y=0;
static lv_obj_t *wifi_add_hidden_net_widget;
static lv_obj_t *static_ip_input_widget;
static lv_obj_t *kb;
static bool wifi_had_con_by_cast = false;
static bool wifi_is_scaning = false;
static bool wifi_is_conning = false;
lv_obj_t *searching_label = NULL;//搜索时提示的指针
lv_timer_t *timer_scan=NULL;
extern lv_font_t* select_font_normal[3];
extern lv_font_t *select_middle_font[3];
extern lv_obj_t* slave_scr_obj;//top层上的对象
extern lv_obj_t *wifi_show;

static void wifi_list_clean();
void wifi_list_add(void *p);

static lv_obj_t* create_keypad_widget(lv_obj_t *, lv_event_cb_t,int);
static void list_sub_wifi_btn_3_event_handle(lv_event_t *e);
static void list_sub_wifi_btn_1_event_handle(lv_event_t *e);

static void event_handle(lv_event_t *e);
void wifi_screen_init();
extern lv_obj_t* create_list_obj1(lv_obj_t *parent, int w, int h);
extern lv_obj_t* create_list_sub_text_obj1(lv_obj_t *parent, int w, int h, int str1, int font_id);
lv_obj_t* create_list_sub_wifi_text_obj(lv_obj_t *parent, int str1);
static lv_obj_t* create_list_sub_text_obj(lv_obj_t *parent,int w, int h, list_sub_param param, int type);
static lv_obj_t* create_list_sub_wifi_ip_msg_btn_obj(lv_obj_t *parent, int key, bool prompt1, char *value, bool prompt2, bool is_disable);
static lv_obj_t* create_list_sub_wifi_setting_btn_obj(lv_obj_t *parent, void *icon, int str);
static lv_obj_t* create_list_sub_wifi_btn_obj(lv_obj_t *parent, void *icon, char* str1);
static lv_obj_t* create_list_sub_wifi_btn_obj0(lv_obj_t *parent, void *icon, char* str1, int str2);
static char* get_list_sub_wifi_ssid(int id);
static lv_obj_t* get_list_sub_wifi_status_obj(int id);
static lv_obj_t* get_list_sub_wifi_prompt_obj(int id);
//static void wifi_conne_task(void* parm);
static bool kb_enter_btn_run_cb0();
static void wifi_scan_task(void* parm);
static void seaching_timer_handle(lv_timer_t* t);
void btns_event_handle(lv_event_t *e);
static void checkout_event_handle(lv_event_t*e);
static void wifi_conne_kb_event_handle(lv_event_t* e);
static void pwd_event_handle(lv_event_t *e);
static void focus_kb_timer_handle(lv_timer_t *t);
static void ssid_event_handle(lv_event_t *e);
static void create_wifi_new_pwd_conne_widget();
static lv_obj_t* create_ip_msg_widget(char* ssid,char* net_mode, char* ip_addr, char *net_mask, char *gateway, char *dns, char *mac);
static void create_conn_ip_msg_widget(char* ssid,char* net_mode, char* ip_addr, char *net_mask, char *gateway, char *dns, char *mac);
static void create_wifi_conne_widget_(char* name, char* strength, char *security, char* pwd);
static void create_connection_widget(char* name, char* strength, char *security);
static void create_static_ip_input_widget(char *name, int sel_id);
static void static_ip_keyboard_handle(lv_event_t* e);
static void ip_msg_event_handle(lv_event_t *e);
void wifi_onoff_event_handle(lv_event_t *e);
void wifi_search_event_handle(lv_event_t *e);
void wifi_add_event_handle(lv_event_t *e);
void wifi_return_event_handle(lv_event_t *e);
void wifi_sw_event_handle(lv_event_t *e);
void wifi_lists_event_handle(lv_event_t *e);
static void slave_scr_clear();
static int wifi_connect(char *ssid , char *pwd);
int projector_wifi_scan();
//int projector_wifi_init(void);
int projector_wifi_exit(void);
//int projector_wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out);
static void win_wifi_control(void* arg1, void* arg2);
//wifi 保存列表
extern bool get_m_wifi_connecting();
extern void set_remote_control_disable(bool b);
wifi_scan_node_head wifi_node_h = {NULL};

wifi_scan_list wifi_list = {&wifi_node_h, 0, NULL,NULL};
hccast_wifi_ap_info_t hidden_ap_info = {0};
hccast_wifi_ap_info_t cur_conne = {0};
hccast_wifi_ap_info_t *cur_conne_p = NULL;

hccast_udhcp_result_t udhcp_result={0};


void wifi_list_add(void *p){
    
    if(wifi_list.p_n != NULL){
        memcpy(&(wifi_list.p_n->res), p, sizeof(hccast_wifi_ap_info_t));
        wifi_list.tail = wifi_list.p_n;
        wifi_list.p_n = wifi_list.p_n->next;  
         
    }else{
        wifi_scan_node *node = (wifi_scan_node*)lv_mem_alloc(sizeof(wifi_scan_node));
        lv_memset_00(node, sizeof(wifi_scan_node));
        memcpy(&node->res, p, sizeof(hccast_wifi_ap_info_t));
        node->next = NULL;
         printf("get wifi %s\n", node->res.ssid);
        if(NULL == wifi_list.head->next){
            wifi_list.head->next = node;
            
        }else
        {
            node->prev = wifi_list.tail;
            wifi_list.tail->next = node;
        }
        wifi_list.tail = node;
    }
     wifi_list.len += 1;
}

void wifi_list_swap(wifi_scan_node* p1, wifi_scan_node* p2){
    if(!p1 || !p2){
        return;
    }
    wifi_scan_node *prev, *next;
    prev = p1->prev;
    next = p1->next;
    p1->next = p2->next;
    p1->prev = p2->prev;

    p2->prev = prev;
    p2->next = next;
}



void wifi_list_set_zero(){
    // for(wifi_scan_node *node = wifi_list.head->next; node != NULL; node = node->next){
    //     lv_memset_00(&(node->res), sizeof(hccast_wifi_ap_info_t));
    // }
    wifi_list.len = 0;
    wifi_list.p_n = wifi_list.head->next;
}

void wifi_list_remove(wifi_scan_node *node, bool copy){//copy从list删除并保存到其他地方
    wifi_scan_node *temp = node;
   
    if(temp == wifi_list.head->next){
        temp->next->prev = NULL;
        wifi_list.head->next = temp->next;
        temp = temp->next;
    }else if(temp == wifi_list.tail){
        temp->prev->next = NULL;
        wifi_list.tail = temp->prev;
        temp = temp->prev;
    }else{
        temp->prev->next = temp->next;   
        temp->next->prev = temp->prev; 
        temp = temp->next;          
    }
    if(node == wifi_list.p_n){
        wifi_list.p_n = temp;
    } 


    if(copy){
        memcpy(&cur_conne, &node->res, sizeof(hccast_wifi_ap_info_t));
        cur_conne_p = &cur_conne;
    }

    lv_mem_free(node);
}

void wifi_list_insert(hccast_wifi_ap_info_t* info, wifi_scan_node *p_after){
    wifi_scan_node *node = (wifi_scan_node*)lv_mem_alloc(sizeof(wifi_scan_node));
    lv_memset_00(node, sizeof(wifi_scan_node));
    memcpy(&node->res, info, sizeof(hccast_wifi_ap_info_t));
    node->next=p_after->next;
    if(p_after == wifi_list.tail){
        wifi_list.tail = node;
    }else{
        node->next->prev=node;
    }

    node->prev=p_after;
    p_after->next=node;
}

static void wifi_list_clean(){
    wifi_scan_node *node_p = wifi_list.head->next;
    wifi_scan_node *temp_p;
    while (node_p){
        temp_p = node_p->next;
        lv_mem_free(node_p);
        node_p = temp_p;
    }  
    wifi_list.p_n = NULL;
    wifi_list.head->next = NULL;
    wifi_list.tail = NULL;
    wifi_list.len = 0;
}

enum {
    SET_CUR_FIRST,
    SET_CUR_PREV,    
    SET_CUR_NEXT,
    SET_CUR_LAST
};


void wifi_list_set_cur(int i){
    if(i==SET_CUR_FIRST){
        wifi_list.p_n = wifi_list.head->next;//第一个
    }else if(i==SET_CUR_PREV){
        wifi_list.p_n =  wifi_list.p_n != NULL ? wifi_list.p_n->prev : wifi_list.head->next;
    }else if (i==SET_CUR_NEXT){    
        wifi_list.p_n = wifi_list.p_n != NULL ? wifi_list.p_n->next : wifi_list.head->next;
    }else if (i==SET_CUR_LAST){
        wifi_list.p_n = wifi_list.tail;
    }
}

wifi_scan_node* wifi_list_get_cur(){
    if(wifi_list.p_n == NULL){
        return NULL;
    }
    return wifi_list.p_n;
}

int wifi_list_get_len(){
    return wifi_list.len;
}

static char* wifi_get_quality(int i){
    if(i>=80 && i<=100){
        return "very good";//暂时直接返还
    }else if(i>=60 && i<80){
        return "good";
    }else if (i>=40 && i<60)
    {
       return "normal";
    }else if(i>=20 && i<40){
        return "bad";
    }else{
        return "no signal";
    }
    
}

static char* wifi_get_entryMode(int i){
    switch (i)
    {
    case HCCAST_WIFI_ENCRYPT_MODE_NONE:
        return "None";
    case HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP:
        return "WEP";
    case  HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP:
        return "SHARED WEP";
    case HCCAST_WIFI_ENCRYPT_MODE_WPAPSK_TKIP:
    case HCCAST_WIFI_ENCRYPT_MODE_WPAPSK_AES:
        return "WPA-PSK";
    case HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_TKIP:
    case HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_AES:;
    case  HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE:
        return "WPA2-PSK";
    default:
        break;
    }
    return "";
}

void set_wifi_had_con_by_cast(bool b){
    wifi_had_con_by_cast = b;
}

bool wifi_is_conning_get(){
    return wifi_is_conning;
}

static lv_group_t* kb_g;
static lv_group_t* default_g;
static void kb_g_onoff(bool en){
    if(en){
        default_g = lv_group_get_default();
        kb_g = lv_group_create();
        lv_group_set_default(kb_g);
        lv_indev_set_group(lv_indev_get_act(), kb_g);
    } else{
        lv_group_set_default(default_g);
        lv_indev_set_group(lv_indev_get_act(), default_g);
        lv_group_remove_all_objs(kb_g);
        lv_group_del(kb_g);
        kb_g = NULL;
    }
}

static char old_ip[MAX_IP_STR_LEN]={0};

void wifi_get_udhcp_result(hccast_udhcp_result_t* result){
    memcpy(&udhcp_result, result, sizeof(hccast_udhcp_result_t));
}

static void update_saved_wifi_list(){
    int size = lv_obj_get_index(wifi_nearby_obj);
    for(int i=lv_obj_get_index(wifi_save_obj)+1, j=i; i<size; i++){
        lv_obj_del(lv_obj_get_child(wifi_lists, j));
    }
    int i=0;
    lv_obj_t *obj;
    for (hccast_wifi_ap_info_t * res = sysdata_get_wifi_info_by_index(i); res != NULL; res = sysdata_get_wifi_info_by_index(++i)){
        if(lv_obj_get_index(wifi_save_obj)>0 && strcmp(res->ssid, lv_label_get_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 1)))==0){
            continue;
        }
        if(cur_wifi_conning_id>(int)lv_obj_get_index(wifi_nearby_obj) && strcmp(res->ssid, get_list_sub_wifi_ssid(cur_wifi_conning_id))==0){
            continue;
        }
        int quatity = res->quality;
        void *img = (quatity<=100 && quatity>=75) ? &wifi4 :
                    (quatity<75 && quatity>50) ? &wifi3 :
                    (quatity<50 && quatity>25) ? &wifi2 : &wifi1;
        obj = create_list_sub_wifi_btn_obj0(wifi_lists, img, res->ssid, STR_BT_SAVED);
        lv_obj_move_to_index(obj, lv_obj_get_index(wifi_nearby_obj));
    }
}

static void wifi_reconne_exec_cb(void * obj, int32_t v){//修改WiFi密码后自动重连
    if(app_wifi_connect_status_get() && hccast_wifi_mgr_get_connect_status()){
        set_remote_control_disable(false);
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
        if(prompt_box){
            lv_obj_del(prompt_box);
            prompt_box=NULL;
        }
        create_message_box(api_rsc_string_get(STR_WIFI_CONN_SUCCESS));
        return;
    }
    lv_arc_set_value(obj, v);
}

//当前screen对象事件处理

static void event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        }
    }else if(code == LV_EVENT_SCREEN_LOADED){
        key_set_group(wifi_g);
        lv_group_focus_obj(wifi_onoff);
        if(!network_wifi_module_get()){
            create_message_box((char *)api_rsc_string_get(STR_WIFI_NO_DEVICE));
            return;
        }

         if(lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN)){  
            return;
         }

       if(get_save_wifi_flag()==1){
            if(!wifi_is_conning){
                update_saved_wifi_list();
            }
            set_save_wifi_flag_zero();
       }

        if(app_wifi_connect_status_get()){//m_wifi_config.bConnected
            if(!cur_conne_p){
                memcpy(&cur_conne, sysdata_get_wifi_info_by_index(0), sizeof(hccast_wifi_ap_info_t));
                cur_conne_p = &cur_conne;
            }
           if(lv_obj_get_index(wifi_save_obj)>0 && strncmp(old_ip, app_wifi_local_ip_get(), MAX_IP_STR_LEN)==0){
                if(!wifi_is_conning){
                    int status = hccast_wifi_mgr_get_connect_status();
                    if(status == 1){
                        scan_type =WIFI_SCAN_ENTER;
                        projector_wifi_scan();                        
                    }else{
                        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
                        set_remote_control_disable(true);
                        prompt_box = loader_with_arc((char*)api_rsc_string_get(STR_WIFI_CONNING),  wifi_reconne_exec_cb);
                    }

                }
            }else{
                if(!wifi_is_conning){//在无线投屏中连上了wifi
                    int size = lv_obj_get_index(wifi_nearby_obj);
                    char cur_ssid[64] = {0};
                    hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
                    if(lv_obj_get_index(wifi_save_obj)>0){
                        if(strcmp(cur_ssid, lv_label_get_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 1)))!=0){
                                for(int i=lv_obj_get_index(wifi_save_obj)+1; i<size; i++){
                                    if(strcmp(cur_ssid, lv_label_get_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, i), 1))) == 0){
                                        lv_label_set_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, i), 1), 
                                                            lv_label_get_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 1)));
                                        lv_label_set_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 1), cur_ssid);
                                        break;
                                    }
                            }    
                        }
                    }else{
                        for(int i=lv_obj_get_index(wifi_save_obj)+1; i<size; i++){
                            if(strcmp(cur_ssid, lv_label_get_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, i), 1))) == 0){
                                //language_choose_add_label1(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, i), 2), 0), STR_BT_CONN);
                                set_label_text2(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, i), 2), 0), STR_BT_CONN, FONT_NORMAL);
                                
                                lv_obj_move_to_index(lv_obj_get_child(wifi_lists, i), 0);
                                lv_obj_clear_flag(get_list_sub_wifi_prompt_obj(0), LV_OBJ_FLAG_HIDDEN);
                                break;
                            }
                        }
                    }
                    scan_type =WIFI_SCAN_ENTER;
                    projector_wifi_scan();
                }else{
                    control_msg_t ctl_msg = {0};
                    ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
                    win_wifi_control(&ctl_msg, NULL); 
                }
            }
        }else{
            if(wifi_is_conning && !get_m_wifi_connecting()){//下层已不再连接中，但ui还未更新
                control_msg_t ctl_msg = {0};
                ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
                win_wifi_control(&ctl_msg, NULL);              
            }else if(!wifi_is_conning){
                if(lv_obj_get_index(wifi_save_obj)>0){//第一行显示已连接的wifi
                    control_msg_t ctl_msg = {0};
                    ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
                    win_wifi_control(&ctl_msg, NULL);
                }
                scan_type =WIFI_SCAN_ENTER;
                projector_wifi_scan();                
            }
        }    
    }else if(code == LV_EVENT_SCREEN_UNLOADED){
        

        if(lv_obj_is_valid(wifi_conn_widget)){
            /*修改密码后再连接*/
            if(cur_wifi_conning_id<lv_obj_get_index(wifi_nearby_obj) && cur_wifi_conning_id>lv_obj_get_index(wifi_save_obj)){
                set_label_text2(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), STR_BT_SAVED, FONT_NORMAL);
                cur_wifi_conning_id = -1;
                cur_conne_p = NULL;
            }
        }
        if (reconn_msg_box){
            if(cur_wifi_conning_id>(int)lv_obj_get_index(wifi_nearby_obj)){
                set_label_text2(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), STR_BT_SAVED, FONT_NORMAL);
                cur_wifi_conning_id = -1;
                cur_conne_p = NULL;
            }

        }

        slave_scr_clear();

        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
        set_save_wifi_flag_zero();
        if(cur_wifi_id>=0 && cur_wifi_id<lv_obj_get_child_cnt(wifi_lists)){
            lv_obj_clear_state(lv_obj_get_child(wifi_lists, cur_wifi_id), LV_STATE_CHECKED);
            cur_wifi_id = -1;
        }
        strncpy(old_ip, app_wifi_local_ip_get(), MAX_IP_STR_LEN);
    }
}

static void slave_scr_clear(){
    if(cur_scr_focused_obj){
        lv_group_focus_obj(cur_scr_focused_obj);
        cur_scr_focused_obj = NULL;
    }
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
    if(show_ip_widget){
        lv_obj_del(show_ip_widget);
        show_ip_widget=NULL;
    }
    if(static_ip_input_widget){
        lv_obj_del(static_ip_input_widget);
        static_ip_input_widget = NULL;
    }
    if(wifi_add_hidden_net_widget){
        
        lv_obj_del(wifi_add_hidden_net_widget);
        wifi_add_hidden_net_widget = NULL;
    }

    if(wifi_conn_widget){
        lv_obj_del(wifi_conn_widget);
        wifi_conn_widget=NULL;
    }

    if(reconn_msg_box){
        lv_obj_del(reconn_msg_box);
        reconn_msg_box = NULL;
    }
    if(kb){
        lv_obj_del(kb);
        kb=NULL;
    }
}




void wifi_screen_init(){//wifi初始化
    wifi_g = lv_group_create();
    lv_group_t* d_g = lv_group_get_default();
    lv_group_set_default(wifi_g);
    wifi_scr = lv_obj_create(NULL);
    lv_group_add_obj(wifi_g,wifi_scr);
    lv_obj_set_style_pad_ver(wifi_scr, lv_disp_get_ver_res(lv_disp_get_default())/30, 0);
    lv_obj_set_style_pad_hor(wifi_scr,WIFI_SCR_HOR_PAD, 0);
    lv_obj_set_style_pad_gap(wifi_scr, 0, 0);
    lv_obj_set_style_bg_color(wifi_scr, lv_color_make(81, 100, 117), 0);

    lv_obj_add_event_cb(wifi_scr, event_handle, LV_EVENT_ALL, 0);


    screen_entry_t wifi_entry;
    wifi_entry.screen = wifi_scr;
    wifi_entry.control = win_wifi_control;
    api_screen_regist_ctrl_handle(&wifi_entry);

    //初始化代码

    lv_obj_set_flex_flow(wifi_scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_scr,LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *wifi_setting = lv_obj_create(wifi_scr);
    lv_obj_set_size(wifi_setting,LV_PCT(WIFI_SETTING_WIDTH_PCT),LV_PCT(100));
    lv_obj_set_style_border_side(wifi_setting, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_radius(wifi_setting, 0, 0);
    lv_obj_set_style_pad_hor(wifi_setting, 0, 0);
  lv_obj_set_style_bg_opa(wifi_setting, LV_OPA_0, 0);    

    wifi_setting = create_list_obj1(wifi_setting,100 ,WIFI_SETTING_HEIGHT_PCT);
    lv_obj_set_style_bg_opa(wifi_setting, LV_OPA_0, 0);
    if(WIFI_SETTING_HOR_PAD>=0){
        lv_obj_set_style_pad_hor(wifi_setting, WIFI_SETTING_HOR_PAD, 0);
    }
    wifi_onoff = create_list_sub_wifi_setting_btn_obj(wifi_setting, &wifi_icon, -1);
    lv_obj_add_event_cb(wifi_onoff, wifi_onoff_event_handle,LV_EVENT_ALL, 0);
    if(projector_get_some_sys_param(P_WIFI_ONOFF)){
        lv_obj_add_state(lv_obj_get_child(wifi_onoff, 1) , LV_STATE_CHECKED);
    }
    wifi_search = create_list_sub_wifi_setting_btn_obj(wifi_setting, &refresh_icon, STR_WIFI_SEARCH);
    lv_obj_add_event_cb(wifi_search, wifi_search_event_handle, LV_EVENT_ALL, 0);
    wifi_add = create_list_sub_wifi_setting_btn_obj(wifi_setting, &wifi_add_icon, STR_WIFI_ADD);
    lv_obj_add_event_cb(wifi_add, wifi_add_event_handle, LV_EVENT_ALL, 0);
    wifi_return = create_list_sub_wifi_setting_btn_obj(wifi_setting, &return_main_icon, STR_WIFI_RET);
    lv_obj_add_event_cb(wifi_return, wifi_return_event_handle, LV_EVENT_ALL, 0);

    wifi_lists = create_list_obj1(wifi_scr,100-WIFI_SETTING_WIDTH_PCT, 100);
    lv_obj_add_flag(wifi_lists, LV_OBJ_FLAG_CHECKABLE);
    if(WIFI_LIST_HOR_PAD>=0){
        lv_obj_set_style_pad_hor(wifi_lists, WIFI_LIST_HOR_PAD, 0);
    }
    lv_obj_set_style_bg_opa(wifi_lists, LV_OPA_0, 0);
    if (!projector_get_some_sys_param(P_WIFI_ONOFF)){
        lv_obj_add_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN);
    }
   
    lv_obj_add_event_cb(wifi_lists, wifi_lists_event_handle, LV_EVENT_ALL, 0);

    wifi_save_obj = create_list_sub_wifi_text_obj(wifi_lists, STR_WIFI_SAVED);
    int i=0;

    for (hccast_wifi_ap_info_t * res = sysdata_get_wifi_info_by_index(i); res != NULL; res = sysdata_get_wifi_info_by_index(++i)){
        int quatity = res->quality;
        void *img = (quatity<=100 && quatity>=75) ? &wifi4 :
                    (quatity<75 && quatity>50) ? &wifi3 :
                    (quatity<50 && quatity>25) ? &wifi2 : &wifi1;
        create_list_sub_wifi_btn_obj0(wifi_lists, img, res->ssid, STR_BT_SAVED);
    }
    
    wifi_nearby_obj = create_list_sub_wifi_text_obj(wifi_lists,STR_WIFI_NEARBY);


    lv_group_set_default(d_g);
}




// lv_obj_t* create_list_obj1(lv_obj_t *parent, int w, int h){
//     lv_obj_t *obj = lv_list_create(parent);
//     lv_obj_set_style_radius(obj, 0, 0);
//     lv_obj_set_style_border_width(obj, 0, 0);
//     lv_obj_set_style_outline_width(obj, 0, 0);

//     lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
//     lv_obj_scroll_to_view(obj, LV_ANIM_OFF);
//     lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
//     lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
//     lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
//     lv_group_add_obj(lv_group_get_default(), obj);
//     return obj;
// }

// lv_obj_t* create_list_sub_text_obj1(lv_obj_t *parent, int w, int h, int str1){
//     list_sub_param param;
//     param.str_id = str1;
//     lv_obj_t *list_label =  create_list_sub_text_obj(parent,w,h,param, LIST_PARAM_TYPE_INT);
//     return list_label;
// }


lv_obj_t* create_list_sub_wifi_text_obj(lv_obj_t *parent, int str1){
    list_sub_param param;
    param.str_id = str1;
    return create_list_sub_text_obj(parent,100,WIFI_SUB_TEXT_LIST_V_PCT,param, LIST_PARAM_TYPE_INT);
}


static lv_obj_t* create_list_sub_text_obj(lv_obj_t *parent,int w, int h, list_sub_param param, int type){
    lv_obj_t *list_label;
    list_label = lv_list_add_text(parent, " ");
    lv_obj_set_style_text_align(list_label, LV_TEXT_ALIGN_LEFT, 0);
    if(WIFI_SUB_TEXT_LIST_V_PCT<0){
        lv_obj_set_size(list_label,LV_PCT(w),LV_SIZE_CONTENT);
    }else{
        lv_obj_set_size(list_label,LV_PCT(w),LV_PCT(WIFI_SUB_TEXT_LIST_V_PCT));
    }
    
    lv_obj_set_style_pad_ver(list_label, 0, 0);
    //lv_obj_set_style_pad_left(list_label,LV_PCT(1), 0);

    lv_obj_set_style_border_side(list_label, LV_BORDER_SIDE_FULL, LV_STATE_CHECKED);
    lv_obj_set_style_border_width(list_label, 0, 0);
    lv_obj_set_style_border_color(list_label, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_label, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(list_label, LV_OPA_100, LV_STATE_CHECKED);

    lv_obj_set_style_bg_opa(list_label, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_label, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(list_label, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);

    lv_obj_set_style_text_color(list_label, lv_color_white(), 0);

    if(type == LIST_PARAM_TYPE_INT){
       //language_choose_add_label1(list_label, param.str_id);
       set_label_text2(list_label, param.str_id, FONT_NORMAL);
    }else if(type == LIST_PARAM_TYPE_STR){
        lv_label_set_text(list_label, param.str);
        lv_obj_set_style_text_font(list_label, &lv_font_montserrat_26,0);
    }

    return list_label;
}

static lv_obj_t* create_list_sub_wifi_ip_msg_btn_obj(lv_obj_t *parent, int key, bool prompt1, char *value, bool prompt2, bool is_disable){
    lv_obj_t *list_btn = lv_list_add_btn(parent, NULL, NULL);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_pad_hor(list_btn, 0, 0);
    if(WIFI_IP_MSG_SUB_LIST_PAD_VER>=0){
        lv_obj_set_style_pad_ver(list_btn, WIFI_IP_MSG_SUB_LIST_PAD_VER, 0);
    }
    lv_group_remove_obj(list_btn);

    if(is_disable){
        lv_obj_set_style_text_color(list_btn, lv_color_make(175,175,175), 0);
    }else{
        lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    }
    lv_obj_t *obj;

    obj = lv_label_create(list_btn);
    lv_label_set_text(obj , (char *)api_rsc_string_get(key));
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID), 0);
    lv_obj_set_flex_grow(obj, 11);
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(obj, 0, 0);

    if(prompt1){
        obj = lv_label_create(list_btn);
        lv_label_set_text(obj, LV_SYMBOL_LEFT);
        lv_obj_set_flex_grow(obj, 1);
    }

    obj = lv_label_create(list_btn);
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID), 0);
    lv_label_set_text(obj, value);
    lv_obj_set_flex_grow(obj, 17);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(obj, 0, 0);

    if(prompt2){
        obj = lv_label_create(list_btn);
        lv_label_set_text(obj, LV_SYMBOL_RIGHT);
        lv_obj_set_flex_grow(obj, 1);
    }
    return list_btn;
}



static lv_obj_t* create_list_sub_wifi_setting_btn_obj(lv_obj_t *parent, void *icon, int id){
    lv_obj_t *list_btn = lv_obj_create(parent);
    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);

    lv_group_add_obj(lv_group_get_default(), list_btn);
    if(icon){
        lv_obj_t *img = lv_img_create(list_btn);

        lv_img_set_src(img, icon);
        lv_obj_align(img, LV_ALIGN_LEFT_MID, 0, 0);
    }
    if(id == -1){
        lv_obj_t *sw = lv_switch_create(list_btn);

        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(sw, wifi_sw_event_handle, LV_EVENT_ALL, 0);
        if(WIFI_ONOFF_H>=0){
            lv_obj_set_height(sw, lv_pct(WIFI_ONOFF_H));
        }
        
    } else{
        lv_obj_t *label = lv_label_create(list_btn);
        lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
        //language_choose_add_label1(label, id);
        set_label_text2(label, id, FONT_NORMAL);
        lv_obj_set_style_pad_ver(label, 0, 0);
        //lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
    }


    lv_obj_set_size(list_btn,LV_PCT(100),LV_PCT(25));

    //lv_obj_set_style_pad_gap(list_btn,40, 0);

    lv_obj_set_style_border_side(list_btn, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_border_width(list_btn, 2, 0);
    lv_obj_set_style_border_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_100, LV_STATE_FOCUSED);

    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(list_btn, 0, 0);    

    lv_obj_t *obj = lv_obj_get_child(list_btn, 0);
    lv_obj_set_flex_grow(obj, 1);
    lv_obj_set_style_pad_left(obj, 0, 0);

    obj = lv_obj_get_child(list_btn, 1);
    lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_style_pad_right(obj, 0, 0);

    return list_btn;
}

static char* get_list_sub_wifi_ssid(int id){
    if(id>=0 && id < lv_obj_get_child_cnt(wifi_lists)){
        return lv_label_get_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, id), 1));
    }else{
        return NULL;
    }
    
}

static lv_obj_t* get_list_sub_wifi_status_obj(int id){
    if(id>=0 && id < lv_obj_get_child_cnt(wifi_lists)){
        return lv_obj_get_child(lv_obj_get_child(wifi_lists, id), 2);
    }else{
        return NULL;
    }
}

static lv_obj_t* get_list_sub_wifi_prompt_obj(int id){
        if(id>=0 && id < lv_obj_get_child_cnt(wifi_lists)){
        return lv_obj_get_child(lv_obj_get_child(wifi_lists, id), 3);
    }else{
        return NULL;
    }
}

static lv_obj_t* create_list_sub_wifi_btn_obj0(lv_obj_t *parent, void *icon, char* str1, int str2){
    lv_obj_t *list_btn;
    list_btn = lv_list_add_btn(parent, NULL, NULL);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);

    //lv_obj_set_style_pad_hor(list_btn, 0, 0);
    lv_group_remove_obj(list_btn);
    lv_obj_set_flex_align(list_btn, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(list_btn,LV_PCT(100),LV_PCT(10));
    lv_obj_set_style_border_side(list_btn, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_border_width(list_btn, 2, 0);
    lv_obj_set_style_border_color(list_btn, lv_palette_lighten(LV_PALETTE_BLUE, 1), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_radius(list_btn, 7, 0);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    if(WIFI_SUB_LIST_HOR_PAD>=0){
        lv_obj_set_style_pad_hor(list_btn, WIFI_SUB_LIST_HOR_PAD, 0);
    }
    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);

    lv_obj_t *obj = lv_img_create(list_btn);
    lv_obj_set_flex_grow(obj, WIFI_SUB_LIST_FLEX_GROUP0);
    lv_img_set_src(obj, icon);

    obj = lv_label_create(list_btn);
    
    lv_obj_set_style_text_font(obj, &WIFI_LIST_FONT, 0);
    lv_label_set_text(obj, str1);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_obj_add_event_cb(obj,list_sub_wifi_btn_1_event_handle, LV_EVENT_ALL, 0);
    lv_obj_set_flex_grow(obj, WIFI_SUB_LIST_FLEX_GROUP1);

    obj = lv_obj_create(list_btn);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_flex_grow(obj, WIFI_SUB_LIST_FLEX_GROUP2);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(obj, 0, 0);
    if(str2<0){
        lv_obj_t *img = lv_img_create(obj);
        lv_obj_center(img);
        lv_img_set_src(img, &pwd_icon);        
    }else{
        lv_obj_t *label = lv_label_create(obj);
        lv_obj_center(label);
        lv_obj_set_style_bg_color(label, lv_color_make(23,54,112), 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        //lv_label_set_text(label, str2);
        //language_choose_add_label1(label, str2);
        set_label_text2(label, str2, FONT_NORMAL);
        //lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);   
        lv_obj_set_style_text_color(label, lv_color_white(), 0);     
    }


    obj = lv_img_create(list_btn);
    lv_img_set_src(obj, &prompt_icon);
    lv_obj_set_flex_grow(obj, WIFI_SUB_LIST_FLEX_GROUP3);
    //lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(obj, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(obj, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_FOCUSED);
    lv_group_add_obj(lv_group_get_default(), obj);

    lv_obj_add_event_cb(obj, list_sub_wifi_btn_3_event_handle, LV_EVENT_ALL, 0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_pad_right(obj, 0, 0);

    return list_btn;
}

static lv_obj_t* create_list_sub_wifi_btn_obj(lv_obj_t *parent, void *icon, char* str1){
    return create_list_sub_wifi_btn_obj0(parent, icon, str1, -1);
}


static void list_sub_wifi_btn_1_event_handle(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    int i = (int) lv_event_get_param(e);

    if (code == LV_EVENT_REFRESH) {
        if (i) {
            lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
        } else {
            lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
        }
    }
}

static void list_sub_wifi_btn_3_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC || key == LV_KEY_LEFT){
            lv_group_focus_obj(obj->parent->parent);
        } else if(key == LV_KEY_ENTER || key == LV_KEY_RIGHT){

        }
    }else if(code == LV_EVENT_PRESSED){
        if(cur_wifi_id==0){
            // hccast_wifi_status_result_t res = {0};
            // int ret = wifi_ctrl_get_status(udhcp_result.ifname, HCCAST_WIFI_MODE_STA, &res);
            // if (ret < 0) {
            //     return;
            // }
            char mac[6] = {0};
            if(!api_get_mac_addr(mac)){
                char buffer[19] = {0};
                sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)mac[0], (unsigned char)mac[1],(unsigned char)mac[2],(unsigned char)mac[3], 
                (unsigned char)mac[4],(unsigned char)mac[5]);
                create_conn_ip_msg_widget(lv_label_get_text(lv_obj_get_child(obj->parent, 1)), "DHCP", udhcp_result.ip, udhcp_result.mask,udhcp_result.gw,
                                    udhcp_result.dns, buffer);    

            }else{
                create_conn_ip_msg_widget(lv_label_get_text(lv_obj_get_child(obj->parent, 1)), "DHCP", udhcp_result.ip, udhcp_result.mask,udhcp_result.gw,
                                    udhcp_result.dns, "ff:ff:ff:ff:ff:ff");                   
            }
            
        }
    }
}

void new_pwd_btns_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t*)obj;

    if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);

        dsc->rect_dsc->border_side = LV_BORDER_SIDE_TOP;
        dsc->rect_dsc->border_width = 1;
        dsc->rect_dsc->outline_width = 0;
    }else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
         if(key == LV_KEY_RIGHT || key == LV_KEY_LEFT){
            lv_btnmatrix_set_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
        } else if(key == LV_KEY_ENTER){
             if(btnms->btn_id_sel == 0 || btnms->btn_id_sel == 1){    
                if(btnms->btn_id_sel == 1){
                    if(!kb_enter_btn_run_cb0()){
                        hccast_wifi_ap_info_t *info_p = NULL;
                        info_p = sysdata_get_wifi_info(get_list_sub_wifi_ssid(cur_wifi_conning_id));
                        if(info_p){
                            strcpy(info_p->pwd, lv_textarea_get_text(lv_obj_get_child(lv_obj_get_child(wifi_conn_widget, 0), 1)));
                        }
                        app_wifi_reconnect(info_p);
                    }else{
                        return;
                    }
                }else if(btnms->btn_id_sel == 0){
                    //language_choose_add_label1(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), );
                    set_label_text2(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), STR_BT_SAVED, FONT_NORMAL);
                    cur_wifi_conning_id = -1;
                }
                slave_scr_clear();
                // lv_group_focus_obj(wifi_lists);
                // lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
                // lv_obj_del(obj->parent);
                // wifi_conn_widget = NULL;
                // lv_obj_del(kb);
                // kb=NULL;
             }
        }else if(key == LV_KEY_UP){
            lv_btnmatrix_clear_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
            lv_group_focus_prev(lv_group_get_default());
        }else if(key == LV_KEY_DOWN){
            lv_btnmatrix_clear_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
            lv_group_focus_obj(kb);
        }
    }else if(code == LV_EVENT_FOCUSED){
        btnms->btn_id_sel = 1;
        lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
    }
}

void btns_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t*)obj;
   if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);

        dsc->rect_dsc->border_side = LV_BORDER_SIDE_TOP;
        dsc->rect_dsc->border_width = 1;
        dsc->rect_dsc->outline_width = 0;

    } else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_RIGHT || key == LV_KEY_LEFT){
            lv_btnmatrix_set_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
        } else if(key == LV_KEY_ENTER){
            if(btnms->btn_id_sel == 0 || btnms->btn_id_sel == 1){    
                if(btnms->btn_id_sel == 1){
                    if(strlen(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)))==0){
                        if(!lv_obj_is_valid(msg_box)){
                            msg_box=create_message_box((char *)api_rsc_string_get(STR_WIFI_PWD_NULL));
                        }
                        return;
                    } else if(strlen(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)))<8 || strlen(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)))>32){
                        if(!lv_obj_is_valid(msg_box)){
                            msg_box=create_message_box((char *)api_rsc_string_get(STR_ENTER_WORD));
                            //herer
                        }
                        return;
                    }
                        if (strlen(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)))<8)
                        {
                            return;
                        }
                        
                        wifi_connect(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 0)), lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)));
                }    
                slave_scr_clear();
                // lv_group_focus_obj(wifi_lists);
                // lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
                // lv_obj_del(obj->parent);
                // wifi_conn_widget = NULL;
                // lv_obj_del(kb);
                // kb=NULL;
                

            }
        } else if(key == LV_KEY_UP){
            lv_btnmatrix_clear_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 7));
        }else if(key == LV_KEY_DOWN){
            lv_btnmatrix_clear_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
            lv_group_focus_obj(kb);
        }
    } else if(code == LV_EVENT_FOCUSED){
        btnms->btn_id_sel = 1;
        lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
    }
}

static void checkout_event_handle(lv_event_t*e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *ta = (lv_obj_t*)lv_event_get_user_data(e);
    static bool is_hidden = false;
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_UP){
            //lv_group_focus_obj(lv_obj_get_child(obj->parent, index-1));
            lv_group_focus_prev(lv_group_get_default());
            if(is_hidden){
                lv_obj_clear_state(obj, LV_STATE_CHECKED);
            }

        } else if(key == LV_KEY_DOWN){
            //lv_group_focus_obj(lv_obj_get_child(obj->parent, index+1));
            lv_group_focus_next(lv_group_get_default());
            if(!is_hidden){
                lv_obj_add_state(obj, LV_STATE_CHECKED);
            }
        } else if(key == LV_KEY_ENTER){
            if(lv_obj_has_state(obj, LV_STATE_CHECKED)){
                lv_textarea_set_password_mode(ta, true);
                is_hidden = true;
            } else{
                lv_textarea_set_password_mode(ta, false);
                is_hidden = false;
            }
        }
    } else if(code == LV_EVENT_FOCUSED){
        if(lv_obj_has_state(obj, LV_STATE_CHECKED)){
            is_hidden = false;
        } else{
            is_hidden = true;
        }
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        
        if(dsc && lv_group_get_focused(default_g) == obj){
            dsc->rect_dsc->bg_opa = LV_OPA_10;
              
        }
    }
}



static void wifi_conne_kb_event_handle_(lv_event_t* e, run_cb cb){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *obj1 = (lv_obj_t*)lv_event_get_user_data(e);
    static uint sel_id = 0;
    if(code == LV_EVENT_KEY) {
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            kb_g_onoff(false);
            lv_obj_del(kb);
            kb=NULL;
            wifi_widget_x = lv_obj_get_x(wifi_conn_widget);
            wifi_widget_y = lv_obj_get_y(wifi_conn_widget);
            lv_obj_center(wifi_conn_widget);
            // lv_obj_del();
            // wifi_conn_widget = NULL;
            //lv_group_focus_obj(wifi_lists);
           //slave_scr_clear();
        }else if(key == LV_KEY_ENTER){
             const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
             if (strcmp(txt, LV_SYMBOL_OK) == 0){      
                if(cb()){
					
                    return;
                }
                if(wifi_list_get_cur())
                    wifi_connect(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 0)), lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)));
                // lv_group_focus_obj(wifi_lists);
                // lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
                // lv_obj_del(kb);
                // kb=NULL;
                // lv_obj_del(wifi_conn_widget);
                // wifi_conn_widget = NULL;
				kb_g_onoff(false);
                slave_scr_clear();
             
             }
        }else if (key == LV_KEY_UP) {
            lv_btnmatrix_t *btnm = &((lv_keyboard_t*)obj)->btnm;
            printf("btn y1: %d\n", btnm->button_areas[btnm->btn_id_sel].y1);
            if(btnm->button_areas[sel_id].y1 == btnm->button_areas[0].y1){
                kb_g_onoff(false);
                lv_obj_del(kb);
                kb=NULL;
                wifi_widget_x = lv_obj_get_x(wifi_conn_widget);
                wifi_widget_y = lv_obj_get_y(wifi_conn_widget);
                lv_obj_center(wifi_conn_widget);
                lv_group_focus_obj(obj1);
            }
         }
         sel_id = ((lv_keyboard_t*)obj)->btnm.btn_id_sel;
    }else if(code == LV_EVENT_FOCUSED){
        ((lv_textarea_t*)((lv_keyboard_t*)obj)->ta)->cursor.show=1;
    }
}

static bool kb_enter_btn_run_cb0(){
    if(strlen(lv_textarea_get_text(lv_obj_get_child(lv_obj_get_child(wifi_conn_widget, 0), 1)))==0){
        if(!lv_obj_is_valid(msg_box)){
            msg_box=create_message_box((char *)api_rsc_string_get(STR_WIFI_PWD_NULL));
        }
        return true;
    } else if(strlen(lv_textarea_get_text(lv_obj_get_child(lv_obj_get_child(wifi_conn_widget, 0), 1)))<8 || strlen(lv_textarea_get_text(lv_obj_get_child(lv_obj_get_child(wifi_conn_widget, 0), 1)))>16){
        if(!lv_obj_is_valid(msg_box)){
            msg_box=create_message_box((char *)api_rsc_string_get(STR_ENTER_WORD));
        }
        return true;
    }
    return false;
}

static void wifi_new_pwd_conne_kb_event_handle(lv_event_t *e){
    wifi_conne_kb_event_handle_(e, kb_enter_btn_run_cb0);
}

static bool kb_enter_btn_run_cb1(){
    if(strlen(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)))==0){
        if(!lv_obj_is_valid(msg_box)){
            msg_box=create_message_box((char *)api_rsc_string_get(STR_WIFI_CONNING));;
        }
        return true;
    } else if(strlen(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)))<8 || strlen(lv_textarea_get_text(lv_obj_get_child(wifi_conn_widget, 6)))>16){
        if(!lv_obj_is_valid(msg_box)){
            msg_box=create_message_box((char *)api_rsc_string_get(STR_ENTER_WORD));
        }
        return true;
    }
    return false;
}
static void wifi_conne_kb_event_handle(lv_event_t* e){
    wifi_conne_kb_event_handle_(e, kb_enter_btn_run_cb1);
}

static void focus_kb_timer_handle(lv_timer_t *t){//延时聚焦kb
    lv_obj_t *obj = (lv_obj_t*)t->user_data;
    lv_group_focus_obj(obj);
}

static void pwd_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_UP) {
           // lv_group_focus_obj(lv_obj_get_child(obj->parent, 0));
        }else if(key == LV_KEY_DOWN){
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 7));
        } else if(key == LV_KEY_ENTER){
            if(kb != NULL){
                lv_group_focus_obj(kb);
                return;
            }
            lv_obj_align(wifi_conn_widget,LV_ALIGN_TOP_LEFT, wifi_widget_x, wifi_widget_y);
            kb = create_keypad_widget(lv_obj_get_child(obj->parent, 8), wifi_conne_kb_event_handle,0);
            lv_keyboard_set_textarea(kb, obj);
        }

    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        
        if(dsc && lv_group_get_focused(default_g) == obj){
            dsc->rect_dsc->bg_opa = LV_OPA_10;
            dsc->rect_dsc->outline_width = 0;
        }
        
    }
}

static void new_pwd_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_textarea_t * ta = (lv_textarea_t *)obj;
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_UP) {

        }else if(key == LV_KEY_DOWN){
            lv_group_focus_obj(lv_obj_get_child(obj->parent->parent, 1));
        } else if(key == LV_KEY_ENTER){
            if(kb != NULL){
                lv_group_focus_obj(kb);
                return;
            }
            lv_obj_align(wifi_conn_widget, LV_ALIGN_TOP_LEFT, wifi_widget_x, wifi_widget_y);
            kb = create_keypad_widget(lv_obj_get_child(obj->parent->parent, 2), wifi_conne_kb_event_handle,0);
            lv_keyboard_set_textarea(kb, obj);
        }

    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        
        if(dsc && lv_group_get_focused(default_g) == obj){
            dsc->rect_dsc->bg_opa = LV_OPA_10;
            dsc->rect_dsc->outline_width = 0;
        }
    }
}

static void ssid_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY) {
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 6));
        }
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        
        if(dsc && lv_group_get_focused(default_g) == obj){
            dsc->rect_dsc->bg_opa = LV_OPA_10;
              
        }
    }
}

static void ip_msg_btn_event_handle1(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    bool is_delete = (bool)lv_event_get_user_data(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(cur_conne_p){
                if(wifi_is_conning){
                    if(!lv_obj_is_valid(msg_box)){
                        msg_box = create_message_box(api_rsc_string_get(STR_WIFI_CONNING));                       
                    }
                    return;
                }
               hccast_wifi_mgr_disconnect();
			   int index=sysdata_get_wifi_index_by_ssid(get_list_sub_wifi_ssid(0));
                if(is_delete){
                    if(index>=0){
                        sysdata_wifi_ap_delete(index);
                        projector_sys_param_save();
                        lv_obj_del(lv_obj_get_child(wifi_lists, 0));
                        cur_wifi_id = -1;
                        cur_conne_p = NULL;
                    }
                    scan_type = WIFI_SCAN_SEARCH;
                    projector_wifi_scan();
                    //lv_event_send(wifi_show, LV_EVENT_REFRESH, "");
					//main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_OFF);
                }else{
					if(index>=0){
						sysdata_wifi_ap_set_nonauto(index);
						projector_sys_param_save();
					}
				}
                
                //lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
                // if(show_ip_widget){
                //     lv_obj_del(show_ip_widget);
                //     show_ip_widget = NULL;
                // }
                //lv_group_focus_obj(wifi_lists);  
                slave_scr_clear();
            }            
        }else if (key == LV_KEY_ESC){
            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
            if(show_ip_widget){
                lv_obj_del(show_ip_widget);
                show_ip_widget = NULL;
            }
            lv_group_focus_obj(wifi_lists); 
        }else if(key == LV_KEY_LEFT || key == LV_KEY_RIGHT){
            if(lv_obj_get_index(obj) == 0){
                lv_group_focus_obj(lv_obj_get_child(obj->parent, 1));
            }else if(lv_obj_get_index(obj) == 1){
                lv_group_focus_obj(lv_obj_get_child(obj->parent, 0));
            }
        }
        

    }
}















//添加隐藏wifi 事件处理
static void wifi_add_hidden_net_sure(){
    char *text = lv_textarea_get_text(lv_obj_get_child(wifi_add_hidden_net_widget, 1));
    if(strlen(text) == 0){
        create_message_box(api_rsc_string_get(STR_WIFI_PWD_NULL));
        return;
    }
    strcpy(hidden_ap_info.ssid,text);

    text = lv_textarea_get_text(lv_obj_get_child(wifi_add_hidden_net_widget, 2));


    if(strlen(text)>WIFI_MAX_SSID_LEN || (strlen(text)<8 && strlen(text)>0)){
        create_message_box(api_rsc_string_get(STR_ENTER_WORD));
        return;
    }
    strcpy(hidden_ap_info.pwd,text);

    hidden_ap_info.encryptMode = (int)lv_dropdown_get_selected(lv_obj_get_child(lv_obj_get_child(wifi_add_hidden_net_widget, 3), 0));
    printf("encryptMode%d\n", hidden_ap_info.encryptMode);
    hidden_ap_info.keyIdx = 1;
    hidden_ap_info.special_ap = 1;
    hidden_ap_info.quality = 75;
    conn_type = WIFI_CONN_HIDDEN;
    app_wifi_reconnect(&hidden_ap_info);
    prompt_box = loader_with_arc(api_rsc_string_get(STR_WIFI_ADD), NULL);
    set_remote_control_disable(true);
}

static void wifi_add_hidden_net_kb_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *obj1 = (lv_obj_t*)lv_event_get_user_data(e);

    static uint sel_id = 0;
    if(code == LV_EVENT_KEY) {
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            kb_g_onoff(false);
            lv_obj_del(kb);
            kb=NULL;
            wifi_widget_x = lv_obj_get_x(wifi_add_hidden_net_widget);
            wifi_widget_y = lv_obj_get_y(wifi_add_hidden_net_widget);
            lv_obj_center(wifi_add_hidden_net_widget);
            // wifi_add_hidden_net_widget = NULL;
        }else if(key == LV_KEY_ENTER){
            const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
            if(strlen(lv_textarea_get_text(((lv_keyboard_t*)obj)->ta))==((lv_textarea_t*)(((lv_keyboard_t*)obj)->ta))->max_length){
                create_message_box(api_rsc_string_get(STR_ENTER_WORD));
            }             
            if (strcmp(txt, LV_SYMBOL_OK) == 0){
                char *ssid = lv_textarea_get_text(lv_obj_get_child(wifi_add_hidden_net_widget, 1));
                if(sysdata_get_wifi_info(ssid)){
                    //加一个提示
                    return;
                }
                wifi_add_hidden_net_sure();
                kb_g_onoff(false);
                slave_scr_clear();
            }

        }else if (key == LV_KEY_UP) {
            lv_btnmatrix_t *btnm = &((lv_keyboard_t*)obj)->btnm;
            if(btnm->button_areas[sel_id].y1 == btnm->button_areas[0].y1){
                kb_g_onoff(false);
                lv_obj_del(kb);
                kb=NULL;
                wifi_widget_x = lv_obj_get_x(wifi_add_hidden_net_widget);
                wifi_widget_y = lv_obj_get_y(wifi_add_hidden_net_widget);
                lv_obj_center(wifi_add_hidden_net_widget);
                lv_group_focus_obj(obj1);//上方的取消和连接按钮矩阵
            }
        }
        sel_id = ((lv_keyboard_t*)obj)->btnm.btn_id_sel;
    }
}

static void wifi_add_hidden_net_name_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_DOWN){
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 2));
        }else if(key == LV_KEY_ENTER){
            if (kb){
                lv_group_focus_obj(kb);
            }
            lv_obj_align(wifi_add_hidden_net_widget, LV_ALIGN_TOP_LEFT, wifi_widget_x, wifi_widget_y);
            kb = create_keypad_widget(lv_obj_get_child(wifi_add_hidden_net_widget, 4), wifi_add_hidden_net_kb_event_handle,0);
            lv_keyboard_set_textarea(kb, obj);
        }
    }else if (code == LV_EVENT_FOCUSED){
        //wifi_add_hidden_net_widget->user_data = obj;
        
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        //if(lv_group_get_focused(lv_group_get_default()) == obj){
            lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);
            if(dsc && lv_group_get_focused(default_g) == obj){
                dsc->rect_dsc->bg_opa = LV_OPA_10;
                dsc->rect_dsc->outline_width = 0;
            }
            
    }
}

static void wifi_add_hidden_net_pwd_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_DOWN){
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 3));
        }else if(key == LV_KEY_UP){
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 1));
        }else if(key == LV_KEY_ENTER){
            if (kb){
                lv_group_focus_obj(kb);
            }
            lv_obj_align(wifi_add_hidden_net_widget, LV_ALIGN_TOP_LEFT, wifi_widget_x, wifi_widget_y);
            kb = create_keypad_widget(lv_obj_get_child(wifi_add_hidden_net_widget, 4), wifi_add_hidden_net_kb_event_handle,0);
            lv_keyboard_set_textarea(kb, obj);
        }
    }else if (code == LV_EVENT_FOCUSED){
         //wifi_add_hidden_net_widget->user_data = obj;
         //lv_keyboard_set_textarea(kb, obj);
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
       // if(lv_group_get_focused(lv_group_get_default()) == obj){
            lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);
            //dsc->rect_dsc->outline_color = lv_color_make(255,255,0);  
            if(dsc && lv_group_get_focused(default_g) == obj){
                dsc->rect_dsc->bg_opa = LV_OPA_10;
                dsc->rect_dsc->outline_width = 0;
            }         
            

    }else if(code == LV_EVENT_VALUE_CHANGED){
       
    }
}

static void wifi_add_hidden_net_security_event_handle(lv_event_t *e){//安全协议
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_t *drop_obj = lv_obj_get_child(obj, 0);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        static char parem;
        if(key == LV_KEY_DOWN){
            if(lv_dropdown_is_open(drop_obj)){
                parem = LV_KEY_DOWN;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
            }else{
                lv_group_focus_obj(lv_obj_get_child(obj->parent, 4)); 
            }
        }else if(key == LV_KEY_UP){
            if(lv_dropdown_is_open(drop_obj)){
                parem = LV_KEY_UP;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
            }else{
                lv_group_focus_obj(lv_obj_get_child(obj->parent, 2)); 
            }
        }else if(key == LV_KEY_ENTER){
            if(lv_dropdown_is_open(drop_obj)){
                parem = LV_KEY_ENTER;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
            }else{
                lv_dropdown_open(drop_obj);
            }
        }else if(key == LV_KEY_ESC){
            if(lv_dropdown_is_open(drop_obj)){
                parem = LV_KEY_ESC;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
            }
        }
    }else if (code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        if(dsc && lv_group_get_focused(wifi_g) == obj){
            dsc->rect_dsc->bg_opa = LV_OPA_10;
        }

    }
}

static void wifi_add_hidden_net_btnm_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnm = (lv_btnmatrix_t*)obj;

    if(code == LV_EVENT_PRESSED){
        if(btnm->btn_id_sel == 0){
            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
        }else if(btnm->btn_id_sel == 1){
            //lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
            char *ssid = lv_textarea_get_text(lv_obj_get_child(wifi_add_hidden_net_widget, 1));
            if(sysdata_get_wifi_info(ssid)){
                //加一个提示
                return;
            }
            wifi_add_hidden_net_sure();
        }
            slave_scr_clear();
    }else if(code == LV_EVENT_FOCUSED){
        btnm->btn_id_sel = 0;
        lv_btnmatrix_set_btn_ctrl(obj, 0, LV_BTNMATRIX_CTRL_CHECKED);
        //wifi_add_hidden_net_widget->user_data = obj;
    }else if(code == LV_EVENT_DEFOCUSED){
        //lv_btnmatrix_clear_btn_ctrl(obj, btnm->btn_id_sel, LV_BTNMATRIX_CTRL_CHECKED);//有bug
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);

        dsc->rect_dsc->border_side = LV_BORDER_SIDE_TOP;
        dsc->rect_dsc->border_width = 0;
        dsc->rect_dsc->outline_width = 0;
    }else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_LEFT || key == LV_KEY_RIGHT){
            lv_btnmatrix_set_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
        }else if(key == LV_KEY_UP){
            lv_btnmatrix_clear_btn_ctrl(obj, btnm->btn_id_sel, LV_BTNMATRIX_CTRL_CHECKED);
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 3));
        }else if(key == LV_KEY_DOWN){
            if(kb){
                lv_btnmatrix_clear_btn_ctrl(obj, btnm->btn_id_sel, LV_BTNMATRIX_CTRL_CHECKED);
                lv_group_focus_obj(kb);
            }
        }
    }
}
//添加隐藏wifi 事件处理结束












//ui界面
static void create_conn_ip_msg_widget(char* ssid,char* net_mode, char* ip_addr, char *net_mask, char *gateway, char *dns, char *mac){//wifi 信息ui
    lv_obj_t *obj = create_ip_msg_widget(ssid,net_mode,ip_addr,net_mask,gateway,dns,mac);
    
    lv_obj_t *btn = lv_btn_create(lv_obj_get_child(obj, 2));
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, api_rsc_string_get(STR_BT_MAKE_DISCONN));
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID),0);
    lv_obj_add_event_cb(btn, ip_msg_btn_event_handle1, LV_EVENT_ALL, (void*)false);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_STATE_FOCUSED);
    lv_group_focus_obj(btn);

    btn = lv_btn_create(lv_obj_get_child(obj, 2));
    label = lv_label_create(btn);
    lv_label_set_text(label, api_rsc_string_get(STR_BT_DELETE));
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID),0);
    lv_obj_add_event_cb(btn, ip_msg_btn_event_handle1, LV_EVENT_ALL, (void*)true);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_STATE_FOCUSED);
}

static lv_obj_t* create_ip_msg_widget(char* ssid,char* net_mode, char* ip_addr, char *net_mask, char *gateway, char *dns, char *mac){//wifi 信息ui
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
    show_ip_widget = lv_obj_create(lv_layer_top());
    lv_obj_center(show_ip_widget);
    lv_obj_set_size(show_ip_widget,LV_PCT(WIFI_IP_MSG_WIDGET_WIDTH_PCT),LV_PCT(WIFI_IP_MSG_WIDGET_HEIGHT_PCT));
    lv_obj_set_style_pad_gap(show_ip_widget, 0, 0);
    lv_obj_set_style_pad_hor(show_ip_widget, 0, 0);
    lv_obj_set_style_pad_ver(show_ip_widget, 0, 0);
    lv_obj_set_scrollbar_mode(show_ip_widget, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(show_ip_widget, lv_color_make(81, 100, 117), 0);
    lv_obj_set_style_text_color(show_ip_widget, lv_color_white(), 0);
    //lv_obj_set_style_text_font(show_ip_widget, &lv_font_montserrat_26, 0);

    lv_obj_t *obj = lv_label_create(show_ip_widget);
    lv_obj_set_size(obj,LV_PCT(100),LV_PCT(14));
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(obj,10, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(obj, ssid);
    lv_obj_set_style_text_font(obj, &WIFI_LIST_FONT, 0);


    lv_obj_t *list = create_list_obj1(show_ip_widget, 100, 70);
    if(WIFI_IP_MSG_PAD_HOR>=0){
        lv_obj_set_style_pad_hor(list, WIFI_IP_MSG_PAD_HOR, 0);
    }
    lv_obj_add_event_cb(list, ip_msg_event_handle, LV_EVENT_ALL, 0);

    lv_obj_align_to(list, obj, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_NET_MODE, false, net_mode, false,false);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_IP_ADDR, false, ip_addr, false,true);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_NET_MASK, false, net_mask, false,true);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_GATE_WAY, false, gateway, false,true);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_DNS, false, dns, false,true);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_MAC, false, mac, false,true);

    list->user_data = (void *)true;

    lv_obj_t *foot = lv_obj_create(show_ip_widget);

    lv_obj_set_size(foot, LV_PCT(100),LV_PCT(16));
    lv_obj_set_scrollbar_mode(foot, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(foot, 0, 0);
    lv_obj_set_style_border_width(foot, 0, 0);
    lv_obj_set_style_pad_gap(foot, 0, 0);
    lv_obj_set_style_bg_opa(foot, LV_OPA_0, 0);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    //lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_align(foot, LV_ALIGN_BOTTOM_MID, 0, 0);
    //lv_group_focus_obj(list);
    return show_ip_widget;
}

static void create_wifi_conne_widget_(char* name, char* strength, char *security, char* pwd){//wifi连接ui
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);

    wifi_conn_widget = lv_obj_create(lv_layer_top());
    lv_obj_set_style_text_font(wifi_conn_widget, osd_font_get(FONT_MID), 0);
    lv_obj_set_style_text_color(wifi_conn_widget, lv_color_white(), 0);
    lv_obj_align(wifi_conn_widget, LV_ALIGN_TOP_MID, 0,LV_PCT(WIFI_CONNE_WIDGET_ALIGN_TOP));
    lv_obj_set_size(wifi_conn_widget,LV_PCT(WIFI_CONNE_WIDGET_WIDTH_PCT),LV_PCT(WIFI_CONNE_WIDGET_HEIGHT_PCT));
    lv_obj_set_style_pad_gap(wifi_conn_widget, 0, 0);
    lv_obj_set_style_pad_ver(wifi_conn_widget, 3, 0);
    lv_obj_set_style_pad_hor(wifi_conn_widget, 0, 0);
    lv_obj_set_style_bg_color(wifi_conn_widget, lv_color_make(81, 100, 117), 0);
    lv_obj_set_style_border_width(wifi_conn_widget,2,0);
    lv_obj_set_scrollbar_mode(wifi_conn_widget, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(wifi_conn_widget, 5, 0);
    lv_obj_t *obj, *obj_prev;
    obj = lv_textarea_create(wifi_conn_widget);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_size(obj,LV_PCT(100),LV_PCT(15));
    //lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_textarea_add_text(obj, name);
    lv_textarea_set_one_line(obj, true);
    lv_obj_set_style_text_font(obj, &WIFI_LIST_FONT, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(obj, ssid_event_handle, LV_EVENT_ALL, 0);

    obj_prev=obj;
    obj = lv_label_create(wifi_conn_widget);
    set_label_text2(obj,STR_WIFI_STRENGTH,FONT_MID);
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, 9, WIFI_CONNE_WIDGET_ALIGN_TOP0);
    if(WIFI_CONN_WIDGET_SET_H)
        lv_obj_set_height(obj, lv_pct(8));
    lv_obj_set_style_pad_ver(obj, 0, 0);

    obj_prev=obj;
    obj = lv_label_create(wifi_conn_widget);
    lv_label_set_text(obj, strength);
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, WIFI_CONNE_WIDGET_ALIGN_TOP1);
    if(WIFI_CONN_WIDGET_SET_H)
        lv_obj_set_height(obj, lv_pct(10));
    lv_obj_set_style_pad_ver(obj, 0, 0);

    obj_prev=obj;
    obj = lv_label_create(wifi_conn_widget);
    set_label_text2(obj,STR_WIFI_SECURITY,FONT_MID);
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, WIFI_CONNE_WIDGET_ALIGN_TOP2);
    lv_obj_set_style_pad_ver(obj, 0, 0);
    if(WIFI_CONN_WIDGET_SET_H)
        lv_obj_set_height(obj, lv_pct(8));

    obj_prev=obj;
    obj = lv_label_create(wifi_conn_widget);
    lv_label_set_text(obj, security);
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, WIFI_CONNE_WIDGET_ALIGN_TOP3);
    if(WIFI_CONN_WIDGET_SET_H)
        lv_obj_set_height(obj, lv_pct(10));
    lv_obj_set_style_pad_ver(obj, 0, 0);

    obj_prev=obj;
    obj = lv_label_create(wifi_conn_widget);
    set_label_text2(obj,STR_WIFI_PWD,FONT_MID);
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, WIFI_CONNE_WIDGET_ALIGN_TOP4);
    if(WIFI_CONN_WIDGET_SET_H)
        lv_obj_set_height(obj, lv_pct(8));
    lv_obj_set_style_pad_ver(obj, 0, 0);

    obj_prev=obj;
    obj = lv_textarea_create(wifi_conn_widget);
    lv_textarea_set_max_length(obj, 16);
    lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_BLUE), LV_PART_CURSOR);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(obj, lv_color_make(0,0,255), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_size(obj,LV_PCT(95),LV_PCT(15));
    //lv_obj_set_width(obj, lv_pct(96));
    lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_textarea_set_one_line(obj, true);
    lv_textarea_set_placeholder_text(obj, "please enter password");
    if(pwd){
        lv_textarea_add_text(obj, pwd);
        lv_textarea_set_password_mode(obj, true);
    }
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, WIFI_CONNE_WIDGET_ALIGN_TOP5);
    lv_obj_add_event_cb(obj, pwd_event_handle, LV_EVENT_ALL, 0);
    lv_group_focus_obj(obj);

    obj_prev=obj;
    obj = lv_checkbox_create(wifi_conn_widget);
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID), 0);
    lv_checkbox_set_text(obj, (char *)api_rsc_string_get(STR_WIFI_SHOW_PWD));
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, WIFI_CONNE_WIDGET_ALIGN_TOP6);
    if(!pwd){
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(obj, checkout_event_handle, LV_EVENT_ALL, obj_prev);
    if(WIFI_CONN_WIDGET_SET_H)
        lv_obj_set_height(obj, lv_pct(11));
    lv_obj_set_style_pad_ver(obj, 0, 0);

    obj_prev=obj;
    static const char* strs[3];
    strs[0] = (char *)api_rsc_string_get(STR_WIFI_CANCEL);
    strs[1] = (char *)api_rsc_string_get(STR_BT_MAKE_CONN);
    strs[2] = "";
    obj = lv_btnmatrix_create(wifi_conn_widget);
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID),0);
    lv_obj_align_to(obj, obj_prev, LV_ALIGN_OUT_BOTTOM_LEFT, -9, WIFI_CONNE_WIDGET_ALIGN_TOP7);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(obj, lv_color_black(), LV_STATE_CHECKED | LV_PART_ITEMS);
    lv_obj_set_size(obj,LV_PCT(103),LV_PCT(18));
    lv_btnmatrix_set_map(obj, strs);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_gap(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(obj, 0, LV_PART_ITEMS);

    lv_btnmatrix_set_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(obj, true);
    lv_obj_add_event_cb(obj, btns_event_handle, LV_EVENT_ALL, wifi_conn_widget);

    kb = create_keypad_widget(obj, wifi_conne_kb_event_handle, 100-WIFI_CONNE_WIDGET_HEIGHT_PCT - WIFI_CONNE_WIDGET_ALIGN_TOP-1);
    lv_keyboard_set_textarea(kb, lv_obj_get_child(wifi_conn_widget, 6));
}



static void create_wifi_new_pwd_conne_widget(){//修改密码ui
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
    wifi_conn_widget = lv_obj_create(lv_layer_top());
    lv_obj_set_style_text_color(wifi_conn_widget, lv_color_white(), 0);
    lv_obj_set_style_text_font(wifi_conn_widget, osd_font_get(FONT_MID), 0);
    lv_obj_align(wifi_conn_widget, LV_ALIGN_TOP_MID, 0,LV_PCT(50-WIFI_NEW_PWD_WIDGET_H));
    lv_obj_set_size(wifi_conn_widget,LV_PCT(WIFI_NEW_PWD_WIDGET_W),LV_PCT(WIFI_NEW_PWD_WIDGET_H));
    lv_obj_set_style_pad_hor(wifi_conn_widget, 0, 0);
    lv_obj_set_style_pad_bottom(wifi_conn_widget, 0, 0);
    lv_obj_set_style_bg_color(wifi_conn_widget, lv_color_make(81, 100, 117), 0);
    lv_obj_set_style_border_width(wifi_conn_widget,2,0);
    lv_obj_set_scrollbar_mode(wifi_conn_widget, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(wifi_conn_widget, 5, 0);
    lv_obj_set_flex_flow(wifi_conn_widget, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_conn_widget, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);


    lv_obj_t* panel_obj = lv_obj_create(wifi_conn_widget);
    lv_obj_set_flex_grow(panel_obj, 9);
    lv_obj_set_style_text_color(panel_obj, lv_color_white(), 0);
    lv_obj_set_style_border_width(panel_obj,0,0);
    lv_obj_set_scrollbar_mode(panel_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(panel_obj, 0, 0); 
    lv_obj_set_width(panel_obj, lv_pct(100));
    lv_obj_set_flex_flow(panel_obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(panel_obj, LV_OPA_0, 0);
    lv_obj_set_style_pad_gap(panel_obj, 0, 0);
    lv_obj_set_flex_align(panel_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *obj = lv_label_create(panel_obj);
    lv_label_set_text_fmt(obj,"%s:",(char *)api_rsc_string_get(STR_WIFI_PWD));
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID),0);
    lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_flex_grow(obj, 4);

    lv_obj_t* ta = lv_textarea_create(panel_obj);
    lv_obj_set_flex_grow(ta, 9);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 16);
   // lv_textarea_set_password_mode(ta, false);
    lv_textarea_set_placeholder_text(ta, "New password");
    lv_textarea_set_password_bullet(ta, ".");
    lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_radius(ta, 0, 0);
    lv_obj_set_style_text_color(ta, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_10, 0);
    lv_obj_set_style_radius(ta, 5, 0);
    lv_group_focus_obj(ta);
   
    lv_obj_add_event_cb(ta, new_pwd_event_handle, LV_EVENT_ALL, 0);
 

    lv_obj_t *co = lv_checkbox_create(wifi_conn_widget);
    lv_obj_set_flex_grow(co, 5);
    lv_obj_set_style_text_font(co, osd_font_get(FONT_MID), 0);
    lv_checkbox_set_text(co, api_rsc_string_get(STR_WIFI_SHOW_PWD));
    lv_obj_add_state(co, LV_STATE_CHECKED);
    lv_obj_set_style_pad_left(co, 10, 0);
    lv_obj_add_event_cb(co, checkout_event_handle, LV_EVENT_ALL, ta);

    static const char* strs[3];
    strs[0] = api_rsc_string_get(STR_WIFI_CANCEL);
    strs[1] = api_rsc_string_get(STR_BT_MAKE_CONN);
    strs[2] = "";
    obj = lv_btnmatrix_create(wifi_conn_widget);
    lv_obj_set_flex_grow(obj, 10);
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID),0);
    //lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(obj, lv_color_black(), LV_STATE_CHECKED | LV_PART_ITEMS);
    lv_obj_set_width(obj,LV_PCT(103));
    lv_btnmatrix_set_map(obj, strs);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_bottom(obj, -6, 0);
    lv_obj_set_style_pad_gap(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(obj, 0, LV_PART_ITEMS);

    lv_btnmatrix_set_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(obj, true);
    lv_obj_add_event_cb(obj, new_pwd_btns_event_handle, LV_EVENT_ALL, wifi_conn_widget);
    
    
    kb = create_keypad_widget(obj,  wifi_new_pwd_conne_kb_event_handle,0);
    lv_keyboard_set_textarea(kb, ta);
}

static lv_obj_t* create_keypad_widget(lv_obj_t* obj, lv_event_cb_t event_cb,int height) {//键盘ui
    kb_g_onoff(true);
    kb = lv_keyboard_create(lv_layer_top());
    lv_obj_set_style_text_font(kb, osd_font_get_by_langid(0, FONT_MID), 0);
   
    lv_obj_set_style_bg_color(kb, lv_color_make(81, 100, 117), 0);
    lv_obj_set_size(kb,LV_PCT(100),LV_PCT((height <= 0 ? 38 : height)));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(kb, event_cb, LV_EVENT_ALL, obj);
    

    lv_keyboard_t *keyb = (lv_keyboard_t *)kb;
    lv_obj_t *btns = (lv_obj_t*)(&keyb->btnm);
    lv_obj_set_style_bg_opa(btns, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(btns, lv_palette_darken(LV_PALETTE_GREY, 1), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(btns, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(btns, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btns, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btns, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_pad_hor(btns, lv_disp_get_hor_res(lv_disp_get_default())/10, 0);
    lv_timer_t* timer_focus_kb = lv_timer_create(focus_kb_timer_handle, 250, kb);//延迟focus到kb对象
    lv_timer_set_repeat_count(timer_focus_kb, 1);
    lv_timer_reset(timer_focus_kb);

    return kb;   
}

void create_wifi_add_hidden_net_widget(){//添加隐藏wifi ui
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);

    wifi_add_hidden_net_widget = lv_obj_create(lv_layer_top());
    lv_obj_set_style_text_font(wifi_add_hidden_net_widget, osd_font_get(FONT_MID), 0);
    lv_obj_set_style_text_color(wifi_add_hidden_net_widget, lv_color_white(), 0);
    lv_obj_align(wifi_add_hidden_net_widget, LV_ALIGN_TOP_MID, 0,LV_PCT(10));
    lv_obj_set_size(wifi_add_hidden_net_widget,LV_PCT(STR_WIFI_ADD_HIDDEN_NET_WIDGET_W),LV_PCT(STR_WIFI_ADD_HIDDEN_NET_WIDGET_H));//30 40
    lv_obj_set_style_pad_gap(wifi_add_hidden_net_widget, 0, 0);
    lv_obj_set_style_pad_top(wifi_add_hidden_net_widget, 3, 0);
    lv_obj_set_style_pad_bottom(wifi_add_hidden_net_widget, 0, 0);
    lv_obj_set_style_pad_hor(wifi_add_hidden_net_widget, 0, 0);
    lv_obj_set_style_bg_color(wifi_add_hidden_net_widget, lv_color_make(81, 100, 117), 0);
    lv_obj_set_style_border_width(wifi_add_hidden_net_widget,2,0);
    lv_obj_set_style_radius(wifi_add_hidden_net_widget, 10, 0);
    lv_obj_set_scrollbar_mode(wifi_add_hidden_net_widget, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wifi_add_hidden_net_widget, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_add_hidden_net_widget, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *obj;
    obj = lv_label_create(wifi_add_hidden_net_widget);
    lv_label_set_text(obj, api_rsc_string_get(STR_WIFI_ADD));
    lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_width(obj, lv_pct(100));
   
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID),0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    
    obj = lv_textarea_create(wifi_add_hidden_net_widget);
    lv_obj_set_style_pad_ver(obj, 0, 0);
   lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_width(obj, lv_pct(98));
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_textarea_set_placeholder_text(obj, "Net name");
    lv_textarea_set_max_length(obj, WIFI_MAX_SSID_LEN);
    
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_textarea_set_one_line(obj, true);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_style_border_color(obj, lv_color_make(30,80,160), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(obj, wifi_add_hidden_net_name_event_handle, LV_EVENT_ALL, 0);
    
    obj = lv_textarea_create(wifi_add_hidden_net_widget);
    lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_width(obj, lv_pct(98));
    lv_textarea_set_placeholder_text(obj, "Password");
    lv_textarea_set_max_length(obj, WIFI_MAX_PWD_LEN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_textarea_set_one_line(obj, true);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_style_border_color(obj, lv_color_make(30,80,160), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(obj, wifi_add_hidden_net_pwd_event_handle,LV_EVENT_ALL, 0);

    obj = lv_obj_create(wifi_add_hidden_net_widget);
    lv_group_add_obj(lv_group_get_default(),obj);
    lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM,0);
    lv_obj_set_style_border_color(obj, lv_color_make(30,80,160), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(obj, wifi_add_hidden_net_security_event_handle, LV_EVENT_ALL, 0);
    lv_obj_t *drop_obj =  lv_dropdown_create(obj);
    lv_dropdown_set_options(drop_obj, "None\n"
                                  "WEP\n"
                                  "SHARED WEP\n"
                                  "WPA-PSK\n"
                                  "WPA2-PSK");
    lv_obj_set_size(drop_obj, LV_PCT(STR_WIFI_ADD_HIDDEN_DROP_OBJ_W), LV_PCT(100));
    lv_dropdown_set_selected(drop_obj, 6);
    lv_obj_set_style_pad_ver(drop_obj, 0, 0);
    lv_obj_set_style_border_width(drop_obj, 0, 0);
    lv_dropdown_set_dir(drop_obj, LV_DIR_RIGHT);
    lv_dropdown_set_symbol(drop_obj, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(drop_obj, lv_color_white(), 0);
    lv_obj_set_style_text_font(drop_obj, osd_font_get_by_langid(LANGUAGE_ENGLISH, FONT_MID), 0);
    //lv_dropdown_set_text(obj, "security");
    lv_obj_set_style_bg_opa(drop_obj, LV_OPA_0, LV_PART_MAIN);
    //lv_obj_add_event_cb(drop_obj, wifi_add_hidden_net_dropdown_obj_event_handle, LV_EVENT_ALL, 0);
   
    lv_obj_t *list = lv_dropdown_get_list(drop_obj);
    lv_obj_set_style_text_font(list, osd_font_get(FONT_MID), 0);
    if( STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_LINE_SPACE>=0){
        lv_obj_set_style_text_line_space(list, STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_LINE_SPACE, 0);
    }
    if(STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_PAD>=0){
        lv_obj_set_style_pad_all(list,  STR_WIFI_ADD_HIDDEN_DROP_OBJ_LIST_PAD, 0);
    }
    static const char* strs[3];
    strs[0] = api_rsc_string_get(STR_WIFI_CANCEL);
    strs[1] = api_rsc_string_get(STR_BT_MAKE_CONN);
    strs[2] = "";
    obj = lv_btnmatrix_create(wifi_add_hidden_net_widget);
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID),0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(obj, lv_color_black(), LV_STATE_CHECKED | LV_PART_ITEMS);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_flex_grow(obj, 3);
    lv_obj_set_width(obj, lv_pct(105));
    lv_btnmatrix_set_map(obj, strs);
    lv_obj_set_style_pad_gap(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(obj, 0, LV_PART_ITEMS);
    lv_btnmatrix_set_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(obj, true);
    lv_obj_add_event_cb(obj, wifi_add_hidden_net_btnm_event_handle, LV_EVENT_ALL, wifi_add_hidden_net_widget);
    
    kb = create_keypad_widget(obj, wifi_add_hidden_net_kb_event_handle,0);
    lv_keyboard_set_textarea(kb, lv_obj_get_child(wifi_add_hidden_net_widget, 1));
    lv_group_focus_obj(lv_obj_get_child(wifi_add_hidden_net_widget, 1));
}

static void create_connection_widget(char* name, char* strength, char *security){
    create_wifi_conne_widget_(name, strength, security, NULL);

}

static void create_static_ip_input_widget(char *name, int sel_id){
    static_ip_input_widget = lv_obj_create(lv_layer_top());
    lv_obj_center(static_ip_input_widget);
    lv_obj_set_size(static_ip_input_widget,LV_PCT(55),LV_PCT(40));
    lv_obj_set_style_pad_gap(static_ip_input_widget, 0, 0);
    lv_obj_set_scrollbar_mode(static_ip_input_widget, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(static_ip_input_widget, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(static_ip_input_widget, lv_color_make(81, 100, 117), 0);
    lv_obj_set_style_text_font(static_ip_input_widget, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(static_ip_input_widget, lv_color_white(), 0);

    lv_obj_t *obj;

    obj = lv_label_create(static_ip_input_widget);
    lv_obj_set_size(obj,LV_PCT(100),LV_PCT(20));
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(obj, name);
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);

    obj = lv_textarea_create(static_ip_input_widget);
    lv_textarea_set_password_mode(obj, false);
    lv_obj_set_size(obj,LV_PCT(100),LV_PCT(30));
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);

    lv_obj_t  *keyboard= lv_keyboard_create(static_ip_input_widget);
    lv_keyboard_set_textarea(keyboard, obj);
    static const char* map[] = {"1", "2", "3","4","5","6","7","\n","8","9","0",".","DES","RET","OK",""};
    lv_btnmatrix_ctrl_t ctrl_map[] = {1 | LV_BTNMATRIX_CTRL_POPOVER,1,1,1,1,1,1,
                                      1,1,1,1,LV_KEYBOARD_CTRL_BTN_FLAGS |1,LV_KEYBOARD_CTRL_BTN_FLAGS |1,LV_KEYBOARD_CTRL_BTN_FLAGS | 1};
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, map, ctrl_map);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);

    lv_obj_set_size(keyboard,LV_PCT(100),LV_PCT(50));
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_0, 0);
    lv_obj_t *cur_focused = lv_group_get_focused(lv_group_get_default());
    lv_obj_remove_event_dsc(keyboard, keyboard->spec_attr->event_dsc);

    lv_obj_add_event_cb(keyboard, static_ip_keyboard_handle, LV_EVENT_ALL, (void*)sel_id);

    lv_group_focus_obj(keyboard);
}
//UI代码结束














static void static_ip_keyboard_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_keyboard_t *kb = (lv_keyboard_t*)obj;
    int sel_id = (int)lv_event_get_user_data(e);

    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());

       if(key == LV_KEY_ENTER){
            const char* str = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
            if(kb->btnm.btn_id_sel == 12){
                lv_obj_del(obj->parent);
                lv_group_focus_obj(show_ip_widget);
                static_ip_input_widget = NULL;
            }else if(kb->btnm.btn_id_sel == 11){
                lv_textarea_del_char(kb->ta);
            }else if(kb->btnm.btn_id_sel == 13){
                char* str = lv_textarea_get_text(kb->ta);
                lv_label_set_text(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(show_ip_widget, 1), sel_id), 1),
                                  str);
                lv_obj_del(obj->parent);
                static_ip_input_widget = NULL;
                lv_group_focus_obj(show_ip_widget);
            }else{
                lv_textarea_add_text(kb->ta, str);
            }
        }else if(key == LV_KEY_ESC){
                lv_obj_del(obj->parent);
                kb_g_onoff(false);
                lv_group_focus_obj(show_ip_widget);
                static_ip_input_widget = NULL;
        }
    }
}

static void ip_msg_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    static int cur_id = -1;
    bool is_disable = (bool)obj->user_data;
    if(code == LV_EVENT_KEY) {
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
            if(show_ip_widget){
                lv_obj_del(show_ip_widget);
                show_ip_widget = NULL;
            }

            lv_group_focus_obj(wifi_lists);
        } else if(key == LV_KEY_DOWN){

            if(!is_disable){
                lv_obj_clear_state(lv_obj_get_child(obj, cur_id), LV_STATE_CHECKED);
                cur_id++;
                if(cur_id>= lv_obj_get_child_cnt(obj)){
                    cur_id = 0;
                }
                lv_obj_add_state(lv_obj_get_child(obj, cur_id), LV_STATE_CHECKED);
            }
        } else if(key == LV_KEY_UP){
            bool is_disable = (bool)obj->user_data;
            if(!is_disable){
                lv_obj_clear_state(lv_obj_get_child(obj, cur_id), LV_STATE_CHECKED);
                cur_id--;
                if(cur_id<0){
                    cur_id = lv_obj_get_child_cnt(obj)-1;
                }
                lv_obj_add_state(lv_obj_get_child(obj, cur_id), LV_STATE_CHECKED);
            }
        } else if(key == LV_KEY_RIGHT || key == LV_KEY_LEFT){

        } else if(key == LV_KEY_ENTER){
            if(cur_id>0){
                create_static_ip_input_widget(lv_label_get_text(lv_obj_get_child(lv_obj_get_child(obj, cur_id), 0)), cur_id);
            }
        }
    } else if(code == LV_EVENT_FOCUSED){
        if(cur_id>=0 && cur_id< lv_obj_get_child_cnt(obj)){

        } else if(lv_obj_get_child_cnt(obj)>0){
            cur_id = 0;
        }
        lv_obj_add_state(lv_obj_get_child(obj, cur_id), LV_STATE_CHECKED);
    } else if(code == LV_EVENT_DELETE){
        cur_id = -1;
    }
}

void wifi_onoff_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        //static bool sta_mode = false;
        if(key == LV_KEY_ENTER){
            if(lv_obj_has_state(lv_obj_get_child(obj, 1) , LV_STATE_CHECKED)){
                if(wifi_is_scaning){
                    if(!lv_obj_is_valid(msg_box)){
                        msg_box = create_message_box(api_rsc_string_get(STR_WIFI_SEARCHING));                       
                    }
                    return;
                }
                if(wifi_is_conning){
                    if(!lv_obj_is_valid(msg_box)){
                        msg_box = create_message_box(api_rsc_string_get(STR_WIFI_CONNING));                       
                    }
                    return;                   
                }
                lv_obj_clear_state(lv_obj_get_child(obj, 1) , LV_STATE_CHECKED);
                projector_set_some_sys_param(P_WIFI_ONOFF, 0);
                if(cur_conne_p){
                    hccast_wifi_mgr_disconnect();                  
                }

                // if(sta_mode){
                //    //hccast_wifi_mgr_exit_sta_mode();     
                //     sta_mode=false;               
                // }

                cur_wifi_id = -1;
            }else{
                lv_obj_add_state(lv_obj_get_child(obj, 1) , LV_STATE_CHECKED);
                projector_set_some_sys_param(P_WIFI_ONOFF, 1);
                if(network_wifi_module_get()){
                    //hccast_wifi_mgr_init(projector_wifi_mgr_callback_func);
                    
                   // sta_mode=true;
                   //update_saved_wifi_list();
                    scan_type = WIFI_SCAN_ONOFF;
                    projector_wifi_scan();    
                }
            }
            projector_sys_param_save();
            lv_event_send(lv_obj_get_child(obj, 1), LV_EVENT_VALUE_CHANGED, NULL);
        }else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
            #ifdef REMOTE_KEY
            if(key == LV_KEY_RIGHT){
                if(!lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN) && !wifi_is_scaning && lv_obj_get_child_cnt(wifi_lists)>2){
                    lv_group_focus_obj(wifi_lists);
                }
                return;
            }
            #endif 
            lv_group_focus_obj(wifi_search);
        } else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
            lv_group_focus_obj(wifi_return);
        }else if(key == LV_KEY_ESC){
            cur_wifi_id = -1;
            cur_wifi_setting_obj = wifi_onoff;
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        }
    } else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        dsc->rect_dsc->bg_color = lv_color_white();
    }else if(code == LV_EVENT_FOCUSED){
        cur_wifi_setting_obj = wifi_onoff;
    }
}

void wifi_search_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(!lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN)){  
                if(wifi_is_scaning){
                    if(!lv_obj_is_valid(msg_box)){
                        msg_box = create_message_box(api_rsc_string_get(STR_WIFI_SEARCHING));                       
                    }
                    return;
                }
                if(wifi_is_conning){
                    if(!lv_obj_is_valid(msg_box)){
                        msg_box = create_message_box(api_rsc_string_get(STR_WIFI_CONNING));                       
                    }
                    return;                   
                }     
                if(network_wifi_module_get()){
                    projector_wifi_scan();                      
                }else{
                    create_message_box(api_rsc_string_get(STR_WIFI_NO_DEVICE));
                }
             
            }
        } else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
            lv_group_focus_obj(wifi_onoff);
        } else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
            #ifdef REMOTE_KEY
            if(key == LV_KEY_RIGHT){
                if(!lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN) && !wifi_is_scaning && lv_obj_get_child_cnt(wifi_lists)>2){
                    lv_group_focus_obj(wifi_lists);
                }
                return;
            }
            #endif
            lv_group_focus_obj(wifi_add);
        }else if(key == LV_KEY_ESC){
            cur_wifi_id = -1;
            cur_wifi_setting_obj = wifi_onoff;
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        }
    } else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        dsc->rect_dsc->bg_color = lv_color_white();
    } else if(code == LV_EVENT_FOCUSED){
        cur_wifi_setting_obj = wifi_search;
    }
}



void wifi_add_event_handle(lv_event_t *e){
     lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(wifi_is_scaning){
                if(!lv_obj_is_valid(msg_box)){
                    msg_box = create_message_box(api_rsc_string_get(STR_WIFI_SEARCHING));                       
                }
                return;
            }
            if(wifi_is_conning){
                if(!lv_obj_is_valid(msg_box)){
                    msg_box = create_message_box(api_rsc_string_get(STR_WIFI_CONNING));                       
                }
                return;                   
            }
           if(!lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN)){  
                cur_scr_focused_obj = obj;
                create_wifi_add_hidden_net_widget();
           }
        } else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
            lv_group_focus_obj(wifi_search);
        } else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
            #ifdef REMOTE_KEY
            if(key == LV_KEY_RIGHT){
                if(!lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN) && !wifi_is_scaning && lv_obj_get_child_cnt(wifi_lists)>2){
                    lv_group_focus_obj(wifi_lists);
                }
                return;
            }
            #endif
            lv_group_focus_obj(wifi_return);
        }else if(key == LV_KEY_ESC){
            cur_wifi_id = -1;
            cur_wifi_setting_obj = wifi_onoff;
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        }
    } else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        dsc->rect_dsc->bg_color = lv_color_white();
    } else if(code == LV_EVENT_FOCUSED){
        cur_wifi_setting_obj = wifi_search;
    }
}

void wifi_return_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            cur_wifi_id = -1;
            cur_wifi_setting_obj = wifi_onoff;
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        } else  if(key == LV_KEY_RIGHT && !wifi_is_scaning && lv_obj_get_child_cnt(wifi_lists)>2){
            if(!lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN)){
                lv_group_focus_obj(wifi_lists);
            }

        } 
        #ifdef REMOTE_KEY
        else if(key == LV_KEY_UP){
        #else
        else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
        #endif
            lv_group_focus_obj(wifi_add);
        } else if(key == LV_KEY_DOWN){
            lv_group_focus_obj(wifi_onoff);
        }else if(key == LV_KEY_ESC){
            cur_wifi_id = -1;
            cur_wifi_setting_obj = wifi_onoff;
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        }
    } else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        dsc->rect_dsc->bg_color = lv_color_white();
    }else if(code == LV_EVENT_FOCUSED){
        cur_wifi_setting_obj = wifi_return;
    }
}

void wifi_sw_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED){
        if(lv_obj_has_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN)){
            lv_obj_clear_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN);
        }else{
            lv_obj_add_flag(wifi_lists, LV_OBJ_FLAG_HIDDEN);
        }
    }
}


static void saved_wifi_event_handle(lv_event_t *e){
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    bool end_handle = false;
    if(code == LV_EVENT_PRESSED){
        if(lv_msgbox_get_active_btn(target) == 0){
            char *ssid = get_list_sub_wifi_ssid(cur_wifi_id);
            if(sysdata_get_wifi_info(ssid))
                wifi_connect(ssid, sysdata_get_wifi_info(ssid)->pwd);
            end_handle = true;
        }else if(lv_msgbox_get_active_btn(target) == 1){

            int i=sysdata_get_wifi_index_by_ssid(get_list_sub_wifi_ssid(cur_wifi_id));
            if(i>=0 && i<MAX_WIFI_SAVE){
                sysdata_wifi_ap_delete(i);
            }
            projector_sys_param_save();
            lv_obj_del(lv_obj_get_child(wifi_lists, cur_wifi_id));
            if(cur_wifi_id == lv_obj_get_index(wifi_save_obj)+1){
                cur_wifi_id = 0;
            }else{
                cur_wifi_id-=1;
            }
            end_handle=true;
            scan_type =WIFI_SCAN_SEARCH;
            projector_wifi_scan();
        }
    }else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            end_handle = true;
        }
    }

    if(end_handle){
        lv_group_focus_obj(wifi_lists);
        lv_obj_del(target);
        reconn_msg_box = NULL;
    }
}

void wifi_lists_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
   
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        #ifdef REMOTE_KEY
        if( key == LV_KEY_ESC || key == LV_KEY_LEFT){
        #else
        if( key == LV_KEY_ESC){
        #endif
            lv_group_focus_obj( cur_wifi_setting_obj);
        } 
        #ifdef REMOTE_KEY
        else if(key == LV_KEY_UP){
        #else
        else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
        #endif   
            lv_obj_clear_state(lv_obj_get_child(obj, cur_wifi_id), LV_STATE_CHECKED);
            lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, 0);
            cur_wifi_id--;
            bool scroll_max = false;
            while (cur_wifi_id>=0 && lv_obj_get_child(obj, cur_wifi_id)->class_p == &lv_list_text_class){
                cur_wifi_id--;
            }
            if(cur_wifi_id<0){
                cur_wifi_id = lv_obj_get_child_cnt(obj)-1;
                while (cur_wifi_id>=0 && lv_obj_get_child(obj, cur_wifi_id)->class_p == &lv_list_text_class){
                    cur_wifi_id--;
                }
                scroll_max=true;
            }
            if(cur_wifi_id>= 0 ){
                lv_obj_add_state(lv_obj_get_child(obj, cur_wifi_id), LV_STATE_CHECKED);
                // if(scroll_max){
                if(cur_wifi_id == lv_obj_get_index(wifi_nearby_obj)+1 || cur_wifi_id == lv_obj_get_index(wifi_save_obj + 1)){

                    lv_obj_scroll_to_view(lv_obj_get_child(obj, 0), LV_ANIM_OFF);   
                }else{
                    lv_obj_scroll_to_view(lv_obj_get_child(obj, cur_wifi_id), LV_ANIM_OFF);   
                }
                if(scroll_max){
                    wifi_list_set_cur(SET_CUR_LAST);
                }else{
                    if(cur_wifi_id>lv_obj_get_index(wifi_nearby_obj)){
                        wifi_list_set_cur(SET_CUR_PREV);
                    }
                }
                lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, (void*)1);
            }

        } 
        #ifdef REMOTE_KEY
         else if(key == LV_KEY_DOWN){
        #else
        else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
        #endif
            lv_obj_clear_state(lv_obj_get_child(obj, cur_wifi_id), LV_STATE_CHECKED);
            lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, 0);
            cur_wifi_id++;
            bool scroll_min = false;
            while (cur_wifi_id< lv_obj_get_child_cnt(obj) && lv_obj_get_child(obj, cur_wifi_id)->class_p == &lv_list_text_class){
                cur_wifi_id++;
            }
            if(cur_wifi_id >= lv_obj_get_child_cnt(obj)){
                
                cur_wifi_id = 0;
                while (cur_wifi_id < lv_obj_get_child_cnt(obj) && lv_obj_get_child(obj, cur_wifi_id)->class_p == &lv_list_text_class){
                    cur_wifi_id++;
                }
                scroll_min = true;
            }
            if( cur_wifi_id< lv_obj_get_child_cnt(obj)){//cur_wifi_id >= 0 && 
                printf("CUR ID: %d\n", cur_wifi_id);//26
                lv_obj_add_state(lv_obj_get_child(obj, cur_wifi_id), LV_STATE_CHECKED);
                
                if(scroll_min){
                    wifi_list_set_cur(SET_CUR_FIRST);//bug 1
                    lv_obj_scroll_to_view(lv_obj_get_child(obj, 0), LV_ANIM_OFF);
                }else{
                    if(cur_wifi_id>lv_obj_get_index(wifi_nearby_obj)+1){
                        wifi_list_set_cur(SET_CUR_NEXT);
                    }else{
                         wifi_list_set_cur(SET_CUR_FIRST);
                    }
                    lv_obj_scroll_to_view(lv_obj_get_child(obj, cur_wifi_id), LV_ANIM_OFF);
                   
                }
                lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, (void*)1);

            }

        } else if( key == LV_KEY_ENTER){

            if(cur_wifi_id< lv_obj_get_index(wifi_save_obj)){
                cur_scr_focused_obj = obj;
                lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, 0), 3), LV_EVENT_PRESSED, NULL);
            }else if(cur_wifi_id < lv_obj_get_index(wifi_nearby_obj) && cur_wifi_id != lv_obj_get_index(wifi_save_obj)){
                if(cur_wifi_conning_id>(int)lv_obj_get_index(wifi_save_obj)){
                    create_message_box(api_rsc_string_get(STR_WIFI_CONNING));
                    return;
                }
                reconn_msg_box = create_message_box1(STR_NONE, STR_BT_MAKE_CONN, STR_BT_DELETE, saved_wifi_event_handle,
                                                STR_WIFI_SAVED_CONNECT_MSG_BOX_W,STR_WIFI_SAVED_CONNECT_MSG_BOX_H);

            } else{
                if(cur_wifi_conning_id>(int)lv_obj_get_index(wifi_save_obj)){
                    create_message_box(api_rsc_string_get(STR_WIFI_CONNING));
                    return;
                }
                if(wifi_list_get_cur() &&  wifi_list_get_cur()->res.encryptMode == HCCAST_WIFI_ENCRYPT_MODE_NONE){
                    wifi_connect(wifi_list_get_cur()->res.ssid, NULL);
                }else{
                    if(!wifi_conn_widget){
                        cur_scr_focused_obj = obj;
                        create_connection_widget(get_list_sub_wifi_ssid(cur_wifi_id),
                        wifi_get_quality(wifi_list_get_cur()->res.quality), wifi_get_entryMode(wifi_list_get_cur()->res.encryptMode) );                                       
                    }                  
                }

            }
        }
    } else if(code == LV_EVENT_FOCUSED){
        //slave_scr_clear();
        if(cur_wifi_id>=0 && cur_wifi_id< lv_obj_get_child_cnt(obj) && lv_obj_get_child(obj, cur_wifi_id)->class_p != &lv_list_text_class){
            if(cur_wifi_id==0 || cur_wifi_id == lv_obj_get_index(wifi_nearby_obj)+1){
                wifi_list_set_cur(SET_CUR_FIRST);
            }
            lv_obj_add_state(lv_obj_get_child(obj, cur_wifi_id), LV_STATE_CHECKED);
            lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, (void*)1);
            lv_obj_scroll_to_view(lv_obj_get_child(obj, cur_wifi_id), LV_ANIM_OFF);     
            return;
        }
        cur_wifi_id = 0;
        while (cur_wifi_id < lv_obj_get_child_cnt(obj) && lv_obj_get_child(obj, cur_wifi_id)->class_p == &lv_list_text_class){
            //lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, (void*)1);
            cur_wifi_id++;
        }

        if(cur_wifi_id<lv_obj_get_child_cnt(obj)){
            if(cur_wifi_id==0 || cur_wifi_id == lv_obj_get_index(wifi_nearby_obj)+1){
                wifi_list_set_cur(SET_CUR_FIRST);
            }
        
            lv_obj_add_state(lv_obj_get_child(obj, cur_wifi_id), LV_STATE_CHECKED);
            lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, (void*)1);
            lv_obj_scroll_to_view(lv_obj_get_child(obj, cur_wifi_id), LV_ANIM_OFF);  
        }else{
            cur_wifi_id=-1;
            lv_group_focus_obj(wifi_onoff);
        }

    } else if(code == LV_EVENT_DEFOCUSED){
        if(cur_wifi_id<0){
            return;
        }
        lv_obj_clear_state(lv_obj_get_child(obj, cur_wifi_id), LV_STATE_CHECKED);
        lv_event_send(lv_obj_get_child(lv_obj_get_child(obj, cur_wifi_id), 1), LV_EVENT_REFRESH, 0);
    }
}




////联网逻辑



// static void wifi_msg_box_handle(lv_event_t *e){
//     lv_event_code_t code = lv_event_get_code(e);
//     lv_obj_t* obj = lv_event_get_current_target(e);

//     if(code == LV_EVENT_KEY){
//         uint16_t key = lv_indev_get_key(lv_indev_get_act());
//         lv_btnmatrix_t* btns = (lv_btnmatrix_t*)lv_msgbox_get_btns(obj);
//         if(key == LV_KEY_ENTER){
//         if( lv_msgbox_get_active_btn(obj)==0){
             
//         }else{
//             hccast_wifi_ap_info_t *ap_wifi=cur_conne_p;
//             int index = sysdata_check_ap_saved(ap_wifi);
//             printf("ssid index: %d\n",index);
//             if(index >= 0)//set the index ap to first.
//             {
//                 sysdata_wifi_ap_delete(index);
//             }
//             sysdata_wifi_ap_save(ap_wifi);

//             projector_sys_param_save();
//             sleep(1);
//         }
//         lv_msgbox_close(obj);
//          lv_group_focus_obj(wifi_lists);
//         }
//     }
// }

static void saved_wifi_reconn_event_handle(lv_event_t *e){
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e); 
    bool end_handle = false;
    if(code == LV_EVENT_PRESSED){
        if(lv_msgbox_get_active_btn(target) == 0){
            create_wifi_new_pwd_conne_widget();
            lv_obj_del(target);
            reconn_msg_box = NULL;
        }else if(lv_msgbox_get_active_btn(target) == 1){
            char *ssid = get_list_sub_wifi_ssid(cur_wifi_conning_id);
            //wifi_connect(ssid, sysdata_get_wifi_info(ssid)->pwd);
            app_wifi_reconnect(sysdata_get_wifi_info(ssid));
            end_handle = true;
        }
    }else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            end_handle = true;
             set_label_text2(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), STR_BT_SAVED,FONT_NORMAL);
            cur_wifi_conning_id = -1;
            cur_conne_p = NULL;
        }
    }

    if(end_handle){
        lv_group_focus_obj(wifi_lists);
        lv_obj_del(target);
        reconn_msg_box = NULL;
    }
}

static void saved_wifi_conn_faild_event_handle (lv_event_t *e){
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    bool end_handle = false;
    if(code == LV_EVENT_PRESSED){
        if(lv_msgbox_get_active_btn(target) == 0){
                char *ssid = get_list_sub_wifi_ssid(cur_wifi_conning_id);
                printf("pwd is: %s\n", sysdata_get_wifi_info(ssid)->pwd);
                reconn_msg_box = create_message_box1(STR_WIFI_RE_ENTER_PWD, STR_PROMPT_YES, STR_PROMPT_NO, saved_wifi_reconn_event_handle, WIFI_RECONN_MSG_BOX_W, WIFI_RECONN_MSG_BOX_H);

                lv_obj_del(target);
        }else if(lv_msgbox_get_active_btn(target) == 1){
            if(cur_wifi_id == cur_wifi_conning_id){
                if(cur_wifi_id == lv_obj_get_index(wifi_save_obj)+1){
                    cur_wifi_id = 0;
                }else{
                    cur_wifi_id-=1;
                }
            }
           
            int i=sysdata_get_wifi_index_by_ssid(get_list_sub_wifi_ssid(cur_wifi_conning_id));
            if(i>=0 && i<MAX_WIFI_SAVE){
                sysdata_wifi_ap_delete(i);
            }
            lv_obj_del(lv_obj_get_child(wifi_lists, cur_wifi_conning_id));
            cur_wifi_conning_id = -1;
            end_handle=true;
            scan_type =WIFI_SCAN_SEARCH;
            projector_wifi_scan();
        }
    }else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            end_handle = true;
            set_label_text2(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), STR_BT_SAVED, FONT_NORMAL);
            cur_wifi_conning_id = -1;
            cur_conne_p = NULL;
        }
    }

    if(end_handle){
        lv_group_focus_obj(wifi_lists);
        lv_obj_del(target);
        reconn_msg_box = NULL;
    }
}

static void win_wifi_control(void* arg1, void* arg2){
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    switch (ctl_msg->msg_type)
    {
    case MSG_TYPE_NETWORK_WIFI_SCAN_DONE:
        wifi_list_set_cur(SET_CUR_FIRST);
        int i=0;
        for(wifi_scan_node *node = wifi_list_get_cur(); node != NULL && i<wifi_list_get_len();wifi_list_set_cur(SET_CUR_NEXT), node =wifi_list_get_cur()){
            hccast_wifi_ap_info_t* res = &node->res;
            if (lv_obj_get_index(wifi_save_obj)>0){
                if (strcmp(res->ssid, get_list_sub_wifi_ssid(0))==0){
                    wifi_list_remove(node, false);
                    continue;
                }
                
            }

            if(strlen(res->ssid) == 0){
                continue;
            }
            
            int quatity = res->quality;
            void *img = (quatity<=100 && quatity>=80) ? &wifi4 :
                            (quatity<80 && quatity>=60) ? &wifi3 :
                            (quatity<60 && quatity>=40) ? &wifi2 :
                            (quatity<40 && quatity>=20) ? &wifi1: &wifi0;
            i++;
            if(wifi_list_get_cur()->res.encryptMode == HCCAST_WIFI_ENCRYPT_MODE_NONE){
                create_list_sub_wifi_btn_obj0(wifi_lists, img, res->ssid,STR_NONE);
            }else{
                create_list_sub_wifi_btn_obj(wifi_lists, img, res->ssid);
            }
            
        }

         lv_obj_set_style_text_align(searching_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text_fmt(searching_label, "%d", i);

        if(scan_type == WIFI_SCAN_ONOFF || scan_type == WIFI_SCAN_ENTER){
           if(saved_wifi_sig_strength_max_id>=0 && saved_wifi_sig_strength_max_id<MAX_WIFI_SAVE){
                hccast_wifi_ap_info_t *info = sysdata_get_wifi_info_by_index(saved_wifi_sig_strength_max_id);
                if(!cur_conne_p && info){
                    update_saved_wifi_list();
                    cur_wifi_id = saved_wifi_sig_strength_max_id+lv_obj_get_index(wifi_save_obj)+1;
                    wifi_connect(info->ssid, info->pwd); 
                }
                
            }   
             saved_wifi_sig_strength_max_id=-1;
            scan_type = WIFI_SCAN_SEARCH;
        }


        break;
    case MSG_TYPE_NETWORK_WIFI_CONNECTED:
         wifi_is_conning=false;
        if(lv_obj_get_index(wifi_save_obj)>0){//之前有连接的wifi
            if(cur_conne_p && sysdata_check_ap_saved(cur_conne_p)>=0){
                //language_choose_add_label1(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 2), 0), );
                set_label_text2(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 2), 0), STR_BT_SAVED, FONT_NORMAL);
                if(cur_wifi_id == 0){
                    cur_wifi_id = lv_obj_get_index(wifi_save_obj);
                }
                lv_obj_add_flag(get_list_sub_wifi_prompt_obj(0), LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_to_index(lv_obj_get_child(wifi_lists, 0), lv_obj_get_index(wifi_save_obj));

            }          
        }    
        if(conn_type == WIFI_CONN_NORMAL){
            if(cur_wifi_conning_id<0){
                return;
            }
            lv_obj_t *label;
            if(cur_wifi_conning_id>(int)lv_obj_get_index(wifi_nearby_obj)){
                lv_obj_del(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0));
                label = lv_label_create(get_list_sub_wifi_status_obj(cur_wifi_conning_id));
                lv_obj_center(label);
                lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER,0);
                lv_obj_set_style_text_color(label, lv_color_white(), 0);
                wifi_list_remove(cur_wifi_conning_node_p, true);
                cur_wifi_conning_node_p = NULL;
            }else{
                //printf("id:%d, ssid:%s\n", cur_wifi_conning_id, lv_label_get_text(lv_obj_get_child(lv_obj_get_child(wifi_lists, cur_wifi_conning_id), 1)));
                hccast_wifi_ap_info_t *info = sysdata_get_wifi_info(get_list_sub_wifi_ssid(cur_wifi_conning_id));
                if(info){
                    memcpy(&cur_conne, info, sizeof(hccast_wifi_ap_info_t));
                    cur_conne_p = &cur_conne;
                }
                label = lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id),0);
            }
            //language_choose_add_label1(label, );
            set_label_text2(label, STR_BT_CONN, FONT_NORMAL);    
            lv_obj_clear_flag(get_list_sub_wifi_prompt_obj(cur_wifi_conning_id), LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(lv_obj_get_child(wifi_lists, cur_wifi_conning_id), 0);
            
            
            if(cur_wifi_id == cur_wifi_conning_id){
                cur_wifi_id=0;
            }else if(cur_wifi_id<cur_wifi_conning_id){
                cur_wifi_id+=1;
            }
            cur_wifi_conning_id=-1;             

            // lv_btnmatrix_get_selected_btn(btns, 0);

        }else if(conn_type == WIFI_CONN_HIDDEN){
            memcpy(&cur_conne, &hidden_ap_info, sizeof(hccast_wifi_ap_info_t));
            cur_conne_p = &cur_conne;
            lv_obj_t * sub_list = create_list_sub_wifi_btn_obj0(wifi_lists, &wifi3, cur_conne_p->ssid, STR_BT_CONN);
            lv_obj_move_to_index(sub_list, 0);
            lv_obj_clear_flag(get_list_sub_wifi_prompt_obj(0), LV_OBJ_FLAG_HIDDEN);
            conn_type = WIFI_CONN_NORMAL;
            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
            if(prompt_box){
                lv_obj_del(prompt_box);
               prompt_box = NULL;
            }
            set_remote_control_disable(false);
            create_message_box(api_rsc_string_get(STR_WIFI_CONN_SUCCESS));
        }

        lv_obj_scroll_to_view(lv_obj_get_child(wifi_lists, 0), LV_ANIM_OFF);

        // hccast_wifi_ap_info_t *ap_wifi=cur_conne_p;
        // int index = sysdata_check_ap_saved(ap_wifi);
        // printf("ssid index: %d\n",index);
        // if(index >= 0)//set the index ap to first.
        // {
        //     sysdata_wifi_ap_delete(index);
        // }
        // sysdata_wifi_ap_save(ap_wifi);
        // projector_sys_param_save(); 
		if(lv_obj_get_index(wifi_nearby_obj)-lv_obj_get_index(wifi_save_obj) - 1 > MAX_WIFI_SAVE){
			update_saved_wifi_list();
		}
        sleep(1);        

        break;
    case MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL:
    case MSG_TYPE_NETWORK_WIFI_DISCONNECTED:
        wifi_is_conning=false;
        // lv_event_send(wifi_show, LV_EVENT_REFRESH, "");
        if(lv_obj_get_index(wifi_save_obj)>0){
            if(cur_conne_p && sysdata_check_ap_saved(cur_conne_p)>=0){
                //lv_label_set_text(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 2), 0), "已保存");
                //language_choose_add_label1(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 2), 0), );
                set_label_text2(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, 0), 2), 0), STR_BT_SAVED, FONT_NORMAL);
                if(cur_wifi_id==0){
                    cur_wifi_id = lv_obj_get_index(wifi_save_obj);
                }
                lv_obj_add_flag(get_list_sub_wifi_prompt_obj(0), LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_to_index(lv_obj_get_child(wifi_lists, 0), lv_obj_get_index(wifi_save_obj));
                //cur_wifi_id = lv_obj_get_index(wifi_nearby_obj)-1;
            }
        }
        if (ctl_msg->msg_type == MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL){
            if(cur_wifi_conning_id>(int)lv_obj_get_index(wifi_nearby_obj)){
                if(cur_wifi_conning_node_p->res.encryptMode == HCCAST_WIFI_ENCRYPT_MODE_NONE){
                    //language_choose_add_label1(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), );
                    set_label_text2(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0), STR_NONE, FONT_NORMAL);
                    lv_label_set_text(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0),"");
                }else{
                    lv_obj_del(lv_obj_get_child(get_list_sub_wifi_status_obj(cur_wifi_conning_id), 0));
                    lv_obj_t *img = lv_img_create(get_list_sub_wifi_status_obj(cur_wifi_conning_id));
                    lv_img_set_src(img, &pwd_icon);
                    lv_obj_center(img);                
                }
                cur_wifi_conning_node_p = NULL;
                
                create_message_box(api_rsc_string_get(STR_WIFI_CONNECT_ERR));            
            }else if(cur_wifi_conning_id>(int)lv_obj_get_index(wifi_save_obj)){
                reconn_msg_box = create_message_box1(STR_WIFI_CONNECT_ERR, STR_BT_MAKE_CONN, STR_BT_DELETE, saved_wifi_conn_faild_event_handle,
                                                     STR_WIFI_CONNECT_ERR_MSG_BOX_W,STR_WIFI_CONNECT_ERR_MSG_BOX_H);
                return;
                
            }              
        }
        cur_wifi_conning_id=-1;
        cur_conne_p = NULL;
        conn_type = WIFI_CONN_NORMAL;  
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
        if(prompt_box){
            lv_obj_del(prompt_box);
           prompt_box=NULL;
            set_remote_control_disable(false);
            create_message_box(api_rsc_string_get(STR_WIFI_CONN_FAILED));
        
        }        
        break;
    default:
        break;
    }
}

static void wifi_scan_task(void* parm){
    hccast_wifi_scan_result_t *scan_res = (hccast_wifi_scan_result_t *)calloc(1, sizeof(hccast_wifi_scan_result_t));
    int ret = 0;
    int i = 0;
    ret = hccast_wifi_mgr_scan(scan_res);
    if(ret != 0){
        if(ret == HCCAST_WIFI_ERR_CMD_WPAS_NO_RUN){
            create_message_box(api_rsc_string_get(STR_WIFI_READY));
        }else{
            printf("%s:%d: hccast_wifi_mgr_scan failed, ret=%d\n", __func__, __LINE__, ret);
        }
        
    }
    free(scan_res);
    wifi_is_scaning = false;   
#ifdef __HCRTOS__    
    vTaskDelete(NULL);
#endif    
 
}


static void seaching_timer_handle(lv_timer_t* t){
    if (!wifi_is_scaning){
        lv_timer_del(t);
        timer_scan = NULL;
        if(searching_label){
           
           // lv_obj_add_flag(searching_label, LV_OBJ_FLAG_HIDDEN);
        } 
        return;
    }
    
    static int i=0;
    i = (++i)%4;
    char *dot_str = i == 0 ? " " : i == 1 ? "." : i==2 ? ".." : "...";
   
    lv_label_set_text_fmt(searching_label, "%s%s", (char*)api_rsc_string_get(STR_WIFI_SEARCHING), dot_str);
    
}

int projector_wifi_scan(){
    if(!wifi_is_scaning && !wifi_is_conning){
        app_wifi_switch_work_mode(WIFI_MODE_STATION);

        wifi_list_set_zero();
        cur_wifi_id = -1;
        wifi_list.p_n = wifi_list.head->next;

        int size = lv_obj_get_child_cnt(wifi_lists);
        int j = lv_obj_get_index(wifi_nearby_obj)+1;
        for(int i = j; i<size; i++){
            printf("%d/%d\n",size, i);
            lv_obj_del(lv_obj_get_child(wifi_lists, j));
        }      

        wifi_is_scaning = true;
        if (!searching_label){
            searching_label = lv_label_create(wifi_nearby_obj);
            lv_obj_align(searching_label, LV_ALIGN_TOP_RIGHT, 0,0);
           
            lv_obj_set_size(searching_label, LV_PCT(SEARCHING_LABEL_HOR_PCT), LV_PCT(100));//23
            lv_label_set_text(searching_label, (char*)api_rsc_string_get(STR_WIFI_SEARCHING));
            //set_label_text2(searching_label, STR_WIFI_SEARCHING, FONT_NORMAL);
            lv_obj_set_style_text_color(searching_label, lv_color_white(), 0);  
                   
        }
        //lv_obj_clear_flag(searching_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_align(searching_label, LV_TEXT_ALIGN_LEFT, 0);
        if(!timer_scan){
            timer_scan = lv_timer_create(seaching_timer_handle, 300, NULL);
            lv_timer_reset(timer_scan);           
        }
    #ifdef __HCRTOS__
        xTaskCreate(wifi_scan_task, "wifi scan", 0x1000,NULL , portPRI_TASK_NORMAL ,NULL );
    #else
        pthread_t thread_id = 0;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x1000);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
        pthread_create(&thread_id, &attr, wifi_scan_task, NULL);
        pthread_attr_destroy(&attr);
    #endif
    }

    return 0;
}


// static void wifi_conne_task(void* parm){
//     hccast_wifi_ap_info_t *ap_info = (hccast_wifi_ap_info_t *)parm;

//     hccast_wifi_mgr_connect(ap_info);
//     if (hccast_wifi_mgr_get_connect_status()){
//         printf("hccast_wifi_mgr_get_connect_status: %d\n", hccast_wifi_mgr_get_connect_status());
//         hccast_wifi_mgr_udhcpc_start();
//     }else{
// 		hccast_wifi_mgr_disconnect();
// 	}

//     vTaskDelete(NULL);
// }


int wifi_connect(char *ssid , char *pwd){
    printf("CONNECTION\n");

    hccast_wifi_ap_info_t *ap_info = NULL;

    if((cur_wifi_id>lv_obj_get_index(wifi_save_obj) && cur_wifi_id<lv_obj_get_index(wifi_nearby_obj)) || cur_wifi_id==-1){//-1表示打开wifi后自动连接
        ap_info = sysdata_get_wifi_info(ssid);
        if(!ap_info){
            return -1;
        }
    }else if(cur_wifi_id > (int)lv_obj_get_index(wifi_nearby_obj)){
        ap_info = &wifi_list.p_n->res;
        if(!(ap_info->encryptMode == HCCAST_WIFI_ENCRYPT_MODE_NONE)){
            strcpy(ap_info->pwd, pwd); 
        }
    }
    
    ap_info->keyIdx = 1;

    lv_obj_del(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(wifi_lists, cur_wifi_id), 2), 0));
    lv_obj_t *label = lv_label_create(lv_obj_get_child(lv_obj_get_child(wifi_lists, cur_wifi_id), 2));
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER,0);
    //language_choose_add_label1(label, );
    set_label_text2(label, STR_WIFI_CONNING, FONT_NORMAL); 

    cur_wifi_conning_id = cur_wifi_id;
    if(cur_wifi_conning_id>lv_obj_get_index(wifi_nearby_obj)){
        cur_wifi_conning_node_p = wifi_list.p_n;
    }
    
    //app_wifi_switch_work_mode(WIFI_MODE_STATION);
    //xTaskCreate(wifi_conne_task, "wifi conne", 0x1000,(void*)ap_info , portPRI_TASK_NORMAL ,NULL );
    // hccast_wifi_mgr_udhcpc_stop();     
    if(!app_wifi_reconnect(ap_info)){
        wifi_is_conning = true;
    }


    return 0;
}



int projector_wifi_init(void)
{

    return 0;
}

int projector_wifi_exit(void)
{
    hccast_wifi_mgr_uninit();

    return 0;
}
#endif

