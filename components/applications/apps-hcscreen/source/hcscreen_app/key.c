#include <pthread.h>
#ifdef __linux__
#include <linux/input.h>
#else
#include <hcuapi/input.h>
#endif
#include <sys/poll.h>
#include <fcntl.h>
#include "key.h"

#include "com_api.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/misc/lv_types.h"
#include <errno.h>

static int fd_key;
static lv_indev_t * indev_keypad;

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
	
	fd_key = open("/dev/input/event0", O_RDONLY);
}


/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_get_key(void)
{

    /*Your code comes here*/
	struct pollfd pfd;
	struct input_event t;
	pfd.fd = fd_key;
	pfd.events = POLLIN | POLLRDNORM;
	if (poll(&pfd, 1, 1) <= 0){
			return 0;
	}

	if (read(fd_key, &t, sizeof(t)) != sizeof(t))
		return 0;

//	printf("type:%d, code:%d, value:%ld\n", t.type, t.code, t.value);
	return t.code;
}


/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    /*Get whether the a key is pressed and save the pressed key*/
    uint32_t act_key = keypad_get_key();
	
    if(act_key != 0) {
        data->state = LV_INDEV_STATE_PR;
		//printf("=========%s,%d,%ld\n",__func__,__LINE__,act_key);
        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch(act_key) {
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
        	act_key = USER_KEY_FLAG | act_key;
        	break;
        }

        last_key = act_key;
//		printf("+%s,%d,%ld\n",__func__,__LINE__,act_key&0xffff);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
	
    data->key = last_key;
}


static void *hc_key_task(void *arg)
{
	int fd;
	struct input_event t;
	struct pollfd pfd;
	int cnt;


	fd = open("/dev/input/event0", O_RDONLY);
	if (fd == -1)
		printf("%s(), open file fail!\n", __FUNCTION__);
	else
		printf("%s(), open file OK, fd = %d!\n", __FUNCTION__, fd);

	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

	while (1) {
		if (poll(&pfd, 1, -1) <= 0){
			continue;
		}
		cnt = read(fd, &t, sizeof(t));
		if (cnt != sizeof(t)){
			printf("cnt = %d, err(%d):%s\n", cnt, errno, strerror(errno));
			api_sleep_ms(10);
			continue;
		}

		printf("type:%d, code:%d, value:%d\n", t.type, t.code, t.value);
		if (t.type == EV_KEY) {
			printf("key %d %s\n", t.code,
			       (t.value) ? "Pressed" : "Released");
			if (t.code == KEY_POWER && !t.value) {
				while (read(fd, &t, sizeof(t)) == sizeof(t))
					;
				break;
			}
		}
	}

	close(fd);

	return 0;	
}

uint32_t key_convert_vkey(uint32_t lv_key)
{

	uint32_t vkey = VKEY_NULL;
	if (lv_key & USER_KEY_FLAG){
		lv_key = lv_key & 0xFFFF;
		switch(lv_key){
		case KEY_NUMERIC_0:
			vkey = V_KEY_0;
			break;
		case KEY_NUMERIC_1:
			vkey = V_KEY_1;
			break;
		case KEY_NUMERIC_2:
			vkey = V_KEY_2;
			break;
		case KEY_NUMERIC_3:
			vkey = V_KEY_3;
			break;
		case KEY_NUMERIC_4:
			vkey = V_KEY_4;
			break;
		case KEY_NUMERIC_5:
			vkey = V_KEY_5;
			break;
		case KEY_NUMERIC_6:
			vkey = V_KEY_6;
			break;
		case KEY_NUMERIC_7:
			vkey = V_KEY_7;
			break;
		case KEY_NUMERIC_8:
			vkey = V_KEY_8;
			break;
		case KEY_NUMERIC_9:
			vkey = V_KEY_9;
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
		case KEY_VOLUMEUP:
			vkey = V_KEY_V_UP;
			break;
		case KEY_VOLUMEDOWN:
			vkey = V_KEY_V_DOWN;
			break;
			
		}

	}else{


		switch(lv_key){
		case LV_KEY_UP:
			vkey = V_KEY_UP;
			break;
		case LV_KEY_DOWN:
			vkey = V_KEY_DOWN;
			break;
		case LV_KEY_LEFT:
			vkey = V_KEY_LEFT;
			break;
		case LV_KEY_RIGHT:
			vkey = V_KEY_RIGHT;
			break;
		case LV_KEY_NEXT:
			vkey = V_KEY_NEXT;
			break;
		case LV_KEY_PREV:
			vkey = V_KEY_PREV;
			break;
		case LV_KEY_ENTER:
			vkey = V_KEY_ENTER;
			break;
		}

	}

	return vkey;
}



int key_regist(lv_group_t * group)
{

#if 1
	lv_indev_set_group(indev_keypad, group);
	lv_group_set_default(group);

#else
	lv_group_set_default(group);

	lv_indev_t* cur_drv = NULL;
    for (;;) {
        cur_drv = lv_indev_get_next(cur_drv);
        if (!cur_drv) {
            break;
        }

        if (cur_drv->driver->type == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(cur_drv, group);
        }

    }
#endif

	return 0;
}

static volatile bool m_key_init = false;
int key_init()
{

	printf("Entering %s()!\n", __FUNCTION__);
    static lv_indev_drv_t keypad_driver;

    if (true == m_key_init)
    	return API_SUCCESS;
#if 1
    keypad_init();
    lv_indev_drv_init(&keypad_driver);
    keypad_driver.type = LV_INDEV_TYPE_KEYPAD;
    keypad_driver.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&keypad_driver);

#else
	pthread_t thread_id = 0;
    pthread_attr_t attr;    
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, hc_key_task, NULL)) {
        return API_FAILURE;
    }
#endif    
    m_key_init = true;
    return API_SUCCESS;
}



