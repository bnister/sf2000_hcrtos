LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= $(srctree)/src
INC = -I$(LVGL_DIR) \
      -I$(STAGING_DIR)/usr/include/lvgl/ \
      -I$(STAGING_DIR)/usr/include/lvgl/lvgl

#-I$(LVGL_DIR)/ -I$(LVGL_DIR)/lvgl/src/draw/hcge/

cflags-y += -O3 -g0  -Wall -Wshadow -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare -Wno-error=format=
cflags-y += $(INC)

CSRCS = main.c


include $(LVGL_DIR)/hccast_app/hccast_app.mk


COBJS = $(CSRCS:.c=.o)

OBJS = $(patsubst $(LVGL_DIR)/%,%,$(COBJS))

lvgl-y = $(OBJS)
obj-y += lvgl.o
debug_p: FORCE
	echo $(OBJS)
