#!/bin/sh
	
#first compile 	
make O=output-bl T211_a210_bl_defconfig
make O=output-bl kernel-rebuild all
make T211_a210_defconfig
make all

#make liblvgl-rebuild

