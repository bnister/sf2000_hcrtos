#!/bin/sh

# $1: <PKG>_PKGD : the pkg/source dir
# $2: <PKG>_DIR : the stamps dir

if [ -e $1 ]; then
	submodule="`git submodule status $1 2>/dev/null`"
	if [ "$submodule" != "" ]; then
		echo "rm -rf $1/*"
		echo "rm -rf $1/.applied_patches_list"
		rm -rf $1/*
		rm -rf $1/.applied_patches_list
		echo "rm -rf $2"
		rm -rf $2
		echo "git -C $1 reset --hard"
		git -C $1 reset --hard
	else
		git submodule status $1 2>/dev/null
		if [ "$?" = "0" ]; then
			echo "rm -rf $1/*"
			echo "rm -rf $1/.applied_patches_list"
			rm -rf $1/*
			rm -rf $1/.applied_patches_list
			echo "rm -rf $2"
			rm -rf $2
			echo "git checkout $1"
			git checkout $1
		else
			echo "rm -rf $1/*"
			echo "rm -rf $1/.applied_patches_list"
			rm -rf $1/*
			rm -rf $1/.applied_patches_list
			echo "rm -rf $2"
			rm -rf $2
		fi
	fi
else
	echo "rm -rf $2"
	rm -rf $2
fi
