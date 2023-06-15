#ifndef _KERNEL_HCFB_H_
#define _KERNEL_HCFB_H_

#include <kernel/types.h>
#include <hcuapi/iocbase.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define FBIOGET_VSCREENINFO	_IOR(HCFB_IOCBASE, 20, struct fb_var_screeninfo)
#define FBIOPUT_VSCREENINFO	_IOW(HCFB_IOCBASE, 21, struct fb_var_screeninfo)
#define FBIOGET_FSCREENINFO	_IOR(HCFB_IOCBASE, 22, struct fb_fix_screeninfo)
#define FBIOPAN_DISPLAY		_IOW(HCFB_IOCBASE, 23, struct fb_var_screeninfo)
#define FBIO_WAITFORVSYNC	_IOWR(HCFB_IOCBASE, 24, uint32_t)
#define FBIOBLANK		_IO  (HCFB_IOCBASE, 25)
#define FBIOGETCMAP             _IOR(HCFB_IOCBASE, 26, struct fb_cmap)
#define FBIOPUTCMAP             _IOW(HCFB_IOCBASE, 27, struct fb_cmap)

/* Only for HCRTOS */
#define HCFBIOPUT_FSCREENINFO		_IOW(HCFB_IOCBASE, 28, struct fb_fix_screeninfo)

#define FB_VMODE_YWRAP          256     /* ywrap instead of panning     */

struct fb_fix_screeninfo {
	unsigned long smem_start;	/* Start of frame buffer mem */
					/* (physical address) */
	uint32_t smem_len;		/* Length of frame buffer mem */
	uint32_t line_length;		/* length of a line in bytes    */
	uint16_t xpanstep;		/* zero if no hardware panning  */
	uint16_t ypanstep;		/* zero if no hardware panning  */
	uint16_t ywrapstep;		/* zero if no hardware ywrap    */
};

/* Interpretation of offset for color fields: All offsets are from the right,
 * inside a "pixel" value, which is exactly 'bits_per_pixel' wide (means: you
 * can use the offset as right argument to <<). A pixel afterwards is a bit
 * stream and is written to video memory as that unmodified.
 *
 * For pseudocolor: offset and length should be the same for all color
 * components. Offset specifies the position of the least significant bit
 * of the pallette index in a pixel value. Length indicates the number
 * of available palette entries (i.e. # of entries = 1 << length).
 */
struct fb_bitfield {
	uint32_t offset;		/* beginning of bitfield	*/
	uint32_t length;		/* length of bitfield		*/
	uint32_t msb_right;		/* != 0 : Most significant bit is */ 
					/* right */ 
};

struct fb_var_screeninfo {
	uint32_t xres;			/* visible resolution           */
	uint32_t yres;
	uint32_t xres_virtual;		/* virtual resolution           */
	uint32_t yres_virtual;
	uint32_t xoffset;		/* offset from virtual to visible */
	uint32_t yoffset;		/* resolution                   */

	uint32_t bits_per_pixel;	/* guess what                   */
	uint32_t vmode;			/* see FB_VMODE_*               */

	struct fb_bitfield red;   
	struct fb_bitfield green; 
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

struct fb_cmap {
	uint32_t start;			/* First entry	*/
	uint32_t len;			/* Number of entries */
	uint16_t *red;			/* Red values	*/
	uint16_t *green;
	uint16_t *blue;
	uint16_t *transp;		/* transparency, can be NULL */
};

/* VESA Blanking Levels */
#define VESA_NO_BLANKING        0

enum {
	/* screen: unblanked, hsync: on,  vsync: on */
	FB_BLANK_UNBLANK = VESA_NO_BLANKING,

	/* screen: blanked,   hsync: on,  vsync: on */
	FB_BLANK_NORMAL = VESA_NO_BLANKING + 1,
};

struct fb_ops;
struct fb_info {
	int node;
	SemaphoreHandle_t lock;
	struct fb_var_screeninfo var;	/* Current var */
	struct fb_fix_screeninfo fix;	/* Current fix */
	struct fb_cmap cmap;		/* Current cmap */
	char *screen_base;		/* Virtual address */
	unsigned long screen_size;	/* Amount of ioremapped VRAM or 0 */ 
	struct fb_ops *fbops;
	void *par;
};

struct fb_ops {
	/* open/release and usage marking */
	int (*fb_open)(struct fb_info *info, int user);
	int (*fb_release)(struct fb_info *info, int user);

	/* checks var and eventually tweaks it to something supported,
	 * DO NOT MODIFY PAR */
	int (*fb_check_var)(struct fb_var_screeninfo *var, struct fb_info *info);

	/* set the video mode according to info->var */
	int (*fb_set_par)(struct fb_info *info);

	/* set color register */
	int (*fb_setcolreg)(unsigned regno, unsigned red, unsigned green,
			    unsigned blue, unsigned transp, struct fb_info *info);

	/* set color registers in batch */
	int (*fb_setcmap)(struct fb_cmap *cmap, struct fb_info *info);

	/* blank display */
	int (*fb_blank)(int blank, struct fb_info *info);

	/* pan display */
	int (*fb_pan_display)(struct fb_var_screeninfo *var, struct fb_info *info);

	/* perform fb specific ioctl (optional) */
	int (*fb_ioctl)(struct fb_info *info, unsigned int cmd,
			unsigned long arg);
};

int fb_alloc_cmap(struct fb_cmap *cmap, int len, int transp);
void fb_dealloc_cmap(struct fb_cmap *cmap);
int fb_copy_cmap(const struct fb_cmap *from, struct fb_cmap *to);
int fb_set_cmap(struct fb_cmap *cmap, struct fb_info *info);
extern const struct fb_cmap *fb_default_cmap(int len);
int fb_cmap_to_user(const struct fb_cmap *from, struct fb_cmap *to);
int fb_set_user_cmap(struct fb_cmap *cmap, struct fb_info *info);

int register_framebuffer(struct fb_info *fb_info);
struct fb_info *framebuffer_alloc(size_t size);
void framebuffer_release(struct fb_info *info);

#endif
