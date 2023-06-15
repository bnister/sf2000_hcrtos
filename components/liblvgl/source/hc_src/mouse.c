#include "lvgl/lvgl.h"
#include "lv_drv_conf.h"
#if USE_EVDEV
#include "lv_drivers/indev/evdev.h"

static lv_indev_t * lv_test_mouse_indev;
LV_IMG_DECLARE(mouse_cursor_icon); /*Declare the image file.*/

void hc_mouse_init(void)
{   
	evdev_init();
	static lv_indev_drv_t indev_mouse_drv;
	lv_indev_drv_init(&indev_mouse_drv);
	indev_mouse_drv.type = LV_INDEV_TYPE_POINTER;
	indev_mouse_drv.read_cb = evdev_read;
	lv_test_mouse_indev = lv_indev_drv_register(&indev_mouse_drv);

	/*Set cursor. For simplicity set a HOME symbol now.*/
	lv_obj_t * mouse_cursor = lv_img_create(lv_scr_act());
	lv_img_set_src(mouse_cursor, &mouse_cursor_icon);
	lv_indev_set_cursor(lv_test_mouse_indev, mouse_cursor);
} 

#endif

