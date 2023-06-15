#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <stdint.h>
#include <hcge/ge_api.h>

static int fd_fb;
static struct fb_fix_screeninfo fix;    /* Current fix */
static struct fb_var_screeninfo var;    /* Current var */
static uint32_t screen_size;
static uint8_t *fb_base;
static uint32_t line_width;
static uint32_t pixel_width;
static hcge_context *ctx = NULL;

static void lcd_draw_screen(uint8_t *buf, unsigned int color1, int screen_size)
{
	(void)screen_size;
#if 0
    int i;
    for( i = 0; i < screen_size/sizeof(unsigned int); i++)
        base[i] = color;
#else
	HCGE_CoreSurfaceBuffer dst;
	HCGE_CoreSurface surface;
	uint32_t w = 1200;
	uint32_t h = 600;
	HCGERectangle rect = {1280/2 - w/2, 720/2 - h/2, w, h};
	HCGEColor color;
	dst.pitch = 1280 * 4;
	dst.phys = (unsigned long)((char *)fix.smem_start + (buf - fb_base));
	memset(&surface, 0, sizeof(surface));
	surface.config.size.w = 1280;
	surface.config.size.h = 720;
	surface.config.format = HCGE_DSPF_ARGB;
	color.a = (color1>>24)&0xff;
	color.r = (color1>>16)&0xff;
	color.g = (color1>>8)&0xff;
	color.b = (color1>>0)&0xff;
	hcge_fill_rect_ext(ctx, &dst, &surface, &rect, &color);
#endif
}
int main(int argc, char **argv)
{
    int i;
    int ret;
    int buffer_num;
    int buf_idx = 1;
    char *buf_next;
    unsigned int colors[] = {0x33FF0000, 0x3300FF00, 0xFF0000FF, 0xFF<<24, 0xFFFFFFFF, 0x0};  /* 0x00RRGGBB */
    struct timespec time = {2, 0};

	if(hcge_open(&ctx) != 0){
			printf("Init hcge error.\n");
			return -1;
	}

    fd_fb = open("/dev/fb0", O_RDWR);

    ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix);
    ioctl(fd_fb, FBIOGET_VSCREENINFO, &var);

    line_width  = var.xres * var.bits_per_pixel / 8;
    pixel_width = var.bits_per_pixel / 8;
    screen_size = var.xres * var.yres * var.bits_per_pixel / 8;

    // 1. 获得 buffer 个数
    buffer_num = fix.smem_len / screen_size;
    fb_base = (unsigned char *)mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
    if (fb_base == MAP_FAILED) {
        printf("can't mmap\n");
        return -1;
    }

    memset(fb_base, 0x00, fix.smem_len);

    //Make sure that the display is on.
    if (ioctl(fd_fb, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
        printf("%s:%d\n", __func__, __LINE__);
    }

    /*var.activate = FB_ACTIVATE_VBL;*/
    var.activate = FB_ACTIVATE_NOW;
    var.yoffset = 0;
    var.xoffset = 0;
    var.transp.length = 8;
	//set variable information
    if(ioctl(fd_fb, FBIOPUT_VSCREENINFO, &var) == -1) {
        perror("Error reading variable information");
        return -1;
    }

    // 4. 通知驱动切换 buffer
    ret = ioctl(fd_fb, FBIOPAN_DISPLAY, &var);
    if(ret < 0)
        printf("FBIOPAN_DISPLAY error. ret: %d\n", ret);

    ioctl(fd_fb, FBIO_WAITFORVSYNC, &ret);
    if (ret < 0) {
        perror("ioctl() / FBIO_WAITFORVSYNC");
    }

    if ( ((argc > 1) && argv[1][0] == 's') || (buffer_num == 1)) {
        printf("single buffer:\n");
        while (1) {
            for (i = 0; i < (int)(sizeof(colors)/sizeof(colors[0])); i++) {
                lcd_draw_screen(fb_base, colors[i], screen_size);
                nanosleep(&time, NULL);
            }
        }
    } else {
        printf("double buffer:\n");
        // 2. 使能多 buffer
        var.yres_virtual = buffer_num * var.yres;

        ioctl(fd_fb, FBIOPUT_VSCREENINFO, &var);

        while (1) {
            for (i = 0; i < (int)(sizeof(colors)/sizeof(colors[0])); i++) {

                // 3. 更新 buffer 里的数据
                buf_next =  (char *)(fb_base + buf_idx * screen_size);

		lcd_draw_screen((uint8_t*)buf_next, colors[i], screen_size);
                /*lcd_draw_screen(fb_base, colors[i], fix.smem_len);*/

                // 4. 通知驱动切换 buffer
				var.yoffset = buf_idx * var.yres;
				printf("var.yoffset: %d\n", var.yoffset);
                ret = ioctl(fd_fb, FBIOPAN_DISPLAY, &var);
                if (ret < 0) {
                    perror("ioctl() / FBIOPAN_DISPLAY");
                }

                // 5. 等待帧同步完成
                ret = 0;
                ioctl(fd_fb, FBIO_WAITFORVSYNC, &ret);
                if (ret < 0) {
                    perror("ioctl() / FBIO_WAITFORVSYNC");
                }

                buf_idx = !buf_idx;
				/*usleep(2*1000*1000);*/
				nanosleep(&time, NULL);
            }
        }

    }

    munmap(fb_base, screen_size);
    close(fd_fb);

    return 0;
}
