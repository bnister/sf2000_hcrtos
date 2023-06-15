THIRDPARTYNUTTX=../../third_party/nuttx

LOCAL_SRCS-y := \
$(THIRDPARTYNUTTX)/fs/fs_initialize.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_close.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_dup2.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_dup.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_fcntl.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_fsync.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_ioctl.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_lseek.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_mkdir.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_open.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_poll.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_epoll.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_pread.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_pwrite.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_read.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_rename.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_rmdir.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_select.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_sendfile.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_stat.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_statfs.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_fstat.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_truncate.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_unlink.c \
$(THIRDPARTYNUTTX)/fs/vfs/fs_write.c \
\
$(THIRDPARTYNUTTX)/fs/inode/fs_files.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_foreachinode.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inodeaddref.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inode.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inodefind.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inodefree.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inoderelease.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inoderemove.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inodereserve.c \
$(THIRDPARTYNUTTX)/fs/inode/fs_inodesearch.c \
\
$(THIRDPARTYNUTTX)/fs/dirent/fs_closedir.c \
$(THIRDPARTYNUTTX)/fs/dirent/fs_opendir.c \
$(THIRDPARTYNUTTX)/fs/dirent/fs_readdir.c \
$(THIRDPARTYNUTTX)/fs/dirent/fs_rewinddir.c \
$(THIRDPARTYNUTTX)/fs/dirent/fs_seekdir.c \
$(THIRDPARTYNUTTX)/fs/dirent/fs_telldir.c \
\
$(THIRDPARTYNUTTX)/fs/mount/fs_foreachmountpoint.c \
$(THIRDPARTYNUTTX)/fs/mount/fs_mount.c \
$(THIRDPARTYNUTTX)/fs/mount/fs_umount.c \
\
$(THIRDPARTYNUTTX)/fs/driver/fs_blockproxy.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_findmtddriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_closeblockdriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_findblockdriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_openblockdriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_registerblockdriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_registermtddriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_registerdriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_unregisterblockdriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_unregistermtddriver.c \
$(THIRDPARTYNUTTX)/fs/driver/fs_unregisterdriver.c \

LOCAL_SRCS-$(CONFIG_MTD) += \
$(THIRDPARTYNUTTX)/fs/driver/fs_mtdproxy.c

vfs-y = $(LOCAL_SRCS-y:.c=.o)

obj-y += vfs.o
