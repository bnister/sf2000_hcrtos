HCPROJECTOR_DIR_NAME ?= hcprojector_app
HCPROJECTOR_SRC += $(wildcard $(CUR_APP_DIR)/$(HCPROJECTOR_DIR_NAME)/*.c)
HCPROJECTOR_SRC += $(shell find -L $(CUR_APP_DIR)/$(HCPROJECTOR_DIR_NAME)/channel -name "*.c")
HCPROJECTOR_SRC += $(wildcard $(CUR_APP_DIR)/$(HCPROJECTOR_DIR_NAME)/public/*.c)
HCPROJECTOR_SRC += $(shell find -L $(CUR_APP_DIR)/$(HCPROJECTOR_DIR_NAME)/setup -name "*.c")
HCPROJECTOR_SRC += $(wildcard $(CUR_APP_DIR)/$(HCPROJECTOR_DIR_NAME)/volume/*.c)


ifeq ($(APPS_RESOLUTION_240P_SUPPORT),APPS_RESOLUTION_240P_SUPPORT)
	HCPROJECTOR_SRC += $(shell find -L $(CUR_APP_DIR)/$(HCPROJECTOR_DIR_NAME)/ui_rsc/320x240 -name "*.c")
else
	HCPROJECTOR_SRC += $(shell find -L $(CUR_APP_DIR)/$(HCPROJECTOR_DIR_NAME)/ui_rsc/1280x720 -name "*.c")
endif

#HCPROJECTOR_CPP_SRC += $(shell find -L $(CUR_APP_DIR)/hcprojector_app -name "*.cpp")

