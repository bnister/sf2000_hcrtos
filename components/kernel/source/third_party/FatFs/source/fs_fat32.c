#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/mount.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/kmalloc.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/dirent.h>
#include <fsutils/mkfatfs.h>
#include <kernel/elog.h>
#include <kernel/list.h>

#include "ff.h"
#include "diskio.h"

/* Mount status flags (ff_bflags) */

#define UMOUNT_FORCED        8

struct fat_file_s;
struct fat_mountpt_s {
	struct fat_blkdrv blkdrv;
	struct list_head __files;        /* list head to all files opened on this mountpoint */
	bool fs_mounted;                 /* true: The file system is ready */
	FATFS fatfs;
};

struct fat_file_s
{
	struct list_head list;           /* Retained in a dual linked list */
	uint8_t ff_bflags;               /* The file buffer/mount flags */
	FIL fil;
};

typedef union {
	struct {
		uint16_t mday : 5;       /* Day of month, 1 - 31 */
		uint16_t mon : 4;        /* Month, 1 - 12 */
		uint16_t year : 7;       /* Year, counting from 1980. E.g. 37 for 2017 */
	};
	uint16_t as_int;
} fat_date_t;

typedef union {
	struct {
		uint16_t sec : 5;       /* Seconds divided by 2. E.g. 21 for 42 seconds */
		uint16_t min : 6;       /* Minutes, 0 - 59 */
		uint16_t hour : 5;      /* Hour, 0 - 23 */
	};
	uint16_t as_int;
} fat_time_t;

static int     fat_open(FAR struct file *filep, FAR const char *relpath,
                 int oflags, mode_t mode);
static int     fat_close(FAR struct file *filep);
static ssize_t fat_read(FAR struct file *filep, FAR char *buffer,
                 size_t buflen);
static ssize_t fat_write(FAR struct file *filep, FAR const char *buffer,
                 size_t buflen);
static off_t   fat_seek(FAR struct file *filep, off_t offset, int whence);
static int     fat_ioctl(FAR struct file *filep, int cmd,
                 unsigned long arg);
static int     fat_sync(FAR struct file *filep);
static int     fat_dup(FAR const struct file *oldp, FAR struct file *newp);
static int     fat_fstat(FAR const struct file *filep,
                 FAR struct stat *buf);
static int     fat_truncate(FAR struct file *filep, off_t length);
static int     fat_opendir(FAR struct inode *mountpt,
                 FAR const char *relpath, FAR struct fs_dirent_s *dir);
static int     fat_closedir(FAR struct inode *mountpt,
                 FAR struct fs_dirent_s *dir);
static int     fat_readdir(FAR struct inode *mountpt,
                 FAR struct fs_dirent_s *dir);
static int     fat_rewinddir(FAR struct inode *mountpt,
                 FAR struct fs_dirent_s *dir);
static int     fat_bind(FAR struct inode *blkdriver, FAR const void *data,
                 FAR void **handle);
static int     fat_unbind(FAR void *handle,
                 FAR struct inode **blkdriver, unsigned int flags);
static int     fat_statfs(FAR struct inode *mountpt,
                 FAR struct statfs *buf);
static int     fat_unlink(FAR struct inode *mountpt,
                 FAR const char *relpath);
static int     fat_mkdir(FAR struct inode *mountpt, FAR const char *relpath,
                 mode_t mode);
static int     fat_rmdir(FAR struct inode *mountpt, FAR const char *relpath);
static int     fat_rename(FAR struct inode *mountpt,
                 FAR const char *oldrelpath, FAR const char *newrelpath);
static int     fat_stat(struct inode *mountpt, const char *relpath,
                 FAR struct stat *buf);

const struct mountpt_operations elmfat_operations =
{
	fat_open,          /* open */
	fat_close,         /* close */
	fat_read,          /* read */
	fat_write,         /* write */
	fat_seek,          /* seek */
	fat_ioctl,         /* ioctl */
	
	fat_sync,          /* sync */
	fat_dup,           /* dup */
	fat_fstat,         /* fstat */
	NULL,              /* truncate */
	
	fat_opendir,       /* opendir */
	fat_closedir,      /* closedir */
	fat_readdir,       /* readdir */
	fat_rewinddir,     /* rewinddir */
	
	fat_bind,          /* bind */
	fat_unbind,        /* unbind */
	fat_statfs,        /* statfs */
	
	fat_unlink,        /* unlink */
	fat_mkdir,         /* mkdir */
	fat_rmdir,         /* rmdir */
	fat_rename,        /* rename */
	fat_stat           /* stat */
};

static int fatfs_errno(int result)
{
	int status = 0;

	if (result < 0) {
		return result;
	}

	/* FatFs errno to Libc errno */
	switch (result) {
	case FR_OK:
		break;

	case FR_NO_FILE:
	case FR_NO_PATH:
	case FR_NO_FILESYSTEM:
		status = -ENOENT;
		break;

	case FR_INVALID_NAME:
		status = -EINVAL;
		break;

	case FR_EXIST:
	case FR_INVALID_OBJECT:
		status = -EEXIST;
		break;

	case FR_NOT_ENABLED:
	case FR_INVALID_DRIVE:
	case FR_DISK_ERR:
	case FR_NOT_READY:
	case FR_INT_ERR:
		status = -EIO;
		break;

	case FR_WRITE_PROTECTED:
		status = -EROFS;
		break;
	case FR_MKFS_ABORTED:
	case FR_INVALID_PARAMETER:
		status = -EINVAL;
		break;

	case FR_DENIED:
		status = -EPERM;
		break;
	case FR_LOCKED:
		status = -EBUSY;
		break;
	case FR_TIMEOUT:
		status = -ETIMEDOUT;
		break;
	case FR_NOT_ENOUGH_CORE:
		status = -ENOMEM;
		break;
	case FR_TOO_MANY_OPEN_FILES:
		status = -ENFILE;
		break;
	default:
		status = -result;
		break;
	}

	return status;
}

static int fat_checkmount(struct fat_mountpt_s *fs)
{
	/* If the fs_mounted flag is false, then we have already handled the loss
	 * of the mount.
	 */

	if (fs && fs->fs_mounted) {
		/* We still think the mount is healthy.  Check an see if this is
		 * still the case
		 */

		if (fs->blkdrv.fs_blkdriver) {
			struct inode *inode = fs->blkdrv.fs_blkdriver;
			if (inode && inode->u.i_bops &&
					inode->u.i_bops->geometry) {
				struct geometry geo;
				int errcode =
					inode->u.i_bops->geometry(inode, &geo);
				if (errcode == OK && geo.geo_available &&
						!geo.geo_mediachanged) {
					return OK;
				}
			}
		}

		/* If we get here, the mount is NOT healthy */

		fs->fs_mounted = false;
	}

	return -ENODEV;
}

static BYTE fat_get_mode(int oflags)
{
	BYTE fmode = FA_READ;

	if ((uint32_t)oflags & O_WRONLY) {
		fmode |= FA_WRITE;
	}

	if (((uint32_t)oflags & O_ACCMODE) & O_RDWR) {
		fmode |= FA_WRITE;
	}
	/* Creates a new file if the file is not existing, otherwise, just open it. */
	if ((uint32_t)oflags & O_CREAT) {
		fmode |= FA_OPEN_ALWAYS;
		/* Creates a new file. If the file already exists, the function shall fail. */
		if ((uint32_t)oflags & O_EXCL) {
			fmode |= FA_CREATE_NEW;
		}
	}
	/* Creates a new file. If the file already exists, its length shall be truncated to 0. */
	if ((uint32_t)oflags & O_TRUNC) {
		fmode |= FA_CREATE_ALWAYS;
	}

	return fmode;
}

static int fat_open(FAR struct file *filep, FAR const char *relpath,
                    int oflags, mode_t mode)
{
	FAR struct inode *inode;
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *ff;
	BYTE fmode;
	FRESULT res;
	int ret;

	DEBUGASSERT(filep->f_priv == NULL && filep->f_inode != NULL);
	inode = filep->f_inode;
	fs = inode->i_private;

	DEBUGASSERT(fs != NULL);

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	ff = (FAR struct fat_file_s *)kmm_zalloc(sizeof(struct fat_file_s));
	if (!ff) {
		return -ENOMEM;
	}

	fmode = fat_get_mode(oflags);

	res = f_open(&fs->fatfs, &ff->fil, relpath, fmode);
	if (res != FR_OK) {
		kmm_free(ff);
		return fatfs_errno(res);
	}

	filep->f_priv = ff;

	taskENTER_CRITICAL();
	list_add_tail(&ff->list, &fs->__files);
	taskEXIT_CRITICAL();

	return OK;
}

static int fat_close(FAR struct file *filep)
{
	FAR struct inode *inode;
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *ff;
	FRESULT res = OK;
	int ret;

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);

	inode = filep->f_inode;
	fs = inode->i_private;

	ff = filep->f_priv;

	ret = fat_checkmount(fs);
	if (ret == OK) {
		res = f_close(&ff->fil);
		if (res != FR_OK) {
			ret = fatfs_errno(res);
		}
	}

	taskENTER_CRITICAL();
	list_del_init(&ff->list);
	taskEXIT_CRITICAL();

	kmm_free(ff);
	filep->f_priv = NULL;
	return ret;
}

static ssize_t fat_read(FAR struct file *filep, FAR char *buffer,
                        size_t buflen)
{
	FAR struct inode *inode;
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *ff;
	FRESULT res;
	ssize_t nbytes;
	int ret;

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);
	inode = filep->f_inode;
	fs = inode->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	ff = filep->f_priv;

	/* Check for the forced mount condition */
	if ((ff->ff_bflags & UMOUNT_FORCED) != 0) {
		return -EPIPE;
	}

	res = f_read(&ff->fil, (void *)buffer, (UINT)buflen, (UINT *)&nbytes);
	if (res != FR_OK) {
		return fatfs_errno(res);
	}

	return nbytes;
}

static ssize_t fat_write(FAR struct file *filep, FAR const char *buffer,
                         size_t buflen)
{
	FAR struct inode *inode;
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *ff;
	FRESULT res;
	ssize_t nbytes;
	int ret;

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);
	ff = filep->f_priv;

	/* Check for the forced mount condition */
	if ((ff->ff_bflags & UMOUNT_FORCED) != 0) {
		return -EPIPE;
	}

	inode = filep->f_inode;
	fs = inode->i_private;
	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_write(&ff->fil, (void *)buffer, (UINT)buflen, (UINT *)&nbytes);
	if (res != FR_OK) {
		return fatfs_errno(res);
	}

	return nbytes;
}

static off_t fat_seek(FAR struct file *filep, off_t offset, int whence)
{
	FAR struct inode *inode;
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *ff;
	off_t position;
	FRESULT res;
	int ret;

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);
	ff = filep->f_priv;

	/* Check for the forced mount condition */
	if ((ff->ff_bflags & UMOUNT_FORCED) != 0) {
		return -EPIPE;
	}

	switch (whence) {
	case SEEK_SET: /* The offset is set to offset bytes. */
		position = offset;
		break;

	case SEEK_CUR: /* The offset is set to its current location plus
			* offset bytes. */

		position = offset + filep->f_pos;
		break;

	case SEEK_END: /* The offset is set to the size of the file plus
			* offset bytes. */

		position = offset + f_size(&ff->fil);
		break;

	default:
		return -EINVAL;
	}

	inode = filep->f_inode;
	fs = inode->i_private;
	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_lseek(&ff->fil, position);
	if (res != FR_OK) {
		return fatfs_errno(res);
	}

	filep->f_pos = position;

	return position;
}

static int fat_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	return -ENOSYS;
}

static int fat_sync(FAR struct file *filep)
{
	FAR struct inode *inode;
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *ff;
	FRESULT res;
	int ret;

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);
	ff = filep->f_priv;

	/* Check for the forced mount condition */
	if ((ff->ff_bflags & UMOUNT_FORCED) != 0) {
		return -EPIPE;
	}

	inode = filep->f_inode;
	fs = inode->i_private;
	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_sync(&ff->fil);
	return fatfs_errno(res);
}

static int fat_dup(FAR const struct file *oldp, FAR struct file *newp)
{
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *oldff;
	FAR struct fat_file_s *newff;
	int ret;

	DEBUGASSERT(oldp->f_priv != NULL && newp->f_priv == NULL &&
		    newp->f_inode != NULL);
	oldff = oldp->f_priv;

	/* Check for the forced mount condition */
	if ((oldff->ff_bflags & UMOUNT_FORCED) != 0) {
		return -EPIPE;
	}

	fs = (struct fat_mountpt_s *)oldp->f_inode->i_private;
	DEBUGASSERT(fs != NULL);

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	/* Create a new instance of the file private date to describe the
	 * dup'ed file.
	 */

	newff = (FAR struct fat_file_s *)kmm_malloc(sizeof(struct fat_file_s));
	if (!newff) {
		return -ENOMEM;
	}

	/* Create a file buffer to support partial sector accesses */

	newff->ff_bflags = 0; /* File buffer flags */
	memcpy(&newff->fil, &oldff->fil, sizeof(FIL));
	newp->f_priv = newff;

	taskENTER_CRITICAL();
	list_add_tail(&newff->list, &fs->__files);
	taskEXIT_CRITICAL();

	return OK;
}

static int fat_opendir(FAR struct inode *mountpt, FAR const char *relpath,
                       FAR struct fs_dirent_s *dir)
{
	FAR struct fat_mountpt_s *fs;
	FDIR *fdir;
	FRESULT res;
	int ret;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	fdir = (FDIR *)kmm_zalloc(sizeof(FDIR));
	if (!fdir)
		return -ENOMEM;

	res = f_opendir(&fs->fatfs, fdir, relpath);
	if (res != FR_OK) {
		kmm_free(fdir);
		return fatfs_errno(res);
	}

	dir->u.elmfat.fs_dir = fdir;
	return OK;
}

static int fat_closedir(FAR struct inode *mountpt, FAR struct fs_dirent_s *dir)
{
	FAR struct fat_mountpt_s *fs;
	FDIR *fdir;
	FRESULT res;
	int ret;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	fdir = dir->u.elmfat.fs_dir;
	if (!fdir)
		return -EBADF;

	ret = fat_checkmount(fs);
	if (ret == OK) {
		res = f_closedir(fdir);
		ret = fatfs_errno(res);
	}

	dir->u.elmfat.fs_dir = NULL;
	kmm_free(fdir);

	return ret;
}

static int fat_fstat(FAR const struct file *filep, FAR struct stat *buf)
{
	FAR struct inode *inode;
	FAR struct fat_mountpt_s *fs;
	FAR struct fat_file_s *ff;
	int ret;

	if (buf == NULL) {
		return -EFAULT;
	}

	inode = filep->f_inode;
	fs = inode->i_private;
	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);
	ff    = filep->f_priv;

	buf->st_size = f_size(&ff->fil);
	buf->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR |
		       S_IWGRP | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH;
	if (ff->fil.obj.attr & AM_RDO) {
		buf->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	}

	return OK;
}

static int fat_readdir(FAR struct inode *mountpt,
                       FAR struct fs_dirent_s *dir)
{
	FAR struct fat_mountpt_s *fs;
	FDIR *fdir;
	FILINFO finfo = {0};
	FRESULT res;
	int ret;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	fdir = dir->u.elmfat.fs_dir;
	if (!fdir)
		return -EBADF;

	dir->fd_dir.d_name[0] = '\0';
	res = f_readdir(fdir, &finfo);
	if ((res != FR_OK) || (finfo.fname[0] == 0x0)) {
		return -ENOENT;
	}

	if (finfo.fattrib & AM_DIR) {
		dir->fd_dir.d_type = DTYPE_DIRECTORY;
	} else {
		dir->fd_dir.d_type = DTYPE_FILE;
	}

	strncpy(dir->fd_dir.d_name, finfo.fname, CONFIG_NAME_MAX);
	dir->fd_dir.d_name[CONFIG_NAME_MAX] = '\0';

	return OK;
}

static int fat_rewinddir(FAR struct inode *mountpt,
                         FAR struct fs_dirent_s *dir)
{
	FAR struct fat_mountpt_s *fs;
	FDIR *fdir;
	FRESULT res;
	int ret;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	fdir = dir->u.elmfat.fs_dir;
	if (!fdir)
		return -EBADF;
	
	res = f_rewinddir(fdir);
	return fatfs_errno(res);
}

static int fat_mount(struct fat_mountpt_s *fs, bool writeable)
{
	FAR struct inode *inode;
	struct geometry geo;
	int ret;
	FRESULT res;

	/* Assume that the mount is successful */

	fs->fs_mounted = true;

	/* Check if there is media available */

	inode = fs->blkdrv.fs_blkdriver;
	if (!inode || !inode->u.i_bops || !inode->u.i_bops->geometry ||
			inode->u.i_bops->geometry(inode, &geo) != OK ||
			!geo.geo_available) {
		ret = -ENODEV;
		goto errout;
	}

	/* Make sure that that the media is write-able (if write access is
	 * needed).
	 */

	if (writeable && !geo.geo_writeenabled) {
		ret = -EACCES;
		goto errout;
	}

	/* Save the hardware geometry */

	fs->blkdrv.fs_hwsectorsize = geo.geo_sectorsize;
	fs->blkdrv.fs_hwnsectors = geo.geo_nsectors;
	switch (fs->blkdrv.fs_hwsectorsize) {
	case 512:
		fs->blkdrv.fs_hwsectshift = 9;
		break;
	case 1024:
		fs->blkdrv.fs_hwsectshift = 10;
		break;
	case 2048:
		fs->blkdrv.fs_hwsectshift = 11;
		break;
	case 4096:
		fs->blkdrv.fs_hwsectshift = 12;
		break;
	default:
		log_e("ERROR: Unsupported sector size: %" PRId64 "\n", fs->blkdrv.fs_hwsectorsize);
		ret = -EPERM;
		goto errout;;
	}

	/* We did it! */
	res = f_mount(&fs->fatfs, &fs->blkdrv, 1);
	if (res != FR_OK) {
		f_unmount(&fs->fatfs, &fs->blkdrv, 0);
		log_e("ERROR: mount fail: %d\n", -res);
		ret = fatfs_errno(res);
		goto errout;
	}

	return OK;

errout:
	fs->fs_mounted = false;
	return ret;
}


static int fat_bind(FAR struct inode *blkdriver, FAR const void *data,
                    FAR void **handle)
{
	struct fat_mountpt_s *fs;
	int ret;

	/* Open the block driver */

	if (!blkdriver || !blkdriver->u.i_bops) {
		return -ENODEV;
	}

	if (blkdriver->u.i_bops->open &&
	    blkdriver->u.i_bops->open(blkdriver) != OK) {
		return -ENODEV;
	}

	/* Create an instance of the mountpt state structure */

	fs = (struct fat_mountpt_s *)kmm_zalloc(sizeof(struct fat_mountpt_s));
	if (!fs) {
		if (blkdriver->u.i_bops && blkdriver->u.i_bops->close) {
			blkdriver->u.i_bops->close(blkdriver);
		}
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&fs->__files);

	/* Initialize the allocated mountpt state structure.  The filesystem is
	 * responsible for one reference on the blkdriver inode and does not
	 * have to addref() here (but does have to release in unbind().
	 */

	fs->blkdrv.fs_blkdriver = blkdriver; /* Save the block driver reference */

	ret = fat_mount(fs, true);
	if (ret != 0) {
		if (blkdriver->u.i_bops && blkdriver->u.i_bops->close) {
			blkdriver->u.i_bops->close(blkdriver);
		}
		kmm_free(fs);
		return ret;
	}

	*handle = (FAR void *)fs;
	return OK;
}

static int fat_unbind(FAR void *handle, FAR struct inode **blkdriver,
                      unsigned int flags)
{
	FAR struct fat_mountpt_s *fs = (FAR struct fat_mountpt_s *)handle;

	if (!fs) {
		return -EINVAL;
	}

	if (fs->fs_mounted == true) {
		f_unmount(&fs->fatfs, &fs->blkdrv, 0);
		fs->fs_mounted = false;
	}

	/* Check if there are sill any files opened on the filesystem. */
	taskENTER_CRITICAL();
	if (!list_empty(&fs->__files)) {
		/* There are open files.  We umount now unless we are forced with the
		 * MNT_FORCE flag.  Forcing the unmount will cause data loss because
		 * the filesystem buffers are not flushed to the media. MNT_DETACH,
		 * the 'lazy' unmount, could be implemented to fix this.
		 */

		if ((flags & MNT_FORCE) != 0) {
			FAR struct fat_file_s *ff;

			/* Set a flag in each open file structure.  This flag will signal
			 * the file system to fail any subsequent attempts to used the
			 * file handle.
			 */

			list_for_each_entry(ff, &fs->__files, list) {
				ff->ff_bflags |= UMOUNT_FORCED;
			}
		} else {
			/* We cannot unmount now.. there are open files.  This
			 * implementation does not support any other umount2()
			 * options.
			 */
			taskEXIT_CRITICAL();
			return (flags != 0) ? -ENOSYS : -EBUSY;
		}
	}
	taskEXIT_CRITICAL();

	/* Unmount ... close the block driver */

	if (fs->blkdrv.fs_blkdriver) {
		FAR struct inode *inode = fs->blkdrv.fs_blkdriver;
		if (inode) {
			if (inode->u.i_bops && inode->u.i_bops->close) {
				inode->u.i_bops->close(inode);
			}

			/* We hold a reference to the block driver but should not but
			 * mucking with inodes in this context.  So, we will just return
			 * our contained reference to the block driver inode and let the
			 * umount logic dispose of it.
			 */

			if (blkdriver) {
				*blkdriver = inode;
			}
		}
	}

	/* Release the mountpoint private data */
	kmm_free(fs);
	return OK;
}

static int fat_statfs(FAR struct inode *mountpt, FAR struct statfs *buf)
{
	FAR struct fat_mountpt_s *fs;
	DWORD freeClust;
	FRESULT res;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}

	DEBUGASSERT(mountpt && mountpt->i_private);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_getfree(&fs->fatfs, &freeClust);
	if (res != FR_OK) {
		return fatfs_errno(res);
	}

	/* Fill in the statfs info */
	memset(buf, 0, sizeof(struct statfs));
	buf->f_type = MSDOS_SUPER_MAGIC;

	buf->f_bfree = freeClust;
	buf->f_bavail = freeClust;
	/* Cluster #0 and #1 is for VBR, reserve sectors and fat */
	buf->f_blocks = fs->fatfs.n_fatent - 2;
#if FF_MAX_SS != FF_MIN_SS
	buf->f_bsize = fs->fatfs.ssize * fs->fatfs.csize;
#else
	buf->f_bsize = FF_MIN_SS * fs->fatfs.csize;
#endif
#if FF_USE_LFN == 0
	buf->f_namelen = (8 + 1 + 3); /* Maximum length of filenames */
#else
	buf->f_namelen = FF_MAX_LFN; /* Maximum length of filenames */
#endif
	return OK;
}

static int fat_unlink(FAR struct inode *mountpt, FAR const char *relpath)
{
	FAR struct fat_mountpt_s *fs;
	FRESULT res;
	int ret;

	if (relpath == NULL)
		return -EFAULT;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_unlink(&fs->fatfs, relpath);
	return fatfs_errno(res);
}

static int fat_mkdir(FAR struct inode *mountpt, FAR const char *relpath,
                     mode_t mode)
{
	FAR struct fat_mountpt_s *fs;
	FRESULT res;
	int ret;

	if (relpath == NULL)
		return -EFAULT;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_mkdir(&fs->fatfs, relpath);
	return fatfs_errno(res);
}

int fat_rmdir(FAR struct inode *mountpt, FAR const char *relpath)
{
	FAR struct fat_mountpt_s *fs;
	FRESULT res;
	int ret;

	if (relpath == NULL)
		return -EFAULT;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_rmdir(&fs->fatfs, relpath);
	return fatfs_errno(res);
}

int fat_rename(FAR struct inode *mountpt, FAR const char *oldrelpath,
               FAR const char *newrelpath)
{
	FAR struct fat_mountpt_s *fs;
	FRESULT res;
	int ret;

	if ((oldrelpath == NULL) || (newrelpath == NULL)) {
		return -EFAULT;
	}

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	res = f_rename(&fs->fatfs, oldrelpath, newrelpath);
	return fatfs_errno(res);
}

static int fat_stat(FAR struct inode *mountpt, FAR const char *relpath,
                    FAR struct stat *buf)
{
	FAR struct fat_mountpt_s *fs;
	FILINFO finfo = { 0 };
	FRESULT res;
	int ret;

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);
	fs = mountpt->i_private;

	ret = fat_checkmount(fs);
	if (ret != OK) {
		return ret;
	}

	if (*relpath == '\0') {
		buf->st_size = 0;
		buf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFDIR;
		buf->st_blksize = fs->blkdrv.fs_hwsectorsize;
		return OK;
	}

	res = f_stat(&fs->fatfs, relpath, &finfo);
	if (res != FR_OK) {
		return fatfs_errno(res);
	}

	buf->st_size = finfo.fsize;
	buf->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR |
		       S_IWGRP | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH;

	if (finfo.fattrib & AM_RDO) {
		buf->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	}

	if (finfo.fattrib & AM_DIR) {
		buf->st_mode &= ~S_IFREG;
		buf->st_mode |= S_IFDIR;
	}

	fat_date_t fdate = { .as_int = finfo.fdate };
	fat_time_t ftime = { .as_int = finfo.ftime };
	struct tm tm = { .tm_mday = fdate.mday,
			 .tm_mon = fdate.mon - 1, /* unlike tm_mday, tm_mon is zero-based */
			 .tm_year = fdate.year + 80,
			 .tm_sec = ftime.sec * 2,
			 .tm_min = ftime.min,
			 .tm_hour = ftime.hour };

	fat_date_t cdate = { .as_int = finfo.cdate };
	fat_time_t ctime = { .as_int = finfo.ctime };
	struct tm ct = { .tm_mday = cdate.mday,
			 .tm_mon = cdate.mon - 1, /* unlike tm_mday, tm_mon is zero-based */
			 .tm_year = cdate.year + 80,
			 .tm_sec = ctime.sec * 2,
			 .tm_min = ctime.min,
			 .tm_hour = ctime.hour };
	buf->st_mtime = mktime(&tm);
	buf->st_ctime = mktime(&ct);
	buf->st_blksize = fs->blkdrv.fs_hwsectorsize;
	buf->st_atime = 0;

	return OK;
}

int mkfatfs(const char *pathname, struct fat_format_s *fmt)
{
	struct fat_blkdrv blkdrv = { 0 };
	BYTE work[FF_MAX_SS]; /* Work area (larger is better for processing time) */
	FRESULT res;
	struct geometry geometry = { 0 };
	int ret;

	if (!pathname) {
		log_e("ERROR: No block driver path\n");
		return -EINVAL;
	}

	blkdrv.fs_fd = open(pathname, O_RDWR);
	if (blkdrv.fs_fd < 0) {
		log_e("ERROR: Failed to open %s\n", pathname);
		return -ENODEV;
	}

	ret = ioctl(blkdrv.fs_fd, BIOC_GEOMETRY, (unsigned long)((uintptr_t)&geometry));
	if (ret < 0) {
		log_e("ERROR: geometry() returned %d\n", ret);
		goto errout_with_driver;
	}

	if (!geometry.geo_available || !geometry.geo_writeenabled) {
		log_e("ERROR: Media is not available\n");
		ret = -ENODEV;
		goto errout_with_driver;
	}

	blkdrv.fs_hwsectorsize = geometry.geo_sectorsize;
	blkdrv.fs_hwnsectors = geometry.geo_nsectors;
	switch (blkdrv.fs_hwsectorsize) {
	case 512:
		blkdrv.fs_hwsectshift = 9;
		break;
	case 1024:
		blkdrv.fs_hwsectshift = 10;
		break;
	case 2048:
		blkdrv.fs_hwsectshift = 11;
		break;
	case 4096:
		blkdrv.fs_hwsectshift = 12;
		break;
	default:
		log_e("ERROR: Unsupported sector size: %" PRId64 "\n", blkdrv.fs_hwsectorsize);
		ret = -EPERM;
		goto errout_with_driver;
	}

	res = f_mkfs(&blkdrv, 0, work, sizeof work);
	if (res != FR_OK) {
		log_e("ERROR: Failed to open %s: %d\n", pathname, -res);
		ret = fatfs_errno(res);
		goto errout_with_driver;
	}

	close(blkdrv.fs_fd);
	return OK;

errout_with_driver:
	close(blkdrv.fs_fd);
	return ret;
}
