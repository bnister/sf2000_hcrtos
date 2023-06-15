#include "media_spectrum.h"
#include "mp_spectdis.h"
#include "os_api.h"
#include "com_api.h"
#include "local_mp_ui.h"

lv_chart_series_t * ser1;
static lv_obj_t *ui_chart_spect=NULL;
void spectrum_refresh_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target =lv_event_get_target(event);
    if(code == LV_EVENT_REFRESH){
        int * spect_data=get_music_spect_data();
        //get spect_data in other music pthread 
        if(spect_data!=NULL)
        {
            for(int i=0;i<10;i++)
            {   
                lv_chart_set_value_by_id(target,ser1,i,spect_data[i]);
            }
        }
    }
}
int spectrum_uimsg_handle(uint32_t msg_type)
{
    if(msg_type==REFRESH_CODE){
        int * spect_data=get_music_spect_data();
        //get spect_data in other music pthread 
        if(spect_data!=NULL){
            for(int i=0;i<10;i++)
                lv_chart_set_value_by_id(ui_chart_spect,ser1,i,spect_data[i]);
        }
    }
    return 0;
}



int  create_music_spectrum(lv_obj_t* p)
{
    ui_chart_spect = lv_chart_create(p);
    lv_chart_set_type(ui_chart_spect, LV_CHART_TYPE_BAR);
    
    lv_obj_set_size(ui_chart_spect,LV_PCT(40),LV_PCT(50));
    lv_obj_align(ui_chart_spect, LV_ALIGN_LEFT_MID,SPECT_CHART_X_OFS,SPECT_CHART_Y_OFS);
    lv_obj_set_style_radius(ui_chart_spect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_chart_spect, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_chart_spect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_chart_spect, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_chart_spect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(ui_chart_spect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ui_chart_spect, 0, LV_PART_MAIN | LV_STATE_DEFAULT|LV_PART_ITEMS);
    lv_obj_set_style_pad_gap(ui_chart_spect,SPECT_CHART_PADGAP,0);
    lv_obj_set_style_line_color(ui_chart_spect, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui_chart_spect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui_chart_spect, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui_chart_spect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui_chart_spect, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_chart_set_div_line_count(ui_chart_spect, 0, 0);  //division lines set 0
    lv_chart_set_update_mode(ui_chart_spect, LV_CHART_UPDATE_MODE_CIRCULAR);
    ser1 = lv_chart_add_series(ui_chart_spect,  lv_color_hex(0x031FFF), LV_CHART_AXIS_PRIMARY_Y);
    for(int i = 0; i < 10; i++) {
        lv_chart_set_next_value(ui_chart_spect, ser1, 0);
    }
    lv_obj_add_event_cb(ui_chart_spect,spectrum_refresh_event_cb, LV_EVENT_ALL, NULL);
    return 0;
}

/// @brief after argc count to refresh dis
/// @return 
int music_spect_refresh_dis(void)
{
    control_msg_t objfresh_msg;
    objfresh_msg.msg_type =  MSG_TYPE_CMD;
    objfresh_msg.msg_code = REFRESH_CODE;
    api_control_send_msg(&objfresh_msg);
    return 0;
}
