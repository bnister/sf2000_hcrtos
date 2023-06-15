/**
 * @file lv_gpu_hichip.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../core/lv_refr.h"
#include <hcge/ge_api.h>
#include <stdio.h>
#include <pthread.h>
#ifdef __HCRTOS__
#include <cpu_func.h>
#else
#include <asm/cachectl.h>
#endif
#include "../../../lv_drivers/display/fbdev.h"
#include "lv_gpu_hichip.h"

#if LV_USE_GPU_HICHIP

/*#include LV_GPU_DMA2D_CMSIS_INCLUDE*/

/*********************
 *      DEFINES
 *********************/

#if LV_COLOR_16_SWAP
// TODO: F7 has red blue swap bit in control register for all layers and output
#error "Can't use DMA2D with LV_COLOR_16_SWAP 1"
#endif

#if LV_COLOR_DEPTH == 8
#error "Can't use DMA2D with LV_COLOR_DEPTH == 8"
#endif

#if LV_COLOR_DEPTH == 16
#define LV_DMA2D_COLOR_FORMAT LV_DMA2D_RGB565
#elif LV_COLOR_DEPTH == 32
#define LV_DMA2D_COLOR_FORMAT LV_DMA2D_ARGB8888
#else
/*Can't use GPU with other formats*/
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool lv_draw_hichip_blend_fill(lv_color_t * dest_buf, lv_coord_t dest_stride, const lv_area_t * fill_area,
                                      lv_color_t color, const lv_opa_t * mask_buf,const lv_area_t * mask_area, lv_coord_t mask_stride);


static bool lv_draw_hichip_blend_map(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                                     const lv_color_t * src_buf,const lv_area_t * src_area, lv_coord_t src_stride,
                                     const lv_opa_t * mask_buf,const lv_area_t * mask_area, lv_coord_t mask_stride,
                                     lv_opa_t opa);

static void lv_draw_hichip_img_decoded(lv_draw_ctx_t * draw, const lv_draw_img_dsc_t * dsc,
                                       const lv_area_t * coords, const uint8_t * map_p, lv_img_cf_t color_format);


static void invalidate_cache(void);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

hcge_context *hcge_ctx = NULL;
pthread_mutex_t hcge_mutex;

void lv_draw_hichip_lock(void)
{
	/*pthread_mutex_lock(&hcge_mutex);*/
}

void lv_draw_hichip_unlock(void)
{
	/*pthread_mutex_unlock(&hcge_mutex);*/
}

void lv_draw_hichip_deinit(void)
{
	if(hcge_ctx) {
		hcge_close(hcge_ctx);
		hcge_ctx = NULL;
	}
	pthread_mutex_destroy(&hcge_mutex);
}

/**
 * Turn on the peripheral and set output color mode, this only needs to be done once
 */
void lv_draw_hichip_init(void)
{
	if(!hcge_ctx && hcge_open(&hcge_ctx) != 0) {
		perror("Init hcge error.\n");
		return;
	}
	pthread_mutex_init(&hcge_mutex, NULL);
}


void lv_draw_hichip_ctx_init(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{

	lv_draw_sw_init_ctx(drv, draw_ctx);

	lv_draw_hichip_ctx_t * dma2d_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

	dma2d_draw_ctx->blend = lv_draw_hichip_blend;
	dma2d_draw_ctx->base_draw.draw_img_decoded = lv_draw_hichip_img_decoded;
	dma2d_draw_ctx->base_draw.wait_for_finish = lv_gpu_hichip_wait_cb;
	dma2d_draw_ctx->base_draw.buffer_copy = lv_draw_hichip_buffer_copy;
}

void lv_draw_hichip_ctx_deinit(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
	LV_UNUSED(drv);
	LV_UNUSED(draw_ctx);
	lv_draw_hichip_deinit();
}


void lv_draw_hichip_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc)
{
	lv_area_t blend_area;
	if(!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) return;

	bool done = false;
	if(dsc->mask_buf && false) {
		printf("dsc->mask_buf: %p, dsc->blend_mode: %d, area_size: %d\n",dsc->mask_buf, dsc->blend_mode, lv_area_get_size(&blend_area));
		printf("dsc->mask_res: %d, mask_area:x1: %d, y1:%d, x2: %d, y2: %d\n",dsc->mask_res, dsc->mask_area->x1, dsc->mask_area->y1, 
		dsc->mask_area->x2, dsc->mask_area->y2);
		printf("blend_area: x1: %d, y1: %d, x2: %d, y2: %d\n", blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);
		printf("%d, area: %d\n", __LINE__, lv_area_get_size(&blend_area));
	}

	if((dsc->mask_buf == NULL || dsc->mask_res == LV_DRAW_MASK_RES_FULL_COVER) && dsc->blend_mode == LV_BLEND_MODE_NORMAL && lv_area_get_size(&blend_area) > 240) {
	/*if([>dsc->mask_buf == NULL &&<] lv_area_get_size(&blend_area) > 100) {*/
		lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area);
		lv_color_t * dest_buf = draw_ctx->buf;
		const lv_color_t * src_buf = dsc->src_buf;

		lv_coord_t mask_stride = 0;
		lv_opa_t *mask_buf = NULL;
		lv_area_t mask_area;
		if(dsc->mask_buf && dsc->mask_res != LV_DRAW_MASK_RES_FULL_COVER) {
			mask_stride = lv_area_get_width(dsc->mask_area);
			mask_area.x1 = blend_area.x1 - dsc->blend_area->x1;
			mask_area.y1 = blend_area.y1 - dsc->blend_area->y1;
			mask_area.x2 = blend_area.x2 - blend_area.x1;
			mask_area.y2 = blend_area.y2 - blend_area.y1;
			/*printf("mask_area: x1: %d, y1: %d, x2: %d, y2: %d\n", mask_area.x1, mask_area.y1, mask_area.x2, mask_area.y2);*/
		}

		if(src_buf) {
			/*printf("src_buf: %p, dest_buf: %p\n", src_buf, dest_buf);*/
			/*printf("src_buf: %02x, %02x, %02x, %02x\n", src_buf[0].full&0xff, src_buf[1].full&0xff, src_buf[2].full&0xff, src_buf[3].full&0xff);*/
			lv_coord_t src_stride;
			/*lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);*/
			lv_area_t src_area;
			src_stride = lv_area_get_width(dsc->blend_area);
			src_area.x1 = blend_area.x1 - dsc->blend_area->x1;
			src_area.y1 = blend_area.y1 - dsc->blend_area->y1;
			src_area.x2 = blend_area.x2 - blend_area.x1;
			src_area.y2 = blend_area.y2 - blend_area.y1;
			if(lv_draw_hichip_blend_map(dest_buf, &blend_area, dest_stride, src_buf, &src_area, src_stride, 
						mask_buf, &mask_area, mask_stride, dsc->opa))
				done = true;
		} else if(dsc->opa >= LV_OPA_MAX) {
			/*printf("dsc->opa: %d\n", dsc->opa);*/
			lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
			/*printf("dest_buf: %p, buf_area: x1: %d, y1: %d, x2: %d, y2: %d\n", dest_buf, draw_ctx->buf_area->x1, draw_ctx->buf_area->y1, */
			/*draw_ctx->buf_area->x2, draw_ctx->buf_area->y2);*/
			if(lv_draw_hichip_blend_fill(dest_buf, dest_stride, &blend_area, dsc->color, mask_buf, &mask_area, mask_stride)) {
				done = true;
			}
			/* if rendering fail, using software render */
		}
	}

	if(!done) lv_draw_sw_blend_basic(draw_ctx, dsc);
}

void lv_draw_hichip_buffer_copy(lv_draw_ctx_t * draw_ctx,
                                void * dest_buf, lv_coord_t dest_stride, const lv_area_t * dest_area,
                                void * src_buf, lv_coord_t src_stride, const lv_area_t * src_area)
{
	if(!lv_draw_hichip_blend_map(dest_buf, dest_area, dest_stride, src_buf, dest_area, src_stride, NULL, NULL, 0, LV_OPA_MAX))
		lv_draw_sw_buffer_copy(draw_ctx, dest_buf, dest_stride, dest_area, src_buf, src_stride, src_area);
}

static void lv_draw_hichip_img_decoded(lv_draw_ctx_t * draw_ctx, const lv_draw_img_dsc_t * dsc,
                                       const lv_area_t * coords, const uint8_t * map_p, lv_img_cf_t color_format)
{
	/*TODO basic ARGB8888 image can be handles here*/
	// printf("%s\n",__func__);

	lv_draw_sw_img_decoded(draw_ctx, dsc, coords, map_p, color_format);
}

static bool lv_draw_hichip_blend_fill(lv_color_t * dest_buf, lv_coord_t dest_stride, const lv_area_t * fill_area,
                                      lv_color_t color, const lv_opa_t * mask_buf,const lv_area_t * mask_area, lv_coord_t mask_stride)
{
	/*Simply fill an area*/
	int32_t area_w = lv_area_get_width(fill_area);
	int32_t area_h = lv_area_get_height(fill_area);
	invalidate_cache();
	if(!hcge_ctx) {
		return false;
	}
	/*printf("dest_buf: %p\n", dest_buf);*/
	if(!fbdev_buffer_addr_virt_check((uint32_t)dest_buf, (fill_area->y2 * dest_stride + fill_area->x2)*sizeof(lv_color_t)))
		return false;
	if(mask_buf && mask_area && !fbdev_buffer_addr_virt_check((uint32_t)mask_buf, mask_area->y2 * mask_stride + mask_area->x2))
		return false;

	/*printf("dest_buf: %p, dest_stride: %d, x1: %d, y1: %d, w: %d, h: %d\n", dest_buf,*/
	/*dest_stride * sizeof(lv_color_t), fill_area->x1, fill_area->y1, area_w, area_h);*/
	/*return;*/

	hcge_state *state = &hcge_ctx->state;
	lv_draw_hichip_lock();

	state->render_options = HCGE_DSRO_NONE;
	state->drawingflags = HCGE_DSDRAW_NOFX;
	state->blittingflags = HCGE_DSBLIT_NOFX;

	state->src_blend = HCGE_DSBF_SRCALPHA;
	state->dst_blend = HCGE_DSBF_ZERO;

	state->destination.config.size.w = dest_stride;
	state->destination.config.size.h = fill_area->y2 + 1;
	state->destination.config.format = HCGE_DSPF_ARGB;
	state->dst.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(dest_buf);
	state->dst.pitch = dest_stride * sizeof(lv_color_t);

	state->color.a = color.ch.alpha;
	state->color.r = color.ch.red;
	state->color.g = color.ch.green;
	state->color.b = color.ch.blue;



#if 1
	state->mod_hw = HCGE_SMF_CLIP;
	state->mod_hw = 0;
	state->clip.x1 = 0;
	state->clip.y1 = 0;
	state->clip.x2 = CONFIG_LV_HC_SCREEN_HOR_RES - 1;
	state->clip.y2 = CONFIG_LV_HC_SCREEN_VER_RES - 1;
#else
	state->mod_hw = 0;
#endif

	if(mask_buf && mask_stride && mask_area) {
		printf("mask:%s\n", __func__);
		state->blittingflags |= HCGE_DSBLIT_SRC_MASK_ALPHA;
		state->src_mask_offset.x = mask_area->x1;
		state->src_mask_offset.y = mask_area->y1;
		state->src_mask_flags = HCGE_DSMF_STENCIL;
		state->src_mask_flags = 0;
		state->source_mask.config.size.w = mask_stride;
		state->source_mask.config.size.h = mask_area->y2 + 1;
		state->source_mask.config.format = HCGE_DSPF_A8;
		state->src_mask.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(mask_buf);
		state->src.pitch = mask_stride;
	}

	HCGERectangle drect;
	drect.x = fill_area->x1;
	drect.y = fill_area->y1;
	drect.w = area_w;
	drect.h = area_h;

	state->accel = HCGE_DFXL_FILLRECTANGLE;
	/*printf("drect.x: %d, drect.y: %d, drect.w: %d, drect.h: %d\n", drect.x, drect.y, drect.w, drect.h);*/
	/*printf("dest_buf: %p, dest_width: %d, state->dst.phys: 0x%08x\n", dest_buf, dest_stride, state->dst.phys);*/
	cacheflush(dest_buf, state->dst.pitch * fill_area->y2 + fill_area->x2 * sizeof(lv_color_t), DCACHE);
	hcge_set_state(hcge_ctx, &hcge_ctx->state, state->accel);
	hcge_fill_rect(hcge_ctx, &drect);
	/*hcge_engine_sync(hcge_ctx);*/
	lv_draw_hichip_unlock();

	return true;
}


static bool lv_draw_hichip_blend_map(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                                     const lv_color_t * src_buf,const lv_area_t * src_area, lv_coord_t src_stride,
                                     const lv_opa_t * mask_buf,const lv_area_t * mask_area, lv_coord_t mask_stride,
                                     lv_opa_t opa)
{
	if(!fbdev_buffer_addr_virt_check((uint32_t)dest_buf, (dest_area->y2 * dest_stride + dest_area->x2)*sizeof(lv_color_t)))
		return false;
	if(!fbdev_buffer_addr_virt_check((uint32_t)src_buf, (dest_area->y2 * src_stride + dest_area->x2)*sizeof(lv_color_t)))
		return false;
	if(mask_buf && mask_area && !fbdev_buffer_addr_virt_check((uint32_t)mask_buf, mask_area->y2 * mask_stride + mask_area->x2))
		return false;

	invalidate_cache();

	lv_draw_hichip_lock();
	hcge_state *state = &hcge_ctx->state;
	HCGERectangle srect;
	HCGERectangle drect;

	/*printf("src_buf: %p, dest_buf: %p\n", src_buf, dest_buf);*/
	/*if(opa >= LV_OPA_MAX) {*/
	/*printf("%s: %d\n", __func__, __LINE__);*/
	state->render_options = HCGE_DSRO_NONE;
	state->drawingflags = HCGE_DSDRAW_NOFX;
	state->blittingflags = HCGE_DSBLIT_NOFX;

	state->src_blend = HCGE_DSBF_SRCALPHA;
	state->dst_blend = HCGE_DSBF_ZERO;
#if 1
	state->blend_mode = HCGE_BLEND_ALPHA;
#if 1
	state->blend_operation = HCGE_DSBF_SRC_OVER;
#else
	state->blend_operation = HCGE_DSBF_DST_OVER;
#endif
	state->color.a =  opa;
#endif

	state->destination.config.size.w = dest_stride;
	state->destination.config.size.h = dest_area->y2 + 1;
	state->destination.config.format = HCGE_DSPF_ARGB;
	state->dst.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(dest_buf);
	state->dst.pitch = dest_stride * sizeof(lv_color_t);

	state->mod_hw = 0;

	state->source.config.size.w = src_stride;
	state->source.config.size.h = dest_area->y2 + 1;
	state->source.config.format = HCGE_DSPF_ARGB;
	state->src.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(src_buf);
	state->src.pitch = src_stride * sizeof(lv_color_t);

	if(mask_buf && mask_stride && mask_area) {
		printf("mask:%s\n", __func__);
		state->blittingflags |= HCGE_DSBLIT_SRC_MASK_ALPHA;
		state->src_mask_offset.x = mask_area->x1;
		state->src_mask_offset.y = mask_area->y1;
		state->src_mask_flags = HCGE_DSMF_STENCIL;
		state->src_mask_flags = 0;
		state->source_mask.config.size.w = mask_stride;
		state->source_mask.config.size.h = mask_area->y2 + 1;
		state->source_mask.config.format = HCGE_DSPF_A8;
		state->src_mask.phys = (unsigned long)fbdev_buffer_addr_virt_to_phy(mask_buf);
		state->src.pitch = mask_stride;
	}

	drect.x = dest_area->x1;
	drect.y = dest_area->y1;
	drect.w = lv_area_get_width(dest_area);
	drect.h = lv_area_get_height(dest_area);

	srect.x = src_area->x1;
	srect.y = src_area->y1;
	srect.w = lv_area_get_width(src_area);
	srect.h = lv_area_get_height(src_area);
	/*printf("srect.x: %d, srect.y: %d, srect.w: %d, srect.h\n", srect.x, srect.y, srect.w, srect.h);*/
	cacheflush(dest_buf, state->dst.pitch * dest_area->y2 + dest_area->x2 * sizeof(lv_color_t), DCACHE);
	cacheflush(src_buf, state->src.pitch * src_area->y2 + src_area->x2 * sizeof(lv_color_t), DCACHE);

	state->accel = HCGE_DFXL_STRETCHBLIT;
	hcge_set_state(hcge_ctx, state, state->accel);
	hcge_stretch_blit(hcge_ctx, &srect, &drect);

	lv_draw_hichip_unlock();
	return true;
	/*return false;*/
}

void lv_gpu_hichip_wait_cb(lv_draw_ctx_t * draw_ctx)
{
#if 0
	lv_disp_t * disp = _lv_refr_get_disp_refreshing();

	if(disp->driver && disp->driver->wait_cb) {
		disp->driver->wait_cb(disp->driver);
	}
#endif

	lv_draw_hichip_lock();
	hcge_engine_sync(hcge_ctx);
	lv_draw_hichip_unlock();

	lv_draw_sw_wait_for_finish(draw_ctx);

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void invalidate_cache(void)
{
	lv_disp_t * disp = _lv_refr_get_disp_refreshing();
	if(disp->driver->clean_dcache_cb) disp->driver->clean_dcache_cb(disp->driver);
	else {
#if __CORTEX_M >= 0x07
		if((SCB->CCR) & (uint32_t)SCB_CCR_DC_Msk)
			SCB_CleanInvalidateDCache();
#endif
	}
}

#endif
