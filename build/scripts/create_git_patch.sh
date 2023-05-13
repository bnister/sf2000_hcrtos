#!/bin/bash


CURRENT_DIR=`pwd`
LOCAL_DIR=`dirname $0`

PATCH_DESCRIBE=$1
PROJECT_PATH=$2
GIT_REPOSITORY=$3
START_COMMITID=$4
END_COMMITID=$5

PATCH_SOURCE_DIR=${PROJECT_PATH}/patch_source
SOURCE_DIR=""
PATCH_DIR=""
GIT_FILE=""
COMMIT_FILE=""
README=""

#
#./support/scripts/create_git_patch.sh "hclinux-2022.08.y-支持FM25M4AA_nor_flash" "/media/ps/data/home/javen.xu/hclinux" "./" "cce09b6d95da2d9a6f142bfad1aae0e27f29988b" "37b771ec18474eaa4c975626fa7467e123734710"
#./support/scripts/create_git_patch.sh "hclinux-2022.08.y-支持FM25M4AA_nor_flash" "/media/ps/data/home/javen.xu/hclinux" "SOURCE/avp" "749cf879e8c14bb1f49a69e892ce592a6a2a9f25" "ad210802cda05d9fb0708be1a5738838d51055e4"
#./support/scripts/create_git_patch.sh "hclinux-2022.08.y-支持FM25M4AA_nor_flash" "/media/ps/data/home/javen.xu/hclinux" "SOURCE/avp/components/kernel/source" "317ec0b22082a01f69c4d3d73c105f6ea29ad0d9" "c59cd31462ba41f09d82ec263bb0990ca48661dc"
#

function get_help()
{
    echo "Usage:"
	echo "    ./create_git_patch.sh [patch describe] [project path] [git repository path] [start commit id] [end commit id]"
    echo ""
    echo "eg:"
	echo "    ./support/scripts/create_git_patch.sh \"Solve the problem of compilation failure\" \"/media/ps/data/home/javen.xu/hclinux\" \"SOURCE/hc-examples\" xxx xxx"
    echo "    ./support/scripts/create_git_patch.sh \"hclinux-2022.08.y-支持FM25M4AA_nor_flash\" \"/media/ps/data/home/javen.xu/hclinux\" \"./\" \"cce09b6d95da2d9a6f142bfad1aae0e27f29988b\" \"37b771ec18474eaa4c975626fa7467e123734710\" "
    echo "    ./support/scripts/create_git_patch.sh \"hclinux-2022.08.y-支持FM25M4AA_nor_flash\" \"/media/ps/data/home/javen.xu/hclinux\" \"SOURCE/avp\" \"749cf879e8c14bb1f49a69e892ce592a6a2a9f25\" \"ad210802cda05d9fb0708be1a5738838d51055e4\" "
    echo "    ./support/scripts/create_git_patch.sh \"hclinux-2022.08.y-支持FM25M4AA_nor_flash\" \"/media/ps/data/home/javen.xu/hclinux\" \"SOURCE/avp/components/kernel/source\" \"317ec0b22082a01f69c4d3d73c105f6ea29ad0d9\" \"c59cd31462ba41f09d82ec263bb0990ca48661dc\" "
  echo ""
}

function prepare_env_variables()
{
	local current_time=`date +%Y%m%d%H`

    PATCH_SOURCE_DIR="${PATCH_SOURCE_DIR}"/"${PATCH_DESCRIBE}"_"${current_time}"
    SOURCE_DIR=${PATCH_SOURCE_DIR}/source/${GIT_REPOSITORY}
    PATCH_DIR=${PATCH_SOURCE_DIR}/patch/${GIT_REPOSITORY}
    GIT_FILE=${PATCH_SOURCE_DIR}/log.txt
    COMMIT_FILE=${PATCH_SOURCE_DIR}/commit.txt
    README=${PATCH_SOURCE_DIR}/readme.txt

    echo "PATCH_SOURCE_DIR=${PATCH_SOURCE_DIR}"
    echo "SOURCE_DIR=${SOURCE_DIR}"
    echo "PATCH_DIR=${PATCH_DIR}"
    echo "GIT_FILE=${GIT_FILE}"
    echo "COMMIT_FILE=${COMMIT_FILE}"
    echo "README=${README}"

    if [ ! -d $SOURCE_DIR ] ; then
        mkdir -p $SOURCE_DIR
    fi

    if [ ! -d $PATCH_DIR ] ; then
        mkdir -p $PATCH_DIR
    fi
}

function get_commit()
{
	local start_commit=$1
	local end_commit=$2

	if [ -z "$start_commit" ] ; then
		start_commit="HEAD~0"
	fi

	if [ -z "$end_commit" ] ; then
		end_commit="HEAD~0"
	fi

	git log --pretty=format:"%H" --reverse $start_commit...$end_commit > $COMMIT_FILE
	echo " " >> $COMMIT_FILE
	if [ $? -ne 0 ] ; then
		echo "error: get commit falied"
		exit 1
	fi
}

function get_patch()
{
	local start_commit=$1
	local end_commit=$2

	if [ -z "$start_commit" ] ; then
		start_commit="HEAD~0"
	fi

	if [ -z "$end_commit" ] ; then
		end_commit="HEAD~0"
	fi

	git format-patch $start_commit...$end_commit -o $PATCH_DIR
	if [ $? -ne 0 ] ; then
		echo "error: git patch failed"
		exit 1
	fi
}

function get_patch_source()
{
	local count=0
	local start_commit=$1
	local end_commit=$2

	local commit=""

	get_commit $start_commit $end_commit

	while read line
	do
		count=0

		commit=$line
		git log --oneline --name-only $commit -1 > $GIT_FILE
		if [ $? -ne 0 ] ; then
			echo "error:get_patch_source failed"
			exit 1
		fi

		while read line
		do
			count=$(($count+1))
			if [ $count -eq 1 ] ; then
				echo "first line skip"
			else
				if [ -e $line ] ; then
					cp --parents $line $SOURCE_DIR
					md5sum $line >> $README
				else
					echo "$line not exist"
				fi
			fi
		done < $GIT_FILE
	done < $COMMIT_FILE
}

function create_patch()
{
	local start_commit=$1
	local end_commit=$2

    cd $GIT_REPOSITORY
	get_patch $start_commit $end_commit
	get_patch_source $start_commit $end_commit
    cd -
}

if [ "x$PATCH_DESCRIBE" = "x" ] ||  [ "x$PROJECT_PATH" = "x" ] ||  [ "x$GIT_REPOSITORY" = "x" ] ||  [ "x$START_COMMITID" = "x" ] ||  [ "x$END_COMMITID" = "x" ] ; then
	get_help
	exit 1
fi

prepare_env_variables
create_patch $START_COMMITID $END_COMMITID

