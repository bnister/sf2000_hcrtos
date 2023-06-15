include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

obj-$(CONFIG_CMDS_OS) += os/
obj-$(CONFIG_CMDS_PTHREAD) += pthread/
obj-$(CONFIG_CMDS_NSH) += third_party/nuttx/apps/nshlib/
obj-$(CONFIG_CMDS_LSMOD) += lsmod/
obj-$(CONFIG_CMDS_SOUND_TEST) += sound_test/
obj-$(CONFIG_CMDS_INPUT) += input_event/
obj-$(CONFIG_CMDS_MTD) += mtd/
obj-$(CONFIG_CMDS_SPI) += spi/
obj-$(CONFIG_CMDS_PERSISTENTMEM) += persistentmem/
obj-$(CONFIG_CMDS_POK_TEST) += pok/
obj-$(CONFIG_CMDS_WATCHDOG_TEST) += watchdog/
obj-$(CONFIG_CMDS_EFUSE_DUMP) += efuse/
obj-$(CONFIG_CMDS_HDMI_RX) += hdmi_rx/
obj-$(CONFIG_CMDS_TV_DECODER) += tv_decoder/
obj-$(CONFIG_CMDS_ADC_TEST) += saradc/
obj-$(CONFIG_CMDS_PQ) += pq/
obj-$(CONFIG_CMDS_HCFOTA) += hcfota/
obj-$(CONFIG_CMDS_STANDBY) += standby/
obj-$(CONFIG_CMDS_BLUETOOTH_TEST) += bluetooth/
obj-$(CONFIG_CMDS_WS2811) += ws2811/
obj-$(CONFIG_CMDS_SORTING) += sorting/
obj-$(CONFIG_CMDS_VIN_DVP) += vin_dvp/
obj-$(CONFIG_CMDS_USB) += usb/
obj-$(CONFIG_CMDS_PWM) += pwm/
obj-$(CONFIG_CMDS_FB_TEST) += fb/
obj-$(CONFIG_CMDS_WIFI) += wifi/
obj-$(CONFIG_CMD_NETWORK) += network/
obj-$(CONFIG_CMD_MIPI_TEST) += mipi/
obj-$(CONFIG_CMD_UNITY) += unity/
obj-$(CONFIG_CMD_UART) += uart/

TARGETS := libcmds.a

all : $(TARGETS) FORCE

$(TARGETS): built-in.o
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/lib
	install -m 0664 libcmds.a $(STAGING_DIR)/usr/lib/libcmds.a

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
