#
# Makefile
#
CC ?= gcc
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}

SYS_DIR = -I$(LVGL_DIR) \
		-I$(STAGING_DIR)/usr/include \
		-I$(STAGING_DIR)/usr/include/hcuapi \
		-I$(STAGING_DIR)/usr/include/libgoahead

LVGL_CONF=include/generated/autoconf.h

CFLAGS ?= -fPIC -O3 $(SYS_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare

CFLAGS +=  -DLV_CONF_SKIP=1 -DLV_CONF_KCONFIG_EXTERNAL_INCLUDE=\"$(LVGL_CONF)\"

LDFLAGS ?= -lm
BIN = demo

# 
DEMO_LIBS = -lffplayer -lpthread -lge -Wl,--start-group -lstdc++ $(CAST_LIBS) -Wl,--end-group

#LIBS = -lffplayer -lpthread

TARGET = liblvgl.so

#Collect the files to compile
MAINSRC = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk
include $(LVGL_DIR)/hc_src/hc_src.mk
include $(LVGL_DIR)/hc_examples/hc_examples.mk

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

## MAINOBJ -> OBJFILES

-include include/config/auto.conf

all_depends := $(patsubst %.c,%.d,$(CSRCS))
all_depends += $(patsubst %.S,%.d,$(ASRCS))

all: $(TARGET)

ifeq ($(CONFIG_LVGL_DEMO_APP),y)
all_depends:=$(patsubst %.c,%.d,$(MAINSRC))
all: default
endif

default: $(MAINOBJ) $(TARGET)
	@$(CC) -o $(BIN) $(MAINOBJ) $(LDFLAGS) $(TARGET) $(DEMO_LIBS)

	
$(TARGET): $(COBJS)
	@$(CC) -fPIC -shared -o $@  $(COBJS) $(LDFLAGS) $(DEMO_LIBS)


%.d: %.c
	@set -e;rm -rf $@;echo -n "$(dir $@)" >$@.$$$$;$(CC) $(CFLAGS) -MM $^ >> $@.$$$$;cp $@.$$$$ $@;rm -rf $@.$$$$

%.o: %.c $(LVGL_CONF)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $<"

%.o: %.cpp $(LVGL_CONF)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $<"
clean: 
	rm -f $(BIN) $(TARGET) $(AOBJS) $(COBJS) $(MAINOBJ)

install:
	@echo "install ......"
	$(INSTALL)  -m 0755 -d $(STAGING_DIR)/usr/include/lvgl
	$(INSTALL)  -m 0755 -d $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 777 $(TARGET) $(STAGING_DIR)/usr/lib/$(TARGET)
	@cp -rf *.h $(STAGING_DIR)/usr/include/lvgl
	@rsync -ar --include="*/" --include="*.h" --exclude=*   lvgl $(STAGING_DIR)/usr/include/lvgl
	@rsync -ar --include="*/" --include="*.h" --exclude=*   lv_drivers $(STAGING_DIR)/usr/include/lvgl
	@rsync -ar --include="*/" --include="*.h" --exclude=*   include $(STAGING_DIR)/usr/include/lvgl
	@rsync -ar --include="*/" --include="*.h" --exclude=*   hc_src $(STAGING_DIR)/usr/include/lvgl


-include Makefile.conf

-include $(all_depends)
