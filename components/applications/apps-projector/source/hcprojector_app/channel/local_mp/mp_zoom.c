/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-11-29 16:07:56
 */
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include <dirent.h>
#include <sys/stat.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/dis.h>
#include <ffplayer.h>
#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "mp_zoom.h"

#include "local_mp_ui.h"
#include "mp_ctrlbarpage.h"
#include "screen.h" 
#include "setup.h"
#include "factory_setting.h"
#include "mp_fspage.h"

//for zoom 
static Zoom_Param_t zoom_param={0};
dis_screen_info_t dis_info={0};
dis_area_t zoomin_src_disarea={0};
//for zoom move
zoom_move_t zoom_move={0};
dis_screen_info_t cur_zoomdis_info={0};



void zoommove_event_handler(lv_event_t *event);

/**
 * @description:reset param ,need to reset param when playing next or replay
 * beacause zoom factor change when press zoom+/-,reset /dev/dis area 
 * @return {*}
 * @author: Yanisin
 */
void app_reset_diszoom_param(void)
{
    if(zoom_param.zoom_size!=ZOOM_NORMAL){
        api_set_display_area(projector_get_some_sys_param(P_ASPECT_RATIO));
        memset(&zoom_param,0,sizeof(Zoom_Param_t));
    }
}

void* app_get_zoom_param(void)
{
    return &zoom_param;
}

static void app_get_zoom_src_area(struct dis_area* src_area){
    src_area->w = DIS_ZOOM_FULL_W;
    src_area->x = DIS_ZOOM_FULL_X;
    src_area->h = DIS_ZOOM_FULL_H;
    src_area->y = DIS_ZOOM_FULL_Y;   

    HCPlayerVideoInfo info = {0};
    media_handle_t * hdl = (media_handle_t *)mp_get_cur_player_hdl();
    if(!hdl){
        return;
    }
    hcplayer_get_cur_video_stream_info(hdl->player, &info);

    double w = info.width;
    double h = info.height;
    double rate = 1.0;    
    if(w*9 > h*16){
        rate = DIS_ZOOM_FULL_W/w;
        src_area->x = 0;
        src_area->h = rate*h;
        src_area->y = (DIS_ZOOM_FULL_H - src_area->h)/2;
    }else if(w*9 < h*16){
        rate = DIS_ZOOM_FULL_H/h;
        src_area->y = 0;
        src_area->w = rate*w;
        src_area->x = (DIS_ZOOM_FULL_W - src_area->w)/2;
    }

    //printf("x: %d, y: %d, w: %d, h: %d\n", src_area->x, src_area->y, src_area->w, src_area->h);
}

/**
 * @description: 
 * @param {float} zoom_factor:0< zoom_factor<2
 * @return {*}
 * @author: Yanisin
 */
dis_zoom_t app_set_disdev_param(float zoom_factor)
{
    dis_zoom_t diszoom_param;
    diszoom_param.layer= DIS_LAYER_MAIN;
    diszoom_param.distype=DIS_TYPE_HD;
    if(zoom_factor==1.0){
        // 缩放的是整个屏幕，要以屏幕为对象
        diszoom_param.src_area.x=DIS_ZOOM_FULL_X;
        diszoom_param.src_area.y=DIS_ZOOM_FULL_Y;
        diszoom_param.src_area.w=DIS_ZOOM_FULL_W;
        diszoom_param.src_area.h=DIS_ZOOM_FULL_H;

        diszoom_param.dst_area.x=dis_info.area.x;
        diszoom_param.dst_area.y=dis_info.area.y;
        diszoom_param.dst_area.w=dis_info.area.w;
        diszoom_param.dst_area.h=dis_info.area.h;

    }else if(zoom_factor<1.0){
        //zoom -

        // 缩放整个屏幕
        diszoom_param.src_area.x=DIS_ZOOM_FULL_X;
        diszoom_param.src_area.y=DIS_ZOOM_FULL_Y;
        diszoom_param.src_area.w=DIS_ZOOM_FULL_W;
        diszoom_param.src_area.h=DIS_ZOOM_FULL_H;

        diszoom_param.dst_area.x=(int)dis_info.area.x+(dis_info.area.w-dis_info.area.w*zoom_factor)/2;
        diszoom_param.dst_area.y=(int)dis_info.area.y+(dis_info.area.h-dis_info.area.h*zoom_factor)/2;
        diszoom_param.dst_area.w=(int)dis_info.area.w*zoom_factor;
        diszoom_param.dst_area.h=(int)dis_info.area.h*zoom_factor;
        
    }else if(zoom_factor>1.0){
        float temp_factor=2.0-zoom_factor;

        // diszoom_param.src_area.x=(int)dis_info.area.x+(dis_info.area.w-dis_info.area.w*temp_factor)/2;
        // diszoom_param.src_area.y=(int)dis_info.area.y+(dis_info.area.h-dis_info.area.h*temp_factor)/2;
        // diszoom_param.src_area.w=(int)dis_info.area.w*temp_factor;
        // diszoom_param.src_area.h=(int)dis_info.area.h*temp_factor;

        app_get_zoom_src_area(&diszoom_param.src_area);
        diszoom_param.src_area.x+=diszoom_param.src_area.w*(1-temp_factor)/2;
        diszoom_param.src_area.y+=diszoom_param.src_area.h*(1-temp_factor)/2;
        diszoom_param.src_area.w*=temp_factor;
        diszoom_param.src_area.h*=temp_factor;  

        memcpy(&zoomin_src_disarea,&diszoom_param.src_area,sizeof(dis_area_t));

        diszoom_param.dst_area.x=get_display_x();//DIS_ZOOM_FULL_X;
        diszoom_param.dst_area.y=get_display_y();//DIS_ZOOM_FULL_Y;
        diszoom_param.dst_area.w=get_display_h();//DIS_ZOOM_FULL_W;
        diszoom_param.dst_area.h=get_display_v();//DIS_ZOOM_FULL_H;

    }
    return diszoom_param;
    
}

/**
 * @description: 将LCD上图像显示的坐标转换到de 设备显示的坐标上
 * de 设备的坐标默认是1920 *1080 ，而不同LCD上获取到的信息通过
 * api_get_display_area 
 * @return {*}
 * @author: Yanisin
 */
int lcd2dis_coord_conv(dis_screen_info_t * dis_info)
{
    dis_screen_info_t lcd_area={0};
    dis_screen_info_t pic_inlcd_area={0};
    api_get_screen_info(&lcd_area);
    api_get_display_area(&pic_inlcd_area);
    dis_info->area.x=(pic_inlcd_area.area.x*DIS_ZOOM_FULL_W)/lcd_area.area.w;
    dis_info->area.y=(pic_inlcd_area.area.y*DIS_ZOOM_FULL_H)/lcd_area.area.h;
    dis_info->area.w=(pic_inlcd_area.area.w*DIS_ZOOM_FULL_W)/lcd_area.area.w;
    dis_info->area.h=(pic_inlcd_area.area.h*DIS_ZOOM_FULL_H)/lcd_area.area.h;
    return 0;
}

int app_set_diszoom(Zoom_mode_e Zoom_mode)
{
    dis_zoom_t zoom_argv={0};
    if(zoom_param.zoom_size==ZOOM_NORMAL){
        // api_get_display_area(&dis_info);
        lcd2dis_coord_conv(&dis_info);
    }
    if(Zoom_mode==MPZOOM_IN){
        zoom_param.zoom_state=zoom_param.zoom_state<3?zoom_param.zoom_state+1:0;
    }else{
        zoom_param.zoom_state=zoom_param.zoom_state>-3?zoom_param.zoom_state-1:0;
    }
    switch (zoom_param.zoom_state){
        case ZOOM_OUTSIZE_3:
            zoom_argv=app_set_disdev_param(0.5);
            api_set_display_zoom2(&zoom_argv);
            zoom_param.zoom_size=ZOOM_OUTSIZE_3;
            break;
        case ZOOM_OUTSIZE_2:
            zoom_argv=app_set_disdev_param(0.66);
            api_set_display_zoom2(&zoom_argv);
            zoom_param.zoom_size=ZOOM_OUTSIZE_2;
            break;
        case ZOOM_OUTSIZE_1://--
            zoom_argv=app_set_disdev_param(0.83);
            api_set_display_zoom2(&zoom_argv);
            zoom_param.zoom_size=ZOOM_OUTSIZE_1;
            break;
        case ZOOM_NORMAL:
            //get de source size so that you can set zoom
            // api_set_display_aspect(DIS_TV_AUTO,DIS_NORMAL_SCALE);
            // api_set_display_area(projector_get_some_sys_param(P_ASPECT_RATIO));
            zoom_argv=app_set_disdev_param(1.0);
            api_set_display_zoom2(&zoom_argv);
            zoom_param.zoom_size=ZOOM_NORMAL;
            break;
        case ZOOM_INSIZE_1://zoom +
            // api_set_display_zoom((1920-1920*0.8)/2,(1080-1080*0.8)/2,1920*0.8,1080*0.8,0,0,1920,1080);
            // api_set_display_aspect(DIS_TV_AUTO,DIS_PILLBOX);
            zoom_argv=app_set_disdev_param(1.2);
            api_set_display_zoom2(&zoom_argv);
            zoom_param.zoom_size=ZOOM_INSIZE_1;
            break;
        case ZOOM_INSIZE_2:
            zoom_argv=app_set_disdev_param(1.6);
            api_set_display_zoom2(&zoom_argv);
            zoom_param.zoom_size=ZOOM_INSIZE_2;
            break;
        case ZOOM_INSIZE_3:
            zoom_argv=app_set_disdev_param(1.8);
            api_set_display_zoom2(&zoom_argv);
            zoom_param.zoom_size=ZOOM_INSIZE_3;
            break;
        default :
            break;
    }

    return 0;

}

int zoom_move_init(void* argc)
{
    //cur dis info is after zoom 
    // api_get_display_area(&cur_zoomdis_info);
    memcpy(&cur_zoomdis_info.area,&zoomin_src_disarea,sizeof(dis_area_t));
    zoom_move.move_step=ZOOM_MOVE_STEP;//move ZOOM_MOVE_STEP unit when press key
    zoom_move.zoom_area.up_range=cur_zoomdis_info.area.y;
    zoom_move.zoom_area.down_range=DIS_ZOOM_FULL_H-cur_zoomdis_info.area.y-cur_zoomdis_info.area.h;
    // zoom_move.zoom_area.left_range=cur_zoomdis_info.area.x-dis_info.area.x;
    // zoom_move.zoom_area.right_range=(dis_info.area.x+dis_info.area.w)-(cur_zoomdis_info.area.x+cur_zoomdis_info.area.w);
    zoom_move.zoom_area.left_range=cur_zoomdis_info.area.x;
    zoom_move.zoom_area.right_range=DIS_ZOOM_FULL_W-(cur_zoomdis_info.area.x+cur_zoomdis_info.area.w);
    return 0;
}

dis_area_t zoom_move_set_param(uint32_t key)
{
    static dis_area_t dst_area_aftermove={0};

    //边界处理,还要处理当0<range<step情况
    switch (key){
        case LV_KEY_UP:
            if(zoom_move.zoom_area.up_range>0&&zoom_move.zoom_area.up_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.up_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;
                if(zoom_move.zoom_area.up_range==0)
                    break;
            }
            zoom_move.zoom_area.up_range=zoom_move.zoom_area.up_range-zoom_move.move_step;
            zoom_move.zoom_area.down_range=zoom_move.zoom_area.down_range+zoom_move.move_step;
            break;
        case LV_KEY_DOWN:
            if(zoom_move.zoom_area.down_range>0&&zoom_move.zoom_area.down_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.down_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;
                if(zoom_move.zoom_area.down_range==0)
                    break;
            }
            zoom_move.zoom_area.down_range=zoom_move.zoom_area.down_range-zoom_move.move_step;
            zoom_move.zoom_area.up_range=zoom_move.zoom_area.up_range+zoom_move.move_step;
            break;
        case LV_KEY_LEFT:
            if(zoom_move.zoom_area.left_range>0&&zoom_move.zoom_area.left_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.left_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;                   
                if(zoom_move.zoom_area.left_range==0)
                    break;
            }
            zoom_move.zoom_area.left_range=zoom_move.zoom_area.left_range-zoom_move.move_step;
            zoom_move.zoom_area.right_range=zoom_move.zoom_area.right_range+zoom_move.move_step;
            break;
        case LV_KEY_RIGHT:
            if(zoom_move.zoom_area.right_range>0&&zoom_move.zoom_area.right_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.right_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;                   
                if(zoom_move.zoom_area.right_range==0)
                    break;
            }
            zoom_move.zoom_area.right_range=zoom_move.zoom_area.right_range-zoom_move.move_step;
            zoom_move.zoom_area.left_range=zoom_move.zoom_area.left_range+zoom_move.move_step;
            break;
        default :
            break;
    }




    
    //计算出 dis dst_area(x,y)
    // dst_area_aftermove.x=dis_info.area.x+zoom_move.zoom_area.left_range;
    dst_area_aftermove.x=zoom_move.zoom_area.left_range;
    dst_area_aftermove.y=zoom_move.zoom_area.up_range;
    dst_area_aftermove.w=cur_zoomdis_info.area.w;
    dst_area_aftermove.h=cur_zoomdis_info.area.h;



    return dst_area_aftermove;
}
int zoom_move_operation(uint32_t lv_key)
{
    dis_area_t cur_zoomdis_area={0};
    dis_zoom_t move_zoom={0};
    cur_zoomdis_area=zoom_move_set_param(lv_key);
    move_zoom.layer = DIS_LAYER_MAIN;
    move_zoom.distype = DIS_TYPE_HD;
    move_zoom.active_mode=DIS_SCALE_ACTIVE_IMMEDIATELY;
    move_zoom.dst_area.x=get_display_x();//DIS_ZOOM_FULL_X;
    move_zoom.dst_area.y=get_display_y();//DIS_ZOOM_FULL_Y;
    move_zoom.dst_area.w=get_display_h();//DIS_ZOOM_FULL_W;
    move_zoom.dst_area.h=get_display_v();//DIS_ZOOM_FULL_H;
    memcpy(&move_zoom.src_area,&cur_zoomdis_area,sizeof(dis_area_t));
    api_set_display_zoom2(&move_zoom);
    return 0;
}



/*lvgl show&operation*/
lv_obj_t*  create_zoommove_cont(lv_obj_t *p)
{
    lv_obj_t* obj = lv_obj_create(p);
    lv_obj_set_size(obj,ZOOMMOVE_CONT_SIZE,ZOOMMOVE_CONT_SIZE);
    lv_obj_set_align(obj, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, ZOOMMOVE_LABSIZE, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* sublabel = lv_label_create(obj);
    lv_obj_set_width(sublabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel, LV_ALIGN_TOP_MID);
    lv_label_set_text(sublabel,LV_SYMBOL_UP);
    lv_obj_set_style_text_color(sublabel,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);

    lv_obj_t* sublabel1 = lv_label_create(obj);
    lv_obj_set_width(sublabel1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel1, LV_ALIGN_LEFT_MID);
    lv_label_set_text(sublabel1,LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(sublabel1,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);


    lv_obj_t* sublabel2 = lv_label_create(obj);
    lv_obj_set_width(sublabel2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel2, LV_ALIGN_BOTTOM_MID);
    lv_label_set_text(sublabel2,LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(sublabel2,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);


    lv_obj_t* sublabel3 = lv_label_create(obj);
    lv_obj_set_width(sublabel3, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel3, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(sublabel3,LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(sublabel3,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);

    return obj;
}

int create_zoommoove_win(lv_obj_t* p,lv_obj_t * subbtn)
{
    lv_obj_t* inst_cont=create_zoommove_cont(p);
    lv_obj_add_event_cb(inst_cont,zoommove_event_handler,LV_EVENT_ALL,subbtn);
    lv_group_add_obj(lv_group_get_default(),inst_cont);
    lv_group_focus_obj(inst_cont);
    //init param
    zoom_move_init(NULL);
    return 0;
}


void key_operation_show_reset(lv_timer_t* t)
{
    lv_obj_t  * user_data = t->user_data;
    for(int i=0;i<lv_obj_get_child_cnt(user_data);i++)
        lv_obj_clear_state(lv_obj_get_child(user_data,i), LV_STATE_FOCUS_KEY);
}
int key_operation_show(lv_obj_t* t,uint32_t lv_key)
{
    static lv_timer_t* timer_handler=NULL;
    switch(lv_key)
    {
        case LV_KEY_UP:
            lv_obj_add_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            for(int i=1;i<lv_obj_get_child_cnt(t);i++)
                lv_obj_clear_state(lv_obj_get_child(t,i), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_LEFT:
            lv_obj_add_state(lv_obj_get_child(t,1), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,2), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,3), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_DOWN:
            lv_obj_add_state(lv_obj_get_child(t,2), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,1), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,3), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_RIGHT:
            lv_obj_add_state(lv_obj_get_child(t,3), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,1), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,2), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_ESC:    
            if(timer_handler!=NULL){
                lv_timer_del(timer_handler);
                timer_handler=NULL;
            }
            return 0;
        default : 
            break;
    }
    if(timer_handler==NULL){
        timer_handler=lv_timer_create(key_operation_show_reset,500,t);
    }else{
        lv_timer_pause(timer_handler);
        lv_timer_reset(timer_handler);
        lv_timer_resume(timer_handler);
    }
}
void zoommove_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target =lv_event_get_target(event);
    lv_obj_t *user_data=lv_event_get_user_data(event);
    if(code==LV_EVENT_PRESSED){
        printf("Press obj state:%#x \n",lv_obj_get_state(target));
    }else if(code==LV_EVENT_KEY){
        int key_val = lv_indev_get_key(lv_indev_get_act());
        show_play_bar(true);
        lv_timer_reset(bar_show_timer);
        zoom_move_operation(key_val);
        key_operation_show(target,key_val);
        if(key_val==LV_KEY_ESC){
            lv_group_focus_obj(user_data);
            lv_obj_del(target);
        }
    }
}

void zoom_transfer_dst_rect_for_screen(int rotate,int h_flip,struct dis_area *p_dst_area)
{
    int x = 0 , y = 0 , w = 0 , h = 0;

    x = p_dst_area->x;
    y = p_dst_area->y;
    w = p_dst_area->w;
    h = p_dst_area->h;

    printf("%s rotate:%d,h_flip:%d\n",__FUNCTION__, rotate, h_flip);
    switch(rotate)
    {
        case ROTATE_TYPE_90:
        {
            x = (DIS_ZOOM_FULL_H - y - h) * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            y = p_dst_area->x * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            w = p_dst_area->h * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            h = p_dst_area->w * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            break;
        }
        case ROTATE_TYPE_180:
        {
            x = (DIS_ZOOM_FULL_W - x - w);
            break;
        }
        case ROTATE_TYPE_270:
        {
            x = y * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            y = (DIS_ZOOM_FULL_W - p_dst_area->x - w) * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            w = p_dst_area->h * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            h = p_dst_area->w * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            break;
        }
        default:
            break;
    }

    if(h_flip == 1)
    {
        x = (DIS_ZOOM_FULL_W - x - w);
    }

    p_dst_area->x = x;
    p_dst_area->y = y;
    p_dst_area->w = w;
    p_dst_area->h = h;
}

/**
 * @description: reset zoom args when filp so that use this param to set zoom pos
 * @param {flip_mode_e} flip_mode
 * @return {*}
 * @author: Yanisin
 */
dis_zoom_t app_reset_mainlayer_param(int rotate , int h_flip)
{
    dis_zoom_t musiccover_param={0};
    musiccover_param.active_mode=DIS_SCALE_ACTIVE_IMMEDIATELY;
    musiccover_param.distype= DIS_TYPE_HD;
    musiccover_param.layer= DIS_LAYER_MAIN;
    musiccover_param.src_area.x=DIS_ZOOM_FULL_X;
    musiccover_param.src_area.y=DIS_ZOOM_FULL_Y;
    musiccover_param.src_area.w=DIS_ZOOM_FULL_W;
    musiccover_param.src_area.h=DIS_ZOOM_FULL_H;

    musiccover_param.dst_area.x = MUSIC_COVER_ZOOM_X*get_display_h()/DIS_ZOOM_FULL_W+get_display_x();
    musiccover_param.dst_area.y = MUSIC_COVER_ZOOM_Y*get_display_v()/DIS_ZOOM_FULL_H+get_display_y();
    musiccover_param.dst_area.w = MUSIC_COVER_ZOOM_W*get_display_h()/DIS_ZOOM_FULL_W;
    musiccover_param.dst_area.h = MUSIC_COVER_ZOOM_H*get_display_v()/DIS_ZOOM_FULL_H;
    zoom_transfer_dst_rect_for_screen(rotate, h_flip,&(musiccover_param.dst_area));
    return musiccover_param;
}

/**
 * @description: because rotate do not change the origin of coordinates in DE dev.
 * so had to reset main layer zoom pos when play music to show music cover 
 * only use for music cover when playing music.
 * @return {*}
 * @author: Yanisin
 */
int app_reset_mainlayer_pos(int rotate , int h_flip)
{
    dis_zoom_t musiccover_param2={0};
    musiccover_param2=app_reset_mainlayer_param(rotate , h_flip);
    api_set_display_zoom2(&musiccover_param2);
    return 0;
}

