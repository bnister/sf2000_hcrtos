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
#include <stdlib.h>
#include <hcge/ge_api.h>

static int fbdev;
static struct fb_fix_screeninfo fix;    /* Current fix */
static struct fb_var_screeninfo var;    /* Current var */
static uint32_t screen_size;
static uint8_t *fb_base;
static uint32_t line_width;
static uint32_t pixel_width;
static uint8_t *screen_buffer[2];
static uint8_t *free_ptr;
static uint32_t free_size;
//1280*720*4= 3686400
static uint8_t *bg_picture;
int buffer_num  = 0;
#define HCGE_RED 0
#define HCGE_GREEN 1
#define HCGE_BLUE 2
#define HCGE_BLACK 3
#define HCGE_WHITE 4
static unsigned int colors[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFF<<24, 0xFFFFFFFF, 0x0};  /* 0x00RRGGBB */
static hcge_context *ctx = NULL;

struct ppm_data {
    int32_t w;
    int32_t h;
    uint8_t *data; //argb format, 4 bytes per one pixel
};

static struct ppm_data ppm_info;


static void draw_background(uint8_t *buf)
{
    int ret;
    hcge_state *state = &ctx->state;
    HCGERectangle srect = {0, 0, ppm_info.w, ppm_info.h};
    HCGERectangle drect = {0, 0, 1280, 720};

    state->render_options = HCGE_DSRO_NONE;
    state->drawingflags = HCGE_DSDRAW_NOFX;
    state->blittingflags = HCGE_DSBLIT_NOFX;

    state->src_blend = HCGE_DSBF_SRCALPHA;
    state->dst_blend = HCGE_DSBF_ZERO;

    state->destination.config.size.w = 1280;
    state->destination.config.size.h = 720;
    state->destination.config.format = HCGE_DSPF_ARGB;
    state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (buf - fb_base));
    state->dst.pitch = 1280*4;

    state->mod_hw = HCGE_SMF_CLIP;
    state->clip.x1 = 0;
    state->clip.y1 = 0;
    state->clip.x2 = 1280 - 1;
    state->clip.y2 = 720 - 1;

    //fill background with black
    state->accel = HCGE_DFXL_FILLRECTANGLE;
    uint32_t color1 = colors[HCGE_BLACK];
    state->color.a = (color1>>24)&0xff;
    state->color.r = (color1>>16)&0xff;
    state->color.g = (color1>>8)&0xff;
    state->color.b = (color1>>0)&0xff;
    hcge_set_state(ctx, state, state->accel);
    hcge_fill_rect(ctx, &drect);
    
	state->mod_hw = 0;

    printf("**************** test stretch blit *****************\n");
    
	state->accel = HCGE_DFXL_STRETCHBLIT;
    state->source.config.size.w = ppm_info.w;
    state->source.config.size.h = ppm_info.h;
    state->source.config.format = HCGE_DSPF_ARGB;
    state->src.phys =(unsigned long)((uint8_t *)fix.smem_start + (bg_picture - fb_base));
    state->src.pitch = 4*ppm_info.w;

	state->destination.config.size.w = ppm_info.w;
    state->destination.config.size.h = 1280;
    state->destination.config.format = HCGE_DSPF_ARGB;
	state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (free_ptr - fb_base));
    state->dst.pitch = ppm_info.w * 4;
	drect.x = 0;
	drect.y = 0;
	drect.w = ppm_info.w;
	drect.h = 1280;

    hcge_set_state(ctx, &ctx->state, state->accel);
    hcge_stretch_blit(ctx, &srect, &drect);

#if 1
	state->accel = HCGE_DFXL_BLIT;
    state->source.config.size.w = ppm_info.w;
    state->source.config.size.h = 1280;
    state->source.config.format = HCGE_DSPF_ARGB;
    state->src.phys =(unsigned long)((uint8_t *)fix.smem_start + (free_ptr - fb_base));
    state->src.pitch = ppm_info.w * 4; 

	state->destination.config.size.w = 1280;
    state->destination.config.size.h = 720;
    state->destination.config.format = HCGE_DSPF_ARGB;
	state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (buf - fb_base));
    state->dst.pitch = 1280*4;

	srect.x = 0;
	srect.y = 0;
	srect.w = ppm_info.w;
	srect.h = 1280;

    hcge_set_state(ctx, &ctx->state, state->accel);
    hcge_blit(ctx, &srect, 0, 0);
    hcge_engine_sync(ctx);
#endif

    // 4. 通知驱动切换 buffer
    var.yoffset = var.yres;
    printf("var.yoffset: %d\n", var.yoffset);
    ret = ioctl(fbdev, FBIOPAN_DISPLAY, &var);
    if (ret < 0) {
        perror("ioctl() / FBIOPAN_DISPLAY");
    }

    // 5. 等待帧同步完成
    ret = 0;
    ioctl(fbdev, FBIO_WAITFORVSYNC, &ret);
    if (ret < 0) {
        perror("ioctl() / FBIO_WAITFORVSYNC");
    }
    usleep(4*1000*1000);


    printf("******************test rotate 90******************\n");
	state->accel = HCGE_DFXL_BLIT;
    state->blittingflags = HCGE_DSBLIT_ROTATE90;
    state->source.config.size.w = ppm_info.w;
    state->source.config.size.h =1280;
    state->source.config.format = HCGE_DSPF_ARGB;
    state->src.phys =(unsigned long)((uint8_t *)fix.smem_start + (free_ptr - fb_base));
    state->src.pitch = ppm_info.w * 4; 

	state->destination.config.size.w = 1280;
    state->destination.config.size.h = 720;
    state->destination.config.format = HCGE_DSPF_ARGB;
	state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (buf - fb_base));
    state->dst.pitch = 1280*4;

	srect.x = 0;
	srect.y = 0;
	srect.w = ppm_info.w;
	srect.h = 1280;

    hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_blit(ctx, &srect, 0, (720 - ppm_info.w)/2);
    hcge_engine_sync(ctx);
    usleep(100*1000*1000);
}

static int init_fb_device(void)
{
    int ret;
    if(hcge_open(&ctx) != 0) {
        printf("Init hcge error.\n");
        return -1;
    }
    fbdev = open("/dev/fb0", O_RDWR);

    ioctl(fbdev, FBIOGET_FSCREENINFO, &fix);
    ioctl(fbdev, FBIOGET_VSCREENINFO, &var);

    line_width  = var.xres * var.bits_per_pixel / 8;
    pixel_width = var.bits_per_pixel / 8;
    screen_size = var.xres * var.yres * var.bits_per_pixel / 8;

    // 1. 获得 buffer 个数
    buffer_num = fix.smem_len / screen_size;

    fb_base = (unsigned char *)mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
    if (fb_base == MAP_FAILED) {
        printf("can't mmap\n");
        return -1;
    }

    memset(fb_base, 0, fix.smem_len);

    //Make sure that the display is on.
    if (ioctl(fbdev, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
        printf("%s:%d\n", __func__, __LINE__);
    }

    /*var.activate = FB_ACTIVATE_VBL;*/
    var.activate = FB_ACTIVATE_NOW;
    var.yoffset = 0;
    var.xoffset = 0;
    var.transp.length = 8;
    var.yres_virtual = buffer_num * var.yres;
    //set variable information
    if(ioctl(fbdev, FBIOPUT_VSCREENINFO, &var) == -1) {
        perror("Error reading variable information");
        return -1;
    }

    // 4. 通知驱动切换 buffer
    ret = ioctl(fbdev, FBIOPAN_DISPLAY, &var);
    if(ret < 0)
        printf("FBIOPAN_DISPLAY error. ret: %d\n", ret);

    ioctl(fbdev, FBIO_WAITFORVSYNC, &ret);
    if (ret < 0) {
        perror("ioctl() / FBIO_WAITFORVSYNC");
    }

    screen_buffer[0] = fb_base;

    if(buffer_num > 2) {
        screen_buffer[1] = fb_base + screen_size;
        free_ptr = fb_base + 2*screen_size;
    }

    bg_picture = free_ptr;
    free_ptr += 1280*720*4;
    free_size -= 1280*720*4;
    return 0;
}

static void deinit_fb_device(void)
{
    if(fbdev > 0) {
        if(fb_base) {
            munmap(fb_base, screen_size);
            fb_base = NULL;
        }
        close(fbdev);
        fbdev = -1;
    }
}

static int load_ppm_data(char *path)
{
    FILE *fp = fopen(path, "r+");
    char buf[256];
    if(!fp) {
        perror("Open error.\n");
        return -1;
    }
    fgets(buf, 256, fp);
    if(strncmp(buf, "P6", 2)) {
        printf("file is not ppm format.\n");
        fclose(fp);
        return -1;
    }
    fgets(buf, 256, fp);
    printf("wxh=%s\n", buf);
    sscanf(buf, "%d %d", &ppm_info.w, &ppm_info.h);
    printf("ppm_info.w=%d, ppm_info.h=%d\n", ppm_info.w, ppm_info.h);
    fgets(buf, 256, fp);
    printf("color: %s, pos: %ld\n", buf, ftell(fp));
    int size = ppm_info.w * ppm_info.h * 3;
    uint8_t *tmp = malloc(size);
    if(!tmp) {
        printf("Not enough memory.\n");
        fclose(fp);
        return -1;
    }
    if(size != (int)fread(tmp, 1, size, fp)) {
        perror("read data error.\n");
        free(tmp);
        fclose(fp);
        return -1;
    }
    int i = 0;
    for(i = 0; i < ppm_info.w*ppm_info.h; i++) {
        bg_picture[i*4+2] = tmp[i*3+0];
        bg_picture[i*4+1] = tmp[i*3+1];
        bg_picture[i*4+0] = tmp[i*3+2];
        bg_picture[i*4+3] = 0xff;
    }
    ppm_info.data = bg_picture;
    return 0;
}


int main(int argc, char **argv)
{
    int ret;
    char *buf_next;
    struct timespec time = {1000, 0};

    if(init_fb_device() != 0) {
        perror("Init framebuffer error.\n");
        exit(-1);
    }
    if(argc != 2) {
        printf("usage: %s <ppm file>\n", argv[0]);
        return -1;
    }
    printf("argv[1]: %s\n", argv[1]);

    load_ppm_data(argv[1]);

    //draw background
    buf_next = (char *) screen_buffer[1];
    draw_background((uint8_t*)buf_next);

    // 4. 通知驱动切换 buffer
    var.yoffset = var.yres;
    printf("var.yoffset: %d\n", var.yoffset);
    ret = ioctl(fbdev, FBIOPAN_DISPLAY, &var);
    if (ret < 0) {
        perror("ioctl() / FBIOPAN_DISPLAY");
    }

    // 5. 等待帧同步完成
    ret = 0;
    ioctl(fbdev, FBIO_WAITFORVSYNC, &ret);
    if (ret < 0) {
        perror("ioctl() / FBIO_WAITFORVSYNC");
    }

    nanosleep(&time, NULL);

    deinit_fb_device();
    return 0;
}
