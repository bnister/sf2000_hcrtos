#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <nuttx/fs/fs.h>
#include "hcfb.h"

static inline void unlock_fb_info(struct fb_info *info)
{
	xSemaphoreGive(info->lock);
}

int lock_fb_info(struct fb_info *info)
{
	xSemaphoreTake(info->lock, portMAX_DELAY);
}

int
fb_pan_display(struct fb_info *info, struct fb_var_screeninfo *var)
{
	struct fb_fix_screeninfo *fix = &info->fix;
	unsigned int yres = info->var.yres;
	int err = 0;

	if (var->yoffset > 0) {
		if (var->vmode & FB_VMODE_YWRAP) {
			if (!fix->ywrapstep || (var->yoffset % fix->ywrapstep))
				err = -EINVAL;
			else
				yres = 0;
		} else if (!fix->ypanstep || (var->yoffset % fix->ypanstep))
			err = -EINVAL;
	}

	if (var->xoffset > 0 && (!fix->xpanstep ||
				 (var->xoffset % fix->xpanstep)))
		err = -EINVAL;

	if (err || !info->fbops->fb_pan_display ||
	    var->yoffset > info->var.yres_virtual - yres ||
	    var->xoffset > info->var.xres_virtual - info->var.xres)
		return -EINVAL;

	if ((err = info->fbops->fb_pan_display(var, info)))
		return err;
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
	if (var->vmode & FB_VMODE_YWRAP)
		info->var.vmode |= FB_VMODE_YWRAP;
	else
		info->var.vmode &= ~FB_VMODE_YWRAP;
	return 0;
}

static int fb_open(struct file *filep)
{
	int res = 0;
	struct inode *inode;
	struct fb_info *info;

	inode = filep->f_inode;
	info = (struct fb_info *)inode->i_private;

	lock_fb_info(info);
	if (info->fbops->fb_open) {
		res = info->fbops->fb_open(info, 1);
	}
	unlock_fb_info(info);
	return res;
}

static int fb_close(FAR struct file *filep)
{
	struct inode *inode;
	struct fb_info *info;

	inode = filep->f_inode;
	info = (struct fb_info *)inode->i_private;

	lock_fb_info(info);
	if (info->fbops->fb_release)
		info->fbops->fb_release(info, 1);
	unlock_fb_info(info);
}

static ssize_t fb_read(struct file *filep, char *buffer, size_t len)
{
	struct inode *inode;
	struct fb_info *info;
	size_t start;
	size_t end;
	size_t size;
	size_t total_size;

	inode = filep->f_inode;
	info = (struct fb_info *)inode->i_private;

	/* Get the start and size of the transfer */

	total_size = info->screen_size;

	if (total_size == 0)
		total_size = info->fix.smem_len;

	start = filep->f_pos;
	if (start >= total_size) {
		return 0; /* Return end-of-file */
	}

	end = start + len;
	if (end >= total_size) {
		end = total_size;
	}

	size = end - start;

	/* And transfer the data from the frame buffer */

	memcpy(buffer, info->screen_base + start, size);
	filep->f_pos += size;
	return size;
}

static ssize_t fb_write(struct file *filep, const char *buffer,
                        size_t len)
{
	struct inode *inode;
	struct fb_info *info;
	size_t start;
	size_t end;
	size_t size;
	size_t total_size;

	inode = filep->f_inode;
	info = (struct fb_info *)inode->i_private;

	if (!info || !info->screen_base)
		return -ENODEV;

	total_size = info->screen_size;
	if (total_size == 0)
		total_size = info->fix.smem_len;

	start = filep->f_pos;
	if (start >= total_size) {
		return -EFBIG; /* Cannot extend the framebuffer */
	}

	end = start + len;
	if (end >= total_size) {
		end = total_size;
	}

	size = end - start;

	/* And transfer the data into the frame buffer */

	memcpy(info->screen_base + start, buffer, size);
	filep->f_pos += size;
	return size;
}

static off_t fb_seek(struct file *filep, off_t offset, int whence)
{
	struct inode *inode;
	struct fb_info *info;
	off_t newpos;
	int ret;
	size_t total_size;

	inode = filep->f_inode;
	info = (struct fb_info *)inode->i_private;

	if (!info || !info->screen_base)
		return -ENODEV;

	total_size = info->screen_size;
	if (total_size == 0)
		total_size = info->fix.smem_len;

	/* Determine the new, requested file position */

	switch (whence) {
	case SEEK_CUR:
		newpos = filep->f_pos + offset;
		break;

	case SEEK_SET:
		newpos = offset;
		break;

	case SEEK_END:
		newpos = total_size + offset;
		break;

	default:

		/* Return EINVAL if the whence argument is invalid */

		return -EINVAL;
	}

	/* Opengroup.org:
	 *
	 *  "The lseek() function shall allow the file offset to be set beyond the
	 *   end of the existing data in the file. If data is later written at this
	 *   point, subsequent reads of data in the gap shall return bytes with the
	 *   value 0 until data is actually written into the gap."
	 *
	 * We can conform to the first part, but not the second.  Return EINVAL if
	 *  "...the resulting file offset would be negative for a regular file,
	 *   block special file, or directory."
	 */

	if (newpos >= 0) {
		filep->f_pos = newpos;
		ret = newpos;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

int
fb_set_var(struct fb_info *info, struct fb_var_screeninfo *var)
{
	int ret = 0;

	if (memcmp(&info->var, var, sizeof(struct fb_var_screeninfo))) {
		ret = info->fbops->fb_check_var(var, info);

		if (ret)
			goto done;

		if (1) {
			struct fb_var_screeninfo old_var;

			old_var = info->var;
			info->var = *var;

			if (info->fbops->fb_set_par) {
				ret = info->fbops->fb_set_par(info);

				if (ret) {
					info->var = old_var;
					printf("detected "
						"fb_set_par error, "
						"error code: %d\n", ret);
					goto done;
				}
			}

			fb_pan_display(info, &info->var);
			fb_set_cmap(&info->cmap, info);
		}
	}

 done:
	return ret;
}


static int fb_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	struct inode *inode;
	struct fb_info *info;
	struct fb_ops *fb;
	int ret = 0;

	inode = filep->f_inode;
	info = (struct fb_info *)inode->i_private;

	/* Process the IOCTL command */

	switch (cmd) {
	case FIOC_MMAP: /* Get color plane info */
	{
		void **ppv = (void **)((uintptr_t)arg);
		struct hcfb_device *fbdev = (struct hcfb_device *)info->par;

		/* Return the address corresponding to the start of frame buffer. */

		if(fbdev->mmap_cache == HCFB_MMAP_NO_CACHE){
			*ppv = (void *)MIPS_UNCACHED_ADDR(info->fix.smem_start);
		} else {
			*ppv = (void *)MIPS_CACHED_ADDR(info->fix.smem_start);
		}
		ret = 0;
		break;
	}

	case FBIOBLANK: /* Blank or unblank video overlay */
		lock_fb_info(info);
		if (info->fbops->fb_blank)
 			ret = info->fbops->fb_blank(arg, info);
		unlock_fb_info(info);
		break;

	case FBIOPAN_DISPLAY:
		lock_fb_info(info);
		ret = fb_pan_display(info, (struct fb_var_screeninfo *)arg);
		unlock_fb_info(info);
		break;

	case FBIOGET_FSCREENINFO:
		lock_fb_info(info);
		memcpy((struct fb_fix_screeninfo *)arg, &info->fix, sizeof(info->fix));
		unlock_fb_info(info);
		ret = 0;
		break;

	case FBIOGET_VSCREENINFO:
		lock_fb_info(info);
		memcpy((struct fb_var_screeninfo *)arg, &info->var, sizeof(info->var));
		unlock_fb_info(info);
		ret = 0;
		break;

	case FBIOPUT_VSCREENINFO:
		lock_fb_info(info);
		ret = fb_set_var(info, (struct fb_var_screeninfo *)arg);
		unlock_fb_info(info);
		break;

	case FBIOPUTCMAP:
		lock_fb_info(info);
		ret = fb_set_user_cmap((struct fb_cmap *)arg, info);
		unlock_fb_info(info);
		break;

	case FBIOGETCMAP:
		lock_fb_info(info);
		ret = fb_cmap_to_user(&info->cmap, (struct fb_cmap *)arg);
		unlock_fb_info(info);
		break;

	default:
		if (!lock_fb_info(info))
			return -ENODEV;
		fb = info->fbops;
		if (fb->fb_ioctl)
			ret = fb->fb_ioctl(info, cmd, arg);
		else
			ret = -ENOTTY;
		unlock_fb_info(info);
	}

	return ret;
}

static const struct file_operations fb_fops = {
	fb_open, /* open */
	fb_close, /* close */
	fb_read, /* read */
	fb_write, /* write */
	fb_seek, /* seek */
	fb_ioctl, /* ioctl */
	NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	NULL /* unlink */
#endif
};

void framebuffer_release(struct fb_info *info)
{
	if (!info)
		return;

	vSemaphoreDelete(info->lock);

	free(info);
}

struct fb_info *framebuffer_alloc(size_t size)
{
#define BYTES_PER_LONG (4)
#define PADDING (BYTES_PER_LONG - (sizeof(struct fb_info) % BYTES_PER_LONG))
	int fb_info_size = sizeof(struct fb_info);
	struct fb_info *info;
	char *p;

	if (size)
		fb_info_size += PADDING;

	p = malloc(fb_info_size + size);
	if (!p)
		return NULL;
	memset(p, 0, fb_info_size + size);

	info = (struct fb_info *) p;

	if (size)
		info->par = p + fb_info_size;

	info->lock = xSemaphoreCreateMutex();

	return info;
#undef PADDING
#undef BYTES_PER_LONG
}

int register_framebuffer(struct fb_info *fb_info)
{
	char name[128];
	int ret;

	snprintf(name, 16, "/dev/fb%d", fb_info->node);
	ret = register_driver(name, &fb_fops, 0666, (void *)fb_info);
	if (ret < 0) {
		printf("ERROR: register_driver() failed: %d\n", ret);
		return -1;
	}

	return 0;
}
