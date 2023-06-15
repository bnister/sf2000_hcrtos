#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h> //uint32_t
#include <stdlib.h>

#include "app_config.h"
//#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/dis.h>
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include "dummy_api.h"
#include "screen.h"
#include "factory_setting.h"
#include "com_api.h"
#include "tv_sys.h"
#include "osd_com.h"

#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#endif

#define NODE_ID_PROJECTOR  PERSISTENTMEM_NODE_ID_CASTAPP

static sys_param_t sys_param;

static int _sys_get_sysdata(struct sysdata *sysdata)
{
	int fd;
	struct persistentmem_node node;

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static int _sys_get_sysdata_adc_adjust_value(uint8_t *value)
{
	struct sysdata sysdata;

	if (!_sys_get_sysdata(&sysdata)) {
		*value = sysdata.adc_adjust_value;
		return 0;
	}

	return -1;
}


void projector_factory_init(void)
{
    struct sysdata sysdata; //backup

    memcpy(&sysdata, &sys_param.sys_data, sizeof(struct sysdata));

#ifdef BLUETOOTH_SUPPORT		
	struct bluetooth_slave_dev dev;
	memcpy(&dev,&sys_param.app_data.soundset.bt_dev,  sizeof(struct bluetooth_slave_dev));
#endif	
    memset(&sys_param, 0, sizeof(sys_param));
    memcpy(&sys_param.sys_data, &sysdata,  sizeof(struct sysdata));
#ifdef BLUETOOTH_SUPPORT			
	memcpy(&sys_param.app_data.soundset.bt_dev, &dev, sizeof(struct bluetooth_slave_dev));
#endif

    /* Boot parameters init */
    //sys_param.sys_data.volume = 50;
    sys_param.sys_data.tvtype = TV_LINE_800X480_60;
    sys_param.sys_data.ota_detect_modes = HCFOTA_REBOOT_OTA_DETECT_NONE;
	sys_param.app_data.vmotor_count = 0;
    /* Projector parameters init */
	sys_param.app_data.volume = 50;
#ifdef  MAIN_PAGE_SUPPORT	
	sys_param.app_data.cur_channel = SCREEN_CHANNEL_MAIN_PAGE;
#else
    sys_param.app_data.cur_channel = SCREEN_CHANNEL_MP;
#endif	
    sys_param.sys_data.flip_mode = FLIP_MODE_REAR;
    sys_param.app_data.pictureset.picture_mode = PICTURE_MODE_STANDARD;
	sys_param.app_data.pictureset.contrast = 50;
	sys_param.app_data.pictureset.brightness = 50;
	sys_param.app_data.pictureset.sharpness = 5;
	sys_param.app_data.pictureset.color = 50;
	sys_param.app_data.pictureset.hue = 50;
	sys_param.app_data.pictureset.color_temp = COLOR_TEMP_STANDARD;
	sys_param.app_data.pictureset.noise_redu = NOISE_REDU_OFF;

	sys_param.app_data.soundset.sound_mode = SOUND_MODE_STANDARD;
	sys_param.app_data.soundset.balance = 0;
	sys_param.app_data.soundset.bt_setting = BLUETOOTH_OFF;

	
	sys_param.app_data.soundset.treble = 0;
	sys_param.app_data.soundset.bass = 0;
#ifdef BLUETOOTH_SUPPORT	
	
	memset(&sys_param.app_data.soundset.bt_dev, 0 , sizeof(struct bluetooth_slave_dev));
#endif	

	sys_param.app_data.optset.osd_language = LANGUAGE_CHINESE;
	sys_param.app_data.optset.aspect_ratio = DIS_TV_AUTO;
	sys_param.app_data.optset.keystone_top_w = 0;
	sys_param.app_data.optset.keystone_bottom_w = 0;
	sys_param.app_data.optset.auto_sleep = AUTO_SLEEP_OFF;
	sys_param.app_data.optset.osd_time = OSD_TIME_15S;


    //sys_param.sys_data.tvtype = TV_LINE_1080_60;
    //sys_param.sys_data.volume = 70;
	//cast setting
#ifdef WIFI_SUPPORT	
	memset(sys_param.app_data.cast_setting.wifi_ap, 0, sizeof(hccast_wifi_ap_info_t)*MAX_WIFI_SAVE);
	sys_param.app_data.cast_setting.wifi_onoff = 0;
	sys_param.app_data.cast_setting.wifi_auto_conn = 0;
#endif	
	sys_param.app_data.cast_setting.browserlang = 2;
	sys_param.app_data.cast_setting.mirror_frame = 1;
	sys_param.app_data.cast_setting.mirror_mode = 1;
	sys_param.app_data.cast_setting.aircast_mode = 2;//Auto.
	sys_param.app_data.resolution = APP_TV_SYS_AUTO;//APP_TV_SYS_1080P;
	sys_param.app_data.cast_setting.mirror_rotation = 0;//default disable.
	sys_param.app_data.cast_setting.mirror_vscreen_auto_rotation = 1;//enable disable.

    sys_param.app_data.cast_setting.wifi_mode = 2; // 1: 2.4G, 2: 5G, 3: 60G (res)
    sys_param.app_data.cast_setting.wifi_ch   = HOSTAP_CHANNEL_24G;
    sys_param.app_data.cast_setting.wifi_ch5g = HOSTAP_CHANNEL_5G;

	sys_param.app_data.cast_setting.cast_dev_name_changed = 0;
	memset(sys_param.app_data.cast_setting.cast_dev_name, 0,MAX_DEV_NAME );
	snprintf(sys_param.app_data.cast_setting.cast_dev_psk,MAX_DEV_PSK,"%s",DEVICE_PSK);
#ifdef SYS_ZOOM_SUPPORT
	sys_param.app_data.scale_setting.dis_mode = DIS_TV_AUTO;
	sys_param.app_data.scale_setting.zoom_out_count = 0;
#endif
}

/* factory reset  */
void projector_factory_reset(void)
{
	int fd;
	int node_id;
	struct persistentmem_node node;

	printf("%s(), line:%d.\n", __func__, __LINE__);
	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("Open /dev/persistentmem failed (%d)\n", fd);
		return;
	}

	node_id = NODE_ID_PROJECTOR;
    if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_DELETE, node_id) < 0) {
        printf("%s(), line:%d. delete app data failed\n", __func__, __LINE__);
    }

	sys_param.sys_data.flip_mode = FLIP_MODE_REAR;
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sys_param.sys_data;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		printf("Stroe sys_data failed\n");
	}

	close(fd);
}

static int sys_get_sys_data(struct sysdata *sys_data)
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	int fd;

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		printf("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}

    node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
    node.offset = 0;
    node.size = sizeof(struct sysdata);
	node.buf = sys_data;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
        new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
        new_node.size = sizeof(struct sysdata);
        if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
            printf("create sys_data failed\n");
            close(fd);
            return -1;
        }

		node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		node.offset = 0;
		new_node.size = sizeof(struct sysdata);
		node.buf = sys_data;
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
			printf("Create&Store app_data failed\n");
			close(fd);
			return -1;
		}
		close(fd);

		return 0;
	}

	close(fd);
	return 0;

}


/* Load factory setting of system */
int projector_sys_param_load(void)
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
       int fd;

       sys_get_sys_data(&sys_param.sys_data);

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		printf("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}

	node.id = NODE_ID_PROJECTOR;
	node.offset = 0;
	node.size = sizeof(struct appdata);
	node.buf = &sys_param.app_data;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
            new_node.id = NODE_ID_PROJECTOR;
            new_node.size = sizeof(struct appdata);
            if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
                printf("get sys_data failed\n");
                close(fd);
                return -1;
            }

		node.id = NODE_ID_PROJECTOR;
		node.offset = 0;
		node.size = sizeof(struct appdata);
		node.buf = &sys_param.app_data;
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
			printf("Create&Store app_data failed\n");
			close(fd);
			return -1;
		}
		close(fd);

		return 0;
	}

	close(fd);
	return 0;
}

/* store projector system parameters to flash */
int projector_sys_param_save(void)
{
        int fd;
        struct persistentmem_node node;
		uint8_t adjust = 0;
		
		//the adc driver may change che adc adjust value and save to flash,
		//So here we should sync the adc adjust value in application to avoid overwrite
		//the value.
		if(!_sys_get_sysdata_adc_adjust_value(&adjust))// tmp proc  
			sys_param.sys_data.adc_adjust_value = adjust;
		
        fd = open("/dev/persistentmem", O_RDWR);

        if (fd > 0){
			node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
			node.offset = 0;
			node.size = sizeof(struct sysdata);
			node.buf = &sys_param.sys_data;
			if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
				printf("Stroe sys_data failed\n");
				close(fd);
				return -1;
			}
			node.id = NODE_ID_PROJECTOR;
			node.offset = 0;
			node.size = sizeof(struct appdata);
			node.buf = &sys_param.app_data;
			if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
				printf("Store app_data failed\n");
				close(fd);
				return -1;
			}
        }
		else	{
			printf("Open /dev/persistentmem failed (%d)\n", fd);
			return -1;
		}

        close(fd);
        return 0;
}

sys_param_t * projector_get_sys_param(void)
{
    return &sys_param;
}

#ifdef BLUETOOTH_SUPPORT
unsigned char* projector_get_bt_mac(){
	return sys_param.app_data.soundset.bt_dev.mac;
}

char* projector_get_bt_name(){
	return sys_param.app_data.soundset.bt_dev.name;
}

struct bluetooth_slave_dev *projector_get_bt_dev(){
	return &sys_param.app_data.soundset.bt_dev;
}

void projector_set_bt_dev(struct bluetooth_slave_dev *dev){
	memcpy(&sys_param.app_data.soundset.bt_dev, dev, sizeof(struct bluetooth_slave_dev));
}
#endif

char* projector_get_version_info(){
	static char version_info_v[32] = {0};
	#ifdef LVGL_RESOLUTION_240P_SUPPORT
		unsigned int ver = (unsigned int)sys_param.sys_data.firmware_version;
		snprintf(version_info_v, sizeof(version_info_v), "%u.%u.%u", ver/100000000, (ver%100000000)/1000000, (ver%1000000)/10000);
	#else
		snprintf(version_info_v, sizeof(version_info_v), "%s-%u", sys_param.sys_data.product_id, (unsigned int)sys_param.sys_data.firmware_version);
	#endif

	return version_info_v;
}

 int projector_get_some_sys_param(projector_sys_param param){
	switch (param){
		case P_PICTURE_MODE:
			return sys_param.app_data.pictureset.picture_mode;
		case P_CONTRAST:
			return sys_param.app_data.pictureset.contrast;
		case P_BRIGHTNESS:
			return sys_param.app_data.pictureset.brightness;
		case P_SHARPNESS:
			return sys_param.app_data.pictureset.sharpness;
		case P_COLOR:
			return sys_param.app_data.pictureset.color;
		case P_HUE:
			return sys_param.app_data.pictureset.hue;
		case P_COLOR_TEMP:
			return sys_param.app_data.pictureset.color_temp;
		case P_NOISE_REDU:
			return sys_param.app_data.pictureset.noise_redu;
		case P_SOUND_MODE:
			return sys_param.app_data.soundset.sound_mode;
		case P_BALANCE:
			return sys_param.app_data.soundset.balance;
		case P_BT_SETTING:
			return sys_param.app_data.soundset.bt_setting;
		case P_TREBLE:
			return sys_param.app_data.soundset.treble;
		case P_BASS:
			return sys_param.app_data.soundset.bass;
		case P_OSD_LANGUAGE:
			return sys_param.app_data.optset.osd_language;
		case P_ASPECT_RATIO:
			return sys_param.app_data.optset.aspect_ratio;
		case P_CUR_CHANNEL:
			return sys_param.app_data.cur_channel;
		case P_FLIP_MODE:
			return sys_param.sys_data.flip_mode;
		case P_VOLUME:
			return sys_param.app_data.volume;
		case P_KEYSTONE_TOP:
			return sys_param.app_data.optset.keystone_top_w;
		case P_KEYSTOME_BOTTOM:
			return sys_param.app_data.optset.keystone_bottom_w;
		case P_AUTOSLEEP:
			return sys_param.app_data.optset.auto_sleep;
		case P_OSD_TIME:
			return sys_param.app_data.optset.osd_time;
			break;
		case P_DEV_PRODUCT_ID:
			return (int)(sys_param.sys_data.product_id);
			break;
		case P_DEV_VERSION:
			return (int)(sys_param.sys_data.firmware_version);
			break;
		case P_MIRROR_MODE:
			return sys_param.app_data.cast_setting.mirror_mode;
			break;
		case P_AIRCAST_MODE:
			return sys_param.app_data.cast_setting.aircast_mode;
			break;
		case P_MIRROR_FRAME:
			return sys_param.app_data.cast_setting.mirror_frame;
			break;
		case P_BROWSER_LANGUAGE:
			return sys_param.app_data.cast_setting.browserlang;
			break;
		case P_SYS_RESOLUTION:
			return sys_param.app_data.resolution;
#ifdef WIFI_SUPPORT
		case P_DEVICE_NAME:
		    if (sys_param.app_data.cast_setting.cast_dev_name[0] == 0 && 
		    	sys_param.app_data.cast_setting.cast_dev_name[1] == 0)
		    {
		        sysdata_init_device_name();
		    }
			return (int)sys_param.app_data.cast_setting.cast_dev_name;
			break;
		case P_WIFI_ONOFF:
			return (int)sys_param.app_data.cast_setting.wifi_onoff;
			break;
#endif			
		case P_DEVICE_PSK:
			return (int)sys_param.app_data.cast_setting.cast_dev_psk;
			break;
		case P_WIFI_MODE:
			return sys_param.app_data.cast_setting.wifi_mode;
			break;
		case P_WIFI_CHANNEL:
            if (1 == sys_param.app_data.cast_setting.wifi_mode)
            {
                return sys_param.app_data.cast_setting.wifi_ch;
            }
            else if (2 == sys_param.app_data.cast_setting.wifi_mode)
            {
                return sys_param.app_data.cast_setting.wifi_ch5g;
            }
            return sys_param.app_data.cast_setting.wifi_ch;
            break;
        case P_WIFI_CHANNEL_24G:
            return sys_param.app_data.cast_setting.wifi_ch;
        case P_WIFI_CHANNEL_5G:
            return sys_param.app_data.cast_setting.wifi_ch5g;
		case P_MIRROR_ROTATION:
			return sys_param.app_data.cast_setting.mirror_rotation;
			break;
		case P_MIRROR_VSCREEN_AUTO_ROTATION:
			return sys_param.app_data.cast_setting.mirror_vscreen_auto_rotation;
			break;	
		case P_DE_TV_SYS:
			return sys_param.sys_data.tvtype;
			break;
		case P_VMOTOR_COUNT:
			return sys_param.app_data.vmotor_count;
			break;
#ifdef SYS_ZOOM_SUPPORT
		case P_SYS_ZOOM_DIS_MODE:
			return sys_param.app_data.scale_setting.dis_mode;
			break;
		case P_SYS_ZOOM_OUT_COUNT:
			return sys_param.app_data.scale_setting.zoom_out_count;
			break;
#endif
		default:
			break;
	}
	return -1;
}

void projector_set_some_sys_param(projector_sys_param param, int v){
	switch (param){
		case P_PICTURE_MODE:
			sys_param.app_data.pictureset.picture_mode = v;
			break;
		case P_CONTRAST:
			sys_param.app_data.pictureset.contrast = v;
			break;
		case P_BRIGHTNESS:
			sys_param.app_data.pictureset.brightness = v;
			break;
		case P_SHARPNESS:
			sys_param.app_data.pictureset.sharpness = v;
			break;
		case P_COLOR:
			sys_param.app_data.pictureset.color = v;
			break;
		case P_HUE:
			sys_param.app_data.pictureset.hue = v;
			break;
		case P_COLOR_TEMP:
			sys_param.app_data.pictureset.color_temp = v;
			break;
		case P_NOISE_REDU:
			sys_param.app_data.pictureset.noise_redu = v;
			break;
		case P_SOUND_MODE:
			sys_param.app_data.soundset.sound_mode = v;
			break;
		case P_BALANCE:
			sys_param.app_data.soundset.balance = v;
			break;
		case P_BT_SETTING:
			sys_param.app_data.soundset.bt_setting = v;
			break;
		case P_TREBLE:
			sys_param.app_data.soundset.treble = v;
			break;
		case P_BASS:
			sys_param.app_data.soundset.bass = v;
			break;
		case P_OSD_LANGUAGE:
			sys_param.app_data.optset.osd_language = v;
			break;
		case P_ASPECT_RATIO:
			sys_param.app_data.optset.aspect_ratio = v;
			break;
		case P_CUR_CHANNEL:
			sys_param.app_data.cur_channel = v;
			break;
		case P_FLIP_MODE:
			sys_param.sys_data.flip_mode = v;
			break;
		case P_VOLUME:
			sys_param.app_data.volume = v;
			break;
		case P_KEYSTONE_TOP:
			sys_param.app_data.optset.keystone_top_w = v;
			break;
		case P_KEYSTOME_BOTTOM:
			sys_param.app_data.optset.keystone_bottom_w = v;
			break;
		case P_AUTOSLEEP:
			sys_param.app_data.optset.auto_sleep = v;
			break;
		case P_OSD_TIME:
			sys_param.app_data.optset.osd_time = v;
			break;
		case P_MIRROR_MODE:
			sys_param.app_data.cast_setting.mirror_mode = v;
			break;
		case P_AIRCAST_MODE:
			sys_param.app_data.cast_setting.aircast_mode = v;
			break;
		case P_MIRROR_FRAME:
			sys_param.app_data.cast_setting.mirror_frame = v;
			break;
		case P_BROWSER_LANGUAGE:
			sys_param.app_data.cast_setting.browserlang = v;
			break;
		case P_SYS_RESOLUTION:
			sys_param.app_data.resolution = v;
			break;
		case P_DEVICE_NAME:
			sys_param.app_data.cast_setting.cast_dev_name_changed = 1;
			strncpy(sys_param.app_data.cast_setting.cast_dev_name, (char*)v, MAX_DEV_NAME);
			break;
		case P_DEVICE_PSK:
			strncpy(sys_param.app_data.cast_setting.cast_dev_psk, (char*)v, MAX_DEV_PSK);
			break;
		case P_WIFI_MODE:
			sys_param.app_data.cast_setting.wifi_mode = v;
			break;
		case P_WIFI_CHANNEL:
            if (1 == sys_param.app_data.cast_setting.wifi_mode) // 24G
            {
                sys_param.app_data.cast_setting.wifi_ch = v;
            }
            else if (2 == sys_param.app_data.cast_setting.wifi_mode) // 5G
            {
                sys_param.app_data.cast_setting.wifi_ch5g = v;
            }
            else
            {
                sys_param.app_data.cast_setting.wifi_ch = v;
            }
			break;
		case P_MIRROR_ROTATION:
			sys_param.app_data.cast_setting.mirror_rotation = v;
			break;
		case P_MIRROR_VSCREEN_AUTO_ROTATION:
			sys_param.app_data.cast_setting.mirror_vscreen_auto_rotation = v;
			break;
		case P_VMOTOR_COUNT:
			sys_param.app_data.vmotor_count = v;
			break;
#ifdef WIFI_SUPPORT
		case P_WIFI_ONOFF:
			if(v!=0 && v!= 1){
				break;
			}
			sys_param.app_data.cast_setting.wifi_onoff = v;
			break;
#endif
#ifdef SYS_ZOOM_SUPPORT
		case P_SYS_ZOOM_DIS_MODE:
		 	sys_param.app_data.scale_setting.dis_mode = v;
			break;
		case P_SYS_ZOOM_OUT_COUNT:
			sys_param.app_data.scale_setting.zoom_out_count = v;
			break;
#endif
		default:
			break;
	}
}



#ifdef WIFI_SUPPORT

static int8_t save_flag = 0;// save_flag == 1, mean called func sysdata_wifi_ap_save(hccast_wifi_ap_info_t *)

void set_save_wifi_flag_zero(){
	save_flag = 0;
}

int8_t get_save_wifi_flag(){
	return save_flag;
}


int sysdata_check_ap_saved(hccast_wifi_ap_info_t* check_wifi)
{
	int i = 0;
	int index = -1;
	
	for(i = 0; i < MAX_WIFI_SAVE; i++)
	{
		if(strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid) && strlen(check_wifi->ssid))
		{
			if(strcmp(check_wifi->ssid, sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0)
			{
				index = i;
				return index;
			}
		}	
	}
	
	return index;
}


void sysdata_wifi_ap_save(hccast_wifi_ap_info_t *wifi_ap)
{
    int8_t i = 0;

    for(i = MAX_WIFI_SAVE-2; i >= 0; i--)
    {
        memcpy(&(sys_param.app_data.cast_setting.wifi_ap[i+1]), &(sys_param.app_data.cast_setting.wifi_ap[i]), \
        	sizeof(hccast_wifi_ap_info_t));
    }
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn>>1;
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn | (0x8000);//
    memcpy(sys_param.app_data.cast_setting.wifi_ap, wifi_ap, sizeof(hccast_wifi_ap_info_t));
	save_flag = 1;
	printf("auto_conn save: %x\n", sys_param.app_data.cast_setting.wifi_auto_conn);
}

void sysdata_wifi_ap_set_auto(int index){
	if(index<0 && index>=MAX_WIFI_SAVE){
		return;
	}
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn | (0x8000>>index);
}

void sysdata_wifi_ap_set_nonauto(int index){
	if(index<0 && index>=MAX_WIFI_SAVE){
		return;
	}
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn & ((0x8000>>index)^0xffff);
}

bool sysdata_wifi_ap_get_auto(int index){
	if(index<0 && index>=MAX_WIFI_SAVE){
		return false;
	}
	if (sys_param.app_data.cast_setting.wifi_auto_conn & (0x8000>>index))
		return true;
	else
		return false;
}

void sysdata_wifi_ap_delete(int index)
{
    int i = 0;

    if(index > MAX_WIFI_SAVE-1)
    {
        return;
    }

    for(i = index; i < MAX_WIFI_SAVE-1; i++)
    {
        memcpy(&(sys_param.app_data.cast_setting.wifi_ap[i]), &(sys_param.app_data.cast_setting.wifi_ap[i+1]), \
        	sizeof(hccast_wifi_ap_info_t));
    }
	sys_param.app_data.cast_setting.wifi_auto_conn = (((0xffff>>(index+1))&sys_param.app_data.cast_setting.wifi_auto_conn)<<1)
	 | ((~(0xffff>>index)) & sys_param.app_data.cast_setting.wifi_auto_conn);//去除被删除wifi对应的自动连接位
    memset(&(sys_param.app_data.cast_setting.wifi_ap[MAX_WIFI_SAVE-1]),0x00,sizeof(hccast_wifi_ap_info_t));
	printf("auto_conn delete: %x\n", sys_param.app_data.cast_setting.wifi_auto_conn);
}

hccast_wifi_ap_info_t *sysdata_get_wifi_info(char* ssid)
{
	int i = 0;
	
	for(i = 0; i < MAX_WIFI_SAVE; i++)
	{
		if(strcmp(ssid, sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0)
		{
			return &sys_param.app_data.cast_setting.wifi_ap[i];
		}
	}
	
	return NULL;
}

hccast_wifi_ap_info_t *sysdata_get_wifi_info_by_index(int i){

    if(i > MAX_WIFI_SAVE-1 || strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0)
    {
        return NULL;
    }

	 return &sys_param.app_data.cast_setting.wifi_ap[i];
}

int sysdata_get_saved_wifi_count(){
	int i=0;
	for(; MAX_WIFI_SAVE>i && strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid)>0;i++);
	return i;
}

int sysdata_get_wifi_index_by_ssid(char *ssid){
	for(int i = 0; i < MAX_WIFI_SAVE; i++){
		if(ssid && strcmp(ssid,sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0){
			return i;
		}
	}
	return -1;
}

bool sysdata_wifi_ap_get(hccast_wifi_ap_info_t *wifi_ap)
{
	for(int i= 0; i<MAX_WIFI_SAVE;i++){
		if(!sysdata_wifi_ap_get_auto(i)){
			continue;
		}
		if (strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid)){
			memcpy(wifi_ap, sys_param.app_data.cast_setting.wifi_ap+i, sizeof(hccast_wifi_ap_info_t));
			if(i>0){
				sysdata_wifi_ap_delete(i);
				sysdata_wifi_ap_save(wifi_ap);//移到第0个
				//sys_param.app_data.cast_setting.wifi_auto_conn = j | (sys_param.app_data.cast_setting.wifi_auto_conn & (0xffff>>1));//交换wifi_auto_conn的第0位和第i位
				projector_sys_param_save();				
			}

			return true;
		}
		else{
			return false;
		}
	}
	return false;
        
}

#endif



void sysdata_app_tv_sys_set(app_tv_sys_t app_tv_sys)
{
    int tv_sys;

    sys_param.app_data.resolution = app_tv_sys;

    if (APP_TV_SYS_AUTO == app_tv_sys)
        tv_sys = tv_sys_best_tv_type_get();    
    else
        tv_sys = tv_sys_app_sys_to_de_sys(app_tv_sys);
    sys_param.sys_data.tvtype = tv_sys;
}

#ifdef WIFI_SUPPORT
extern int api_get_mac_addr(char *mac);
int sysdata_init_device_name(void)
{
    unsigned char mac[6] = {0};
    int rand_num = 0;

    wifi_cast_setting_t *cast_set = &(sys_param.app_data.cast_setting);

    if (api_get_mac_addr((char*)mac) == 0){
        if (!cast_set->cast_dev_name_changed && memcmp(mac, cast_set->mac_addr, MAC_ADDR_LEN)){
            snprintf(cast_set->cast_dev_name, MAX_DEV_NAME, "%s-%02X%02X%02X", 
                SSID_NAME, mac[3]&0xff, mac[4]&0xff, mac[5]&0xff);   
            memcpy(cast_set->mac_addr, mac, MAC_ADDR_LEN);
            projector_sys_param_save();
        }
    } else {
        if (cast_set->cast_dev_name[0] == 0 && cast_set->cast_dev_name[1] == 0){
            printf("%s get netif addr failed!Use rand value.\n", __FUNCTION__);
            srand(time(NULL));
            rand_num = rand();
            snprintf(cast_set->cast_dev_name, MAX_DEV_NAME, "%s_%06X", SSID_NAME, rand_num&0xffffff);
            projector_sys_param_save();
        }
    }
    printf("%s device_name=%s\n", __func__, cast_set->cast_dev_name);
    return 0;
}
#endif

// void projector_sys_param_init(){

// }

