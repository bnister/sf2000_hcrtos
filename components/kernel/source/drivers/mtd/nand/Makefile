cflags-y += -Wno-int-conversion
cflags-y += -Wno-error=format= -Wno-unused-variable
cflags-y += -Wno-error=format= -Wno-missing-field-initializers
cflags-y += -Wno-implicit-fallthrough -Wno-sign-compare

nand-y += nand_base.o nand_bbt.o nand_ids.o nand_ecc.o
#nand_timings.o

CFLAGS_nand_base.o += -Wno-incompatible-pointer-types

obj-y += nand.o

