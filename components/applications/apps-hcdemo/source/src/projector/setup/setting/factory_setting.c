#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

//#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/dis.h>
#include <kernel/elog.h>
#include <bluetooth.h>
#include "../../screen.h"
#include "../../factory_setting.h"
#include <kernel/lib/fdt_api.h>

#define NODE_ID_PROJECTOR  PERSISTENTMEM_NODE_ID_CASTAPP

static sys_param_t sys_param;

/* factory reset  */
void projector_factory_reset(void)
{
	u32 rotate=0,h_flip=0,v_flip=0;
	int np;

    memset(&sys_param, 0, sizeof(sys_param));
	np = fdt_node_probe_by_path("/hcrtos/rotate");
	if(np>=0)
	{
		fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
		fdt_get_property_u_32_index(np, "init_h_flip", 0, &h_flip);
		fdt_get_property_u_32_index(np, "init_v_flip", 0, &v_flip);
		sys_param.projector_sysdata.init_rotate = rotate;
		sys_param.projector_sysdata.init_h_flip = h_flip;
		sys_param.projector_sysdata.init_v_flip = v_flip;
	}
	else
	{
		sys_param.projector_sysdata.init_rotate = 0;
		sys_param.projector_sysdata.init_h_flip = 0;
		sys_param.projector_sysdata.init_v_flip = 0;
	}
	printf("->>> init_rotate = %u h_flip %u v_flip = %u\n",rotate,h_flip,v_flip);
    /* Boot parameters init */
    //sys_param.sysdata.volume = 50;
    sys_param.sysdata.tvtype = TV_LINE_800X480_60;
    sys_param.sysdata.ota_detect_modes = HCFOTA_REBOOT_OTA_DETECT_NONE;

    /* Projector parameters init */
	sys_param.projector_sysdata.volume = 50;
    sys_param.projector_sysdata.cur_channel = SCREEN_CHANNEL_MP;
    sys_param.sysdata.flip_mode = FLIP_MODE_NORMAL;
    sys_param.projector_sysdata.pictureset.picture_mode = PICTURE_MODE_STANDARD;
	sys_param.projector_sysdata.pictureset.contrast = 50;
	sys_param.projector_sysdata.pictureset.brightness = 50;
	sys_param.projector_sysdata.pictureset.sharpness = 5;
	sys_param.projector_sysdata.pictureset.color = 50;
	sys_param.projector_sysdata.pictureset.hue = 50;
	sys_param.projector_sysdata.pictureset.color_temp = COLOR_TEMP_STANDARD;
	sys_param.projector_sysdata.pictureset.noise_redu = NOISE_REDU_OFF;

	sys_param.projector_sysdata.soundset.sound_mode = SOUND_MODE_STANDARD;
	sys_param.projector_sysdata.soundset.balance = 0;
	sys_param.projector_sysdata.soundset.bt_setting = BLUETOOTH_OFF;
	sys_param.projector_sysdata.soundset.treble = 0;
	sys_param.projector_sysdata.soundset.bass = 0;
	memset(&sys_param.projector_sysdata.soundset.bt_dev, 0 , sizeof(struct bluetooth_slave_dev));

	sys_param.projector_sysdata.optset.osd_language = English;
	sys_param.projector_sysdata.optset.aspect_ratio = ASPECT_RATIO_AUTO;
	sys_param.projector_sysdata.optset.keystone_top_w = 0;
	sys_param.projector_sysdata.optset.keystone_bottom_w = 0;
	sys_param.projector_sysdata.optset.auto_sleep = AUTO_SLEEP_OFF;
    // to do later...
}

/* Load factory setting of system */
int projector_sys_param_load(void)
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
       int fd;

       sys_get_sysdata(&sys_param.sysdata);

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		log_e("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}

	node.id = NODE_ID_PROJECTOR;
	node.offset = 0;
	node.size = sizeof(struct projector_setting);
	node.buf = &sys_param.projector_sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
            new_node.id = NODE_ID_PROJECTOR;
            new_node.size = sizeof(struct projector_setting);
            if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
                printf("get sysdata failed\n");
                close(fd);
                return -1;
            }

		node.id = NODE_ID_PROJECTOR;
		node.offset = 0;
		node.size = sizeof(struct projector_setting);
		node.buf = &sys_param.projector_sysdata;
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
			printf("Create&Store projector_sysdata failed\n");
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
        fd = open("/dev/persistentmem", O_RDWR);

        if (fd >= 0){
			node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
				node.offset = 0;
				node.size = sizeof(struct sysdata);
				node.buf = &sys_param.sysdata;
				if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
					printf("Stroe sysdata failed\n");
					close(fd);
					return -1;
				}
			node.id = NODE_ID_PROJECTOR;
			node.offset = 0;
			node.size = sizeof(struct projector_setting);
			node.buf = &sys_param.projector_sysdata;
			if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
				printf("Store projector_sysdata failed\n");
				close(fd);
				return -1;
			}
        }
		else	{
			log_e("Open /dev/persistentmem failed (%d)\n", fd);
			return -1;
		}

        close(fd);
        return 0;
}

sys_param_t * projector_get_sys_param(void)
{
    return &sys_param;
}

unsigned char* projector_get_bt_mac(){
	return sys_param.projector_sysdata.soundset.bt_dev.mac;
}

char* projector_get_bt_name(){
	return sys_param.projector_sysdata.soundset.bt_dev.name;
}

struct bluetooth_slave_dev *projector_get_bt_dev(){
	return &sys_param.projector_sysdata.soundset.bt_dev;
}

void projector_set_bt_dev(struct bluetooth_slave_dev *dev){
	memcpy(&sys_param.projector_sysdata.soundset.bt_dev, dev, sizeof(struct bluetooth_slave_dev));
}

 int projector_get_some_sys_param(projector_sys_param param){
	switch (param){
		case P_PICTURE_MODE:
			return sys_param.projector_sysdata.pictureset.picture_mode;
		case P_CONTRAST:
			return sys_param.projector_sysdata.pictureset.contrast;
		case P_BRIGHTNESS:
			return sys_param.projector_sysdata.pictureset.brightness;
		case P_SHARPNESS:
			return sys_param.projector_sysdata.pictureset.sharpness;
		case P_COLOR:
			return sys_param.projector_sysdata.pictureset.color;
		case P_HUE:
			return sys_param.projector_sysdata.pictureset.hue;
		case P_COLOR_TEMP:
			return sys_param.projector_sysdata.pictureset.color_temp;
		case P_NOISE_REDU:
			return sys_param.projector_sysdata.pictureset.noise_redu;
		case P_SOUND_MODE:
			return sys_param.projector_sysdata.soundset.sound_mode;
		case P_BALANCE:
			return sys_param.projector_sysdata.soundset.balance;
		case P_BT_SETTING:
			return sys_param.projector_sysdata.soundset.bt_setting;
		case P_TREBLE:
			return sys_param.projector_sysdata.soundset.treble;
		case P_BASS:
			return sys_param.projector_sysdata.soundset.bass;
		case P_OSD_LANGUAGE:
			return sys_param.projector_sysdata.optset.osd_language;
		case P_ASPECT_RATIO:
			return sys_param.projector_sysdata.optset.aspect_ratio;
		case P_CUR_CHANNEL:
			return sys_param.projector_sysdata.cur_channel;
		case P_FLIP_MODE:
			return sys_param.sysdata.flip_mode;
		case P_VOLUME:
			return sys_param.projector_sysdata.volume;
		case P_KEYSTONE_TOP:
			return sys_param.projector_sysdata.optset.keystone_top_w;
		case P_KEYSTOME_BOTTOM:
			return sys_param.projector_sysdata.optset.keystone_bottom_w;
		case P_AUTOSLEEP:
			return sys_param.projector_sysdata.optset.auto_sleep;
		case P_INIT_ROTATE:
			return sys_param.projector_sysdata.init_rotate;
		case P_INIT_H_FLIP:
			return sys_param.projector_sysdata.init_h_flip;
			break;
		case P_INIT_V_FLIP:
			return sys_param.projector_sysdata.init_v_flip;
			break;
		default:
			break;
	}
	return -1;
}

void projector_set_some_sys_param(projector_sys_param param, int v){
	switch (param){
		case P_PICTURE_MODE:
			sys_param.projector_sysdata.pictureset.picture_mode = v;
			break;
		case P_CONTRAST:
			sys_param.projector_sysdata.pictureset.contrast = v;
			break;
		case P_BRIGHTNESS:
			sys_param.projector_sysdata.pictureset.brightness = v;
			break;
		case P_SHARPNESS:
			sys_param.projector_sysdata.pictureset.sharpness = v;
			break;
		case P_COLOR:
			sys_param.projector_sysdata.pictureset.color = v;
			break;
		case P_HUE:
			sys_param.projector_sysdata.pictureset.hue = v;
			break;
		case P_COLOR_TEMP:
			sys_param.projector_sysdata.pictureset.color_temp = v;
			break;
		case P_NOISE_REDU:
			sys_param.projector_sysdata.pictureset.noise_redu = v;
			break;
		case P_SOUND_MODE:
			sys_param.projector_sysdata.soundset.sound_mode = v;
			break;
		case P_BALANCE:
			sys_param.projector_sysdata.soundset.balance = v;
			break;
		case P_BT_SETTING:
			sys_param.projector_sysdata.soundset.bt_setting = v;
			break;
		case P_TREBLE:
			sys_param.projector_sysdata.soundset.treble = v;
			break;
		case P_BASS:
			sys_param.projector_sysdata.soundset.bass = v;
			break;
		case P_OSD_LANGUAGE:
			sys_param.projector_sysdata.optset.osd_language = v;
			break;
		case P_ASPECT_RATIO:
			sys_param.projector_sysdata.optset.aspect_ratio = v;
			break;
		case P_CUR_CHANNEL:
			sys_param.projector_sysdata.cur_channel = v;
			break;
		case P_FLIP_MODE:
			sys_param.sysdata.flip_mode = v;
			break;
		case P_VOLUME:
			sys_param.projector_sysdata.volume = v;
			break;
		case P_KEYSTONE_TOP:
			sys_param.projector_sysdata.optset.keystone_top_w = v;
			break;
		case P_KEYSTOME_BOTTOM:
			sys_param.projector_sysdata.optset.keystone_bottom_w = v;
			break;
		case P_AUTOSLEEP:
			sys_param.projector_sysdata.optset.auto_sleep = v;
			break;
		case P_INIT_ROTATE:
			sys_param.projector_sysdata.init_rotate = v;
			break;
		case P_INIT_H_FLIP:
			sys_param.projector_sysdata.init_h_flip = v;
			break;
		case P_INIT_V_FLIP:
			sys_param.projector_sysdata.init_v_flip = v;
			break;
		default:
			break;
	}
}

// void projector_sys_param_init(){

// }

