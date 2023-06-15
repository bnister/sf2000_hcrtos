#ifndef __HCGE_API_H_H_
#define __HCGE_API_H_H_

#include <stdbool.h>
#include <stdint.h>

struct cmdq_node_context;

//unit is byte
#define HCGE_IMAGE_PITCH_MAX (4096 * 4)

/*
 * @internal
 */
#define HCGE_DFB_DRAWING_FUNCTION(a)    ((a) & 0x0000FFFF)

/*
 * @internal
 */
#define HCGE_DFB_BLITTING_FUNCTION(a)   ((a) & 0xFFFF0000)


/*
 * Mask of accelerated functions.
 * hcge is only support HCGE_DFXL_BLIT,HCGE_DFXL_STRETCHBLIT, HCGE_DFXL_FILLRECTANGLE.
 * HCGE_DFXL_DRAWRECTANGLE operation is ver bad performance.
 */
typedef enum {
    HCGE_DFXL_NONE           = 0x00000000,  /* None of these. */

    HCGE_DFXL_FILLRECTANGLE  = 0x00000001,  /* FillRectangle() is accelerated. */
    HCGE_DFXL_DRAWRECTANGLE  = 0x00000002,  /* DrawRectangle() is accelerated. */

    HCGE_DFXL_DRAWLINE       = 0x00000004,  /* Not supported at present. DrawLine() is accelerated. */
    HCGE_DFXL_FILLTRIANGLE   = 0x00000008,  /* Not supported at present. FillTriangle() is accelerated. */
    HCGE_DFXL_FILLTRAPEZOID  = 0x00000010,  /* Not supported at present. FillTrapezoid() is accelerated. */
    HCGE_DFXL_FILLQUADRANGLE = 0x00000020,  /* Not supported at present. FillQuadrangle() is accelerated. */
    HCGE_DFXL_FILLSPAN       = 0x00000040,  /* Not supported at present. FillSpan() is accelerated. */
    HCGE_DFXL_DRAWMONOGLYPH  = 0x00001000,  /* Not supported at present. DrawMonoGlyphs() is accelerated. */

    HCGE_DFXL_BLIT           = 0x00010000,  /* Blit() and TileBlit() are accelerated. */
    HCGE_DFXL_STRETCHBLIT    = 0x00020000,  /* StretchBlit() is accelerated. */

    HCGE_DFXL_TEXTRIANGLES   = 0x00040000,  /* Not supported at present. TextureTriangles() is accelerated. */
    HCGE_DFXL_BLIT2          = 0x00080000,  /* Not supported at present. BatchBlit2() is accelerated. */
    HCGE_DFXL_TILEBLIT       = 0x00100000,  /* Not supported at present. TileBlit() is accelerated. */

    HCGE_DFXL_DRAWSTRING     = 0x01000000,  /* Not supported at present. DrawString() and DrawGlyph() are accelerated. */

    HCGE_DFXL_ALL            = 0x011F007F,  /* Not supported at present. All drawing/blitting functions. */
    HCGE_DFXL_ALL_DRAW       = 0x0000107F,  /* Not supported at present. All drawing functions. */
    HCGE_DFXL_ALL_BLIT       = 0x011F0000   /* Not supported at present. All blitting functions. */
} HCGEAccelerationMask;

/*
 * Flags controlling drawing commands.
 */
typedef enum {
    HCGE_DSDRAW_NOFX               = 0x00000000, /* uses none of the effects */
    HCGE_DSDRAW_BLEND              = 0x00000001, /* uses alpha from color */
    HCGE_DSDRAW_DST_COLORKEY       = 0x00000002, /* write to destination only if the destination pixel
                                                matches the destination color key */
    HCGE_DSDRAW_SRC_PREMULTIPLY    = 0x00000004, /* multiplies the color's rgb channels by the alpha
                                                channel before drawing */
    HCGE_DSDRAW_DST_PREMULTIPLY    = 0x00000008, /* modulates the dest. color with the dest. alpha */
    HCGE_DSDRAW_DEMULTIPLY         = 0x00000010, /* divides the color by the alpha before writing the
                                                data to the destination */
    HCGE_DSDRAW_XOR                = 0x00000020  /* bitwise xor the destination pixels with the
                                                specified color after premultiplication */
} HCGESurfaceDrawingFlags;

/*
 * Options for drawing and blitting operations. Not mandatory for acceleration.
 */
typedef enum {
    HCGE_DSRO_NONE                 = 0x00000000, /* None of these. */

    HCGE_DSRO_SMOOTH_UPSCALE       = 0x00000001, /* Use interpolation for upscale StretchBlit(). */
    HCGE_DSRO_SMOOTH_DOWNSCALE     = 0x00000002, /* Use interpolation for downscale StretchBlit(). */
    HCGE_DSRO_MATRIX               = 0x00000004, /* Use the transformation matrix set via IDirectFBSurface::SetMatrix(). */

    HCGE_DSRO_ANTIALIAS            = 0x00000008, /* Not supported at present. Enable anti-aliasing for edges (alphablend must be enabled). */

    HCGE_DSRO_WRITE_MASK_BITS      = 0x00000010, /* Not supported at present. Enable usage of write mask bits setting. */

    HCGE_DSRO_ALL                  = 0x0000001F  /* Not supported at present. All of these. */
} HCGESurfaceRenderOptions;

typedef enum
{
    HCGE_RGB_OP_EQUAL = 0,
    HCGE_RGB_OP_NOT_EQUAL,
    HCGE_ALPHA_OP_EQUAL,
    HCGE_ALPHA_OP_NOT_EQUAL,
    HCGE_ARGB_OP_EQUAL,
    HCGE_ARGB_OP_NOT_EQUAL,
    HCGE_CKEY_OP_MAX,
} HCGEColorKeyOp;

/*
 * Flags controlling blitting commands.
 */
typedef enum {
    //ussing
    HCGE_DSBLIT_NOFX                   = 0x00000000, /* uses none of the effects */
    HCGE_DSBLIT_BLEND_ALPHACHANNEL     = 0x00000001, /* enables blending and uses
                                                    alphachannel from source */
    HCGE_DSBLIT_BLEND_COLORALPHA       = 0x00000002, /* enables blending and uses
                                                    alpha value from color */
    HCGE_DSBLIT_COLORIZE               = 0x00000004, /* modulates source color with
                                                    the color's r/g/b values */
    HCGE_DSBLIT_SRC_COLORKEY           = 0x00000008, /* don't blit pixels matching the source color key */
    HCGE_DSBLIT_DST_COLORKEY           = 0x00000010, /* write to destination only if the destination pixel
                                                    matches the destination color key */
    HCGE_DSBLIT_SRC_PREMULTIPLY        = 0x00000020, /* modulates the source color with the (modulated)
                                                    source alpha */
    HCGE_DSBLIT_DST_PREMULTIPLY        = 0x00000040, /* modulates the dest. color with the dest. alpha */
    HCGE_DSBLIT_DEMULTIPLY             = 0x00000080, /* divides the color by the alpha before writing the
                                                    data to the destination */

    HCGE_DSBLIT_DEINTERLACE            = 0x00000100, /* Not supported at present. deinterlaces the source during blitting by reading
                                                    only one field (every second line of full
                                                    image) scaling it vertically by factor two */
    HCGE_DSBLIT_SRC_PREMULTCOLOR       = 0x00000200, /* modulates the source color with the color alpha */
    HCGE_DSBLIT_XOR                    = 0x00000400, /* bitwise xor the destination pixels with the
                                                    source pixels after premultiplication */
    HCGE_DSBLIT_INDEX_TRANSLATION      = 0x00000800, /* Not supported at present. do fast indexed to indexed translation,
                                                    this flag is mutual exclusive with all others */

    HCGE_DSBLIT_ROTATE90               = 0x00002000, /* rotate the image by 90 degree */
    HCGE_DSBLIT_ROTATE180              = 0x00001000, /* rotate the image by 180 degree */
    HCGE_DSBLIT_ROTATE270              = 0x00004000, /* rotate the image by 270 degree */

    HCGE_DSBLIT_COLORKEY_PROTECT       = 0x00010000, /* make sure written pixels don't match color key (internal only ATM) */

    HCGE_DSBLIT_SRC_COLORKEY_EXTENDED  = 0x00020000, /* Not supported at present. use extended source color key */
    HCGE_DSBLIT_DST_COLORKEY_EXTENDED  = 0x00040000, /* Not supported at present. use extended destination color key */

    HCGE_DSBLIT_SRC_MASK_ALPHA         = 0x00100000, /* modulate source alpha channel with alpha channel from source mask,
                                                    see also IDirectFBSurface::SetSourceMask() */

    HCGE_DSBLIT_SRC_MASK_COLOR         = 0x00200000, /* Not supported at present. modulate source color channels with color channels from source mask,
                                                    see also IDirectFBSurface::SetSourceMask() */
    HCGE_DSBLIT_FLIP_HORIZONTAL        = 0x01000000, /* Not supported at present. flip the image horizontally */
    HCGE_DSBLIT_FLIP_VERTICAL          = 0x02000000, /* Not supported at present. flip the image vertically */

    HCGE_DSBLIT_ROP                    = 0x04000000, /* Not supported at present. use rop setting */
    HCGE_DSBLIT_SRC_COLORMATRIX        = 0x08000000, /* Not supported at present. use source color matrix setting */
    HCGE_DSBLIT_SRC_CONVOLUTION        = 0x10000000,  /* Not supported at present. use source convolution filter */
	HCGE_CUST_DST_COLORKEY = 0x20000000, /* destination apply colorkey */
	HCGE_CUST_SRC_COLORKEY = 0x40000000, /* destination apply colorkey */
} HCGESurfaceBlittingFlags;

/*
 * Blend functions to use for source and destination blending
 */
typedef enum {
    /*
     * pixel color = sc * cf[sf] + dc * cf[df]
     * pixel alpha = sa * af[sf] + da * af[df]
     * sc = source color
     * sa = source alpha
     * dc = destination color
     * da = destination alpha
     * sf = source blend function
     * df = destination blend function
     * cf[x] = color factor for blend function x
     * af[x] = alpha factor for blend function x
     */
    HCGE_DSBF_UNKNOWN            = 0,  /*                             */
    HCGE_DSBF_ZERO               = 1,  /* cf:    0           af:    0 */
    HCGE_DSBF_ONE                = 2,  /* cf:    1           af:    1 */
    HCGE_DSBF_SRCCOLOR           = 3,  /* cf:   sc           af:   sa */
    HCGE_DSBF_INVSRCCOLOR        = 4,  /* cf: 1-sc           af: 1-sa */
    HCGE_DSBF_SRCALPHA           = 5,  /* cf:   sa           af:   sa */
    HCGE_DSBF_INVSRCALPHA        = 6,  /* cf: 1-sa           af: 1-sa */
    HCGE_DSBF_DESTALPHA          = 7,  /* cf:   da           af:   da */
    HCGE_DSBF_INVDESTALPHA       = 8,  /* cf: 1-da           af: 1-da */
    HCGE_DSBF_DESTCOLOR          = 9,  /* cf:   dc           af:   da */
    HCGE_DSBF_INVDESTCOLOR       = 10, /* cf: 1-dc           af: 1-da */
    HCGE_DSBF_SRCALPHASAT        = 11  /* cf: min(sa, 1-da)  af:    1 */
} HCGESurfaceBlendFunction;

/*
 * Blend functions to use for source and destination blending
 * used for blend_mode != HCGE_BLEND_DFB
 */
typedef enum {
    HCGE_DSBF_SRC_OVER,
    HCGE_DSBF_DST_OVER,
    HCGE_DSBF_SRC,
    HCGE_DSBF_DST,
    HCGE_DSBF_SRC_IN,
    HCGE_DSBF_DST_IN,
    HCGE_DSBF_SRC_OUT,
    HCGE_DSBF_DST_OUT,
    HCGE_DSBF_SRC_ATOP,
    HCGE_DSBF_DST_ATOP,
    HCGE_DSBF_XOR,
    HCGE_DSBF_CLEAR,
} HCGESurfaceAlphaBlendFunction;

/*
 * set blending mode
 * */
typedef enum {
	HCGE_BLEND_DFB,
	HCGE_BLEND_ALPHA,
}HCGESurfaceBlendMode;


/*
 * @internal
 *
 * Encodes format constants in the following way (bit 31 - 0):
 *
 * lkjj:hhgg | gfff:eeed | cccc:bbbb | baaa:aaaa
 *
 * a) pixelformat index<br>
 * b) effective color (or index) bits per pixel of format<br>
 * c) effective alpha bits per pixel of format<br>
 * d) alpha channel present<br>
 * e) bytes per "pixel in a row" (1/8 fragment, i.e. bits)<br>
 * f) bytes per "pixel in a row" (decimal part, i.e. bytes)<br>
 * g) smallest number of pixels aligned to byte boundary (minus one)<br>
 * h) multiplier for planes minus one (1/4 fragment)<br>
 * j) multiplier for planes minus one (decimal part)<br>
 * k) color and/or alpha lookup table present<br>
 * l) alpha channel is inverted
 */
#define HCGE_SURFACE_PIXELFORMAT( index, color_bits, alpha_bits, has_alpha,     \
                                 row_bits, row_bytes, align, mul_f, mul_d,     \
                                 has_lut, inv_alpha )                          \
     ( (((index     ) & 0x7F)      ) |                                         \
       (((color_bits) & 0x1F) <<  7) |                                         \
       (((alpha_bits) & 0x0F) << 12) |                                         \
       (((has_alpha ) ? 1 :0) << 16) |                                         \
       (((row_bits  ) & 0x07) << 17) |                                         \
       (((row_bytes ) & 0x07) << 20) |                                         \
       (((align     ) & 0x07) << 23) |                                         \
       (((mul_f     ) & 0x03) << 26) |                                         \
       (((mul_d     ) & 0x03) << 28) |                                         \
       (((has_lut   ) ? 1 :0) << 30) |                                         \
       (((inv_alpha ) ? 1 :0) << 31) )

/*
 * Pixel format of a surface.
 */
typedef enum {
    HCGE_DSPF_UNKNOWN   = 0x00000000,  /* unknown or unspecified format */

    /* 16 bit  ARGB (2 byte, alpha 1@15, red 5@10, green 5@5, blue 5@0) */
    HCGE_DSPF_ARGB1555  = HCGE_SURFACE_PIXELFORMAT(  0, 15, 1, 1, 0, 2, 0, 0, 0, 0, 0 ),

    /* 16 bit   RGB (2 byte, red 5@11, green 6@5, blue 5@0) */
    HCGE_DSPF_RGB16     = HCGE_SURFACE_PIXELFORMAT(  1, 16, 0, 0, 0, 2, 0, 0, 0, 0, 0 ),

    /* Not supported at present. 24 bit   RGB (3 byte, red 8@16, green 8@8, blue 8@0) */
    HCGE_DSPF_RGB24     = HCGE_SURFACE_PIXELFORMAT(  2, 24, 0, 0, 0, 3, 0, 0, 0, 0, 0 ),

    /* 24 bit   RGB (4 byte, nothing@24, red 8@16, green 8@8, blue 8@0) */
    HCGE_DSPF_RGB32     = HCGE_SURFACE_PIXELFORMAT(  3, 24, 0, 0, 0, 4, 0, 0, 0, 0, 0 ),

    /* 32 bit  ARGB (4 byte, alpha 8@24, red 8@16, green 8@8, blue 8@0) */
    HCGE_DSPF_ARGB      = HCGE_SURFACE_PIXELFORMAT(  4, 24, 8, 1, 0, 4, 0, 0, 0, 0, 0 ),

    /*  8 bit alpha (1 byte, alpha 8@0), e.g. anti-aliased glyphs */
    HCGE_DSPF_A8        = HCGE_SURFACE_PIXELFORMAT(  5,  0, 8, 1, 0, 1, 0, 0, 0, 0, 0 ),

    /*  8 bit   LUT (8 bit color and alpha lookup from palette) */
    HCGE_DSPF_LUT8      = HCGE_SURFACE_PIXELFORMAT( 11,  8, 0, 1, 0, 1, 0, 0, 0, 1, 0 ),

    /* 16 bit  ARGB (2 byte, alpha 4@12, red 4@8, green 4@4, blue 4@0) */
    HCGE_DSPF_ARGB4444  = HCGE_SURFACE_PIXELFORMAT( 18, 12, 4, 1, 0, 2, 0, 0, 0, 0, 0 ),

    /* 16 bit   RGB (2 byte, nothing @12, red 4@8, green 4@4, blue 4@0) */
    HCGE_DSPF_RGB444    = HCGE_SURFACE_PIXELFORMAT( 27, 12, 0, 0, 0, 2, 0, 0, 0, 0, 0 ),

    /* 16 bit   RGB (2 byte, nothing @15, red 5@10, green 5@5, blue 5@0) */
    HCGE_DSPF_RGB555    = HCGE_SURFACE_PIXELFORMAT( 28, 15, 0, 0, 0, 2, 0, 0, 0, 0, 0 ),

    /* 16 bit   BGR (2 byte, nothing @15, blue 5@10, green 5@5, red 5@0) */
    HCGE_DSPF_BGR555    = HCGE_SURFACE_PIXELFORMAT( 29, 15, 0, 0, 0, 2, 0, 0, 0, 0, 0 ),
} HCGESurfacePixelFormat;

typedef enum {
    HCGE_SMF_NONE              = 0x00000000,

    HCGE_SMF_CLIP              = 0x00000004,
} HCGEStateModificationFlags;

/*
 * Flags controlling surface masks set via IDirectFBSurface::SetSourceMask().
 */
typedef enum {
    HCGE_DSMF_NONE      = 0x00000000,  /* None of these. */

    HCGE_DSMF_STENCIL   = 0x00000001,  /* Take <b>x</b> and <b>y</b> as fixed start coordinates in the mask. */

    HCGE_DSMF_ALL       = 0x00000001   /* All of these. */
} HCGESurfaceMaskFlags;

#define HC_SUPPORTED_DRAW_FUNCS           \
    (HCGE_DFXL_FILLRECTANGLE/* | DFXL_DRAWRECTANGLE*/) // the performance of draw rect is bad than sw engine.

#define HC_SUPPORTED_BLIT_FUNCS           \
    (HCGE_DFXL_BLIT | HCGE_DFXL_STRETCHBLIT)

#define HC_SUPPORTED_DRAW_FLAGS           \
    (HCGE_DSDRAW_NOFX           |                    \
     HCGE_DSDRAW_BLEND          |                    \
     HCGE_DSDRAW_DST_COLORKEY   |                    \
     HCGE_DSDRAW_SRC_PREMULTIPLY|                    \
     HCGE_DSDRAW_DST_PREMULTIPLY|                    \
     HCGE_DSBLIT_DEMULTIPLY     |                    \
     HCGE_DSDRAW_XOR)

#define HC_SUPPORTED_BLIT_FLAGS           \
    (HCGE_DSBLIT_NOFX              |                 \
     HCGE_DSBLIT_BLEND_ALPHACHANNEL|                 \
     HCGE_DSBLIT_BLEND_COLORALPHA  |                 \
     HCGE_DSBLIT_SRC_COLORKEY      |                 \
     HCGE_DSBLIT_DST_COLORKEY      |                 \
     HCGE_DSBLIT_COLORIZE          |                 \
     HCGE_DSBLIT_SRC_PREMULTIPLY   |                 \
     HCGE_DSBLIT_DST_PREMULTIPLY   |                 \
     HCGE_DSBLIT_SRC_PREMULTCOLOR  |                 \
     HCGE_DSBLIT_FLIP_HORIZONTAL   |                 \
     HCGE_DSBLIT_FLIP_VERTICAL     |                 \
     HCGE_DSBLIT_XOR               |                 \
     HCGE_DSBLIT_SRC_MASK_ALPHA    |                 \
     HCGE_DSBLIT_COLORKEY_PROTECT  |                 \
     HCGE_DSBLIT_ROTATE90 | HCGE_DSBLIT_ROTATE180 | HCGE_DSBLIT_ROTATE270 | \
     HCGE_DSBLIT_DEMULTIPLY)

#define SUPPORTED_BLIT(accel) (accel & HC_SUPPORTED_BLIT_FUNCS)
#define SUPPORTED_DRAW(accel) (accel & HC_SUPPORTED_DRAW_FUNCS)

#define SUPPORTED_BLIT_FLAG(flag) ((flag == HCGE_DSBLIT_NOFX) || (flag & HC_SUPPORTED_BLIT_FLAGS))
#define SUPPORTED_DRAW_FLAG(flag) ((flag == HCGE_DSDRAW_NOFX) || (flag & HC_SUPPORTED_DRAW_FLAGS))

/*
 * A color defined by channels with 8bit each.
 */
typedef struct {
    uint8_t             a;   /* alpha channel */
    uint8_t             r;   /* red channel */
    uint8_t             g;   /* green channel */
    uint8_t             b;   /* blue channel */
} HCGEColor;

/*
 * A dimension specified by width and height.
 */
typedef struct {
    int            w;   /* width of it */
    int            h;   /* height of it */
} HCGEDimension;

typedef struct {
    HCGESurfacePixelFormat    format;
    HCGEDimension             size;
} HCGE_CoreSurfaceConfig;

typedef struct {
    unsigned int  num_entries;
    HCGEColor     *entries;
} HCGE_CorePalette;


typedef struct {
    HCGE_CoreSurfaceConfig config;
    HCGE_CorePalette       palette;
} HCGE_CoreSurface;

/*
 * A Lock on a Surface Buffer
 */
typedef struct  {
    unsigned long            phys;               /* " */
    unsigned int             pitch;              /* " */
} HCGE_CoreSurfaceBuffer;

/*
 * A region specified by two points.
 *
 * The defined region includes both endpoints.
 */
typedef struct {
    int            x1;  /* X coordinate of top-left point */
    int            y1;  /* Y coordinate of top-left point */
    int            x2;  /* X coordinate of lower-right point */
    int            y2;  /* Y coordinate of lower-right point */
} HCGERegion;

/*
 * A rectangle specified by a point and a dimension.
 */
typedef struct {
    int            x;   /* X coordinate of its top-left point */
    int            y;   /* Y coordinate of its top-left point */
    int            w;   /* width of it */
    int            h;   /* height of it */
} HCGERectangle;

/*
 * A point specified by x/y coordinates.
 */
typedef struct {
    int            x;   /* X coordinate of it */
    int            y;   /* Y coordinate of it */
} HCGEPoint;

typedef struct hcge_state {
    HCGESurfaceRenderOptions    render_options;
    HCGESurfaceDrawingFlags     drawingflags;
    HCGEAccelerationMask        accel;
    HCGESurfaceBlittingFlags    blittingflags; /* blitting flags */

    //only for directfb used
    HCGEAccelerationMask        set;

    HCGEPoint                   src_mask_offset;    /* relative or absolute coordinates */
    HCGESurfaceMaskFlags        src_mask_flags;     /* controls coordinate mode and more */

    int32_t                     matrix[9];          /* transformation matrix for HCGE_DSRO_MATRIX (fixed 16.16) */
    bool                        affine_matrix;
    HCGE_CoreSurface            destination;   /* destination surface */
    HCGE_CoreSurface            source;        /* source surface */
    HCGE_CoreSurface            source_mask;        /* source mask surface */

    HCGEColor                   color;
    uint32_t                    dst_colorkey;  /* colorkey for destination */
    uint32_t                    src_colorkey;  /* colorkey for source */


    //blend_mode !=  HCGE_BLEND_DFB
    uint32_t  colorkey_max;//source
    uint32_t  colorkey_min;//dest
    
    HCGESurfaceBlendMode blend_mode;
    HCGESurfaceAlphaBlendFunction blend_operation;

    HCGESurfaceBlendFunction    src_blend;     /* blend function for source */
    HCGESurfaceBlendFunction    dst_blend;     /* blend function for destination */

    /* if blittingflags set HCGE_CUST_SRC_COLORKEY, this field neet set. */
    HCGEColorKeyOp src_colorkey_opt;
    /* if blittingflags set HCGE_CUST_DST_COLORKEY, this field neet set. */
    HCGEColorKeyOp dst_colorkey_opt;

    HCGE_CoreSurfaceBuffer  	dst;
    HCGE_CoreSurfaceBuffer  	src;
    HCGE_CoreSurfaceBuffer  	src_mask;           /* source mask surface lock */

    HCGEStateModificationFlags  mod_hw;
    HCGERegion                  clip;          /* clipping rectangle */
} hcge_state;

typedef struct hcge_context {
    hcge_state state;
    uint32_t                         soc_chip_id;/* soc chip id */
    volatile struct ge_reg_buf *ge_regs;
    uint32_t                         cmdq_buf_map_phyaddr;
    volatile uint8_t                *cmdq_buf_map_vaddr; /* HW cmdQ buffer mmaped to userspace */
    uint32_t                         cmdq_buf_map_size;

    /*
     * message queue buffer
     */
    //command queue physical address start
    uint32_t                         cmdq_buf_phyaddr;
    //command queue virtual address start
    uint32_t                         cmdq_buf_vaddr;
    //command queue buffer size
    uint32_t                         cmdq_buf_size;

    /* clut table physical address */
    uint32_t                         clut_tbl_paddr;
    uint32_t                         ge_ip_id;
    int ge_fd;

    /*
     * When blit from src to dst with blitting flag DSBLIT_NOFX, and
     * dst/src has the same color format, and the matrix is not
     * enabled. use GE to direct-copy feature to gain max preformace.
     */
    bool blit_direct;
    bool clip_en;

    /* enable log dump or not */
    bool log_en;

    /*
     * msk
     */
    bool                     msk_en;

    /*
     * set clut_utlized flag to true when the encounter a clut cmdq
     * node the FIRST time.
     */
    bool clut_utilized;

    bool last_op_not_matrix;
    /*
     * GE cmdq context for DFB, used by SetState callback of GE's DFB
     * gfx driver to prepaire registers that will be contructed to
     * be CmdQ nodes to be feeded to GE HW.
     */
    uint32_t                      draw_color; /* draw color for draw/fill ops */

    /*
     * optimize translate matrix to use blit offset instead
     */
    uint32_t                      x_translate;
    uint32_t                      y_translate;

    /*
     * matrix elements convert to float from DFB
     */
    bool                     matrix_en;
    double                    matrix[6];

    /*
     * node contex: programmed registeres to be contructed to become q
     * cmdq node
     */
    void *nd_ctx;

    /*
     * Node buffer where a node(nodes for a draw/blit/fill) is constructed:
     *    - 4 sources     : bitmap addr takes 2*4
     *    - 168 word      : node buffer takes at least 168 word
     *    - 2 word margin :
     */
    uint32_t                   node_buf[2*4+168+2];

    uint32_t rotation_degree;
} hcge_context;

int hcge_open(hcge_context **pctx);
void hcge_close(hcge_context *ctx);
void hcge_hw_reset(hcge_context *ctx);
void hcge_reset(struct hcge_context *ctx);

//this api for directfb, only directfb used
void hcge_check_state(hcge_state  *state, HCGEAccelerationMask  accel);

int hcge_engine_sync(hcge_context *ctx);
void hcge_engine_reset( void *drv, void *dev);
void hcge_flush_texture_cache(void *drv, void *dev);
void hcge_emit_jommands(void *drv, void *dev);
bool hcge_draw_rect(hcge_context *ctx, HCGERectangle *rect);
bool hcge_fill_rect(hcge_context *ctx, HCGERectangle *rect);
bool hcge_blit(hcge_context *ctx, HCGERectangle *srect, int dx, int dy);
bool hcge_stretch_blit(hcge_context *ctx, HCGERectangle *srect, HCGERectangle *drect);
void hcge_set_state(hcge_context *ctx, hcge_state *state, HCGEAccelerationMask  accel);

void hcge_fill_rect_ext(hcge_context *ctx, HCGE_CoreSurfaceBuffer *dst, HCGE_CoreSurface *surface, HCGERectangle *rect, HCGEColor *color);
#endif
