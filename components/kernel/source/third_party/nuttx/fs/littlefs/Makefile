cflags-y += -DLFS_TRACE=finfo
cflags-y += -DLFS_DEBUG=finfo
cflags-y += -DLFS_WARN=fwarn
cflags-y += -DLFS_ERROR=ferr
cflags-y += -DLFS_ASSERT=assert
cflags-y += -DLFS_CONFIG=../lfs_vfs.h
cflags-y += -Wno-sign-compare

littlefs-y := \
	lfs_vfs.o \
	littlefs/lfs.o \
	littlefs/lfs_util.o

obj-y += littlefs.o
