#include <stdio.h>
#include <hcge/ge_api.h>
#include "mi_gfx.h"


static hcge_context *ctx;

#define ASSIGN_SURFACE(destination,dst, pstDst, u32ColorVal)do{\
			(destination)->config.format = conv2pixelformat((pstDst)->eColorFmt, u32ColorVal, &state->color);\
    if((destination)->config.format == HCGE_DSPF_UNKNOWN) {\
        printf("Dont support pixel format, u32ColorVal: %u, (pstDst)->eColorFmt: %d\n", u32ColorVal, (pstDst)->eColorFmt);\
        return  E_MI_GFX_ERR_GFX_DRV_NOT_SUPPORT;\
    }\
    (destination)->config.size.w = (pstDst)->u32Width;\
    (destination)->config.size.h = (pstDst)->u32Height;\
    (dst)->phys = (unsigned long)((pstDst)->phyAddr);\
    (dst)->pitch = (pstDst)->u32Stride;\
}while(0)

MI_S32 MI_GFX_Open(void)
{
	if(ctx) {
		return E_MI_GFX_ERR_SUCCESS;
	}

	if(hcge_open(&ctx) != 0) {
		printf("Init hcge error.\n");
		return E_MI_GFX_ERR_NOT_INIT;
	}
	return E_MI_GFX_ERR_SUCCESS;
}

MI_S32 MI_GFX_Close(void)
{
	if(ctx) {
		hcge_close(ctx);
		ctx = NULL;
	}
	return E_MI_GFX_ERR_SUCCESS;
}

MI_S32 MI_GFX_WaitAllDone(MI_BOOL bWaitAllDone, MI_U16 u16TargetFence)
{
	(void)bWaitAllDone;
	(void)u16TargetFence;
	if(!ctx) {
		return E_MI_GFX_ERR_NOT_INIT;
	}
	return hcge_engine_sync(ctx);
}

static  HCGESurfacePixelFormat conv2pixelformat(MI_GFX_ColorFmt_e color, MI_U32 u32ColorVal,HCGEColor *out_color)
{
	HCGESurfacePixelFormat hcge_color = HCGE_DSPF_UNKNOWN;
	switch(color) {
	case E_MI_GFX_FMT_RGB565:
		hcge_color = HCGE_DSPF_RGB16;
		out_color->r = u32ColorVal>>11&0x1F;
		out_color->g = u32ColorVal>>6&0x3F;
		out_color->b = u32ColorVal&0x1F;
		break;
	case E_MI_GFX_FMT_ARGB1555:
	case E_MI_GFX_FMT_ARGB1555_DST:
		hcge_color = HCGE_DSPF_ARGB1555;
		out_color->r = u32ColorVal>>15&0x1;
		out_color->r = u32ColorVal>>10&0x1F;
		out_color->g = u32ColorVal>>5&0x1F;
		out_color->b = u32ColorVal&0x1F;
		break;
	case E_MI_GFX_FMT_ARGB4444:
		hcge_color =  HCGE_DSPF_ARGB4444;
		out_color->a = u32ColorVal>>12&0xF;
		out_color->r = u32ColorVal>>8&0xF;
		out_color->g = u32ColorVal>>4&0xF;
		out_color->b = u32ColorVal&0xF;
		break;
	case E_MI_GFX_FMT_ARGB8888:
		hcge_color = HCGE_DSPF_ARGB;
		out_color->a = u32ColorVal>>24&0xFF;
		out_color->r = u32ColorVal>>16&0xFF;
		out_color->g = u32ColorVal>>8&0xFF;
		out_color->b = u32ColorVal&0xFF;
		break;
	default:
		break;
	}
	return hcge_color;
}

static  HCGESurfacePixelFormat conv2pixelformat_colorkey(MI_GFX_ColorFmt_e color, MI_U32 u32ColorVal, uint32_t *out_color)
{
	HCGESurfacePixelFormat hcge_color = HCGE_DSPF_UNKNOWN;
	switch(color) {
	case E_MI_GFX_FMT_RGB565:
		hcge_color = HCGE_DSPF_RGB16;
		break;
	case E_MI_GFX_FMT_ARGB1555:
	case E_MI_GFX_FMT_ARGB1555_DST:
		hcge_color = HCGE_DSPF_ARGB1555;
		break;
	case E_MI_GFX_FMT_ARGB4444:
		hcge_color = HCGE_DSPF_ARGB4444;
		break;
	case E_MI_GFX_FMT_ARGB8888:
		hcge_color = HCGE_DSPF_ARGB;
		break;
	default:
		break;
	}
	if(hcge_color != HCGE_DSPF_UNKNOWN)
		*out_color = u32ColorVal;
	return hcge_color;
}


static void conv2colorkeyopt(MI_GFX_ColorKeyOp_e *mi_opt, HCGEColorKeyOp *hcge_opt)
{

	switch(*mi_opt) {
	case E_MI_GFX_RGB_OP_EQUAL:
		*hcge_opt = HCGE_RGB_OP_EQUAL;
		break;
	case E_MI_GFX_RGB_OP_NOT_EQUAL:
		*hcge_opt = HCGE_RGB_OP_NOT_EQUAL;
		break;
	case E_MI_GFX_ALPHA_OP_EQUAL:
		*hcge_opt = HCGE_ALPHA_OP_EQUAL;
		break;
	case E_MI_GFX_ALPHA_OP_NOT_EQUAL:
		*hcge_opt = HCGE_ALPHA_OP_NOT_EQUAL;
		break;
	case E_MI_GFX_ARGB_OP_EQUAL:
		*hcge_opt = HCGE_ARGB_OP_EQUAL;
		break;
	case E_MI_GFX_ARGB_OP_NOT_EQUAL:
		*hcge_opt = HCGE_ARGB_OP_NOT_EQUAL;
		break;
	default:
		*hcge_opt = HCGE_RGB_OP_EQUAL;
		break;
	}
}

MI_S32 conv2colorkey(MI_GFX_ColorKeyInfo_t *mi_src_colorkey, MI_GFX_ColorKeyInfo_t *mi_dst_colorkey, hcge_state *state)
{
	MI_GFX_ColorKeyInfo_t *colorkey;
	HCGESurfacePixelFormat pf;
	colorkey = mi_src_colorkey;
	if(colorkey->bEnColorKey) {
		pf = conv2pixelformat_colorkey(colorkey->eCKeyFmt, colorkey->stCKeyVal.u32ColorStart, &state->src_colorkey);
		if(pf == HCGE_DSPF_UNKNOWN) {
			return E_MI_GFX_ERR_GFX_DRV_NOT_SUPPORT;
		}
		conv2colorkeyopt(&colorkey->eCKeyOp, &state->src_colorkey_opt);
		state->blittingflags |= HCGE_CUST_SRC_COLORKEY;
	}

	colorkey = mi_dst_colorkey;
	if(colorkey->bEnColorKey) {
		pf = conv2pixelformat_colorkey(colorkey->eCKeyFmt, colorkey->stCKeyVal.u32ColorStart, &state->dst_colorkey);
		if(pf == HCGE_DSPF_UNKNOWN) {
			return E_MI_GFX_ERR_GFX_DRV_NOT_SUPPORT;
		}
		conv2colorkeyopt(&colorkey->eCKeyOp, &state->dst_colorkey_opt);
		state->blittingflags |= HCGE_CUST_DST_COLORKEY;
	}
	return E_MI_GFX_ERR_SUCCESS;
}

MI_S32 MI_GFX_QuickFill(MI_GFX_Surface_t *pstDst, MI_GFX_Rect_t *pstDstRect,
                        MI_U32 u32ColorVal, MI_U16 *pu16Fence)
{
	(void)pu16Fence;
	hcge_state *state = &ctx->state;
	HCGERectangle srect;
	state->render_options = 0;
	state->drawingflags = 0;
	state->accel = HCGE_DFXL_FILLRECTANGLE;
	/*state->set = HC_SUPPORTED_DRAW_FUNCS | HC_SUPPORTED_BLIT_FUNCS;*/
#if 0
	state->destination.config.format = conv2pixelformat(pstDst->eColorFmt, u32ColorVal, &state->color);
	if(state->destination.config.format == HCGE_DSPF_UNKNOWN) {
		printf("Dont support pixel format, u32ColorVal: %lu, pstDst->eColorFmt: %d\n", pstDst->eColorFmt);
		return  E_MI_GFX_ERR_GFX_DRV_NOT_SUPPORT;
	}
	state->destination.config.size.w = pstDst->u32Width;
	state->destination.config.size.h = pstDst->u32Height;
	state->dst.phy = pstDst->phyAddr;
	state->dst.pitch = pstDst->u32Stride;
#else
	ASSIGN_SURFACE(&state->destination, &state->dst, pstDst, u32ColorVal);
#endif

	state->blittingflags = 0;
	state->src_blend = HCGE_DSBF_SRCALPHA;
	state->dst_blend = HCGE_DSBF_ZERO;
	srect.x = pstDstRect->s32Xpos;
	srect.y = pstDstRect->s32Ypos;
	srect.w = pstDstRect->u32Width;
	srect.h = pstDstRect->u32Height;

	hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_fill_rect(ctx, &srect);
	return E_MI_GFX_ERR_SUCCESS;
}

MI_S32 MI_GFX_GetAlphaThresholdValue(MI_U8 *pu8ThresholdValue)
{
	(void)pu8ThresholdValue;
	return E_MI_GFX_ERR_GFX_DRV_NOT_SUPPORT;
}

MI_S32 MI_GFX_SetAlphaThresholdValue(MI_U8 u8ThresholdValue)
{
	(void)u8ThresholdValue;
	return E_MI_GFX_ERR_GFX_DRV_NOT_SUPPORT;
}

MI_S32 MI_GFX_BitBlit(MI_GFX_Surface_t *pstSrc, MI_GFX_Rect_t *pstSrcRect,
                      MI_GFX_Surface_t *pstDst,  MI_GFX_Rect_t *pstDstRect, MI_GFX_Opt_t *pstOpt, MI_U16 *pu16Fence)
{
	(void)pu16Fence;
	hcge_state *state = &ctx->state;
	if(!ctx) {
		return E_MI_GFX_ERR_NOT_INIT;
	}

	state->render_options = 0;
	state->drawingflags = 0;
	state->blittingflags = 0;
	state->affine_matrix = false;

	state->mod_hw = HCGE_SMF_CLIP;

	state->clip.x1 = pstOpt->stClipRect.s32Xpos;
	state->clip.y1 = pstOpt->stClipRect.s32Ypos;
	state->clip.x2 = pstOpt->stClipRect.s32Xpos + pstOpt->stClipRect.u32Width - 1;
	state->clip.y2 = pstOpt->stClipRect.s32Ypos + pstOpt->stClipRect.u32Height - 1;

	if(conv2colorkey(&pstOpt->stSrcColorKeyInfo, &pstOpt->stDstColorKeyInfo, state)) {
		printf("Not support colorkey settings\n");
		return E_MI_GFX_ERR_GFX_DRV_NOT_SUPPORT;
	}

	state->src_blend = pstOpt->eSrcDfbBldOp + 1;
	state->dst_blend = pstOpt->eDstDfbBldOp + 1;

	if(E_MI_GFX_MIRROR_HORIZONTAL == pstOpt->eMirror)
		state->blittingflags |= HCGE_DSBLIT_FLIP_HORIZONTAL;
	else if(E_MI_GFX_MIRROR_VERTICAL == pstOpt->eMirror)
		state->blittingflags |= HCGE_DSBLIT_FLIP_VERTICAL;
	else if(E_MI_GFX_MIRROR_BOTH == pstOpt->eMirror)
		state->blittingflags |= HCGE_DSBLIT_FLIP_HORIZONTAL | HCGE_DSBLIT_FLIP_VERTICAL;

	if(state->blittingflags & (HCGE_DSBLIT_FLIP_HORIZONTAL | HCGE_DSBLIT_FLIP_VERTICAL))
		state->mod_hw = HCGE_SMF_NONE;

	if( E_MI_GFX_ROTATE_90 == pstOpt->eRotate)
		state->blittingflags |= HCGE_DSBLIT_ROTATE90;
	else if ( E_MI_GFX_ROTATE_180 == pstOpt->eRotate)
		state->blittingflags |= HCGE_DSBLIT_ROTATE180;
	else if ( E_MI_GFX_ROTATE_270 == pstOpt->eRotate)
		state->blittingflags |= HCGE_DSBLIT_ROTATE270;
	switch(pstOpt->eDFBBlendFlag) {
	case E_MI_GFX_DFB_BLEND_COLORALPHA:
		state->blittingflags |= HCGE_DSBLIT_BLEND_COLORALPHA;
		break;
	case E_MI_GFX_DFB_BLEND_ALPHACHANNEL:
		state->blittingflags |= HCGE_DSBLIT_BLEND_ALPHACHANNEL;
		break;
	case E_MI_GFX_DFB_BLEND_COLORIZE:
		state->blittingflags |= HCGE_DSBLIT_COLORIZE;
		break;
	case E_MI_GFX_DFB_BLEND_SRC_PREMULTIPLY:
		state->blittingflags |= HCGE_DSBLIT_SRC_PREMULTIPLY;
		break;
	case E_MI_GFX_DFB_BLEND_SRC_PREMULTCOLOR:
		state->blittingflags |= HCGE_DSBLIT_SRC_PREMULTCOLOR;
		break;
	case E_MI_GFX_DFB_BLEND_DST_PREMULTIPLY:
		state->blittingflags |= HCGE_DSBLIT_DST_PREMULTIPLY;
		break;
	case E_MI_GFX_DFB_BLEND_XOR:
		state->blittingflags |= HCGE_DSBLIT_XOR;
		break;
	case E_MI_GFX_DFB_BLEND_DEMULTIPLY:
		state->blittingflags |= HCGE_DSBLIT_DEMULTIPLY;
		break;
#if 1
	case E_MI_GFX_DFB_BLEND_SRC_COLORKEY:
		/*state->blittingflags |= HCGE_DSBLIT_SRC_COLORKEY;*/
		printf("%s:%d,TODO: may be a bug\n",__func__, __LINE__);
		break;
	case E_MI_GFX_DFB_BLEND_DST_COLORKEY:
		/*state->blittingflags |= HCGE_DSBLIT_DST_COLORKEY;*/
		printf("%s:%d,TODO: may be a bug\n",__func__, __LINE__);
		break;
#endif
	default:
		break;
	}
	HCGERectangle srect;
	HCGERectangle drect;

	srect.x = pstSrcRect->s32Xpos;
	srect.y = pstSrcRect->s32Ypos;
	srect.w = pstSrcRect->u32Width;
	srect.h = pstSrcRect->u32Height;

	drect.x = pstDstRect->s32Xpos;
	drect.y = pstDstRect->s32Ypos;
	drect.w = pstDstRect->u32Width;
	drect.h = pstDstRect->u32Height;

	ASSIGN_SURFACE(&state->destination, &state->dst, pstDst, pstOpt->u32GlobalDstConstColor);
	ASSIGN_SURFACE(&state->source, &state->src, pstSrc, pstOpt->u32GlobalSrcConstColor);
	if((srect.w == drect.w && srect.h == drect.h) ||
			(state->blittingflags & (HCGE_DSBLIT_ROTATE90 | HCGE_DSBLIT_ROTATE180 | HCGE_DSBLIT_ROTATE270))
			||  (state->blittingflags & (HCGE_DSBLIT_FLIP_HORIZONTAL | HCGE_DSBLIT_FLIP_VERTICAL))) {
		state->accel = HCGE_DFXL_BLIT;
		hcge_set_state(ctx, state, state->accel);
		hcge_blit(ctx, &srect, drect.x, drect.y);
	} else {
		state->accel = HCGE_DFXL_STRETCHBLIT;
		hcge_set_state(ctx, state, state->accel);
		hcge_stretch_blit(ctx, &srect, &drect);
	}

	return E_MI_GFX_ERR_SUCCESS;
}
