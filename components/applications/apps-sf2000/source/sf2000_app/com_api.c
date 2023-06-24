#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>
#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
#include <ffplayer.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/watchdog.h>
#include <linux/fb.h>
#else
#include <hcuapi/watchdog.h>
#include <kernel/fb.h>
#include <sys/ioctl.h>

#include <kernel/io.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/sys-blocking-notify.h>
#include <freertos/FreeRTOS.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/fb.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#include <sys/mount.h>
#endif

#endif
#include <hcuapi/fb.h>
#include <hcuapi/standby.h>
#include <dirent.h>

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "com_api.h"
#include "os_api.h"

static uint32_t m_control_msg_id = INVALID_ID;
static bool m_ffplay_init = false;


static uint16_t screen_init_rotate,screen_init_h_flip,screen_init_v_flip;
void api_get_screen_rotate_info(void)
{
    unsigned int rotate=0,h_flip=0,v_flip=0;
#ifdef __HCRTOS__
	int np;
	np = fdt_node_probe_by_path("/hcrtos/rotate");
	if(np>=0)
	{
		fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
		fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
		fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
		screen_init_rotate = rotate;
		screen_init_h_flip = h_flip;
		screen_init_v_flip = v_flip;
	}
	else
	{
		screen_init_rotate = 0;
		screen_init_h_flip = 0;
		screen_init_v_flip = 0;
	}
#else
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
	char status[16] = {0};
	api_dts_string_get(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
	if(!strcmp(status, "okay")){
		rotate = api_dts_uint32_get(ROTATE_CONFIG_PATH "/rotate");
		h_flip = api_dts_uint32_get(ROTATE_CONFIG_PATH "/h_flip");
		v_flip = api_dts_uint32_get(ROTATE_CONFIG_PATH "/v_flip");
	}else{
		rotate = 0;
		h_flip = 0;
		v_flip = 0;
	}
	screen_init_rotate = rotate;
	screen_init_h_flip = h_flip;
	screen_init_v_flip = v_flip;
#endif
	printf("->>> init_rotate = %u h_flip %u v_flip = %u\n",rotate,h_flip,v_flip);
}

uint16_t api_get_screen_init_rotate(void)
{
    return screen_init_rotate;
}

uint16_t api_get_screen_init_h_flip(void)
{
    return screen_init_h_flip;
}

uint16_t api_get_screen_init_v_flip(void)
{
    return screen_init_v_flip;
}
int api_system_init()
{
    if (!api_watchdog_init()){
        api_watchdog_timeout_set(30000);
        api_watchdog_start();
        uint32_t timeout_ms = 0;;
        api_watchdog_timeout_get(&timeout_ms);
        printf("%s(), line:%d. set watchdog timeout: %d ms\n", __func__, __LINE__, timeout_ms);
    }

    api_get_screen_rotate_info();

    return 0;
}

int api_video_init()
{
    return 0;
}

int api_audio_init()
{
    return 0;
}

int api_get_flip_info(int *rotate_type, int *flip_type)
{
   
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    int init_rotate = api_get_screen_init_rotate();
    int init_h_flip = api_get_screen_init_h_flip();
    int init_v_flip = api_get_screen_init_v_flip();

    api_get_rotate_by_flip_mode(1 ,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    api_transfer_rotate_mode_for_screen(
        init_rotate,init_h_flip,init_v_flip,
        &rotate , &h_flip , &v_flip , NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}

int api_lvgl_init(int width, int height)
{
    extern int hc_lvgl_init(void);
    return hc_lvgl_init();
}

#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#include <sys/mount.h>
#endif

#endif



static void *m_logo_player = NULL;
static char m_logo_file[128] = {0};

static int api_logo_off_no_black()
{
#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
    if (m_logo_player){
        hcplayer_stop2(m_logo_player, 0, 0);
    }
#endif   
    m_logo_player = NULL;
    return 0;

}

int api_logo_reshow(void)
{
    if (m_logo_player && strlen(m_logo_file))
    {
        api_logo_off_no_black();
        api_logo_show(m_logo_file);
    }

    return 0;
}
int api_logo_show(const char *file)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return 0;
#else


    char *file_path = file;

    api_logo_off();

    if (!file)
        file_path = MUSIC_LOGO;

    strcpy(m_logo_file, file_path);

    HCPlayerInitArgs player_args;
    memset(&player_args, 0, sizeof(player_args));
    player_args.uri = file_path;
    player_args.msg_id = 0;

    player_args.img_effect.mode = IMG_SHOW_NORMAL;  
#if 0    
    int rotate_type = ROTATE_TYPE_0;
    int flip_type = MIRROR_TYPE_NONE;
    api_get_flip_info(&rotate_type, &flip_type);
    player_args.rotate_type = rotate_type;
    player_args.mirror_type = flip_type;
    player_args.rotate_enable = 1;
#endif

    // if (data_mgr_cast_rotation_get())  
    //     player_args.rotate_type = ROTATE_TYPE_270; //90
    // player_args.rotate_enable = 1;

    m_logo_player = hcplayer_create(&player_args);
    if (!m_logo_player)
    {
        printf("hcplayer_create() fail!\n");
        return -1;
    }
    //not block play, create a task to play.
    //hcplayer_play(m_logo_player);

    //block play
    hcplayer_play2(m_logo_player);

    //stop
    //hcplayer_stop2(m_logo_player, false, false);
    printf("=============================Show logo: %s ok!=============================\n", file_path);

    return 0;
	#endif
}

int api_logo_off()
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return 0;
#else

    if (m_logo_player)
    {
        //hcplayer_stop(m_logo_player);
        hcplayer_stop2(m_logo_player,true,true);
        m_logo_player = NULL;
        printf("%s =============================Close logo: ok!=============================\n", __func__);
    }
    return 0;
	#endif
}

int api_logo_off2(int closevp, int fillblack)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return 0;
#else

    if (m_logo_player)
    {
        //hcplayer_stop(m_logo_player);
        hcplayer_stop2(m_logo_player,closevp,fillblack);
        m_logo_player = NULL;
        printf("%s =============================Close logo: ok!=============================\n", __func__);
    }
    return 0;
	#endif
}


/**
 * @brief turn on/off the video frame output
 *
 * @param on_off
 * @return int
 */
int api_dis_show_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = { 0 };

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }

    winon.distype = DIS_TYPE_HD;
    winon.layer =  DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;

    ioctl(fd, DIS_SET_WIN_ONOFF, &winon);
    close(fd);

    return 0;
}

int api_control_send_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id)
    {
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, control_msg, sizeof(control_msg_t));
}

int api_control_receive_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id)
    {
        return -1;
    }
    return api_message_receive_tm(m_control_msg_id, control_msg, sizeof(control_msg_t), 5);
}

int api_control_send_key(uint32_t key)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_KEY;
    control_msg.msg_code = key;

    if (INVALID_ID == m_control_msg_id)
    {
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, &control_msg, sizeof(control_msg_t));
}

/** check the string if it is IP address
 * @param
 * @return
 */
bool api_is_ip_addr(char *ip_buff)
{
    int ip1, ip2, ip3, ip4;
    char temp[64];
    if((sscanf(ip_buff,"%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4))!=4)
        return false;
    sprintf(temp,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
    if(strcmp(temp, ip_buff) != 0)
        return false;
    if(!((ip1 <= 255 && ip1 >= 0)&&(ip2 <= 255 && ip2 >= 0)&&(ip3 <= 255 && ip1 >= 0)))
        return false;
    else
        return true;
}

void app_ffplay_init(void)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return;
#else

    if (m_ffplay_init) return;

    hcplayer_init(5);
    m_ffplay_init = true;
#endif
}

void app_ffplay_deinit(void)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return;
#else
    hcplayer_deinit();
    m_ffplay_init = false;
#endif
}

/**
 * @brief linux system will send the exit signal, then release system resource here
 *
 */
void app_exit(void)
{
    if (INVALID_ID != m_control_msg_id)
        api_message_delete(m_control_msg_id);

    app_ffplay_deinit();
}

void api_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

#ifdef __linux__
int api_shell_exe_result(char *cmd)
{
    int ret = API_FAILURE;
    char result_buf[32];
    int result_val = -1;
    FILE *fp = NULL;

    //step1: excute the shell command
    system(cmd);

    //step2: get the excuting result
    fp = popen("echo $?", "r");
    if (fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        if('\n' == result_buf[strlen(result_buf)-1])
        {
            result_buf[strlen(result_buf)-1] = '\0';
        }

        sscanf(result_buf,"%d", &result_val);
    }
    pclose(fp);
    if (0 == result_val)
    {
        printf("Excute cmd: [%s] OK!\n", cmd);
        ret = API_SUCCESS;
    }
    else
    {
        printf("Excute cmd: [%s] fail, ret = %d!\n", cmd, result_val);
        ret = API_FAILURE;
    }

    return ret;
}
#endif

static unsigned int crc_table[256];
static volatile bool m_crc_init = false;
static void api_crc_table_init(void)
{
    unsigned int c;
    unsigned int i, j;

    if (m_crc_init)
        return;

    for (i = 0; i < 256; i++)
    {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++)
        {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[i] = c;
    }

    m_crc_init = true;
}

unsigned int api_crc32(unsigned int crc,unsigned char *buffer, unsigned int size)
{
    unsigned int i;

    api_crc_table_init();

    for (i = 0; i < size; i++)
    {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ;
}

int api_osd_show_onoff(bool on_off)
{
    // Open the file for reading and writing
    int fbfd = open("/dev/fb0", O_RDWR);
    uint32_t blank_mode;

    if(fbfd == -1)
    {
        printf("%s(), line: %d. Error: cannot open framebuffer device", __func__, __LINE__);
        return API_FAILURE;
    }

    if (on_off)
        blank_mode = FB_BLANK_UNBLANK;
    else
        blank_mode = FB_BLANK_NORMAL;

    if (ioctl(fbfd, FBIOBLANK, blank_mode) != 0)
    {
        printf("%s(), line: %d. Error: FBIOBLANK", __func__, __LINE__);
    }

    close(fbfd);
    return API_SUCCESS;
}


void api_system_reboot(void)
{
    printf("%s(): reboot now!!\n", __func__);
#ifdef __linux__
    system("reboot");
#else
    extern int reset(void);
    reset();
#endif

    while(1);
}

/*
we can wake up by 2 ways: ir and sacadc key.
 */
void api_system_standby(void)
{
    int fd = -1;

    fd = open("/dev/standby", O_RDWR);
    if (fd < 0)
    {
        printf("%s(), line:%d. open standby device error!\n", __func__, __LINE__);
        return;
    }

    printf("%s(), line:%d.\n", __func__, __LINE__);

    //step 1: off the display
    api_osd_show_onoff(false);
    api_logo_off();
    api_dis_show_onoff(false);
    //sleep for a while so that hardware display is really off.
    api_sleep_ms(100);

    //Step 2: off other devices

    //Step 3: config the standby wake up methods

    api_watchdog_stop();


    //config wake up ir scancode(so far, default is power key:28)
    //check hclinux\SOURCE\linux-drivers\drivers\hcdrivers\rc\keymaps\rc-hcdemo.c
    //for scan code.
    struct standby_ir_setting ir = { 0 };
    ir.num_of_scancode = 1;
    ir.scancode[0] = 28;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_IR, (unsigned long)&ir);

#if 0
    //config wake up adc key
    struct standby_saradc_setting adc = { 0 };
    adc.channel = 1;
    adc.min = 1000;
    adc.max = 1500;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_SARADC, (unsigned long)&adc);
#endif

    //Step 4: entering system standby
    ioctl(fd, STANDBY_ENTER, 0);
    close(fd);
    while(1);
}

static volatile int m_osd_off_time_cnt = 0;
#define ONE_COUNT_TIME  200
static void *osd_off_for_time(void *arg)
{
    uint32_t timeout = (uint32_t)arg;
    m_osd_off_time_cnt = timeout/ONE_COUNT_TIME;
    do
    {
        api_sleep_ms(ONE_COUNT_TIME);
    }
    while(m_osd_off_time_cnt --);

    m_osd_off_time_cnt = 0;

    api_osd_show_onoff(true);

    return NULL;
}

//Turn off OSD for a time, then turn on OSD.
//because sometimes enter dlna music play, the OSD is still show
//but the video screen is black(BUG #2848), so we turn off OSD for some time.
void api_osd_off_time(uint32_t timeout)
{

    //update the wait time
    if (m_osd_off_time_cnt)
    {
        int timeout_cnt = timeout/ONE_COUNT_TIME;
        if (timeout_cnt > m_osd_off_time_cnt)
            m_osd_off_time_cnt = timeout_cnt;

        return;
    }

    api_osd_show_onoff(false);
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, osd_off_for_time, (void*)timeout))
    {
        return;
    }
}

void api_set_aspect_mode(dis_tv_mode_e ratio, 
                            dis_mode_e dis_mode, 
                            dis_scale_avtive_mode_e active_mode)
{
    int fd = open("/dev/dis", O_RDWR);
    if ( fd < 0)
        return;
    dis_aspect_mode_t aspect = {0};
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    aspect.active_mode = dis_mode;
    ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}


typedef void *(player_get_func)(void);
static player_get_func *ffmpeg_player_get = NULL;
void *api_ffmpeg_player_get(void)
{
    if (ffmpeg_player_get)
        return ffmpeg_player_get();
    else
        return NULL;
}


void api_ffmpeg_player_get_regist(void *(func)(void))
{
    ffmpeg_player_get = func;
}

void api_transfer_rotate_mode_for_screen(
                                        int init_rotate,
                                        int init_h_flip,
                                        int init_v_flip,
                                        int *p_rotate_mode ,
                                        int *p_h_flip ,
                                        int *p_v_flip,
                                        int *p_fbdev_rotate)
{
    int fbdev_rotate[4] = { 0,270,180,90 }; //setting is anticlockwise for fvdev
    int rotate = 0 , h_flip = 0 , v_flip = 0;

    rotate = *p_rotate_mode;

    //if screen is V screen,h_flip and v_flip exchange
    if(init_rotate == 0 || init_rotate == 180)
    {
        h_flip = *p_h_flip;
        v_flip = *p_v_flip;
    }
    else
    {
        h_flip = *p_v_flip;
        v_flip = *p_h_flip;
    }
 
    /*setting in dts is anticlockwise */
    /*calc rotate mode*/
    if(init_rotate == 270)
    {
        rotate = (rotate + 1) & 3;
    }
    else if(init_rotate == 90)
    {
        rotate = (rotate + 3) & 3;
    }
    else if(init_rotate == 180)
    {
        rotate = (rotate + 2) & 3;
    }

    /*transfer v_flip to h_flip with rotate
    *rotate 0 + H
    *rotate 0 + V--> rotate 180 +H
    *rotate 180 + H
    *rotate 180 + V --> rotate 0  + H 
    *rotate 90 + H
    *rotate 90 + V--> rotate 270 +H
    *rotate 270 +H
    *rotate 270 +V--> rotate 90 + H 
    */
    if(v_flip == 1)
    {
        switch(rotate)
        {
            case ROTATE_TYPE_0:
                rotate = ROTATE_TYPE_180;
                break;
            case ROTATE_TYPE_90:
                rotate = ROTATE_TYPE_270;
                break;
            case ROTATE_TYPE_180:
                rotate = ROTATE_TYPE_0;
                break;
            case ROTATE_TYPE_270:
                rotate = ROTATE_TYPE_90;
                break;
            default:
                break;
        }
        v_flip = 0;
        h_flip = 1;
    }

    h_flip = h_flip ^ init_h_flip;

    if(p_rotate_mode != NULL)
    {
        *p_rotate_mode = rotate;
    }
    
    if(p_h_flip != NULL)
    {
        *p_h_flip = h_flip;
    }
    
    if(p_v_flip != NULL)
    {
        *p_v_flip = 0;
    }
    
    if(p_fbdev_rotate !=  NULL)
    {
        *p_fbdev_rotate = fbdev_rotate[rotate];
    }
    


}

#ifdef __linux__
int api_dts_uint32_get(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = 0;;
    if(fd >= 0){
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    printf("dts value: %d\n", value);
    return value;
}


void api_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    int value = 0;;
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
    printf("dts string: %s\n", string);
}
#endif


static const char *m_dev_dog = "/dev/watchdog";
static int m_dog_fd = -1;

int api_watchdog_init(void)
{
    if (m_dog_fd >= 0)
        return 0;

    m_dog_fd = open(m_dev_dog, O_RDWR);
    if (m_dog_fd < 0) {
        printf("%s(), line:%d. No watchdog!!!\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}


//Set the timeout of watchdog , system would reboot if watchdog not feed
//within the timeout.
int api_watchdog_timeout_set(uint32_t timeout_ms)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__    
    ioctl(m_dog_fd, WDIOC_SETMODE, WDT_MODE_WATCHDOG);
    return ioctl(m_dog_fd, WDIOC_SETTIMEOUT, timeout_ms*1000);
#else
    uint32_t timeout_second = timeout_ms/1000;
    return ioctl(m_dog_fd, WDIOC_SETTIMEOUT, &timeout_second);
#endif    
}

int api_watchdog_timeout_get(uint32_t *timeout_ms)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__    
    uint32_t timeout_us = 0;
    if (ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_us))
        return -1;
    *timeout_ms = timeout_us/1000;
#else
    uint32_t timeout_second;
    if (ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_second))
        return -1;

    *timeout_ms = timeout_second*1000;

#endif    

    return 0;
}

int api_watchdog_start(void)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__        
    return ioctl(m_dog_fd, WDIOC_START, 0);
#else
    uint32_t val = WDIOS_ENABLECARD;
    return ioctl(m_dog_fd, WDIOC_SETOPTIONS, &val);
#endif
}

int api_watchdog_stop(void)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__        

    ioctl(m_dog_fd, WDIOC_STOP, 0);
#else    

    uint32_t val;

    ioctl(m_dog_fd, WDIOC_GETSTATUS, &val);
    printf("%s(), line: %d: watchdog status = %s \n", __func__, __LINE__,
        val ? "running" : "stop");

    uint32_t timeout_second;
    ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_second);
    printf("%s(), line: %d: watchdog timeout = %d \n", __func__, __LINE__, timeout_second);


    write(m_dog_fd, "V", 1);
    val = WDIOS_DISABLECARD;
    ioctl(m_dog_fd, WDIOC_SETOPTIONS, &val);

    ioctl(m_dog_fd, WDIOC_GETSTATUS, &val);
    printf("%s(), line: %d: watchdog status = %s \n", __func__, __LINE__,
           val ? "running" : "stop");


#endif
    return 0;
}


int api_watchdog_feed(void)
{
    if (m_dog_fd < 0)
        return -1;

    return ioctl(m_dog_fd, WDIOC_KEEPALIVE, 0);    
}

