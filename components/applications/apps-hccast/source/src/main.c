#include <stdio.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
/*#include "lv_gpu_hcge.h"*/

/*#define DISP_BUF_SIZE (128 * 1024)*/
#define DISP_BUF_SIZE (1280 * 720)
#if 0
static lv_color_t buf[DISP_BUF_SIZE];
static lv_color_t buf1[DISP_BUF_SIZE];
#endif

static void my_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px)
{
	  printf("%d px refreshed in %d ms\n", px, time);
}

void *lv_mem_adr;


extern void fbdev_wait_cb(struct _lv_disp_drv_t * disp_drv);
int lvgl_main(void)
{
    /*Linux frame buffer device init*/
    fbdev_init();
    lv_mem_adr = fbdev_static_malloc_virt(LV_MEM_SIZE);
    printf("lv_mem_adr: %p\n", lv_mem_adr);

    /*LittlevGL init*/
    lv_init();


    /*A small buffer for LittlevGL to draw the screen's content*/

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
#if 0
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);
    /*lv_disp_draw_buf_init(&disp_buf, fbp, NULL, 1280*720);*/
#else
    /*lv_disp_draw_buf_init(&disp_buf, fbdev_static_malloc_virt(DISP_BUF_SIZE * 4), NULL, DISP_BUF_SIZE);*/
    lv_disp_draw_buf_init(&disp_buf, fbdev_static_malloc_virt(DISP_BUF_SIZE * 4), fbdev_static_malloc_virt(DISP_BUF_SIZE * 4), DISP_BUF_SIZE);
    /*lv_disp_draw_buf_init(&disp_buf, fbp0, fbp1, 1280*720);*/
    /*lv_disp_draw_buf_init(&disp_buf, fbp0, NULL, 1280*720);*/
#endif

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 1280;//1920;
    disp_drv.ver_res    = 720;//1080;
    disp_drv.full_refresh = 0;
    disp_drv.direct_mode = 0;
    disp_drv.monitor_cb = my_monitor_cb;
    /*disp_drv.wait_cb = fbdev_wait_cb;*/
    /*disp_drv.gpu_wait_cb = lv_gpu_hcge_wait_cb;*/
    /*disp_drv.gpu_fill_cb = lv_gpu_hcge_fill_cb;*/
    lv_disp_drv_register(&disp_drv);
    printf("lv_disp_register: w-%d,h-%d\n",disp_drv.hor_res,disp_drv.ver_res);


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
