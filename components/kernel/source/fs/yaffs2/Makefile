YAFFS2DIR=../../third_party/yaffs2

cflags-y += -I$(src)/$(YAFFS2DIR)

yaffs2-y = \
	yaffs_port.o \
	yaffs_osglue.o \
	$(YAFFS2DIR)/yaffs_allocator.o \
	$(YAFFS2DIR)/yaffs_bitmap.o \
	$(YAFFS2DIR)/yaffs_cache.o \
	$(YAFFS2DIR)/yaffs_checkptrw.o \
	$(YAFFS2DIR)/yaffs_ecc.o \
	$(YAFFS2DIR)/yaffs_endian.o \
	$(YAFFS2DIR)/yaffs_guts.o \
	$(YAFFS2DIR)/yaffs_nameval.o \
	$(YAFFS2DIR)/yaffs_nand.o \
	$(YAFFS2DIR)/yaffs_packedtags1.o \
	$(YAFFS2DIR)/yaffs_packedtags2.o \
	$(YAFFS2DIR)/yaffs_summary.o \
	$(YAFFS2DIR)/yaffs_tagscompat.o \
	$(YAFFS2DIR)/yaffs_tagsmarshall.o \
	$(YAFFS2DIR)/yaffs_verify.o \
	$(YAFFS2DIR)/yaffs_yaffs1.o \
	$(YAFFS2DIR)/yaffs_yaffs2.o \
	$(YAFFS2DIR)/direct/yaffs_attribs.o \
	$(YAFFS2DIR)/direct/yaffs_error.o \
	$(YAFFS2DIR)/direct/yaffsfs.o \
	$(YAFFS2DIR)/direct/yaffs_hweight.o

obj-y += yaffs2.o
