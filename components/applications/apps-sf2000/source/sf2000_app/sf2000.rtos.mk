#CSRCS += $(shell find -L $(PROJECTOR_DIR)/sf2000_app -name "*.c")
CSRCS += $(PROJECTOR_DIR)/sf2000_app/main_sf2000_retroarch.c
CFLAGS += -I$(PROJECTOR_DIR)
