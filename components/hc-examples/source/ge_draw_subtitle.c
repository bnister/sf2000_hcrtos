#include "ge_draw_subtitle.h"

static unsigned int colors[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF,
	0xFF<<24, 0xFFFFFFFF, 0x0};  /* 0x00RRGGBB */
void draw_background(uint8_t *buf, struct HCGeDrawSubtitle *ge_info)
{
	uint8_t* fb_base = (uint8_t*)ge_info->fb_base;
	uint8_t *bg_picture = ge_info->bg_picture;

	struct fb_fix_screeninfo fix = ge_info->fix;

	hcge_context *ctx = ge_info->ctx;
    hcge_state *state = &ctx->state;
    HCGERectangle srect = {0, 0, ge_info->w, ge_info->h};
    HCGERectangle drect = {
		ge_info->x * 1280/ge_info->screen_w,
		ge_info->y * 720/ge_info->screen_h,
		ge_info->w * 1280/ge_info->screen_w,
		ge_info->h * 720/ge_info->screen_h
	};

    state->render_options = HCGE_DSRO_NONE;
    state->drawingflags = HCGE_DSDRAW_NOFX;
    state->blittingflags = HCGE_DSBLIT_NOFX;

    state->src_blend = HCGE_DSBF_SRCALPHA;
    state->dst_blend = HCGE_DSBF_ZERO;

    state->destination.config.size.w = SCREEN_WIDTH;
    state->destination.config.size.h = SCREEN_HEIGHT;
    state->destination.config.format = HCGE_DSPF_ARGB;
    state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (buf - fb_base));
    state->dst.pitch = SCREEN_WIDTH*4;

    state->mod_hw = HCGE_SMF_CLIP;
    state->clip.x1 = 0;
    state->clip.y1 = 0;
    state->clip.x2 = SCREEN_WIDTH - 1;
    state->clip.y2 = SCREEN_HEIGHT - 1;

    state->source.config.size.w = ge_info->w;
    state->source.config.size.h = ge_info->h;
    state->source.config.format = HCGE_DSPF_ARGB;
    state->src.phys =(unsigned long)((uint8_t *)fix.smem_start + (bg_picture - fb_base));
    state->src.pitch = 4 * ge_info->w;

    state->accel = HCGE_DFXL_STRETCHBLIT;
    hcge_set_state(ctx, &ctx->state, state->accel);
    hcge_stretch_blit(ctx, &srect, &drect);

}

int init_fb_device(struct HCGeDrawSubtitle *ge_info)
{
	if(ge_info->ctx == NULL) {
		int ret = 0;
		int fb = -1;
		int dis_fb = -1;
		struct fb_var_screeninfo *var = &ge_info->var;
		struct fb_fix_screeninfo *fix = &ge_info->fix;
		struct dis_display_info mpinfo = {0};

		dis_fb = open("/dev/dis" , O_WRONLY);
		if(dis_fb < 0) {
			return -1;
		}
		mpinfo.distype = DIS_TYPE_HD;
		mpinfo.info.layer = DIS_PIC_LAYER_MAIN;

        ioctl(dis_fb, DIS_GET_DISPLAY_INFO, (uint32_t)&mpinfo);
        ge_info->screen_h = mpinfo.info.pic_height;
        ge_info->screen_w = mpinfo.info.pic_width;

		printf("ge_info->screen_h %d\n",ge_info->screen_h);
		printf("ge_info->screen_w %d\n",ge_info->screen_w);
		close(dis_fb);

	    if(hcge_open(&ge_info->ctx) != 0) {
	        printf("Init hcge error.\n");
	        return -1;
	    }
	    fb = open("/dev/fb0", O_RDWR);
		printf("fb0 %d %s： %d\n", fb, __func__, __LINE__);
		ge_info->fbdev = fb;
	    ioctl(fb, FBIOGET_FSCREENINFO, fix);
	    ioctl(fb, FBIOGET_VSCREENINFO, var);

	    ge_info->line_width  = var->xres * var->bits_per_pixel / 8;
	    ge_info->pixel_width = var->bits_per_pixel / 8;
	    ge_info->screen_size = var->xres * var->yres * var->bits_per_pixel / 8;
	    ge_info->buffer_num = fix->smem_len / ge_info->screen_size;

	    //Make sure that the display is on.
	    if (ioctl(fb, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
	        printf("%s:%d\n", __func__, __LINE__);
	    }

	    /*var.activate = FB_ACTIVATE_VBL;*/
	    var->activate = FB_ACTIVATE_NOW;
	    var->yoffset = 0;
	    var->xoffset = 0;
	    var->transp.length = 8;
	    var->yres_virtual = ge_info->buffer_num * var->yres;
	    printf("var.yres_virtual: %d\n", var->yres_virtual);
	    //set variable information
	    if(ioctl(fb, FBIOPUT_VSCREENINFO, var) == -1) {
	        printf("Error reading variable information\n");
	        return -1;
	    }
	    ge_info->fb_base = (unsigned char *)mmap(NULL, fix->smem_len,
	    	PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	    if (ge_info->fb_base == MAP_FAILED) {
	        printf("can't mmap\n");
	        return -1;
	    }
	    memset(ge_info->fb_base, 0, ge_info->fix.smem_len);

		printf("ge_info->fb_base %p %s:%d\n",ge_info->fb_base, __func__, __LINE__);
	    memset(ge_info->fb_base, 0, fix->smem_len);


	    // 4. 通知驱动切换 buffer
	    ret = ioctl(fb, FBIOPAN_DISPLAY, var);
	    if(ret < 0)
	        printf("FBIOPAN_DISPLAY error. ret: %d\n", ret);

	    ioctl(fb, FBIO_WAITFORVSYNC, &ret);
	    if (ret < 0) {
	        perror("ioctl() / FBIO_WAITFORVSYNC");
	    }
		ge_info->screen_buffer[0] = ge_info->fb_base;

	    if(ge_info->buffer_num > 2) {
	        ge_info->screen_buffer[1] = ge_info->fb_base + ge_info->screen_size;
			printf("screen_buffer[1] %p\n",ge_info->screen_buffer[1]);
	        ge_info->free_ptr = ge_info->fb_base + 2*ge_info->screen_size;
	    }

	    ge_info->bg_picture = ge_info->free_ptr;
	    ge_info->free_ptr += SCREEN_WIDTH*SCREEN_HEIGHT*4;

	}

    return 0;
}

int ge_stop_draw_subtitle(struct HCGeDrawSubtitle *ge_info)
{
	hcge_context *ctx = ge_info->ctx;
	hcge_state *state = &ctx->state;
	state->accel = HCGE_DFXL_FILLRECTANGLE;

    HCGERectangle drect = {
		ge_info->x * 1280/ge_info->screen_w,
		ge_info->y * 720/ge_info->screen_h,
		ge_info->w * 1280/ge_info->screen_w,
		ge_info->h * 720/ge_info->screen_h
	};
	uint32_t color1 = colors[HCGE_TRANSPARENT];
    state->color.a = (color1>>24)&0xff;
    state->color.r = (color1>>16)&0xff;
    state->color.g = (color1>>8)&0xff;
    state->color.b = (color1>>0)&0xff;
    hcge_set_state(ctx, state, state->accel);
    hcge_fill_rect(ctx, &drect);
	return 0;
}

void deinit_fb_device(struct HCGeDrawSubtitle *ge_info)
{
    if(ge_info->fbdev > 0) {
        if(ge_info->fb_base) {
            munmap(ge_info->fb_base, ge_info->screen_size);
            ge_info->fb_base = NULL;
        }
        close(ge_info->fbdev);
        ge_info->fbdev = -1;
    }
}
