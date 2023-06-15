CSRCS += lv_gpu_hichip.c

DEPPATH += --dep-path $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/draw/hcge
VPATH += :$(LVGL_DIR)/$(LVGL_DIR_NAME)/src/draw/hcge

CFLAGS += "-I$(LVGL_DIR)/$(LVGL_DIR_NAME)/src/draw/hcge"
