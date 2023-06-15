#include <stdio.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <lvgl/hc_src/hc_lvgl_init.h>

int lvgl_main(void)
{

    hc_lvgl_init();
    /*Create a Demo*/
    /*lv_demo_music();*/
     /*lv_demo_widgets();*/
    /*lv_obj_t *ui_HomeScreen = lv_scr_act();*/
    /*lv_scr_load_anim(ui_HomeScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, true);*/
	/*lv_demo_benchmark();*/

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_task_handler();
	usleep(2000);
	/*lv_tick_inc(2);*/
    }

    return 0;
}

void lvgl_exit(void)
{
	lv_deinit();
}
