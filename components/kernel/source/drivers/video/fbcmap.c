#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <kernel/fb.h>

#define min_t(type, x, y)                                                      \
	({                                                                     \
		type __min1 = (x);                                             \
		type __min2 = (y);                                             \
		__min1 < __min2 ? __min1 : __min2;                             \
	})

static uint16_t red16[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
    0x5555, 0x5555, 0x5555, 0x5555, 0xffff, 0xffff, 0xffff, 0xffff
};
static uint16_t green16[] = {
    0x0000, 0x0000, 0xaaaa, 0xaaaa, 0x0000, 0x0000, 0x5555, 0xaaaa,
    0x5555, 0x5555, 0xffff, 0xffff, 0x5555, 0x5555, 0xffff, 0xffff
};
static uint16_t blue16[] = {
    0x0000, 0xaaaa, 0x0000, 0xaaaa, 0x0000, 0xaaaa, 0x0000, 0xaaaa,
    0x5555, 0xffff, 0x5555, 0xffff, 0x5555, 0xffff, 0x5555, 0xffff
};

static const struct fb_cmap default_16_colors = {
    .len=16, .red=red16, .green=green16, .blue=blue16
};

const struct fb_cmap *fb_default_cmap(int len)
{
    return &default_16_colors;
}

int fb_copy_cmap(const struct fb_cmap *from, struct fb_cmap *to)
{
	unsigned int tooff = 0, fromoff = 0;
	size_t size;

	if (to->start > from->start)
		fromoff = to->start - from->start;
	else
		tooff = from->start - to->start;
	if (fromoff >= from->len || tooff >= to->len)
		return -EINVAL;

	size = min_t(size_t, to->len - tooff, from->len - fromoff);
	if (size == 0)
		return -EINVAL;
	size *= sizeof(uint16_t);

	memcpy(to->red+tooff, from->red+fromoff, size);
	memcpy(to->green+tooff, from->green+fromoff, size);
	memcpy(to->blue+tooff, from->blue+fromoff, size);
	if (from->transp && to->transp)
		memcpy(to->transp+tooff, from->transp+fromoff, size);
	return 0;
}

void fb_dealloc_cmap(struct fb_cmap *cmap)
{
	if (cmap->red)
		free(cmap->red);
	if (cmap->green)
		free(cmap->green);
	if (cmap->blue)
		free(cmap->blue);
	if (cmap->transp)
		free(cmap->transp);

	cmap->red = cmap->green = cmap->blue = cmap->transp = NULL;
	cmap->len = 0;
}

int fb_alloc_cmap(struct fb_cmap *cmap, int len, int transp)
{
	int size = len * sizeof(uint16_t);
	int ret = -ENOMEM;

	if (cmap->len != len) {
		fb_dealloc_cmap(cmap);
		if (!len)
			return 0;

		cmap->red = malloc(size);
		if (!cmap->red)
			goto fail;
		cmap->green = malloc(size);
		if (!cmap->green)
			goto fail;
		cmap->blue = malloc(size);
		if (!cmap->blue)
			goto fail;
		if (transp) {
			cmap->transp = malloc(size);
			if (!cmap->transp)
				goto fail;
		} else {
			cmap->transp = NULL;
		}
	}
	cmap->start = 0;
	cmap->len = len;
	ret = fb_copy_cmap(fb_default_cmap(len), cmap);
	if (ret)
		goto fail;
	return 0;

fail:
	fb_dealloc_cmap(cmap);
	return ret;
}

int fb_set_cmap(struct fb_cmap *cmap, struct fb_info *info)
{
	int i, start, rc = 0;
	uint16_t *red, *green, *blue, *transp;
	unsigned int hred, hgreen, hblue, htransp = 0xffff;

	red = cmap->red;
	green = cmap->green;
	blue = cmap->blue;
	transp = cmap->transp;
	start = cmap->start;

	if (start < 0 || (!info->fbops->fb_setcolreg &&
			  !info->fbops->fb_setcmap))
		return -EINVAL;
	if (info->fbops->fb_setcmap) {
		rc = info->fbops->fb_setcmap(cmap, info);
	} else {
		for (i = 0; i < cmap->len; i++) {
			hred = *red++;
			hgreen = *green++;
			hblue = *blue++;
			if (transp)
				htransp = *transp++;
			if (info->fbops->fb_setcolreg(start++,
						      hred, hgreen, hblue,
						      htransp, info))
				break;
		}
	}
	if (rc == 0)
		fb_copy_cmap(cmap, &info->cmap);

	return rc;
}

int fb_set_user_cmap(struct fb_cmap *cmap, struct fb_info *info)
{
	int rc, size = cmap->len * sizeof(uint16_t);

	if (size < 0 || size < cmap->len)
		return -EINVAL;
	rc = fb_set_cmap(cmap, info);
	return rc;
}

int fb_cmap_to_user(const struct fb_cmap *from, struct fb_cmap *to)
{
	unsigned int tooff = 0, fromoff = 0;
	size_t size;

	if (to->start > from->start)
		fromoff = to->start - from->start;
	else
		tooff = from->start - to->start;
	if (fromoff >= from->len || tooff >= to->len)
		return -EINVAL;

	size = min_t(size_t, to->len - tooff, from->len - fromoff);
	if (size == 0)
		return -EINVAL;
	size *= sizeof(uint16_t);

	memcpy(to->red+tooff, from->red+fromoff, size);
	memcpy(to->green+tooff, from->green+fromoff, size);
	memcpy(to->blue+tooff, from->blue+fromoff, size);
	if (from->transp && to->transp)
		memcpy(to->transp+tooff, from->transp+fromoff, size);
	return 0;
}
