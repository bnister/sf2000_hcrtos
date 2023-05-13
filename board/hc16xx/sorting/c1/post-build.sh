#!/bin/bash

. $BR2_CONFIG > /dev/null 2>&1
export BR2_CONFIG

current_dir=$(dirname $0)

message()
{
	echo -e "\033[47;30m>>> $1\033[0m"
}

message "Installing rcS ....."
cp -vf ${current_dir}/rcS ${IMAGES_DIR}/fs-partition1-root/
message "Installing rcS done!"
