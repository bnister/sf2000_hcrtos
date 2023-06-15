#include <stdio.h>

#include "../../screen.h"

lv_obj_t* cvbs_scr;


void cvbs_screen_init(void)
{
    cvbs_scr = lv_obj_create(NULL);

    lv_obj_set_style_bg_opa(cvbs_scr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(cvbs_scr, LV_OPA_TRANSP, 0);

}
