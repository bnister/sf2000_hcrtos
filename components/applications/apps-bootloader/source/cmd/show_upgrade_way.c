#include <kernel/delay.h>
#include "show_upgrade_way.h"

#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
static u32 y_location;
static int fd = 0;
static uint8_t *fb_base;
static struct fb_fix_screeninfo fix;    /* Current fix */
static struct fb_var_screeninfo var;	/* Current var */
static int color_bit = 32;
static uint32_t line_width;
static uint32_t pixel_width;
static uint32_t screen_size;
static hcfb_scale_t scale_param = { 1280, 720, 1920, 1080 };// to do later...

static struct dis_rect {
	uint32_t x_start;
	uint32_t x_width;
	uint32_t y_start;
	uint32_t y_hight;
}dis_rect;

static void ap_fill_color(struct dis_rect* dis_rect,uint32_t color)
{
	uint32_t i = 0;
	uint32_t j = 0;
	int16_t* fb_dis_16b = NULL;
	int32_t* fb_dis_32b = NULL;
	int8_t* fb_dis_8b = NULL;
	int   ret = 0;
	uint32_t x_end = 0;
	uint32_t y_end = 0;

	// printf("fb_base=0x%x,y_start=%d,hight=%d,x_start=%d,x_width=%d,xres=%d\n",(unsigned int)fb_base,\
	// 	(unsigned int)dis_rect->y_start,(unsigned int)dis_rect->y_hight,\
	// 	(unsigned int)dis_rect->x_start,(unsigned int)dis_rect->x_width,(unsigned int)var.xres);
	//return;
	fb_dis_16b = (int16_t*)fb_base;
	fb_dis_8b = (int8_t*)fb_base;
	fb_dis_32b = (int32_t*)fb_base;
	x_end = dis_rect->x_start+dis_rect->x_width;
	if(x_end>var.xres) {
		x_end = var.xres;
	}

	y_end = dis_rect->y_start+dis_rect->y_hight;
	if(y_end>var.yres) {
		y_end = var.yres;
	}
	
	for(j=dis_rect->y_start;j<y_end;j++) {
		for(i=dis_rect->x_start;i<x_end;i++) {
			if (color_bit == 32) {
				fb_dis_32b[j*var.xres+i] = color;
			}
			else if (color_bit == 16) {
				fb_dis_16b[j*var.xres+i] = color;
			}
			else if (color_bit == 8) {
				fb_dis_8b[j*var.xres+i] = color;
			}
		}
	}

}

void fill_progress_bar(uint32_t len, uint32_t color)
{
	struct dis_rect show_rect;
	uint32_t dis_color = color;
	
    	
	show_rect.x_start = 0;
	show_rect.x_width = len;
	show_rect.y_start =  y_location/2;
	show_rect.y_hight = y_location/10;

	ap_fill_color(&show_rect,dis_color);

}

int init_progress_bar(void) 
{
	int ret = 0;
	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0)
	{
		printf("Couldn't open /dev/fb0\n");
		return -1;
	}

	if(ioctl(fd, FBIOGET_VSCREENINFO, &var)==-1)	
		printf("Error reading variable information");

	#ifdef __linux__
    var.activate |= FB_ACTIVATE_NOW;//B_ACTIVATE_FORCE;
    #endif
	if (color_bit==16) {
		var.bits_per_pixel = 16;
		var.red.length = 5;
		var.green.length = 6;
		var.blue.length = 5;
		var.yres_virtual = var.yres;
	}
	else if (color_bit==32) {
		var.yoffset = 0;
		var.xoffset = 0;
		var.transp.length = 8;
		var.bits_per_pixel = 32;
		var.red.length = 8;
		var.green.length = 8;
		var.blue.length = 8;
		var.yres_virtual = var.yres;
	}
	if(ioctl(fd, FBIOPUT_VSCREENINFO, &var) == -1) {
        	printf("Error reading variable information");
        	return -1;
    	}

	ioctl(fd, FBIOGET_FSCREENINFO, &fix);
	line_width  = var.xres * var.bits_per_pixel / 8;
	pixel_width = var.bits_per_pixel / 8;
	screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
	y_location = var.yres;

	if (ioctl(fd, HCFBIOSET_SCALE, &scale_param) != 0) {
		printf("ioctl(set scale param)");
		return -1;
	}

	ioctl(fd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_NO_CACHE);
	fb_base = (unsigned char *)mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fb_base == MAP_FAILED) {
		printf("can't mmap\n");
		return -1;
	}

	memset(fb_base, 0x00, fix.smem_len);
	//Make sure that the display is on.
	if (ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
		printf("%s:%d\n", __func__, __LINE__);
	}

	return 0;
}

#endif

#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
typedef struct pinpad_gpio
{
	pinpad_e padctl;
	bool active;
}pinpad_gpio_s;

pinpad_gpio_s led_gpio;

static TaskHandle_t  show_led_t = {0};
static int led_flag = -1;

static void show_led(uint32_t time )
{
    gpio_set_output(led_gpio.padctl, GPIO_ACTIVE_HIGH);
    msleep(time);
    gpio_set_output(led_gpio.padctl, GPIO_ACTIVE_LOW);
    msleep(time);
}

static void show_led_main(void *pvParameters)
{
    while(1) {
        if(led_flag == 0)
            show_led(700);
	else if (led_flag == 1) {
		gpio_set_output(led_gpio.padctl, led_gpio.active);
		vTaskDelete(NULL);
	} else if (led_flag == -1)
		gpio_set_output(led_gpio.padctl, !led_gpio.active);
        msleep(1);
    }
}

int led_init(void)
{
	u32 tmpVal = 0;
	int ret = 0;

	int np = fdt_node_probe_by_path("/hcrtos/upgrade_led" );
	if (np < 0){
		printf("no find led gpio\n");
		return -1;
	}

	led_gpio.padctl = PINPAD_INVALID;
	led_gpio.active = GPIO_ACTIVE_LOW;

	ret = fdt_get_property_u_32_index(np, "led-gpio", 0, &tmpVal);
	if (ret == 0)
		led_gpio.padctl = (pinpad_e)tmpVal;

	ret = fdt_get_property_u_32_index(np, "led-gpio", 1, &tmpVal);
	if (ret == 0)
		led_gpio.active = (pinpad_e)tmpVal;

	if(led_gpio.padctl!=PINPAD_INVALID)
	{
		gpio_configure (led_gpio.padctl, GPIO_DIR_OUTPUT);
		gpio_set_output(led_gpio.padctl, GPIO_ACTIVE_HIGH);
	}

	xTaskCreate(show_led_main, (const char *)"show_led_main", configTASK_STACK_DEPTH,
			NULL, portPRI_TASK_NORMAL, (TaskHandle_t *)&show_led_t);

	return 0;
}
#endif

int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata)
{

    switch(event) {
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SERIAL
        case HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS:
        { 
            printf("\n");
            unsigned char progress_sign[100 + 1];
            int per = param;
            int i;

            for (i = 0; i < 100; i++) {
                if (i < per) {
                    progress_sign[i] = '=';
                } else if (per == i) {
                    progress_sign[i] = '>';
                } else {
                    progress_sign[i] = ' ';
                }
            }

            progress_sign[sizeof(progress_sign) - 1] = '\0';

            printf("\033[1A");
            fflush(stdout);
            printf("\033[K");
            fflush(stdout);
            printf("Upgrading: [%s] %3d%%", progress_sign, per);
            fflush(stdout);
        }

#endif
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
        case HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS:
        {
            uint32_t per = 0;
            per = 1280 * param / 100;
            fill_progress_bar(per,0xffff0000);
            break;
        }
#endif
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
	case HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS: {
		if (param < 100) {
			led_flag = 0;
		} else if (param >= 100) {
			led_flag = -1;
		}
		if (param == 0xff && usrdata == 0xff)
			led_flag = 1;
		break;
	}
#endif
            default:
                break;
    }
}
