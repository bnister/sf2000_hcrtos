#
# Makefile
#
CC ?= gcc
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}

SYS_DIR = -I$(LVGL_DIR) \
		-I$(STAGING_DIR)/usr/include \
		-I$(STAGING_DIR)/usr/include/hcuapi

LVGL_HEADERS = -I$(STAGING_DIR)/usr/include/lvgl/ \
		-I$(STAGING_DIR)/usr/include/lvgl/lvgl


MY_DEF =

SCREEN_APP_SUPPORT := $(shell echo $(SF2000APP_DEF) | grep -o SCREEN_APP_SUPPORT)

CFLAGS ?= -O3 $(SYS_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
CFLAGS += $(LVGL_HEADERS)
LDFLAGS ?= -lm -llvgl

DEMO_LIBS = -lffplayer -lpthread -lge -Wl,--start-group -lstdc++ $(CAST_LIBS) -Wl,--end-group

include $(LVGL_DIR)/sf2000_app/sf2000_app.mk
DEMO_NAME = sf2000_demo

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
SF2000_OBJS = $(SF2000_SRC:.c=$(OBJEXT))
SF2000_OBJS += $(SF2000_CPP_SRC:.cpp=$(OBJEXT))

## MAINOBJ -> OBJFILES

all_depends := $(patsubst %.c,%.d,$(CSRCS) $(SF2000_SRC))
all_depends +=$(patsubst %.cpp,%.d,$(SF2000_CPP_SRC))
all_depends += $(patsubst %.S,%.d,$(ASRCS))

all: sf2000_app

%.o: %.c
	$(CC) $(CFLAGS) $(MY_DEF) -c $< -o $@
	echo "CC $<"
	
.PHONY: FORCE

%.o: %.cpp
	$(CC) $(CFLAGS) $(MY_DEF) -c $< -o $@
	echo "CC $<"

%.d: %.c
	set -e;rm -rf $@;$(CC) $(CFLAGS) $(MY_DEF) -MM $^ > $@.$$$$;cp $@.$$$$ $@;rm -rf $@.$$$$

%.d: %.cpp
	set -e;rm -rf $@;$(CC) $(CFLAGS) $(MY_DEF) -MM $^ > $@.$$$$;cp $@.$$$$ $@;rm -rf $@.$$$$

-include $(all_depends)

sf2000_app:	$(COBJS) $(SF2000_OBJS)
	@$(CC) -o $(DEMO_NAME) $(SF2000_OBJS) $(COBJS) $(LDFLAGS) $(DEMO_LIBS)
	
clean: 
	rm -f $(AOBJS) $(COBJS) $(SF2000_OBJS)
	@find ./ -type f -name "*.o" |xargs rm -rf
	rm -rf $(DEMO_NAME)

install:
	@echo "install ......"
