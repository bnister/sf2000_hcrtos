#include <stdio.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "hc_lvgl_init.h"
#include "key.h"

extern void hc_mouse_init(void);

static void my_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px)
{
	/*printf("%d px refreshed in %d ms\n", px, time);*/
}

void *lv_mem_adr;
int lv_mem_size;

static void hc_clear_cb(struct _lv_disp_drv_t * disp_drv, uint8_t * buf, uint32_t size)
{
	/*printf("memset, buf: %p, size: %d\n");*/
	/*memset(buf, 0, size);*/
}

extern void fbdev_wait_cb(struct _lv_disp_drv_t * disp_drv);

int hc_lvgl_init(void)
{
	/*Linux frame buffer device init*/
	fbdev_init();
	
	lv_color_t *buf1 = NULL; 
	lv_color_t *buf2 = NULL;

	buf1 = fbdev_static_malloc_virt(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
	if(!buf1){
		LV_LOG_ERROR("malloc buf1 fail!Not enough memory.\n");
		return -1;
	}

#if LV_HC_DRAW_BUF_COUNT == 2
	buf2 = fbdev_static_malloc_virt(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
	if(!buf2){
		LV_LOG_ERROR("malloc buf2 fail!Not enough memory.\n");
		return -1;
	}
#endif


#if 1
	/* Init lvgl alloc memory */
	lv_mem_size = fbdev_get_buffer_size() & ~0x1F;
	lv_mem_adr = fbdev_static_malloc_virt(lv_mem_size);
#else
	lv_mem_adr = malloc(LV_MEM_SIZE);
#endif
	printf("lv_mem_adr: %p, lv_mem_size: %d\n", lv_mem_adr, lv_mem_size);

	/*LittlevGL init*/
	lv_init();
	printf("%s:%d\n", __func__, __LINE__);

	/*Initialize a descriptor for the buffer*/
	static lv_disp_draw_buf_t disp_buf;

	if(!buf1 && !buf2){
		LV_LOG_ERROR("Need configure HC_LV_DRAW_BUF1 and HC_LV_DRAW_BUF2\n");
		return -1;
	}

	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LV_HC_DRAW_BUF_SIZE);
	LV_LOG_INFO("buf1 = %p, buf2: = %p, size = %d\n", buf1, buf2, LV_HC_DRAW_BUF_SIZE);

	/*Initialize and register a display driver*/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf   = &disp_buf;
	disp_drv.flush_cb   = fbdev_flush;
	disp_drv.hor_res    = LV_HC_SCREEN_HOR_RES;//1920;
	disp_drv.ver_res    = LV_HC_SCREEN_VER_RES;//1080;
	disp_drv.full_refresh = 0;
	disp_drv.direct_mode = 0;
	disp_drv.monitor_cb = my_monitor_cb;
	disp_drv.screen_transp = 1;
	disp_drv.clear_cb = hc_clear_cb;
	disp_drv.wait_cb = fbdev_wait_cb;
	lv_disp_drv_register(&disp_drv);
	printf("lv_disp_register: w-%d,h-%d\n",disp_drv.hor_res,disp_drv.ver_res);

#if USE_EVDEV
	hc_mouse_init();
#endif

#if LVGL_HC_IR
	key_init();
#endif

	return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
#ifdef __HCRTOS__
	return xTaskGetTickCount()/portTICK_PERIOD_MS;
#else
	struct timespec tv_now;
	assert(clock_gettime(CLOCK_BOOTTIME, &tv_now) == 0);
	return tv_now.tv_sec * 1000 + tv_now.tv_nsec / 1000000;
#endif
}
