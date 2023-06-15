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
#include <nuttx/mtd/mtd.h>
#include <kernel/elog.h>

#include <linux/errno.h>
#include <linux/mutex.h>

#include <yaffs_guts.h>
#include <direct/yaffsfs.h>
#include <yaffs_trace.h>
#include <yaffs_packedtags2.h>

#define YCALCBLOCKS(s, b) ((s)/(b))

struct yaffs2_mountpt_s {
	struct inode *blkdriver;
	struct yaffs_dev yaffsdev;
	struct mutex gross_lock;	/* Gross locking mutex*/
	struct mtd_dev_s *mtd;
};

unsigned int yaffs_auto_select = 1;

extern int yaffs_format_reldev(struct yaffs_dev *dev, int unmount_flag,
			       int force_unmount_flag, int remount_flag);

static int     __yaffs_open(struct file *filep, const char *relpath,
                 int oflags, mode_t mode);
static int     __yaffs_close(struct file *filep);
static ssize_t __yaffs_read(struct file *filep, char *buffer,
                 size_t buflen);
static ssize_t __yaffs_write(struct file *filep, const char *buffer,
                 size_t buflen);
static off_t   __yaffs_seek(struct file *filep, off_t offset, int whence);
static int     __yaffs_ioctl(struct file *filep, int cmd,
                 unsigned long arg);
static int     __yaffs_sync(struct file *filep);
static int     __yaffs_dup(const struct file *oldp, struct file *newp);
static int     __yaffs_fstat(const struct file *filep,
                 struct stat *buf);
static int     __yaffs_truncate(struct file *filep, off_t length);
static int     __yaffs_opendir(struct inode *mountpt,
                 const char *relpath, struct fs_dirent_s *dir);
static int     __yaffs_closedir(struct inode *mountpt,
                 struct fs_dirent_s *dir);
static int     __yaffs_readdir(struct inode *mountpt,
                 struct fs_dirent_s *dir);
static int     __yaffs_rewinddir(struct inode *mountpt,
                 struct fs_dirent_s *dir);
static int     __yaffs_bind(struct inode *blkdriver, const void *data,
                 void **handle);
static int     __yaffs_unbind(void *handle,
                 struct inode **blkdriver, unsigned int flags);
static int     __yaffs_statfs(struct inode *mountpt,
                 struct statfs *buf);
static int     __yaffs_unlink(struct inode *mountpt,
                 const char *relpath);
static int     __yaffs_mkdir(struct inode *mountpt, const char *relpath,
                 mode_t mode);
static int     __yaffs_rmdir(struct inode *mountpt, const char *relpath);
static int     __yaffs_rename(struct inode *mountpt,
                 const char *oldrelpath, const char *newrelpath);
static int     __yaffs_stat(struct inode *mountpt, const char *relpath,
                 struct stat *buf);

const struct mountpt_operations yaffs2_operations =
{
	__yaffs_open,          /* open */
	__yaffs_close,         /* close */
	__yaffs_read,          /* read */
	__yaffs_write,         /* write */
	__yaffs_seek,          /* seek */
	__yaffs_ioctl,         /* ioctl */
	
	__yaffs_sync,          /* sync */
	__yaffs_dup,           /* dup */
	__yaffs_fstat,         /* fstat */
	NULL,              /* truncate */
	
	__yaffs_opendir,       /* opendir */
	__yaffs_closedir,      /* closedir */
	__yaffs_readdir,       /* readdir */
	__yaffs_rewinddir,     /* rewinddir */
	
	__yaffs_bind,          /* bind */
	__yaffs_unbind,        /* unbind */
	__yaffs_statfs,        /* statfs */
	
	__yaffs_unlink,        /* unlink */
	__yaffs_mkdir,         /* mkdir */
	__yaffs_rmdir,         /* rmdir */
	__yaffs_rename,        /* rename */
	__yaffs_stat           /* stat */
};

#define yaffs_filep_to_lc(filep) (struct yaffs2_mountpt_s *)(((filep)->f_inode)->i_private);

static void yaffs_gross_lock(struct yaffs2_mountpt_s *fs)
{
	mutex_lock(&fs->gross_lock);
}

static void yaffs_gross_unlock(struct yaffs2_mountpt_s *fs)
{
	mutex_unlock(&fs->gross_lock);
}

static int yaffs_mtd_write_chunk(struct yaffs_dev *dev, int nand_chunk, const u8 *data,
		       int data_len, const u8 *oob, int oob_len)
{
	struct yaffs2_mountpt_s *fs = dev->driver_context;
	struct mtd_dev_s *mtd = fs->blkdriver->u.i_mtd;
	off_t offset = ((off_t) nand_chunk) * dev->param.total_bytes_per_chunk;
	int retval;

	retval = MTD_WRITE_OOB(mtd, offset, data, data_len, oob, oob_len);

	return retval ? YAFFS_FAIL : YAFFS_OK;
}

static int yaffs_mtd_read_chunk(struct yaffs_dev *dev, int nand_chunk, u8 *data,
		      int data_len, u8 *oob, int oob_len,
		      enum yaffs_ecc_result *ecc_result)
{
	struct yaffs2_mountpt_s *fs = dev->driver_context;
	struct mtd_dev_s *mtd = fs->blkdriver->u.i_mtd;
	int retval;
	off_t offset = ((off_t) nand_chunk) * dev->param.total_bytes_per_chunk;

	retval = MTD_READ_OOB(mtd, offset, data, data_len, oob, oob_len);

	switch (retval) {
	case 0:
		/* no error */
		if(ecc_result)
			*ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
		break;

	case -EUCLEAN:
		/* MTD's ECC fixed the data */
		if(ecc_result)
			*ecc_result = YAFFS_ECC_RESULT_FIXED;
		dev->n_ecc_fixed++;
		break;

	case -EBADMSG:
	default:
		/* MTD's ECC could not fix the data */
		dev->n_ecc_unfixed++;
		if(ecc_result)
			*ecc_result = YAFFS_ECC_RESULT_UNFIXED;
		return YAFFS_FAIL;
	}

	return YAFFS_OK;
}

static int yaffs_mtd_erase(struct yaffs_dev *dev, int block_no)
{
	struct yaffs2_mountpt_s *fs = dev->driver_context;
	struct mtd_dev_s *mtd = fs->blkdriver->u.i_mtd;
	int retval;

	retval = MTD_ERASE(mtd, block_no, 1);
	return (retval) ? YAFFS_FAIL : YAFFS_OK;
}

static int yaffs_mtd_mark_bad(struct yaffs_dev *dev, int block_no)
{
	struct yaffs2_mountpt_s *fs = dev->driver_context;
	struct mtd_dev_s *mtd = fs->blkdriver->u.i_mtd;
	int retval;

	retval = MTD_BLOCK_MARKBAD(mtd, block_no);
	return (retval) ? YAFFS_FAIL : YAFFS_OK;
}

static int yaffs_mtd_check_bad(struct yaffs_dev *dev, int block_no)
{
	struct yaffs2_mountpt_s *fs = dev->driver_context;
	struct mtd_dev_s *mtd = fs->blkdriver->u.i_mtd;
	int retval;

	retval = MTD_BLOCK_ISBAD(mtd, block_no);

	return (retval) ? YAFFS_FAIL : YAFFS_OK;
}

static int yaffs_mtd_initialise(struct yaffs_dev *dev)
{
	return YAFFS_OK;
}

static int yaffs_mtd_deinitialise(struct yaffs_dev *dev)
{
	return YAFFS_OK;
}

static int     __yaffs_open(struct file *filep, const char *relpath,
                 int oflags, mode_t mode)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	int fd;

	yaffs_gross_lock(fs);
	fd = yaffs_open_reldev(&fs->yaffsdev, relpath, oflags, mode);
	yaffs_gross_unlock(fs);
	if (fd < 0)
		return -yaffsfs_GetLastError();

	filep->f_priv = (void *)fd;

	return 0;
}

static int     __yaffs_close(struct file *filep)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	int fd;
	int res;

	fd = (int)filep->f_priv;

	yaffs_gross_lock(fs);
	res = yaffs_close(fd);
	yaffs_gross_unlock(fs);

	if (res == 0)
		return 0;

	return -yaffsfs_GetLastError();
}

static ssize_t __yaffs_read(struct file *filep, char *buffer,
                 size_t buflen)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	ssize_t char_read;
	int fd;

	fd = (int)filep->f_priv;

	yaffs_gross_lock(fs);
	char_read = yaffs_read(fd, buffer, buflen);
	yaffs_gross_unlock(fs);
	if (char_read < 0)
		return -yaffsfs_GetLastError();

	return char_read;
}

static ssize_t __yaffs_write(struct file *filep, const char *buffer,
                 size_t buflen)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	ssize_t char_write;
	int fd;

	fd = (int)filep->f_priv;

	yaffs_gross_lock(fs);
	char_write = yaffs_write(fd, buffer, buflen);
	yaffs_gross_unlock(fs);
	if (char_write < 0)
		return -yaffsfs_GetLastError();

	return char_write;
}

static off_t   __yaffs_seek(struct file *filep, off_t offset, int whence)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	off_t position;
	int fd;
	int res;

	fd = (int)filep->f_priv;

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
		position = offset + yaffs_lseek(fd, 0, SEEK_END);
		break;
	default:
		return -EINVAL;
	}

	yaffs_gross_lock(fs);
	res = yaffs_lseek(fd, position, SEEK_SET);
	yaffs_gross_unlock(fs);
	if (res < 0)
		return -yaffsfs_GetLastError();

	filep->f_pos = position;

	return position;
}

static int     __yaffs_ioctl(struct file *filep, int cmd,
                 unsigned long arg)
{
	return -ENOSYS;
}

static int     __yaffs_sync(struct file *filep)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	int fd;
	int res;

	fd = (int)filep->f_priv;

	yaffs_gross_lock(fs);
	res = yaffs_fsync(fd);
	yaffs_gross_unlock(fs);
	if (res < 0)
		return -yaffsfs_GetLastError();

	return 0;
}

static int     __yaffs_dup(const struct file *oldp, struct file *newp)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(oldp);
	int oldfd, newfd;

	oldfd = (int)oldp->f_priv;

	yaffs_gross_lock(fs);
	newfd = yaffs_dup(oldfd);
	yaffs_gross_unlock(fs);

	if (newfd < 0)
		return -yaffsfs_GetLastError();

	newp->f_priv = (void *)newfd;

	return newfd;
}

static int     __yaffs_fstat(const struct file *filep,
                 struct stat *buf)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	struct yaffs_stat s;
	int res;
	int fd;

	fd = (int)filep->f_priv;

	yaffs_gross_lock(fs);
	res = yaffs_fstat(fd, &s) ;
	yaffs_gross_unlock(fs);

	if (res < 0)
		return -yaffsfs_GetLastError();

	buf->st_mode = s.st_mode;
	buf->st_size = s.st_size;
	buf->st_mtime = s.yst_mtime;
	buf->st_ctime = s.yst_ctime;
	buf->st_atime = s.yst_atime;

	return 0;
}

static int     __yaffs_truncate(struct file *filep, off_t length)
{
	struct yaffs2_mountpt_s *fs = yaffs_filep_to_lc(filep);
	int res;
	int fd;

	fd = (int)filep->f_priv;

	yaffs_gross_lock(fs);
	res = yaffs_ftruncate(fd, length);
	yaffs_gross_unlock(fs);

	if (res < 0)
		return -yaffsfs_GetLastError();

	return 0;
}

static int     __yaffs_opendir(struct inode *mountpt,
                 const char *relpath, struct fs_dirent_s *dir)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	yaffs_DIR *dirp;

	yaffs_gross_lock(fs);
	dirp = yaffs_opendir_reldev(&fs->yaffsdev, relpath);
	yaffs_gross_unlock(fs);

	if (dirp == NULL)
		return -yaffsfs_GetLastError();

	dir->u.yaffs2.fs_dir = dirp;

	return 0;
}

static int     __yaffs_closedir(struct inode *mountpt,
                 struct fs_dirent_s *dir)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	yaffs_DIR *dirp;
	int res;

	dirp = (yaffs_DIR *)dir->u.yaffs2.fs_dir;

	yaffs_gross_lock(fs);
	res = yaffs_closedir(dirp);
	yaffs_gross_unlock(fs);

	if (res < 0)
		return -yaffsfs_GetLastError();

	return 0;
}

static int     __yaffs_readdir(struct inode *mountpt,
                 struct fs_dirent_s *dir)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	struct yaffs_dirent *dirent;
	yaffs_DIR *dirp;

	dirp = (yaffs_DIR *)dir->u.yaffs2.fs_dir;

	dir->fd_dir.d_name[0] = '\0';

	yaffs_gross_lock(fs);
	dirent = yaffs_readdir(dirp);
	yaffs_gross_unlock(fs);

	if (dirent == NULL)
		return -ENOENT;

	dir->fd_dir.d_type = dirent->d_type;
	strncpy(dir->fd_dir.d_name, dirent->d_name, CONFIG_NAME_MAX);
	dir->fd_dir.d_name[CONFIG_NAME_MAX] = '\0';

	return 0;
}

static int     __yaffs_rewinddir(struct inode *mountpt,
                 struct fs_dirent_s *dir)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	yaffs_DIR *dirp;

	dirp = (yaffs_DIR *)dir->u.yaffs2.fs_dir;

	yaffs_gross_lock(fs);
	yaffs_rewinddir(dirp);
	yaffs_gross_unlock(fs);

	return 0;
}

struct yaffs_options {
	int inband_tags;
	int skip_checkpoint_read;
	int skip_checkpoint_write;
	int no_cache;
	int tags_ecc_on;
	int tags_ecc_overridden;
	int lazy_loading_enabled;
	int lazy_loading_overridden;
	int empty_lost_and_found;
	int empty_lost_and_found_overridden;
	int disable_summary;
	int forceformat;
	int autoformat;
};

#define MAX_OPT_LEN 30
static int yaffs_parse_options(struct yaffs_options *options,
			       const char *options_str)
{
	char cur_opt[MAX_OPT_LEN + 1];
	int p;
	int error = 0;

	/* Parse through the options which is a comma seperated list */

	while (options_str && *options_str && !error) {
		memset(cur_opt, 0, MAX_OPT_LEN + 1);
		p = 0;

		while (*options_str == ',')
			options_str++;

		while (*options_str && *options_str != ',') {
			if (p < MAX_OPT_LEN) {
				cur_opt[p] = *options_str;
				p++;
			}
			options_str++;
		}

		if (!strcmp(cur_opt, "inband-tags")) {
			options->inband_tags = 1;
		} else if (!strcmp(cur_opt, "tags-ecc-off")) {
			options->tags_ecc_on = 0;
			options->tags_ecc_overridden = 1;
		} else if (!strcmp(cur_opt, "tags-ecc-on")) {
			options->tags_ecc_on = 1;
			options->tags_ecc_overridden = 1;
		} else if (!strcmp(cur_opt, "lazy-loading-off")) {
			options->lazy_loading_enabled = 0;
			options->lazy_loading_overridden = 1;
		} else if (!strcmp(cur_opt, "lazy-loading-on")) {
			options->lazy_loading_enabled = 1;
			options->lazy_loading_overridden = 1;
		} else if (!strcmp(cur_opt, "disable-summary")) {
			options->disable_summary = 1;
		} else if (!strcmp(cur_opt, "empty-lost-and-found-off")) {
			options->empty_lost_and_found = 0;
			options->empty_lost_and_found_overridden = 1;
		} else if (!strcmp(cur_opt, "empty-lost-and-found-on")) {
			options->empty_lost_and_found = 1;
			options->empty_lost_and_found_overridden = 1;
		} else if (!strcmp(cur_opt, "no-cache")) {
			options->no_cache = 1;
		} else if (!strcmp(cur_opt, "no-checkpoint-read")) {
			options->skip_checkpoint_read = 1;
		} else if (!strcmp(cur_opt, "no-checkpoint-write")) {
			options->skip_checkpoint_write = 1;
		} else if (!strcmp(cur_opt, "no-checkpoint")) {
			options->skip_checkpoint_read = 1;
			options->skip_checkpoint_write = 1;
		} else if (!strcmp(cur_opt, "forceformat")) {
			options->forceformat = 1;
		} else if (!strcmp(cur_opt, "autoformat")) {
			options->autoformat = 1;
		} else {
			printf("yaffs: Bad mount option \"%s\"\n", cur_opt);
			error = 1;
		}
	}

	return error;
}

static int yaffs_verify_mtd(struct mtd_dev_s *mtd, int yaffs_version, int inband_tags)
{
	if (yaffs_version == 2) {
		if ((mtd->writesize < YAFFS_MIN_YAFFS2_CHUNK_SIZE ||
		     mtd->oobsize < YAFFS_MIN_YAFFS2_SPARE_SIZE) &&
		    !inband_tags) {
			printf("MTD device does not have the right page sizes");
			return -1;
		}
	} else {
		if (mtd->writesize < YAFFS_BYTES_PER_CHUNK ||
		    mtd->oobsize != YAFFS_BYTES_PER_SPARE) {
			printf("MTD device does not support have the right page sizes");
			return -1;
		}
	}

	return 0;
}

static int     __yaffs_bind(struct inode *blkdriver, const void *data,
                 void **handle)
{
	struct yaffs2_mountpt_s *fs;
	struct yaffs_options options;
	struct yaffs_param *param;
	struct mtd_dev_s *mtd;
	char *data_str = (char *)data;
	int yaffs_version = 2;
	int inband_tags = 0;
	int n_blocks;
	int ret;

	if (!blkdriver || !blkdriver->u.i_mtd || !INODE_IS_MTD(blkdriver)) {
		return -ENOTSUP;
	}

	mtd = blkdriver->u.i_mtd;

	if (!data_str)
		data_str = "";

	memset(&options, 0, sizeof(options));

	if (yaffs_parse_options(&options, data_str)) {
		/* Option parsing failed */
		return -EINVAL;
	}

	if (yaffs_version == 2 && !options.inband_tags &&
	    mtd->writesize == 512) {
		printf("auto selecting yaffs1");
		yaffs_version = 1;
	}

	if (mtd->oobavail < sizeof(struct yaffs_packed_tags2) ||
	    options.inband_tags)
		inband_tags = 1;

	if(yaffs_verify_mtd(mtd, yaffs_version, inband_tags) < 0)
		return -EINVAL;

	fs = (struct yaffs2_mountpt_s *)kmm_zalloc(sizeof(struct yaffs2_mountpt_s));
	if (!fs) {
		return -ENOMEM;
	}

	mutex_init(&fs->gross_lock);

	fs->blkdriver = blkdriver;
	fs->mtd = mtd;

	param = &fs->yaffsdev.param;

	param->name = NULL;

	param->n_reserved_blocks = 5;
	param->n_caches = (options.no_cache) ? 0 : 10;
	param->inband_tags = inband_tags;

	param->enable_xattr = 1;
	if (options.lazy_loading_overridden)
		param->disable_lazy_load = !options.lazy_loading_enabled;

	param->defered_dir_update = 1;

	if (options.tags_ecc_overridden)
		param->no_tags_ecc = !options.tags_ecc_on;

	param->empty_lost_n_found = 1;
	param->refresh_period = 1000;
	param->disable_summary = options.disable_summary;

#ifdef CONFIG_YAFFS_DISABLE_BAD_BLOCK_MARKING
	param->disable_bad_block_marking  = 1;
#endif
	if (options.empty_lost_and_found_overridden)
		param->empty_lost_n_found = options.empty_lost_and_found;

	if (yaffs_version == 2) {
		param->is_yaffs2 = 1;
		param->total_bytes_per_chunk = mtd->writesize;
		param->chunks_per_block = mtd->erasesize / mtd->writesize;
		n_blocks = YCALCBLOCKS(mtd->size, mtd->erasesize);

		param->start_block = 0;
		param->end_block = n_blocks - 1;
	} else {
		param->is_yaffs2 = 0;
		n_blocks = YCALCBLOCKS(mtd->size,
			     YAFFS_CHUNKS_PER_BLOCK * YAFFS_BYTES_PER_CHUNK);

		param->chunks_per_block = YAFFS_CHUNKS_PER_BLOCK;
		param->total_bytes_per_chunk = YAFFS_BYTES_PER_CHUNK;
	}

	param->start_block = 0;
	param->end_block = n_blocks - 1;

	param->use_nand_ecc = 1;

	param->skip_checkpt_rd = options.skip_checkpoint_read;
	param->skip_checkpt_wr = options.skip_checkpoint_write;

	fs->yaffsdev.drv.drv_deinitialise_fn = yaffs_mtd_deinitialise;
	fs->yaffsdev.drv.drv_initialise_fn = yaffs_mtd_initialise;
	fs->yaffsdev.drv.drv_check_bad_fn = yaffs_mtd_check_bad;
	fs->yaffsdev.drv.drv_mark_bad_fn = yaffs_mtd_mark_bad;
	fs->yaffsdev.drv.drv_erase_fn = yaffs_mtd_erase;
	fs->yaffsdev.drv.drv_read_chunk_fn = yaffs_mtd_read_chunk;
	fs->yaffsdev.drv.drv_write_chunk_fn = yaffs_mtd_write_chunk;

	fs->yaffsdev.driver_context = (void *)fs;

	if (options.forceformat)
		ret = yaffs_format_reldev(&fs->yaffsdev, 1, 1, 1);

	ret = yaffs_mount_reldev(&fs->yaffsdev);
	if (ret < 0) {
		if (options.autoformat) {
			yaffs_format_reldev(&fs->yaffsdev, 1, 1, 1);
		}

		ret = yaffs_mount_reldev(&fs->yaffsdev);
		if (ret < 0) {
			kmm_free(fs);
			return -yaffsfs_GetLastError();
		}
	}

	*handle = (void *)fs;
	return OK;
}

static int     __yaffs_unbind(void *handle,
                 struct inode **blkdriver, unsigned int flags)
{
	struct yaffs2_mountpt_s *fs = (struct yaffs2_mountpt_s *)handle;

	if (!fs) {
		return -EINVAL;
	}

	if (yaffs_unmount_reldev(&fs->yaffsdev) < 0)
		return -yaffsfs_GetLastError();

	if (blkdriver) {
		*blkdriver = fs->blkdriver;
	}

	mutex_destroy(&fs->gross_lock);

	kmm_free(fs);
	return OK;
}

static int     __yaffs_statfs(struct inode *mountpt,
                 struct statfs *buf)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	struct yaffs_dev *dev = &fs->yaffsdev;
	uint32_t s_blocksize = fs->mtd->writesize;

	yaffs_gross_lock(fs);

	buf->f_type = YAFFS_MAGIC;
	buf->f_bsize = s_blocksize;
	buf->f_namelen = 255;

	if (dev->data_bytes_per_chunk & (dev->data_bytes_per_chunk - 1)) {
		/* Do this if chunk size is not a power of 2 */

		uint64_t bytes_in_dev;
		uint64_t bytes_free;

		bytes_in_dev =
		    ((uint64_t)
		     ((dev->param.end_block - dev->param.start_block +
		       1))) * ((uint64_t) (dev->param.chunks_per_block *
					   dev->data_bytes_per_chunk));

		buf->f_blocks = bytes_in_dev / s_blocksize;

		bytes_free = ((uint64_t) (yaffs_get_n_free_chunks(dev))) *
		    ((uint64_t) (dev->data_bytes_per_chunk));

		buf->f_bfree = bytes_free / s_blocksize;

	} else if (s_blocksize > dev->data_bytes_per_chunk) {

		buf->f_blocks =
		    (dev->param.end_block - dev->param.start_block + 1) *
		    dev->param.chunks_per_block /
		    (s_blocksize / dev->data_bytes_per_chunk);
		buf->f_bfree =
		    yaffs_get_n_free_chunks(dev) /
		    (s_blocksize / dev->data_bytes_per_chunk);
	} else {
		buf->f_blocks =
		    (dev->param.end_block - dev->param.start_block + 1) *
		    dev->param.chunks_per_block *
		    (dev->data_bytes_per_chunk / s_blocksize);

		buf->f_bfree =
		    yaffs_get_n_free_chunks(dev) *
		    (dev->data_bytes_per_chunk / s_blocksize);
	}

	buf->f_files = 0;
	buf->f_ffree = 0;
	buf->f_bavail = buf->f_bfree;

	yaffs_gross_unlock(fs);

	return 0;
}

static int     __yaffs_unlink(struct inode *mountpt,
                 const char *relpath)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	int res;

	yaffs_gross_lock(fs);
	res = yaffs_unlink_reldev(&fs->yaffsdev, relpath);
	yaffs_gross_unlock(fs);
	if (res < 0)
		return -yaffsfs_GetLastError();

	return 0;
}

static int     __yaffs_mkdir(struct inode *mountpt, const char *relpath,
                 mode_t mode)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	int res;

	yaffs_gross_lock(fs);
	res = yaffs_mkdir_reldev(&fs->yaffsdev, relpath, mode);
	yaffs_gross_unlock(fs);

	if (res < 0)
		return -yaffsfs_GetLastError();

	return 0;
}

static int     __yaffs_rmdir(struct inode *mountpt, const char *relpath)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	int res;

	yaffs_gross_lock(fs);
	res = yaffs_rmdir_reldev(&fs->yaffsdev, relpath);
	yaffs_gross_unlock(fs);

	if (res < 0)
		return -yaffsfs_GetLastError();

	return 0;
}

static int     __yaffs_rename(struct inode *mountpt,
                 const char *oldrelpath, const char *newrelpath)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	int res;

	yaffs_gross_lock(fs);
	res = yaffs_rename_reldev(&fs->yaffsdev, oldrelpath, newrelpath);
	yaffs_gross_unlock(fs);

	if (res < 0)
		return -yaffsfs_GetLastError();

	return 0;
}

static int     __yaffs_stat(struct inode *mountpt, const char *relpath,
                 struct stat *buf)
{
	struct yaffs2_mountpt_s *fs = mountpt->i_private;
	struct yaffs_stat s;
	int res;

	yaffs_gross_lock(fs);
	res = yaffs_stat_reldev(&fs->yaffsdev, relpath, &s);
	yaffs_gross_unlock(fs);

	if (res < 0)
		return -yaffsfs_GetLastError();

	if (*relpath == '\0') {
		buf->st_size = 0;
		buf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFDIR;
		buf->st_blksize = fs->mtd->writesize;
		return OK;
	}

	buf->st_size = s.st_size;
	buf->st_mode = s.st_mode;
	buf->st_mtime = s.yst_mtime;
	buf->st_ctime = s.yst_ctime;
	buf->st_atime = s.yst_atime;
	buf->st_blksize = fs->mtd->writesize;

	return 0;
}
