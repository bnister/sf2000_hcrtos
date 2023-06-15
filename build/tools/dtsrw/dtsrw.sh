#!/bin/bash
echo "dtsrw.sh used for reading dtb form firmware, or writting dtb to a firmware!"
echo "Usage:"
echo "        dtsrw.sh -r <input: firmware file> <output: dts file>"
echo "        dtsrw.sh -w <input: dts file> <output: firmware file>"


#In general, the position of dtb bin data in the firmware is 0x80000(524288), 
#and the length is 0x10000(65536), can check these in hcprog.ini
DTB_OFFSET=524288
DTB_LENGTH=65536

# 3 parameters
param_count=$#

rw_flag=$1
input_name=$2
output_name=$3

echo "$0 $1 $2 $3"

if [ $param_count -ne 3 ]; then
	echo "param count is $param_count, should be 3"
	exit
fi


#read dtb form firmware, and change to dts format.
read_dts(){
	input_offset=$DTB_OFFSET
	input_length=$DTB_LENGTH
	output_offset=0
	output_length=$DTB_LENGTH

	#
	rm -rf tmp.dtb
	./binrw $input_name $input_offset $input_length tmp.dtb $output_offset $output_length
	dtc -I dtb -O dts tmp.dtb -o $output_name
	rm -rf tmp.dtb
}

#change to dts to dtb format, and write to firmware.
write_dts(){
	input_offset=0
	input_length=0
	output_offset=$DTB_OFFSET
	output_length=$DTB_LENGTH

	#
	rm -rf tmp.dtb
	dtc -I dts -O dtb $input_name -o tmp.dtb
	./binrw tmp.dtb $input_offset $input_length $output_name $output_offset $output_length
	rm -rf tmp.dtb
}

if [ "$rw_flag" == "-r" ]; then
	read_dts
else
	write_dts
fi	