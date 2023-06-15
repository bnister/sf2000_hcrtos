#include "lvgl/lvgl.h"
#if LVGL_HC_IR
#include <pthread.h>

#include <sys/poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "key.h"

#include "lvgl/src/misc/lv_types.h"
#include <errno.h>
#ifdef __linux__
#include <linux/input.h>
#else
#include <hcuapi/input.h>
#endif

#if 0
#define KEYPAD_DEBUG printf
#else
#define KEYPAD_DEBUG(...) do{}while(0)
#endif

static int fd_key;
static lv_indev_t *indev_keypad;

/*Initialize your keypad*/
static void keypad_init(void)
{
	/*Your code comes here*/
	printf("%s:%d\n", __func__, __LINE__);
	fd_key = open("/dev/input/event0", O_RDONLY);
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_get_key(void)
{
	static int key_code = 0;
	static int key_pressing = false;
	/*Your code comes here*/
	struct pollfd pfd;
	struct input_event t;
	pfd.fd = fd_key;
	pfd.events = POLLIN | POLLRDNORM;

	while(poll(&pfd, 1, 0) > 0) {
		if (read(fd_key, &t, sizeof(t)) == sizeof(t) && (t.type == EV_KEY)) {
			KEYPAD_DEBUG("t.type: %d, t.code: %d, t.value: %d\n", t.type, t.code, t.value);
			key_code = t.code;
			key_pressing = t.value;
			break;
		}
		KEYPAD_DEBUG("t.type: %d, t.code: %d, t.value: %d\n", t.type, t.code, t.value);
	}

	return key_pressing ? key_code: 0;
}

static uint32_t key_convert_vkey(uint32_t lv_key)
{

	uint32_t vkey = VKEY_NULL;
	switch(lv_key) {
		case KEY_NUMERIC_0:
			vkey = '0';
			break;
		case KEY_NUMERIC_1:
			vkey = '1';
			break;
		case KEY_NUMERIC_2:
			vkey = '2';
			break;
		case KEY_NUMERIC_3:
			vkey = '3';
			break;
		case KEY_NUMERIC_4:
			vkey = '4';
			break;
		case KEY_NUMERIC_5:
			vkey = '5';
			break;
		case KEY_NUMERIC_6:
			vkey = '6';
			break;
		case KEY_NUMERIC_7:
			vkey = '7';
			break;
		case KEY_NUMERIC_8:
			vkey = V_KEY_8;
			vkey = '8';
			break;
		case KEY_NUMERIC_9:
			vkey = V_KEY_9;
			vkey = '9';
			break;
		case KEY_POWER:
			vkey = V_KEY_POWER;
			break;
		case KEY_AUDIO:
			vkey = V_KEY_AUDIO;
			break;
		case KEY_MUTE:
			vkey = V_KEY_MUTE;
			break;
		case KEY_ZOOM:
			vkey = V_KEY_ZOOM;
			break;
		case KEY_SUBTITLE:
			vkey = V_KEY_SUB;
			break;
		case KEY_TV:
			vkey = V_KEY_TVRADIO;
			break;
		case KEY_TEXT:
			vkey = V_KEY_TEXT;
			break;
		case KEY_LIST:
			vkey = V_KEY_LIST;
			break;
		case KEY_MENU:
			vkey = V_KEY_MENU;
			break;
		case KEY_EXIT:
			vkey = V_KEY_EXIT;
			break;
		case KEY_EPG:
			vkey = V_KEY_EPG;
			break;
		case KEY_AGAIN:
			vkey = V_KEY_RECALL;
			break;
		case KEY_FAVORITES:
			vkey = V_KEY_FAV;
			break;
		case KEY_LEFTSHIFT:
			vkey = V_KEY_FB;
			break;
		case KEY_RIGHTSHIFT:
			vkey = V_KEY_FF;
			break;
		case KEY_PLAY:
			vkey = V_KEY_PLAY;
			break;
		case KEY_PAUSE:
			vkey = V_KEY_PAUSE;
			break;
		case KEY_STOP:
			vkey = V_KEY_STOP;
			break;
		case KEY_RECORD:
			vkey = V_KEY_RECORD;
			break;
		case KEY_RED:
			vkey = V_KEY_RED;
			break;
		case KEY_GREEN:
			vkey = V_KEY_GREEN;
			break;
		case KEY_YELLOW:
			vkey = V_KEY_YELLOW;
			break;
		case KEY_BLUE:
			vkey = V_KEY_BLUE;
			break;
	}
	return vkey;
}

/*Will be called by the library to read the mouse*/
static bool keypad_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	static uint32_t last_key = 0;

	/*Get whether the a key is pressed and save the pressed key*/
	uint32_t act_key = keypad_get_key();
	if (act_key != 0) {
		/*printf("+%s,%d,%ld\n",__func__,__LINE__,act_key&0xffff);*/
		data->state = LV_INDEV_STATE_PR;
		switch (act_key) {
		 case 54:
		act_key = LV_KEY_PREV;
		break;
		case 42:
		act_key = LV_KEY_NEXT;
		break;
		case KEY_UP:
			act_key = LV_KEY_UP;
			break;
		case KEY_DOWN:
			act_key = LV_KEY_DOWN;
			break;
		case KEY_LEFT:
			act_key = LV_KEY_LEFT;
			break;
		case KEY_RIGHT:
			act_key = LV_KEY_RIGHT;
			break;
		case KEY_OK:
			act_key = LV_KEY_ENTER;
			break;
		case KEY_NEXT:
			act_key = LV_KEY_NEXT;
			break;
		case KEY_PREVIOUS:
			act_key = LV_KEY_PREV;
			break;
		default:
			act_key = key_convert_vkey(act_key);
			break;
		}

		KEYPAD_DEBUG("data->key = %d, data->state:%d\n", data->key, data->state);
		last_key = act_key;
	} else {
		data->state = LV_INDEV_STATE_REL;
	}

	data->key = last_key;
	return false;
}

int key_init(void)
{
	static lv_indev_drv_t keypad_driver;

	keypad_init();
	lv_indev_drv_init(&keypad_driver);
	keypad_driver.type = LV_INDEV_TYPE_KEYPAD;
	keypad_driver.read_cb = keypad_read;
	indev_keypad = lv_indev_drv_register(&keypad_driver);
#if 0
	lv_group_t *group = lv_group_get_default();
	if(!group) {
		group = lv_group_create();
		lv_indev_set_group(indev_keypad, group);
	}
	lv_indev_set_group(indev_keypad, group);
#endif

	return 0;
}
#endif
