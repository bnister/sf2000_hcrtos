cflags-y += -Wno-sign-compare -Wno-unused-variable

spi-y := spi.o

spi-$(CONFIG_SPI_MEM) += spi-mem.o

spi-$(CONFIG_HC_SPI_SF) += hc-spi.o
spi-$(CONFIG_HC_SPI_MASTER) += hc-spi-master.o
spi-$(CONFIG_HC_GPIO_SPI) += spi-gpio.o spi-bitbang.o

spi-$(CONFIG_SPI_SPIDEV) += spidev.o

obj-y += spi.o
