cflags-y += -Wno-sign-compare -I$(src)/../../third_party/nuttx/fs/partition

partition-y := \
	../../third_party/nuttx/fs/partition/fs_partition.o

partition-$(CONFIG_MBR_PARTITION) += fs_smbr.o

partition-$(CONFIG_PTABLE_PARTITION) += \
	../../third_party/nuttx/fs/partition/fs_ptable.o
partition-$(CONFIG_GPT_PARTITION) += \
	../../third_party/nuttx/fs/partition/fs_gpt.o

obj-y += partition.o
