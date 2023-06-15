#include <pthread.h>

#ifdef __linux__
#include <linux/input.h>
#else
#include <hcuapi/input.h>
#endif

#include <sys/poll.h>
#include <fcntl.h>
#include "key_event.h"

#include "com_api.h"
#include "lvgl/lvgl.h"
//#include "lvgl/src/misc/lv_types.h"
#include <errno.h>

static lv_indev_t * indev_keypad;
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#if 0
static int fd_key;
/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
	
	fd_key = open("/dev/input/event0", O_RDONLY);
}


#if 0
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
#else
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
			key_code = t.code;
			key_pressing = t.value;
			break;
		}   
	}   

	return key_pressing ? key_code: 0;
}
#endif


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

		printf("type:%d, code:%d, value:%ld\n", t.type, t.code, t.value);
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

int key_init()
{

	printf("Entering %s()!\n", __FUNCTION__);
    static lv_indev_drv_t keypad_driver;

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
    pthread_attr_destroy(&attr);
#endif    
    return API_SUCCESS;
}

#endif
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
		case KEY_HOME:
			vkey = V_KEY_HOME;
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
		case LV_KEY_ESC:
			vkey=V_KEY_EXIT;
			break;
		}

	}

	return vkey;
}


//////////////////////////////use for projector case//////////////////////////

uint32_t key_convert2_vkey(uint32_t lv_key)
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
		case KEY_HOME:
			vkey = V_KEY_HOME;
			break;
		}

	}

	return vkey;
}

#if 1 //use task to read key

static pthread_mutex_t key_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void key_mutex_lock(void)
{
    pthread_mutex_lock(&key_lock_mutex);
}

static inline void key_mutex_unlock(void)
{
    pthread_mutex_unlock(&key_lock_mutex);
}

static KEY_QUEUE_s m_key_queue;
// 初始化队列
void key_queue_init(KEY_QUEUE_s *q)
{
	q->front = q->rear = 0;
	q->size = 0;		//队列当前长度为0
}

// 判断队列是否为空
bool key_queue_empty(KEY_QUEUE_s *q)
{
	if (q->size==0)	//队空条件
		return true;
	else
		return false;
}

// 入队
bool key_queue_write(KEY_QUEUE_s *q, KEY_MSG_s *x)
{
	if (q->size == KEY_QUEUE_MAX)
		return false;		//队列满则报错

	key_mutex_lock();

	memcpy(&(q->key_msg[q->rear]), x, sizeof(KEY_MSG_s));	//将x插入(拷贝)队尾
	q->rear = (q->rear + 1) % KEY_QUEUE_MAX;    //队尾指针后移
	q->size++;

	key_mutex_unlock();

	return true;
}

#ifdef BLUETOOTH_SUPPORT
int bt_key_queue_write_ctrl(struct input_event_bt event)
{
	KEY_MSG_s key_msg;
	key_msg.key_type = KEY_TYPE_IR;
	key_msg.key_event.type = event.type;
	key_msg.key_event.code = event.code;
	key_msg.key_event.value = event.value;
	key_queue_write(&m_key_queue,&key_msg);
	return 0;
}
#endif
// 出队
bool key_queue_read(KEY_QUEUE_s *q, KEY_MSG_s **x)
{
	if (q->size==0)
		return false;	//队空则报错

	*x = &(q->key_msg[q->front]);
	q->front = (q->front + 1) % KEY_QUEUE_MAX; //队头指针后移
	q->size--;
	return true;
}

// 获取队头元素
bool key_queue_head_get(KEY_QUEUE_s *q, KEY_MSG_s **x)
{
	if (q->size==0)
		return false;	//队空则报错

	*x = &(q->key_msg[q->front]);
	return true;
}

// 队列中元素的个数
int key_queue_num(KEY_QUEUE_s *q)
{
	return q->size;
}

/*

typedef struct{
	int fd;
	uint8_t input_dev[32];
}INPUT_DEV_s;
static INPUT_DEV_s m_input_dev[] = {
	{0, "/dev/input/event0"}, //IR
	{0, "/dev/input/event1"}, //ADC
	{0, "/dev/input/event2"}, //GPIO
};
*/

#define INTERVAL_FIRST_KEY		350 //ms
#define INTERVAL_NORMAL_KEY		80 //ms
static bool _key_event_is_valid(struct input_event *t)
{
	static uint32_t tick_last = 0;
	static uint32_t tick_intreval = 0;
    static uint32_t pressed_key = 0;
    static uint32_t loop_cnt = 0;
    static uint32_t key_interval = 0;
    static int first_press = 0;

	//Skip redundancy keys.
	if (t->value != 0 && t->value != 1 && t->type != EV_MSC)
	 	return false;
	if (t->value == 0 && t->code == 0)
	 	return false;

    if(t->type == EV_KEY)
    {
        if(t->value == 1)// pressed
        {
        	tick_last = api_sys_tick_get();
            pressed_key = t->code;
            loop_cnt = 0;
            first_press = 1;
            tick_intreval = 0;
            return true;
        }
        else if(t->value == 0)// released
        {
        	t->code = pressed_key;
            pressed_key = 0;
            first_press = 0;
            tick_intreval = 0;
        	tick_last = 0;
            return true;
        }
    }
    else if(t->type == EV_MSC)// repeat key
    {
		tick_intreval = api_sys_tick_get() - tick_last;

        if (tick_intreval > 0 && pressed_key != 0){
            if (first_press)
                key_interval = INTERVAL_FIRST_KEY;
            else
            	key_interval = INTERVAL_NORMAL_KEY;

            if(tick_intreval > key_interval){
		    	tick_last = api_sys_tick_get();
                t->code = pressed_key;
                t->value = 1;
                first_press = 0;
                return true;
            }
        }
    }

    return false;
}


#define INPUT_DEV_MAX	10
static void *_key_task(void *arg)
{
	struct pollfd *pfd;
	int cnt;
	KEY_MSG_s key_msg;
	struct input_event *t;
	int input_cnt = 0;
	uint8_t input_name[32];
	int key_fds[INPUT_DEV_MAX] = {0,};
    int i;


	for (i = 0; i < INPUT_DEV_MAX; i++){
		sprintf(input_name, "/dev/input/event%d", i);
		key_fds[i] = open(input_name, O_RDONLY);
		if (key_fds[i] < 0)
			break;
		input_cnt++;
	}
	if (0 == input_cnt){
		printf("%s(), line:%d. No Input device!!!\n", __func__, __LINE__);
		return NULL;
	}

	printf("%s(), input device num: %d.\n", __func__, input_cnt);
	pfd = (struct pollfd*)malloc(sizeof(struct pollfd)*input_cnt);
	memset(pfd, 0, sizeof(struct pollfd)*input_cnt);

	for (i = 0; i< input_cnt; i ++){
		pfd[i].fd = key_fds[i];
		pfd[i].events = POLLIN | POLLRDNORM;
	}

	t = &key_msg.key_event;
	#ifdef BLUETOOTH_SUPPORT
	bluetooth_ir_key_init(bt_key_queue_write_ctrl);
	#endif
	while (1) {
		if (poll(pfd, input_cnt, -1) <= 0){
			continue;
		}

		for (i = 0; i < input_cnt; i ++){

			if (pfd[i].revents != 0){
				cnt = read(key_fds[i], t, sizeof(struct input_event));
				if (cnt != sizeof(struct input_event)){
					printf("cnt = %d, err(%d):%s\n", cnt, errno, strerror(errno));
					api_sleep_ms(10);
				}

				if (_key_event_is_valid(t)){
					//here may change the key type, for sometims
					//event0 may be IR, may be GPIO or ADC key.
					if (0 == i)
						key_msg.key_type = KEY_TYPE_IR;
					else if (1 == i)
						key_msg.key_type = KEY_TYPE_ADC;
					else if (2 == i)
						key_msg.key_type = KEY_TYPE_GPIO;
					key_queue_write(&m_key_queue, &key_msg);

				}

			}
		}
		api_sleep_ms(5);
	}
	for (i = 0; i < input_cnt; i ++){
		if (key_fds[i] > 0)
		close(key_fds[i]);
	}
	free(pfd);

	return 0;	
}


void api_key_get_init()
{
	pthread_t thread_id = 0;
    pthread_attr_t attr;    

    key_queue_init(&m_key_queue);
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, _key_task, NULL)) {
        return;
    }
    pthread_attr_destroy(&attr);
}

KEY_MSG_s *api_key_msg_get(void)
{
	KEY_MSG_s *key_msg = NULL;

	if (key_queue_read(&m_key_queue, &key_msg))
		return key_msg;
	else
		return NULL;
}

void api_key_queue_send(uint32_t act_key)
{
	KEY_MSG_s key_msg;
	memset(&key_msg, 0, sizeof(KEY_MSG_s));
	
	key_msg.key_type = KEY_TYPE_IR;
	key_msg.key_event.type = EV_KEY;
	key_msg.key_event.code = act_key;
	key_msg.key_event.value = 1;// pressed
	key_queue_write(&m_key_queue,&key_msg);
}

#endif