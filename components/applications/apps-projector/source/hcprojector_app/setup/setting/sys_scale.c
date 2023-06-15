#include "app_config.h"
#include "osd_com.h"
#include "com_api.h"
#ifdef SYS_ZOOM_SUPPORT

#include "setup.h"
#include "../../channel/local_mp/mp_fspage.h"
#include "../../channel/local_mp/media_player.h"
#include "../../channel/local_mp/mp_zoom.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/fb.h>
// #ifdef __linux__
    
// #else
//      #include <kernel/fb.h>
// #endif

#include "factory_setting.h"



#define ZOOM_OUT_COUNT_MAX 16
#define SYS_SCALE_MIN_RATIO 3
#define DIS_ZOOM_FULL_W 1920
#define DIS_ZOOM_FULL_H 1080

// static hcfb_scale_t scale_param = { OSD_MAX_WIDTH, OSD_MAX_HEIGHT, 1920, 1080 };
// static hcfb_lefttop_pos_t start_pos ={0};
static dis_tv_mode_e zoom_mode = DIS_TV_16_9;//16:9或4:3
static dis_tv_mode_e dis_mode = DIS_TV_AUTO;//16:9或4:3或auto
static struct dis_screen_info screen = { 0 };
dis_screen_info_t lcd_area={0};
static int scale_max_hor = 0;
static int ui_adjust_w_v = 0;
static int ui_adjust_h_v = 0;
static int zoom_out_count = 0;
static int x_amendment = 0;
static int y_amendment = 0;
extern lv_timer_t *timer_setting;

typedef struct scale_param_{
    int x;
    int y;
    int w;
    int h;
} scale_param_t;

static scale_param_t scale_param;

typedef enum scale_type {
    SCALE_ZOOM_IN,
    SCALE_ZOOM_OUT,
    SCALE_ZOOM_RECOVERY,
    SCALE_4_3,
    SCALE_16_9
} scale_type_t;

void set_sys_scale();


int get_display_x(){
    if(dis_mode == DIS_TV_AUTO){
        return 0;
    }
    return scale_param.x*DIS_ZOOM_FULL_W/((double)lcd_area.area.w)+x_amendment;
}

int get_display_y(){
    if(dis_mode == DIS_TV_AUTO){
        return 0;
    }
    return scale_param.y*DIS_ZOOM_FULL_H/((double)lcd_area.area.h)+y_amendment;
}

int get_display_h(){
    if(dis_mode == DIS_TV_AUTO){
        return DIS_ZOOM_FULL_W;
    }
    return scale_param.w*DIS_ZOOM_FULL_W/((double)lcd_area.area.w);
}

int get_display_v(){
    if(dis_mode == DIS_TV_AUTO){
        return DIS_ZOOM_FULL_H;
    }
    return scale_param.h*DIS_ZOOM_FULL_H/((double)lcd_area.area.h);
}


int get_cur_osd_h(){
    return scale_param.w;
}

int get_cur_osd_v(){
    return scale_param.h;
}

static void mainlayer_zoom(int x, int y, int w, int h){
    int w1 = w*1920/((double)lcd_area.area.w);
    int h1 = h*1080/((double)lcd_area.area.h);
    int x1 = x*1920/((double)lcd_area.area.w)+x_amendment;
    int y1 = y*1080/((double)lcd_area.area.h)+y_amendment;
    api_set_display_zoom(0,0, 1920, 1080, x1, y1, w1, h1);
}

static void osd_zoom(int x, int y, int w, int h){
    int fd_fb = open(DEV_FB , O_RDWR);
	if (fd_fb < 0) {
		printf("%s(), line:%d. open device: %s error!\n", 
			__func__, __LINE__, DEV_FB);
		return;
	}
    hcfb_scale_t scale_param1 = { OSD_MAX_WIDTH, OSD_MAX_HEIGHT, w, h};
    int x1 = x*OSD_MAX_WIDTH/(double)scale_param.w;
    int y1 = y*OSD_MAX_HEIGHT/(double)scale_param.h;
    hcfb_lefttop_pos_t start_pos1 ={x1,y1};    
    if(dis_mode == DIS_TV_AUTO){
        start_pos1.left = 0;
        start_pos1.top = 0;
        scale_param1.h_mul = lcd_area.area.w;
        scale_param1.v_mul = lcd_area.area.h;        
    }
    ioctl(fd_fb, HCFBIOSET_SET_LEFTTOP_POS, &start_pos1);
    ioctl(fd_fb, HCFBIOSET_SCALE, &scale_param1); 

	close(fd_fb);
}

void set_display_zoom_when_sys_scale(){
    media_handle_t* hdl = (media_handle_t*)mp_get_cur_player_hdl();
    if(hdl && hdl->type == MEDIA_TYPE_MUSIC){
        mainlayer_zoom(MUSIC_COVER_ZOOM_X*get_display_h()/DIS_ZOOM_FULL_W+get_display_x(),
                        MUSIC_COVER_ZOOM_Y*get_display_v()/DIS_ZOOM_FULL_H+get_display_y(),
                        MUSIC_COVER_ZOOM_W*get_display_h()/DIS_ZOOM_FULL_W,
                        MUSIC_COVER_ZOOM_H*get_display_v()/DIS_ZOOM_FULL_H);
        return;
    }else{
    if(dis_mode == DIS_TV_AUTO){
        mainlayer_zoom(0, 0, lcd_area.area.w, lcd_area.area.h);
        return;
    }        
}

    mainlayer_zoom(scale_param.x, scale_param.y, scale_param.w, scale_param.h);
}

void save_sys_scale_param(){
    projector_set_some_sys_param(P_SYS_ZOOM_DIS_MODE, dis_mode);
    projector_set_some_sys_param(P_SYS_ZOOM_OUT_COUNT, zoom_out_count);
    printf("zoom_out_count: %d\n", zoom_out_count);  
    projector_sys_param_save();
}


static void get_scr_h_v_by_ratio(int *h, int *v, int ratio){
    if(ratio == DIS_TV_AUTO){
        return;
    }
    int ratio_h = 16;
    int ratio_v = 9;
    if(ratio == DIS_TV_4_3){
        ratio_h = 4;
        ratio_v = 3;
    }

    if((*h)*ratio_v > (*v)*ratio_h){
        *h = (*v)*ratio_h/ratio_v;
    }else if((*h)*ratio_v < (*v)*ratio_h){
        *v = (*h)*ratio_v/ratio_h;
    }
}

int sys_scala_init(){
    dis_mode = projector_get_some_sys_param(P_SYS_ZOOM_DIS_MODE);
    zoom_mode = dis_mode == DIS_TV_AUTO ? DIS_TV_16_9 : dis_mode;
    zoom_out_count = projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT); 
    printf("zoom_out_count: %d\n", zoom_out_count);  



    api_get_screen_info(&lcd_area);//ui宽和高
    printf("lcd.area.w:%d, lcd.area.h:%d\n",lcd_area.area.w, lcd_area.area.h);

    scale_param.w = lcd_area.area.w;
    scale_param.h = lcd_area.area.h;
    int a=scale_param.w, b=scale_param.h;
    get_scr_h_v_by_ratio(&a, &b, zoom_mode);
    scale_param.w = a;
    scale_param.h = b;
    printf("scale_param.h_mul:%d, scale_param.v_mul:%d\n", scale_param.w, scale_param.h);
    scale_max_hor = scale_param.w;
    ui_adjust_w_v = scale_param.w/60;
    ui_adjust_h_v = scale_param.h/60;  


    if(zoom_out_count>0 && zoom_out_count<=ZOOM_OUT_COUNT_MAX){
        scale_param.w -= zoom_out_count*ui_adjust_w_v;
        scale_param.h -= zoom_out_count*ui_adjust_h_v;
    }
    scale_param.x = (lcd_area.area.w-scale_param.w)/2.0;
    scale_param.y = (lcd_area.area.h-scale_param.h)/2.0;

    set_sys_scale();
}


void set_sys_scale(){
    osd_zoom(scale_param.x, scale_param.y, scale_param.w, scale_param.h);
}

int do_sys_scale(scale_type_t scale_type_v){

    switch (scale_type_v)
    {
    case SCALE_ZOOM_OUT:
        if(zoom_out_count<ZOOM_OUT_COUNT_MAX){
            scale_param.w -=ui_adjust_w_v;
            scale_param.h -= ui_adjust_h_v;   
            zoom_out_count++;
        } 
        if(dis_mode == DIS_TV_AUTO){
            dis_mode = DIS_TV_16_9;
        }       
        break;
    case SCALE_ZOOM_IN:
        if(zoom_out_count>0){
            scale_param.w +=ui_adjust_w_v;
            scale_param.h += ui_adjust_h_v;
            zoom_out_count--; 
        }
        if(dis_mode == DIS_TV_AUTO){
            dis_mode = DIS_TV_16_9;
        }
        break;
    case SCALE_ZOOM_RECOVERY:
        dis_mode = DIS_TV_AUTO;
        zoom_mode = DIS_TV_16_9;        

        scale_param.w = lcd_area.area.w;
        scale_param.h = lcd_area.area.h;
        int a=scale_param.w, b=scale_param.h;
        get_scr_h_v_by_ratio(&a, &b, zoom_mode);
        scale_param.w = a;
        scale_param.h = b;
        scale_max_hor = scale_param.w;
        ui_adjust_w_v = scale_param.w/60;
        ui_adjust_h_v = scale_param.h/60;

        zoom_out_count = 0;
        break; 
    case SCALE_4_3:
        if(dis_mode == DIS_TV_16_9 || dis_mode == DIS_TV_AUTO){
            if(zoom_mode == DIS_TV_16_9){
                scale_param.w = scale_param.w*3/4;
                scale_max_hor = scale_max_hor*3/4;
                ui_adjust_w_v = ui_adjust_w_v*3/4;
            }            
        }
        dis_mode = DIS_TV_4_3;
        zoom_mode = dis_mode;
        break;
    case SCALE_16_9:
        if(dis_mode == DIS_TV_4_3){
            if(zoom_mode == DIS_TV_4_3){
                scale_param.w = scale_param.w*4/3;
                scale_max_hor = scale_max_hor*4/3;
                ui_adjust_w_v = ui_adjust_w_v*4/3;
            }            
        }
        dis_mode = DIS_TV_16_9;
        zoom_mode = dis_mode;
        break;
    default:
        break;
    }
    save_sys_scale_param();    
    scale_param.x = (lcd_area.area.w-scale_param.w)/2.0;//与下层转换fbdev->sx = x * h_mul / h_div相反
    scale_param.y = (lcd_area.area.h-scale_param.h)/2.0;
    set_display_zoom_when_sys_scale();
    set_sys_scale();

    return 0;
}



void scale_widget_event_cb(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t*)obj;

    if(code == LV_EVENT_PRESSED){
        if(timer_setting){
            lv_timer_pause(timer_setting);
        }
        switch (btnms->btn_id_sel)
        {
        case 1:
            do_sys_scale(SCALE_4_3);
            break;
        case 3:
            do_sys_scale(SCALE_ZOOM_OUT);
            break;
        case 4:
            do_sys_scale(SCALE_ZOOM_RECOVERY);
            break;
        case 5:
            do_sys_scale(SCALE_ZOOM_IN);
            break;
        case 7:
            do_sys_scale(SCALE_16_9);
            break;  
        default:
            break;
        }
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    }else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            lv_obj_del(obj);
            turn_to_setup_root();
        }
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        if(dsc->id == btnms->btn_id_sel){
            dsc->rect_dsc->bg_color = lv_color_make(0,0,255);
            dsc->label_dsc->color = lv_color_white();
        }else{
            if(!lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_HIDDEN)){
                dsc->rect_dsc->bg_color = lv_color_white();
                dsc->label_dsc->color = lv_color_black();
            }

        }
    }else if(code == LV_EVENT_DRAW_MAIN_BEGIN){

    }
}

void create_scale_widget(lv_obj_t* btn){
    static const char* scale_map[] = {" ", "4:3", " ","\n","zoom out" , "recovery", "zoom in","\n", " ", "16:9", " ", ""};
    scale_map[4] = api_rsc_string_get(STR_ZOOMOUT);
    scale_map[5] = api_rsc_string_get(STR_RESET);
    scale_map[6] = api_rsc_string_get(STR_ZOOMIN);
    lv_obj_t *btnms = lv_btnmatrix_create(setup_scr);
    extern lv_obj_t* slave_scr_obj;
    slave_scr_obj = btnms;
    lv_obj_center(btnms);
    lv_obj_set_size(btnms, lv_pct(30), lv_pct(50));
    lv_btnmatrix_set_map(btnms, scale_map);
    lv_obj_set_style_text_font(btnms, osd_font_get(FONT_NORMAL),0);
    lv_btnmatrix_set_btn_ctrl(btnms, 0, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnms, 2, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnms, 6, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnms, 8, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_obj_set_style_bg_opa(btnms, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btnms, 0, 0 | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btnms, 0, 0 | LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btnms, scale_widget_event_cb, LV_EVENT_ALL, (void*)btn);
    lv_group_focus_obj(btnms);
}

#else
#include <hcuapi/dis.h>
#define DIS_ZOOM_FULL_X  0
#define DIS_ZOOM_FULL_Y  0
#define DIS_ZOOM_FULL_H  1080
#define DIS_ZOOM_FULL_W  1920


void set_display_zoom_when_sys_scale(){
    return;
}

int get_display_x(){
    return DIS_ZOOM_FULL_X;
}

int get_display_y(){
    return DIS_ZOOM_FULL_Y;
}

int get_display_h(){
    return DIS_ZOOM_FULL_W;
}

int get_display_v(){
    return DIS_ZOOM_FULL_H;
}

int get_cur_osd_h(){
    dis_screen_info_t lcd_area = {0};
    api_get_screen_info(&lcd_area);
    return lcd_area.area.w;
}

int get_cur_osd_v(){
    dis_screen_info_t lcd_area = {0};
    api_get_screen_info(&lcd_area);
    return lcd_area.area.h;
}

#endif
