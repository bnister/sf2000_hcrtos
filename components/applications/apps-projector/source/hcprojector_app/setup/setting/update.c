#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include "screen.h"
#include "factory_setting.h"
#include "setup.h"
#include "com_api.h"
#include "mul_lang_text.h"
#include <hcfota.h>
#include "app_config.h"
#ifdef __HCRTOS__
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#else
#include <sys/stat.h>
#endif
#include "osd_com.h"
#include <pthread.h>

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define UPGRADE_WIDGET_WIDTH_PCT 45
    #define UPGRADE_WIDGET_HEIGHT_PCT 33
#else
    #define UPGRADE_WIDGET_WIDTH_PCT 35
    #define UPGRADE_WIDGET_HEIGHT_PCT 25
#endif


lv_timer_t *timer_update;
static long progress = -1;
lv_obj_t *prompt_label = NULL;
lv_obj_t *progress_bar = NULL;
#ifdef USB_AUTO_UPGRADE
volatile int m_usb_upgrade = 0;
#endif

extern void set_remote_control_disable(bool b);
extern int mmp_get_usb_stat();

static bool find_software(DIR *d,  char str[160]);
static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata);
static void software_update_handler(void *target_);
static void timer_update_handle(lv_timer_t *t);
static void software_update_event_handle(lv_event_t *e);
void software_update_widget(lv_obj_t* btn);

static void software_update_widget_(char* urls);

static char* updata_url;

extern lv_timer_t *timer_setting, *timer_setting_clone;
extern lv_font_t* select_font_normal[3];
extern lv_obj_t* slave_scr_obj;


// static bool find_software(DIR *d, int depth, char str[][40]){
//     struct dirent *file;
//     struct stat sd;
//     if(depth>=4){
//         return false;
//     }

//     while((file = readdir(d)) != NULL){
//         if(strncmp(file->d_name, ".", 1) == 0){
//             continue;
//         }
//         if((strcmp(file->d_name+strlen(file->d_name)-4, ".bin") == 0) && (strncmp(file->d_name, "HCFOTA_HC",9)==0)){//HCFOTA_HC16A3000V10_2210311132.bin
           
//            strcpy((char*)str[depth], file->d_name);

//             return true;
//         }
//         char file_name[150];
//         memset(file_name, 0, 150);
//         strcat(file_name, MOUNT_ROOT_DIR"/");
//         for(int i=0; i<depth; i++){
//             strcat(file_name, str[i]);
//         }
//         strcat(file_name, file->d_name);
//         if(stat(file_name, &sd)>=0 && S_ISDIR(sd.st_mode) && depth<4){
//             DIR *temp;
//             strcat(file_name, "/");
//             if(!(temp = opendir(file_name))){
//                 continue;
//             }
//             strcpy((char*)str[depth], file->d_name);
//             strcat((char*)str[depth], "/");
//             bool result =  find_software(temp, depth+1, str);
//             if(result){
//                 return result;
//             }
//         }
//     }
    
//     return false;
// }


static bool find_software(DIR *d,  char str[160]){
    struct dirent *file;
    struct stat sd;
    char file_name[160] = {0};
    while((file = readdir(d)) != NULL){
            if(strncmp(file->d_name, ".", 1) == 0){
                continue;
            }
            memset(file_name, 0, 160);
            strcat(file_name, MOUNT_ROOT_DIR"/");
            strcat(file_name, file->d_name);
            strcat(file_name, "/HCFOTA.bin");

            if(!stat(file_name, &sd)){
                strcpy(str, file_name);
                printf("update file: %s\n", str);
                return true;
            }

    }
    // struct dirent *file;
    // struct stat sd;
	// char *line_tok; 
	// char tmpbuff[32] = {0};
	// char tmp_name[64]={0};
    // // if(depth>=2){
    // //     return false;
    // // }
	// strcpy(tmpbuff,"HCFOTA_"); 
	// strcat(tmpbuff,(char*)projector_get_some_sys_param(P_DEV_PRODUCT_ID)); 
	// printf("%s,%d,%s\n",__func__,__LINE__,tmpbuff);
    // while((file = readdir(d)) != NULL){
    //     if(strncmp(file->d_name, ".", 1) == 0){
    //         continue;
    //     }
    //     if((strcmp(file->d_name+strlen(file->d_name)-4, ".bin") == 0) && (strncmp(file->d_name, tmpbuff,strlen(tmpbuff))==0)){//HCFOTA_HC16A3000V10_2210311132.bin
    //     	memset(tmp_name,0,64);
	// 		strcpy(tmp_name,file->d_name);
    //        line_tok = strtok(tmp_name,"_");   
	// 	   line_tok = strtok(NULL,"_");
	// 	   line_tok = strtok(NULL,"_");   
	// 	   line_tok = strtok(line_tok,"."); 
		     
	// 	   if((atoll(line_tok)) <= ((unsigned int)projector_get_some_sys_param(P_DEV_VERSION)))    
	// 	   		continue;
    //        strcpy((char*)str, file->d_name);
	// 		printf("%s,%d,%s\n",__func__,__LINE__,file->d_name);
    //         return true;
    //     }
        // char file_name[1024];
        // memset(file_name, 0, 1024);
        // strcat(file_name, MOUNT_ROOT_DIR"/");
        // for(int i=0; i<depth; i++){
        //     strcat(file_name, str[i]);
        // }
        // strcat(file_name, file->d_name);
        // if(stat(file_name, &sd)>=0 && S_ISDIR(sd.st_mode) && depth<4){
        //     DIR *temp;
        //     strcat(file_name, "/");
        //     if(!(temp = opendir(file_name))){
        //         continue;
        //     }
        //     strcpy((char*)str[depth], file->d_name);
        //     strcat((char*)str[depth], "/");
        //     bool result =  find_software(temp, depth+1, str);
        //     if(result){
        //         return result;
        //     }
        // }
    
    printf("%s,%d\n",__func__,__LINE__);
    return false;
}



static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata){
    if(event == HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS){
        progress = param;
        control_msg_t msg = {0};
        msg.msg_type = MSG_TYPE_UPG_BURN_PROGRESS;
        api_control_send_msg(&msg);

    }else if(event == HCFOTA_REPORT_EVENT_DOWNLOAD_PROGRESS){
        progress = param;
        control_msg_t msg = {0};
        msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
        api_control_send_msg(&msg);
    }
    return 0;
}



#define REBOOT_LATER 4

static void update_failed_timer_handle(lv_timer_t *t){
    if(prompt_label && lv_obj_is_valid(prompt_label->parent)){
        lv_obj_del(prompt_label->parent);
        turn_to_setup_root();
    }
}

static void update_control(void* arg1, void* arg2){
    (void)arg2;
     control_msg_t *ctl_msg = (control_msg_t*)arg1;
     switch (ctl_msg->msg_type)
     {
     case MSG_TYPE_UPG_STATUS:
        switch (ctl_msg->msg_code)
        {
        case UPG_STATUS_BURN_FAIL:
            lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_ERR));
            break;
        case UPG_STATUS_VERSION_IS_OLD:
            lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_VERSION_ERR));
            break;
        case UPG_STATUS_USB_READ_ERR:
            lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_LOAD_ERR));
            break;
        case UPG_STATUS_FILE_CRC_ERROR:
            lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_ERR));
            break;
        case UPG_STATUS_FILE_UNZIP_ERROR:                
            lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_DECO_ERR));
            break;
        case UPG_STATUS_BURN_OK:
            lv_label_set_text_fmt(prompt_label,"%s%d%s", api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG1), REBOOT_LATER,api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG2));
        default:
            break;
        }

        lv_timer_t *timer = lv_timer_create(update_failed_timer_handle, 3000, NULL);
        lv_timer_set_repeat_count(timer, 1);
        lv_timer_reset(timer);
        break;
    case MSG_TYPE_UPG_DOWNLOAD_PROGRESS:


       // break;
    case  MSG_TYPE_UPG_BURN_PROGRESS:
        if(ctl_msg->msg_type ==  MSG_TYPE_UPG_DOWNLOAD_PROGRESS){
            lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_DOWNLOADING));
        }else{
            lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADEING));            
        }

        static unsigned long prev_progress = 0;
        if(prev_progress != progress){
            lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);
            char ss[5];
            memset(ss, 0, 4);
            sprintf(ss, "%ld%%", progress);
            lv_label_set_text(lv_obj_get_child(progress_bar, 0), ss);  
            prev_progress = progress;     
        }
        break;
     
     default:
        break;
     }
}

static void reboot_timer_handle(lv_timer_t *t){
    static int count = REBOOT_LATER-1;
    if(count--){
        lv_label_set_text_fmt(prompt_label,"%s%d%s", api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG1), count,api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG2));
    }else{
        hcfota_reboot(hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE, 0));
    }
}

static void software_update_handler(void *target_){//
    lv_obj_t *obj = (lv_obj_t*)target_;
   
    set_remote_control_disable(true);
    int rc =hcfota_url(updata_url, hcfota_report, 0);
    control_msg_t msg = {0};
    msg.msg_type = MSG_TYPE_UPG_STATUS;     
    if(rc==0){
        msg.msg_code = UPG_STATUS_BURN_OK;
        lv_timer_t* reboot_timer = lv_timer_create(reboot_timer_handle, 1000, NULL);
        lv_timer_reset(reboot_timer);
        lv_timer_set_repeat_count(reboot_timer, REBOOT_LATER);
        
    }else{
        lv_mem_free(updata_url);
        set_remote_control_disable(false);
       
        switch (rc){
            case HCFOTA_ERR_LOADFOTA:
                printf("load file err!");
                msg.msg_code = UPG_STATUS_USB_READ_ERR;
                break;
            case HCFOTA_ERR_HEADER_CRC:
                printf("header crc err!");
                msg.msg_code = UPG_STATUS_FILE_CRC_ERROR;
                break;
            case HCFOTA_ERR_VERSION:
                printf("version err!");
                msg.msg_code = UPG_STATUS_VERSION_IS_OLD;
                break;
            case HCFOTA_ERR_DECOMPRESSS:
                printf("decompress err!");
                msg.msg_code = UPG_STATUS_FILE_UNZIP_ERROR;
                break;
            case HCFOTA_ERR_UPGRADE:
                printf("upgrade err");
                msg.msg_code = UPG_STATUS_BURN_FAIL;
            default:
                break;
        }
        #ifdef USB_AUTO_UPGRADE
            m_usb_upgrade = 0;
        #endif
        api_control_send_msg(&msg);
        if(timer_setting_clone){
            lv_timer_resume(timer_setting_clone);
            lv_timer_reset(timer_setting_clone);
            timer_setting = timer_setting_clone;
            timer_setting_clone = NULL;
        }
        //lv_obj_del(obj);

    } 
    if(timer_update){
        lv_timer_pause(timer_update);
        lv_timer_del(timer_update);
        timer_update = NULL;
    }
#ifdef __HCRTOS__
    vTaskDelete( NULL );
#endif    
}


static void software_update_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    //lv_obj_t *btn = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            lv_obj_del(target);
            turn_to_setup_root();

           
        }else if(key == LV_KEY_HOME){
            lv_obj_del(target);
            turn_to_setup_root();
        }
    }
}




void software_update_widget(lv_obj_t* btn){

    lv_obj_t *obj = lv_obj_create(setup_slave_root);
    slave_scr_obj = obj;
    lv_obj_set_style_radius(obj, 20, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_group_add_obj(setup_g, obj);
    lv_group_focus_obj(obj);
    lv_obj_add_event_cb(obj, software_update_event_handle, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_size(obj,LV_PCT(25),LV_PCT(25));
    lv_obj_center(obj);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *label;
    DIR *d = opendir(MOUNT_ROOT_DIR"/");
    int usb_status = mmp_get_usb_stat();
   if(usb_status == USB_STAT_UNMOUNT){
       label = lv_label_create(obj);
       lv_obj_center(label);
       lv_label_set_recolor(label, true);
       lv_label_set_text(label, api_rsc_string_get(STR_UPGRADE_NO_DEV));
       lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
       return;
   }
    char urls[160]={0};
    
    
    
   if(!find_software(d, urls)){
        label = lv_label_create(obj);
        lv_obj_center(label);
        lv_label_set_recolor(label, true);
        lv_label_set_text(label, api_rsc_string_get(STR_UPGRADE_NO_SOFTWATE));
       lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
       return;
   }
    lv_obj_del(obj);
    software_update_widget_(urls);
}

static void software_update_widget_(char* urls){
    updata_url = lv_mem_alloc(strlen(urls)+1);
    memcpy(updata_url, urls, strlen(urls)+1);
    // memset(updata_url, 0, 150);
    // strcat(updata_url, MOUNT_ROOT_DIR"/");
    // for(int i=0; i<3; i++){
    //     printf("%s\n", urls[i]);
    //     strcat(updata_url, urls[i]);
    //     if(strcmp(updata_url+strlen(updata_url)-4, ".bin") == 0){
    //         break;
    //     }
    // }
    printf("%s\n", updata_url);
    lv_obj_t *obj1 = NULL;
    if(lv_scr_act() == setup_scr){
        obj1 = lv_obj_create(setup_slave_root);
    }else{
        obj1 = lv_obj_create(lv_layer_top());
    }
    
    lv_obj_set_style_text_color(obj1, lv_color_white(), 0);
    lv_obj_set_style_outline_width(obj1, 0, 0);
    lv_obj_set_style_border_width(obj1, 0, 0);

    //lv_group_add_obj(setup_g, obj1);
    //lv_group_focus_obj(obj1);
    lv_obj_set_style_bg_color(obj1, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_scrollbar_mode(obj1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj1, 20, 0);
    lv_obj_set_size(obj1,LV_PCT(UPGRADE_WIDGET_WIDTH_PCT),LV_PCT(UPGRADE_WIDGET_HEIGHT_PCT));
    lv_obj_center(obj1);

    lv_obj_t *bar = lv_bar_create(obj1);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_size(bar, LV_PCT(100), LV_PCT(20));
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_center(bar);
    progress_bar = bar;

    lv_obj_t* label = lv_label_create(bar);
    lv_label_set_text(label, "0%");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
   
    prompt_label = lv_label_create(obj1);
    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_DOWNLOADING));
    lv_obj_set_width(prompt_label, LV_PCT(100));
    lv_obj_set_style_text_font(prompt_label, osd_font_get(FONT_MID), 0);
    lv_obj_align_to(prompt_label, bar, LV_ALIGN_OUT_TOP_MID, 0, -3);
    lv_obj_set_style_text_align(prompt_label, LV_TEXT_ALIGN_CENTER, 0);
    

    label = lv_label_create(obj1);
    lv_label_set_text(label, api_rsc_string_get(STR_UPGRADE_NOT_POWER_OFF));
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
    lv_obj_align_to(label, bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    if(timer_setting){
        lv_timer_pause(timer_setting);
        timer_setting_clone = timer_setting;
        timer_setting = NULL;
    }
    //closedir(d);

#ifdef __HCRTOS__    
    xTaskCreate(software_update_handler, "software update", 0x1000, obj1, portPRI_TASK_NORMAL , NULL);
#else
    pthread_t thread_id = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    pthread_create(&thread_id, &attr, software_update_handler, obj1);
    pthread_attr_destroy(&attr);
#endif    

    screen_entry_t update_entry;
    update_entry.screen = lv_scr_act();
    update_entry.control = update_control;
    api_screen_regist_ctrl_handle(&update_entry);
    //((void*)sub_obj);
}
#ifdef USB_AUTO_UPGRADE
static void *sys_upg_usb_task(void *arg){
    uint32_t timeout = (uint32_t)arg;
    int check_count = timeout/100;
    DIR *d = opendir(MOUNT_ROOT_DIR"/");
    char urls[160]={0};
    while(check_count --){
        if(find_software(d, urls)){
            software_update_widget_(urls);
            break;
        }
    }
}

int sys_upg_usb_check(uint32_t timeout)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    if (m_usb_upgrade){
        printf("%s(), line: %d. usb uprade task is running!\n", __func__, __LINE__);
        return API_SUCCESS;
    }
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, sys_upg_usb_task, (void*)timeout)) {
        return API_FAILURE;
    }
    m_usb_upgrade = 1;
    return API_SUCCESS;
}
#endif