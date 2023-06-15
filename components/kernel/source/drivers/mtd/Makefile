cflags-y += -Wno-sign-compare

THIRDPARTYNUTTXMTD=../../third_party/nuttx/drivers/mtd

_mtd-y := ftl.o
_mtd-$(CONFIG_MTD_PARTITION) += mtd_partition.o
_mtd-$(CONFIG_MTD_SECT512) += sector512.o
ifeq ($(CONFIG_MTD_WRBUFFER),y)
_mtd-y += mtd_rwbuffer.o
else
ifeq ($(CONFIG_MTD_READAHEAD),y)
_mtd-y += mtd_rwbuffer.o
endif
endif

mtd-y :=  $(addprefix $(THIRDPARTYNUTTXMTD)/,$(_mtd-y))

mtd-y += mtd-porting.o mtdcore.o of_mtd.o cmd_mtd.o
obj-$(CONFIG_MTD) += mtd.o

obj-$(CONFIG_MTD_SPI_NOR) += spi-nor/
obj-$(CONFIG_MTD_NAND) += nand/
obj-$(CONFIG_MTD_SPINAND_MT29F) += mt29f_spinand/
