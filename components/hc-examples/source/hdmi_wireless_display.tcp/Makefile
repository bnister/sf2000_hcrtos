#SRCS = $(wildcard *.c)
#OBJS = $(patsubst %.c, %.o, $(SRCS))
#DEPS = $(SRCS:.c=.dep)
#OUT_BIN = initplay

#all: display_sink display_source

#display_sink : display_sink.c decoder.c
	#$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

#display_source: display_source.c queue.c
	#$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

#%.dep: %.c
	#@echo "Creating $@ ..."
	#@set -e; \
	#rm -rf $@.tmp; \
	#$(CC) -E -MM $(filter %.c, $^) > $@.tmp; \
	#sed 's,\(.*\)\.o[ :]*,objs/\l.o $@: ,g' < $@.tmp > $@; \
	#rm -rf $@.tmp

#.PHONY: clean
#clean:
	#rm -rf *.o
	#rm -rf display_sink display_source
	#rm -rf *.dep

cflags-y += -Dset_volume=__display_set_volume \
	-Dh264_decode_init=__display_h264_decode_init \
	-Daac_decode=__display_aac_decode \
	-Daac_decoder_flush=__display_aac_decoder_flush \
	-Dh264_decoder_flush=__display_h264_decoder_flush \
	-Daac_decoder_destroy=__display_aac_decoder_destroy \
	-Dh264_decode=__display_h264_decode \
	-Daac_decoder_init=___display_aac_decoder_init \
	-Dh264_decoder_destroy=__display_h264_decoder_destroy \
	-Dset_aspect_mode=__display_set_aspect_mode

hdmi_wireless-y = decoder.o  display_sink.o  display_source.o  queue.o jenc_rdo.o
obj-y += hdmi_wireless.o
