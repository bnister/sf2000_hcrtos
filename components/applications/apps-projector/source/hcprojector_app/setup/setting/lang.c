#include <ctype.h>
#include "screen.h"
#include "factory_setting.h"
#include "setup.h"
#include "osd_com.h"
#include "mul_lang_text.h"
#include "../../app_config.h"
#include "com_api.h"

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define COLOR_TEMP_TEXT "Color Temp"
    #define SOFTWARE_UPDATE_TEXT "Update"
    #define STANDARD_TEXT "Std."
    #define DYNAMIC_TEXT "Dyn."
    #define LANG_WIDGET_W 50
    #define LANG_WIDGET_H 50
    #define LANG_FONT &lv_font_montserrat_14
#else
    #define COLOR_TEMP_TEXT "Color Temperature"
    #define SOFTWARE_UPDATE_TEXT "Software Update"
    #define STANDARD_TEXT "Standard"
    #define DYNAMIC_TEXT "Dynamic"
    #define LANG_WIDGET_W 33
    #define LANG_WIDGET_H 33
    #define LANG_FONT &lv_font_montserrat_26
#endif


#if 0
char* mul_langs_text[][LANGUAGE_MAX] = {
    {"Image Mode", "图像模式","Mode Image"},
    {"Contrast", "对比度", "contraste"},
    {"Hue", "色调","ton"},
    {"Brightness", "亮度","luminosité"},
    {"Color", "颜色","Color"},
    {"Sharpness", "清晰度","clarté"},
    {COLOR_TEMP_TEXT, "色温","Temp de couleur "},
    {"Sound Mode", "声音模式","Sound Mode"},
    {"Treble", "高音","Tweeter"},
    {"Bass", "低音","Bass"},
    {"Balance", "平衡","Balance"},
    {"BT Model", "蓝牙模式","Bluetooth mode"},
    {"Search Bluetooth", "搜索蓝牙","Rechercher Bluetooth"},
    {"BT Device", "蓝牙设备","Appareil Bluetooth"},
    {"My device", "我的设备","Mon appareil"},
    {"Other device", "其它设备","Autre équipement"},
    {"No Device", "无设备","No Device"},
    {"Connected", "已连接","Connecté"},
    {"Disconnected", "未连接","Non connecté"},
    {"Saved", "已保存","Enregistré"},
    {"Disconnect", "断开","Disconnect"},
    {"Connect", "连接","Connect"},
    {"Connecting", "连接中","connecté"},
    {"Searching", "搜索中","Rechercher"},
    {"Searching.", "搜索中.","Rechercher."},
    {"Searching..", "搜索中..","Rechercher.."},
    {"Searching...", "搜索中...","Rechercher..."},
    {"No wifi device", "无网络设备","pas de \n périphérique réseau"},
    {"password cannot be null", "密码不能为空","mot de passe ne peut pas être vide"},
    {"Connection success!", "连接成功！","Connexion réussie!"},
    {"Connection failed!", "连接失败！","échec de la connexion!"},
    {"Re-enter the password?", " 重输入密码？","Retaper le mot de passe?"},
    {"Yes", "是","Yes"},
    {"No", "否","NO"},
    {"", "",""},
    {"Enter the password for 8 to 32 bits", "请输入8到32位密码","Entrez le mot de passe de 8 à 32 chiffres"},
    {"SSID cannot be null", "SSID 不能为空","SSID ne peut pas être vide"},
    {"SSID should be less than 64 bits", "SSID应该小于64位","le SSID doit être inférieur à 64 bits"},
    {"Cancel", "取消","Annuler"},
    {"Delete", "忽略","Ignorer"},
    {"OSD Lang", "OSD语言","OSD Langue"},
    {"OSD Lang", "语言设置","OSD Langue"},
    {"Flip", "投屏模式","Feuilleter"},
    {"Aspect Ratio", "画面比例","Ratio d'image"},
    {"Factory reset", "恢复出厂设置","Retour à l'usine"},
    {SOFTWARE_UPDATE_TEXT, "软件升级","Escaladent"},
    {"Upgrading,please do not\npower Off", "正在升级，请勿断电","mise à niveau, ne pas couper le courant"},
    {"Download...", "下载中...","téléchargement en cours..."},
    {"Upgrate...", "升级中...","en cours de mise à niveau"},
    {"Auto Sleep", "自动休眠","Auto sommeil"},
    {"OSD Timer", "菜单显示时间","OSD Temps"},
    {"Please Insert USB!", "请插入USB！","s'il vous plaît insérer USB!"},
    {"Please Provide The \n Upgrade File!", "请提供升级文件！","s'il vous plaît fournir \n les fichiers de mise à jour!"},
    {"load file err!", "载入文件错误！","erreur de chargement du fichier!"},
    {"crc err!", "校验错误！","erreur de vérification!"},
    {"version err!", "版本错误！","erreur de version"},
    {"decompress err!", "解压缩错误！","erreur de décompression!"},
    {"upgrade err", "升级错误！","erreur de mise à jour!"},
    {"upgrade success, reboot ", "升级成功，","upgrade success"},
    {"s later","s后重启","Sredémarrer"},
    {"Keystone", "梯形校正","correction trapézoïdale"},
    {"Version Info", "版本信息","Version Info"},
    {"Picture", "图像","Picture"},
    {"Sound", "声音","Sound"},
    {"Option", "选项","Option"},
    {"Adjust", "调节","Adjust"},
    {"Enter", "确定","Enter"},
    {"Menu", "菜单","Menu"},
    {"Move", "移动","Move"},
    {"Off", "离开","off"},
    {"Movie", "电影","Movie"},
    {"Music","音乐","Music"},
    {"Photo", "照片","Photo"},
    {"Book", "电子书","Book"},
    {"Back", "返回","Back"},
    {"C", "C","C"},
    {"Play","播放","Jouer"},
    {"Pause", "暂停","Pause"},
    {"Stop", "停止","Stop"},
    {"FF", "快进","FF"},
    {"FB", "快退","FB"},
    {"Next", "下一曲","Next"},
    {"Prev", "上一曲","Préc"},
    {"Round", "列表循环","Round"},
    {"Round", "列表循环","Round"},
    {"Single", "单曲循环","Single"},
    {"Zoom+", "放大","Zoom+"},
    {"Zoom-", "缩小","Zoom-"},
    {"Left", "逆时针","Gauche"},
    {"Right", "顺时针","Droit"},
    {"Effect", "无特效","Effect"},
    {"Normal","普通","Normal"},
    {"Shutter", "百叶窗","Shutter"},
    {"Brush", "飞入","Brush"},
    {"Slide", "滑动","Slide"},
    {"Random", "随机","Aléatoire"},
    {"Fade", "消失","Disparaît"},
    {"Open File Fail", "打开文件失败","échec de l'ouverture du fichier"},
    {"Audio Track Unsupport", "音频轨道不支持","piste Audio non supportée"},
    {"Video Track Unsupport", "视频轨道不支持","piste vidéo non prise en charge"},
    {"Unsupport Video Type", "视频类型不支持","Video Type not supported "},
    {"Unsupport Audio Type", "音频类型不支持","Type Audio non supporté"},
    {"Audio Decode Error", "音频解码失败","échec du décodage Audio"},
    {"Video Decode Error", "视频解码失败"," échec de décodage Vidéo "},
    {"Picture Decode Error","图片解码失败","échec de décodage d'image"},
    #if PROJECTER_C2_VERSION
        {"HDMI1", "高清1","HDMI1"},
        {"HDMI2", "高清2","HDMI2"},
    #else
       {"HDMI", "高清","HDMI"},
    #endif
    {"Media", "多媒体","Média"},
    {"AV", "视频","AV"},
    {"Home", "主页","Home"},
    {"Setting", "设置","Setting"},
    {"Help", "帮助","Help"},
    {"Source", "信号源","Source"},
    {"Wifi", "网络","Réseau"},
    {"Wired \nscreen", "有线同屏","Wired \nscreen"},
    {"Wireless \nscreen", "无线同屏","Wireless \nscreen"},
    {"Off", "关","Off"},
    {"On", "开","On"},
    {STANDARD_TEXT, "标准","Std."},
    {DYNAMIC_TEXT, "动态","Dyn."},
    {"Mild", "温和","Mild"},
    {"User", "用户","User"},
    {"Cold", "冷色温","Froide"},
    {"Warm", "暖色温","Chaude"},
    {"Music", "音乐","Music"},
    {"Movie", "电影","Movie"},
    {"Sports", "运动","Sports"},
    {"English", "英语","Français"},
    {"Chinese", "中文","Chinois"},
    {"Front Table", "桌上正投","Table"},
    {"Front Ceiling", "吊装正投","Levage"},
    {"Back Table", "桌上背投","Back Table"},
    {"Back Ceiling", "吊装背投","Le levage"},
    {"Ok", "确定","Ok"},
    {"Ok?", "确定?","Ok?"},
    {"Close", "关闭","Close"},
    {"5s", "5秒","5"},
    {"10s", "10秒","10s"},
    {"15s", "15秒","15s"},
    {"20s", "20秒","20s"},
    {"25s", "25秒","25s"},
    {"30s", "30秒","30s"},
    {"60m", "60分钟","60s"},
    {"120m", "120分钟","120m"},
    {"180m", "180分钟","180m"},
    {"Source", "信号源","Source"},
    {"Volume", "声音","Son"},
    {"List","播放列表","List"},
    {"INFO","信息","INFO"},
    {"SF","慢放","SF"},
    {"STEP","步进","STEP"},
    {"Resolution:","分辨率:","Résolution :"},
    {"Frame:","帧数:","Cadres:"},
    {"Audio:","音轨:","Pistes :"},
    {"Subtitle:","字幕:","Sous-titres"},
    {"Size:","大小:","Size:"},
    {"Album:","专辑:","Album:"},
    {"Artist:","艺术家:","Artist:"},
    {"Bitrate:","码率:","Débit:"},
    {"Exit","退出","Sortie"},
    {"Close","关闭屏幕","Close"},
    {"Mute","静音","Mute"},
    {"OFF","关闭音乐","OFF"},
    {"ON","打开音乐","ON"},
    {"Source","音乐资源","Source"},
    {"Date:","日期:","Date:"},
    {"Time:","时间:","Time:"},
    {"Ratio","画面比例","Ratio"},
    {"Auto", "自动","Auto"},
    {"4:3", "4:3","4:3"},
    {"16:9", "16:9","16:9"},
    {"Mount failed", "挂载失败","Mount failed"},

    {"Network Error!", "网络错误！","Erreur Réseau!"},
    {"Connect WiFi fail!", "连接WiFi出错！","Erreur de connexion WiFi!"},
    {"Please connect WiFi first!", "请先连接WiFi！","Veuillez d'abord vous connecter au WiFi!"},
    {"Video data decoding error!", "视频数据解码错！","Les données vidéo sont mal décodées!"},
    {"audio data decoding error!", "音频数据解码错！","Les données audio sont mal décodées!"},
    {"Video format not support!", "视频格式不支持！","Le format vidéo n'est pas pris en charge!"},
    {"Audio format not support!", "音频格式不支持！","Le format audio n'est pas pris en charge!"},
    {"Wifi Disconnect!", "WiFi断开连接！","WiFi déconnecté!"},
    {"Media(container) not support!", "媒体容器不支持！","Le conteneur média n'est pas pris en charge!"},
    {"Saved WI-FI","已保存WI-FI","WI-FI enregistré"}, 
    {"Nearby WI-FI", "附近的WI-FI","Wi-Fi proche"}, 
    {"Search", "搜索","Search"}, 
    {"Add Nets", "添加网络","Ajouter"},
    {"Home", "主页","Home"},
    {"Signal Strength", "信号强度","Signal Force"}, 
    {"Security", "安全","Sécurité "}, 
    {"Password", "密码","Password"}, 
    {"Show password", "显示密码","Show password"},
    {"Net Mode", "网络模式","Mode réseau"}, 
    {"IP Address", "IP地址","IP Address"}, 
    {"Net Mask", "掩码","Masque"}, 
    {"Gate Way", "网关","Gate Way"}, 
    {"DNS", "DNS","DNS"}, 
    {"MAC", "MAC","MAC"}, 
    {"Static IP", "静态IP","IP statique"},
    {"Connect timeout or\npassword error", "连接超时或密码错误","délai de connexion expiré \nou mot de passe incorrect "},
    {"First scan QR code\nSet AP network", "1.请扫码连接本设备","Veuillez balayer le code \npour connecter cet appareil"},
    {"Second scan QR code\nSet WiFi network", "2.请扫码联网","Veuillez balayer \nle code réseau"},
    {"Scan QR code\nSet device parameters", "请扫码配置参数","s'il vous plaît balayez les \nparamètres de configuration du code"},
    {"WiFi is scanning ...", "WiFi正在扫描 ...","le WiFi scanne..."},
    {"WiFi is connecting ...", "WiFi连接中 ...","Connexion WiFi en cours..."},
    {"Miracast is connecting ...", "安卓同屏连接中 ...","Connexion android dans le même écran..."},
    {"Miracast connect OK.", "安卓同屏连接成功","Android connecté avec succès"},
    {"Aircast is playing music", "苹果同屏播放音乐"," Apple joue de la musique sur le même écran "},
    {"Please plug in android or apple device!", "请插入苹果或者安卓设备!","s'il vous plaît insérer un appareil apple ou android!"},
    {"Please pull out and plug in android or apple device!", "请拔出，再插入苹果或者安卓设备!","s'il vous plaît débrancher et brancher à nouveau un appareil apple ou android!"},
    {"WiFi not turned on!", "WiFi未打开!","le WiFi n'est pas activé!"},
    {"Reset", "重置","Reset"},
    {"Forward", "前移","Avant "},
    {"Backward", "后移","En arrière"},
    {"Focus", "对焦","Focus"},
    {"Up","上一页","Up"},
    {"Down","下一页","Down"},
    {"Prev","上一部","Prev"},
    {"Next","下一部","Next"},
    {"Unsupport Subtitle Type","字幕类型不支持","Type de sous-titre non supporté"},
    {"Move","移动","Move"},
    {"No Device","无设备","No Device"},
    {"Mirror rotate on","镜像旋转开","Miroir tournant ouvert"},
    {"Mirror rotate off","镜像旋转关","Mirror rotation off"},
    {"French","法语","French"}

};
#endif

language_type_t selected_language = LANGUAGE_ENGLISH;
static language_type_t pre_selected_language = LANGUAGE_ENGLISH;


extern char* osd_language_k;
extern lv_timer_t *timer_setting;
extern const char* lang_title;

void osd_btnmatrix_event(lv_event_t* e);
void osd_language_widget(lv_obj_t* btn);
void set_change_lang(btnmatrix_p *p, int index, int v);
static void _change_language(lv_obj_t* obj, language_type_t id,  lv_font_t *font);
static void delete_from_list_event(lv_event_t* e);
extern void label_set_text_color(lv_obj_t* label,const char* text, char* color);


static bool is_digit(const char* str){
    if(strlen(str)>3){
        return false;
    }
    uint16_t ch = ' ';
    uint16_t i=0;
    while ((ch = str[i++]) != '\0'){
        if (!isdigit(ch) || ch == ':'){
            return false;
        }
    }
    return true;
}


const char* get_some_language_str(const char *str, int index){
    if (is_digit(str) ||  strcmp(str, " ") == 0 || strcmp(str, "") == 0 || strcmp(str, "\n")==0){
        return str;
    }
    uint8_t i=0;
    while (index){
        for(; str[i] != '\0';i++);
        index--;
        i++;
    }
    return str + i;
}

const char* get_some_language_str1(str_n_vec str_v, int index){
    if(index>=0 && index < LANGUAGE_MAX){
        //printf("id: %s\n", str_v[index]);
        return str_v[index];
    }
    return " %d";
}
    


// btnmatrix_p* language_choose_add_label(lv_obj_t* label,const char* p, uint8_t len){
//     if (label->class_p == & lv_btnmatrix_class){
//         lv_obj_add_event_cb(label, delete_from_list_event, LV_EVENT_DELETE, NULL);
//         btnmatrix_p *btnmatrixP = (btnmatrix_p*)lv_mem_alloc(sizeof(btnmatrix_p));
//         btnmatrixP->len = len;
//         btnmatrixP->str_p_p = (const char **)lv_mem_alloc(sizeof(char *)*len);
//         btnmatrixP->change_lang = (const uint8_t*)lv_mem_alloc(sizeof(uint8_t)*len);
//         btnmatrixP->str_p_s = (const char **)p;
//         for(int i=0; i<len; i++){
//             if(strcmp(btnmatrixP->str_p_s[i], "\n") != 0){
//                 btnmatrixP->change_lang[i] = 1;
//             }else{
//                 btnmatrixP->change_lang[i] = 0;
//             }
            
//         }
        
//         label->user_data = (void*)btnmatrixP;
//         return btnmatrixP;
//     }else{
//         label->user_data = p;
//     }
//     return NULL;
// }

// void language_choose_add_label1(lv_obj_t* obj, uint32_t label_id){
//     if(obj->user_data){
//         ((label_p*)obj->user_data)->lang_pp = mul_langs_text[label_id];
//         return;
//     }
//      lv_obj_add_event_cb(obj, delete_from_list_event, LV_EVENT_DELETE, NULL);
//     label_p *labelP = (label_p*)lv_mem_alloc(sizeof(label_p));
//     labelP->font_pp = NULL;
//     labelP->lang_pp = mul_langs_text[label_id];
//     obj->user_data = labelP;
//     //obj->user_data = mul_langs_text[label_id];
// }

// void language_choose_add_label_with_font(lv_obj_t* obj, uint32_t label_id, lv_font_t **font_pp){
//     if(obj->user_data){
//         ((label_p*)obj->user_data)->lang_pp = mul_langs_text[label_id];
//         ((label_p*)obj->user_data)->font_pp = font_pp;
//         return;
//     }
//     lv_obj_add_event_cb(obj, delete_from_list_event, LV_EVENT_DELETE, NULL);
//     label_p *labelP = (label_p*)lv_mem_alloc(sizeof(label_p));
//     labelP->font_pp = font_pp;
//     labelP->lang_pp = api_rsc_string_get(label_id);
//     obj->user_data = labelP;
// }

// btnmatrix_p1* language_choose_add_btns(lv_obj_t* label, int *p, uint8_t len){
//     lv_obj_add_event_cb(label, delete_from_list_event, LV_EVENT_DELETE, NULL);
//     btnmatrix_p1 *btnmatrixP = (btnmatrix_p1*)lv_mem_alloc(sizeof(btnmatrix_p1));
//     btnmatrixP->len = len;
//     btnmatrixP->str_p_p = (const char **)lv_mem_alloc(sizeof(char *)*len);
//     //btnmatrixP->change_lang = (const uint8_t*)lv_mem_alloc(sizeof(uint8_t)*len);
//     btnmatrixP->str_id_vec = p;
//     for(int i=0; i<len; i++){
//         if(btnmatrixP->str_id_vec[i] >= 0){
//             //btnmatrixP->change_lang[i] = 1;
//         }else if(btnmatrixP->str_id_vec[i] == LINE_BREAK_STR){
//             //btnmatrixP->change_lang[i] = 0;
//             btnmatrixP->str_p_p[i] = "\n";
//         }else if(btnmatrixP->str_id_vec[i] == BLANK_SPACE_STR){
//             //btnmatrixP->change_lang[i] = 0;
//              btnmatrixP->str_p_p[i] = " ";
//         }else if(btnmatrixP->str_id_vec[i] == BTNS_VEC_END){
//             //btnmatrixP->change_lang[i] = 0;
//             btnmatrixP->str_p_p[i] = "";
//         }
        
//     }
    
//     label->user_data = (void*)btnmatrixP;
//     return btnmatrixP;
// }

// void set_change_lang(btnmatrix_p *p, int index, int v){
//     p->change_lang[index] = v;
// }

// void language_choose_add_label1(lv_obj_t *label, uint32_t i){
//     label->user_data = (void*)i;
// }

void delete_from_list_event(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (code == LV_EVENT_DELETE){
        if(target->class_p == &lv_btnmatrix_class){
            if(target->user_data){
                lv_mem_free(((btnmatrix_p1*)target->user_data)->str_p_p);
                //lv_mem_free(((btnmatrix_p1*)target->user_data)->str_p_s);
                //lv_mem_free(((btnmatrix_p*)target->user_data)->change_lang);
                lv_mem_free(target->user_data);    
                target->user_data = NULL;             
            }
           
        }else if(target->class_p == &lv_label_class){
            if(target->user_data){
                //lv_mem_free(target->user_data); 
                target->user_data = NULL;
            }
        }
    }
}

// void set_label_text(lv_obj_t * label,int  id, char* color){
//    set_label_text_with_font(label, id, color, select_font_normal[id]);
// }

// void set_label_text1(lv_obj_t * label,int  id, char* color){
//     set_label_text_with_font1(label,id, color,select_font_normal[id]);
// }

void set_label_text_with_font1(lv_obj_t * label,int  id,char* color,  lv_font_t* font){
    char* text = lv_label_get_text(label);
    label_p* labelP = (label_p*)(label->user_data);
    if(label->user_data){
        char *str = get_some_language_str1(labelP->lang_pp, id);
         if (color != NULL){
            label_set_text_color(label, str,color);
        }else{
            char color_pre[9];
            strncpy(color_pre, text, 8);
            color_pre[8]='\0';
            if(color_pre[0] == '#'){
                label_set_text_color(label, str, color_pre);
            }else{
               lv_label_set_text(label, str);
            }
        }
      
    }
    if(label->user_data && labelP->font_pp){
        lv_obj_set_style_text_font(label, labelP->font_pp[id], 0);
    }else{
        lv_obj_set_style_text_font(label, font, 0); 
    }  
    //lv_obj_set_style_text_font(label, font, 0);
}


static void btns_set_text_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    int i = (int)lv_event_get_user_data(e);
    lv_obj_t* obj = lv_event_get_target(e);
  
    
    btnmatrix_p1 *p = ((btnmatrix_p1*)obj->user_data);
    for(int j=0; j<p->len; j++){
        if(p->str_id_vec[j] != LINE_BREAK_STR && p->str_id_vec[j] != BLANK_SPACE_STR && p->str_id_vec[j] != BTNS_VEC_END){
            p->str_p_p[j] = api_rsc_string_get(p->str_id_vec[j]);
        }
    }
    lv_btnmatrix_set_map(obj, p->str_p_p);
    lv_obj_set_style_text_font(obj, osd_font_get(i), 0);
}

void set_btns_lang2(lv_obj_t* btns, int len, int font_id, int *p){
    lv_obj_add_event_cb(btns, delete_from_list_event, LV_EVENT_DELETE, NULL);
    btnmatrix_p1 *btnmatrixP = (btnmatrix_p1*)lv_mem_alloc(sizeof(btnmatrix_p1));
    btnmatrixP->len = len;
    btnmatrixP->str_p_p = (const char **)lv_mem_alloc(sizeof(char *)*len);
    btnmatrixP->str_id_vec = p;
    for(int i=0; i<len; i++){
        if(btnmatrixP->str_id_vec[i] >= 0){
        }else if(btnmatrixP->str_id_vec[i] == LINE_BREAK_STR){
            btnmatrixP->str_p_p[i] = "\n";
        }else if(btnmatrixP->str_id_vec[i] == BLANK_SPACE_STR){
             btnmatrixP->str_p_p[i] = " ";
        }else if(btnmatrixP->str_id_vec[i] == BTNS_VEC_END){
            btnmatrixP->str_p_p[i] = "";
        }
        
    }
    
    btns->user_data = (void*)btnmatrixP;

    lv_obj_add_event_cb(btns, btns_set_text_event_handle, LV_EVENT_REFRESH, (void*)font_id);
    lv_event_send(btns, LV_EVENT_REFRESH, NULL);
}

// void set_btns_lang(lv_obj_t *obj, int id){
//     set_btns_lang_with_font(obj, id,select_font_normal[id]);
// }


static void _change_language(lv_obj_t* obj, language_type_t id, lv_font_t *font){
    for(int i=0; i< lv_obj_get_child_cnt(obj); i++){
        lv_obj_t *temp = lv_obj_get_child(obj, i);
        if (temp->class_p == &lv_label_class && temp->user_data){
          set_label_text_with_font1(temp,id, NULL, font);
            //set_label_text(temp, id, NULL);
        } else if(temp->class_p == &lv_btnmatrix_class && temp->user_data){
            //set_btnmatrix_language(temp, id);
            //set_btns_lang(temp, id);
        }else if(temp->class_p == &lv_list_text_class && temp->user_data){
           set_label_text_with_font1(temp,id, NULL, font);
        }
        else{
            _change_language(temp, id, font);
        }
    }
}

static void _change_language1(lv_obj_t* obj){
        for(int i=0; i< lv_obj_get_child_cnt(obj); i++){
        lv_obj_t *temp = lv_obj_get_child(obj, i);//
        if (temp->class_p == &lv_label_class ||  temp->class_p == &lv_btnmatrix_class ||temp->class_p == &lv_list_text_class){
            lv_event_send(temp, LV_EVENT_REFRESH, NULL);
            if(lv_obj_get_child_cnt(temp)>0){
                _change_language1(temp);
            }
        }else{
            _change_language1(temp);
        }
    }
}

void change_language(){
    lv_disp_t *disp = lv_disp_get_default();
    _change_language1(setup_scr);
#ifdef WIFI_SUPPORT      
    _change_language1(wifi_scr);
#endif    
    _change_language1(main_page_scr);
    _change_language1(channel_scr);
    // _change_language(ui_mainpage, id,&select_font_media[id]);
    // _change_language(ui_subpage, id,&select_font_media[id]);
    // _change_language(ui_fspage, id,&select_font_mplist[id]);
    // _change_language(ui_ctrl_bar, id,&select_font_mplist[id]);
}
static const char* osd_lang_vec[LANGUAGE_MAX+1] = {0};

void osd_language_widget(lv_obj_t* btn){
    lv_obj_t *osd_lang_wid = create_new_widget(LANG_WIDGET_W,LANG_WIDGET_H);
    lv_obj_set_style_opa(osd_lang_wid, LV_OPA_100, 0);
    
    create_widget_head(osd_lang_wid, STR_OSD_LANG_TITLE, 33);

    lv_obj_t *osd_body = lv_obj_create(osd_lang_wid);
    set_pad_and_border_and_outline(osd_body);
    lv_obj_set_style_pad_ver(osd_body, 0, 0);
    lv_obj_set_size(osd_body,LV_PCT(100),LV_PCT(44));
    lv_obj_set_style_radius(osd_body, 0, 0);
    lv_obj_set_scrollbar_mode(osd_body, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *osd_body_sub = lv_obj_create(osd_body);
    set_pad_and_border_and_outline(osd_body_sub);
    lv_obj_set_style_pad_ver(osd_body_sub, 0, 0);
    int len_pct = LANGUAGE_MAX <= 3 ? 100 : LANGUAGE_MAX*100/3;
    lv_obj_set_size(osd_body_sub,LV_PCT(len_pct),LV_PCT(100));
    lv_obj_set_style_radius(osd_body_sub, 0, 0);
    lv_obj_set_style_bg_color(osd_body_sub, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_scrollbar_mode(osd_body_sub, LV_SCROLLBAR_MODE_OFF);

    
    for(int i = 0; i<LANGUAGE_MAX; i++){
        osd_lang_vec[i] = api_rsc_string_get_by_langid(i, STR_LANG);
    }
    osd_lang_vec[LANGUAGE_MAX]="";

    lv_obj_t *body_btnmatrix = create_widget_btnmatrix1(osd_body_sub, 100, 100, osd_lang_vec);
    //lv_obj_t *body_btnmatrix = create_widget_btnmatrix(osd_body_sub, 300, 100, osd_language_v, LANGUAGE_MAX+1);
    lv_obj_add_event_cb(body_btnmatrix, osd_btnmatrix_event, LV_EVENT_ALL, btn);
    lv_obj_set_style_text_font(body_btnmatrix, LANG_FONT, 0);
    lv_obj_t *btn1 = lv_btn_create(osd_body);
    lv_obj_set_style_shadow_width(btn1, 0, 0);
    lv_obj_set_style_radius(btn1, 0 , 0);
    lv_obj_set_size(btn1,LV_PCT(8) ,LV_PCT(100));
    lv_obj_align(btn1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(btn1, LV_OPA_20, 0);

    lv_obj_t *label1 = lv_label_create(btn1);
    lv_obj_set_style_bg_opa(label1, LV_OPA_0, 0);
    lv_label_set_text(label1, "<");
    lv_obj_center(label1);

    btn1 = lv_btn_create(osd_body);
    lv_obj_set_style_shadow_width(btn1, 0, 0);
    lv_obj_set_style_radius(btn1, 0 , 0);
    lv_obj_set_size(btn1,LV_PCT(8) ,LV_PCT(100));
    lv_obj_align(btn1, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(btn1, LV_OPA_20, 0);

    label1 = lv_label_create(btn1);
    lv_obj_set_style_bg_opa(label1, LV_OPA_0, 0);
    lv_label_set_text(label1, ">");
    lv_obj_center(label1);

    create_widget_foot(osd_lang_wid, 24, btn);

    lv_group_focus_obj(body_btnmatrix);
}

static int visible_left=0;

void osd_btnmatrix_event(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *btn = lv_event_get_user_data(e);
   static int index = 0;
   static int prev_lang_id = 0;
    if(code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        uint16_t key = lv_indev_get_key(lv_indev_get_act()); 
        if(key == LV_KEY_RIGHT || key == LV_KEY_LEFT){
            if (key == LV_KEY_RIGHT){
                if(LANGUAGE_MAX<3){
                    index = (index+1)%2;
                }else{
                    index++;
                    if (index >= LANGUAGE_MAX){
                        index = 0;
                        visible_left=0;
                        lv_obj_scroll_by(target->parent, (lv_coord_t )(lv_obj_get_width(target->parent->parent)/3*(LANGUAGE_MAX-3)), 0, LV_ANIM_OFF);
                    }else if (index > visible_left+2){
                        lv_obj_scroll_by(target->parent, (lv_coord_t )(-lv_obj_get_width(target->parent->parent)/3), 0, LV_ANIM_OFF);
                        visible_left++;
                    }
                }
            }
            else {
                if(LANGUAGE_MAX<3){//语言选择界面可见最多三个选项
                    index = (index+3)%2;
                }else{
                    index--;
                    if (index < 0){
                        index =LANGUAGE_MAX-1;
                        visible_left = index-2;
                        lv_obj_scroll_by(target->parent, (lv_coord_t )(-lv_obj_get_width(target->parent->parent)/3*(LANGUAGE_MAX-3)), 0, LV_ANIM_OFF);
                    }else if(index < visible_left){
                        lv_obj_scroll_by(target->parent, (lv_coord_t )(lv_obj_get_width(target->parent->parent)/3), 0, LV_ANIM_OFF);
                        visible_left = index;
                    }
                }
            }
            lv_btnmatrix_set_btn_ctrl(target, index, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_selected_btn(target, index);
            pre_selected_language = index;
            if(index<LANGUAGE_MAX){
                projector_set_some_sys_param(P_OSD_LANGUAGE, index);
                lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(target->parent->parent->parent, 0), 0);
                set_label_text2(label, GET_LABEL_ID((int)label->user_data), GET_FONT_ID((int)label->user_data)); 

                label = lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(target->parent->parent->parent, 2), 0), 0), 1);
                set_label_text2(label, GET_LABEL_ID((int)label->user_data),GET_FONT_ID((int)label->user_data));

                label = lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(target->parent->parent->parent, 2), 1), 0), 1);
                set_label_text2(label, GET_LABEL_ID((int)label->user_data), GET_FONT_ID((int)label->user_data));
            }
        }
        else if (key == LV_KEY_HOME || key == LV_KEY_ESC){
            if(prev_lang_id != index){
                projector_set_some_sys_param(P_OSD_LANGUAGE, prev_lang_id);
            }
            lv_obj_del(target->parent->parent->parent);
            turn_to_setup_root();
        } 
        else if (key == LV_KEY_ENTER){
            uint16_t id  = lv_btnmatrix_get_selected_btn(target);
// for lang test
#if 1
            lv_obj_del(target->parent->parent->parent);
            //if(id == LANGUAGE_ENGLISH || id == LANG_CHINESE)
#endif        
              
            {
               
                projector_set_some_sys_param(P_OSD_LANGUAGE, id);
                projector_sys_param_save();
                change_language();
                
                lv_label_set_text(lv_obj_get_child(btn, 1), osd_lang_vec[id]);
            }
            
            turn_to_setup_root();
        }
        //last_setting_key = key;
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }

    }else if(code == LV_EVENT_DRAW_PART_BEGIN) {
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        if (lv_btnmatrix_get_selected_btn(target) == dsc->id){
            dsc->rect_dsc->radius = 0;
            dsc->rect_dsc->outline_width = 0;
        }
        if(dsc->id == 0){//第一个为创建时默认的字体
            return;
        }
        /*switch (dsc->id)
        {
        case LANGUAGE_ENGLISH:
            dsc->label_dsc->font = osd_font_get_by_langid(LANGUAGE_ENGLISH, FONT_NORMAL);
            break;
        case LANG_CHINESE:
            dsc->label_dsc->font = osd_font_get_by_langid(LANG_CHINESE, FONT_NORMAL);
            break;
        case LANG_FRENCH:
            dsc->label_dsc->font = osd_font_get_by_langid(LANG_FRANCH, FONT_NORMAL);
            break;
        default:
            break;
        }*/
		dsc->label_dsc->font = osd_font_get_by_langid(dsc->id, FONT_NORMAL);
    }else if(code == LV_EVENT_DELETE){
        lv_obj_set_style_bg_opa(setup_slave_root, LV_OPA_0,0);
    }else if(code == LV_EVENT_FOCUSED){
        index = projector_get_some_sys_param(P_OSD_LANGUAGE);
        prev_lang_id = index;
        lv_btnmatrix_set_selected_btn(target, index);
        lv_btnmatrix_set_btn_ctrl(target, index, LV_BTNMATRIX_CTRL_CHECKED);
        visible_left = (index == 0 ? 0 : index == LANGUAGE_MAX-1 ? index-2 : index-1);
        if(visible_left>0)
            lv_obj_scroll_by(target->parent, (lv_coord_t )(-lv_obj_get_width(target->parent->parent)/3*visible_left), 0, LV_ANIM_OFF);
    }
}

const char *get_string_by_string_id(int str_id, int lang_idx)
{
    return api_rsc_string_get(str_id);
}
