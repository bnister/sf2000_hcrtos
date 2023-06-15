HCPROJECTOR_DIR_NAME ?= hcprojector_app

CSRCS += $(wildcard $(PROJECTOR_DIR)/$(HCPROJECTOR_DIR_NAME)/*.c)
CSRCS += $(shell find -L $(PROJECTOR_DIR)/$(HCPROJECTOR_DIR_NAME)/channel -name "*.c")
CSRCS += $(wildcard $(PROJECTOR_DIR)/$(HCPROJECTOR_DIR_NAME)/public/*.c)
CSRCS += $(shell find -L $(PROJECTOR_DIR)/$(HCPROJECTOR_DIR_NAME)/setup -name "*.c")
CSRCS += $(wildcard $(PROJECTOR_DIR)/$(HCPROJECTOR_DIR_NAME)/volume/*.c)

ifeq ($(CONFIG_APPS_PROJECTOR_LVGL_RESOLUTION_240P),y)
	CSRCS += $(shell find -L $(PROJECTOR_DIR)/$(HCPROJECTOR_DIR_NAME)/ui_rsc/320x240 -name "*.c")
else
	CSRCS += $(shell find -L $(PROJECTOR_DIR)/$(HCPROJECTOR_DIR_NAME)/ui_rsc/1280x720 -name "*.c")
endif


CFLAGS += -I$(PROJECTOR_DIR)

