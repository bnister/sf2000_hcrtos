BUILD_TGBDUAL		:= 1

ifeq ($(BUILD_TGBDUAL), 1)
	CORE_DIR := $(PROJECTOR_DIR)/cores/tgbdual-libretro
	include $(PROJECTOR_DIR)/cores/tgbdual-libretro/Makefile.common
	CORE_OBJS := $(SOURCES_CXX:.cpp=.o)
endif