ifeq ($(BR2_PACKAGE_WPA_SUPPLICANT_RTL_V0_8)$(BR2_PACKAGE_WPA_SUPPLICANT_RTL_V2_7),y)
include $(sort $(wildcard components/hclib/wpa_supplicant/wpa_supplicant-rtl/wpa_supplicant/wpa_supplicant.mk))
endif
