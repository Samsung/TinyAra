#!/bin/bash
###########################################################################
#
# Copyright 2017 Samsung Electronics All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###########################################################################
#
# File   : artik053_download.sh
# Description : Download script for ARTIK 053

# Remember, make is invoked from "os" directory

# Include the configuration file
source .config
OS_DIR_PATH=${PWD}
BUILD_DIR_PATH=${OS_DIR_PATH}/../build
OUTPUT_BINARY_PATH=${BUILD_DIR_PATH}/output/bin
BOARD_DIR_PATH=${BUILD_DIR_PATH}/configs/artik053
OPENOCD_DIR_PATH=${BOARD_DIR_PATH}/tools/openocd
FW_DIR_PATH=${BOARD_DIR_PATH}/bin

SYSTEM_TYPE=`getconf LONG_BIT`
if [ "$SYSTEM_TYPE" = "64" ]; then
	OPENOCD_BIN_PATH=${OPENOCD_DIR_PATH}/linux64
else
	OPENOCD_BIN_PATH=${OPENOCD_DIR_PATH}/linux32
fi

prepare_download()
{
	# Prepare for ROMFS
	ROMFS_FLASHING_CFG=${OPENOCD_DIR_PATH}/romfs_flashing.cfg
	echo "set romfs_partition_enable 0" > ${ROMFS_FLASHING_CFG}
	if [ "${CONFIG_FS_ROMFS}" == "y" ]; then
		echo "set romfs_partition_enable 1" > ${ROMFS_FLASHING_CFG}
	fi
}

finish_download()
{
	rm ${ROMFS_FLASHING_CFG}
	exit $1
}

romfs()
{
	# Make romfs.img
	if [ "${CONFIG_FS_ROMFS}" == "y" ]; then
		pushd ${OS_DIR_PATH}
		sh ../tools/fs/mkromfsimg.sh
		if [ ! -f "../build/output/bin/romfs.img" ]; then
			echo "ROMFS image is not present"
			finish_download 1
		fi
		popd

		pushd ${OPENOCD_DIR_PATH}
		${OPENOCD_BIN_PATH}/openocd -f artik053.cfg -c ' 	\
		flash_write rom ../../../../output/bin/romfs.img; \
		exit'
		popd
	fi
}

main()
{
	echo "openocd is picked from ${OPENOCD_BIN_PATH}"
	echo "Binaries are picked from ${OUTPUT_BINARY_PATH}"
	echo "Board path is ${BOARD_DIR_PATH}"

	prepare_download

	# Process arguments
	for arg in $@
	do
		case ${arg} in
		ALL|all)
			echo "ALL :"

			# check existence of os binary
			if [ ! -f "${OUTPUT_BINARY_PATH}/tinyara_head.bin" ]; then
				echo "TinyAra binary is not existed, build first"
				finish_download 1
			fi

			# check existence of firmware binaries
			if [ ! -f "${FW_DIR_PATH}/bl1.bin" ] ||\
				[ ! -f "${FW_DIR_PATH}/bl2.bin" ] ||\
				[ ! -f "${FW_DIR_PATH}/sssfw.bin" ] ||\
				[ ! -f "${FW_DIR_PATH}/wlanfw.bin" ]; then
				echo "Firmware binaries for ARTIK 053 are not existed"
				finish_download 1
			fi

			# Download all binaries using openocd script
			pushd ${OPENOCD_DIR_PATH}
			${OPENOCD_BIN_PATH}/openocd -f artik053.cfg -c ' 	\
			flash_write bl1 ../../bin/bl1.bin; 		\
			flash_write bl2 ../../bin/bl2.bin; 		\
			flash_write sssfw ../../bin/sssfw.bin; 		\
			flash_write wlanfw ../../bin/wlanfw.bin;	\
			flash_write os ../../../../output/bin/tinyara_head.bin;	\
			exit'
			popd

			# check romfs and download it
			romfs
			;;

		ERASE_USERFS|erase_userfs)
			echo "USERFS :"

			pushd ${OPENOCD_DIR_PATH}
			${OPENOCD_BIN_PATH}/openocd -f artik053.cfg -c ' 	\
			flash_erase_part user;	\
			exit'
			popd
			;;

		*)
			echo "${arg} is not suppported in artik053"
			echo "Usage : make download [ ALL | ERASE_USERFS ]"
			finish_download 1
			;;
		esac
	done

	finish_download
}

main $*

