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

# This script reads partition size and name list from .config and
# creates a partition map cfg file for openocd.
# Created partition map cfg file can be included in the main openocd cfg script
# for flashing.

THIS_PATH=`test -d ${0%/*} && cd ${0%/*}; pwd`
# When location of this script is changed, only OS_DIR_PATH should be changed together!!!
OS_DIR_PATH=${THIS_PATH}/../../../../os

source ${OS_DIR_PATH}/.config

# Path ENV
BOARD_NAME=${CONFIG_ARCH_BOARD}
PARTMAP_DIR_PATH=${THIS_PATH}
BOARD_KCONFIG=${OS_DIR_PATH}/board/${BOARD_NAME}/Kconfig

# FLASH BASE ADDRESS (Can it be made to read dynamically from .config?)
FLASH_BASE=0x04000000

# Partition information
partsize_list_default=`grep -A 2 'config ARTIK05X_FLASH_PART_LIST' ${BOARD_KCONFIG} | sed -n 's/\tdefault "\(.*\)".*/\1/p'`
partsize_list=${CONFIG_ARTIK05X_FLASH_PART_LIST:=${partsize_list_default}}
partname_list_default=`grep -A 2 'config ARTIK05X_FLASH_PART_NAME' ${BOARD_KCONFIG} | sed -n 's/\tdefault "\(.*\)".*/\1/p'`
partname_list=${CONFIG_ARTIK05X_FLASH_PART_NAME:=${partname_list_default}}

# OpenOCD cfg file to be created for flashing
PARTITION_MAP_CFG=${PARTMAP_DIR_PATH}/partition_map.cfg

# Partition map text for auto generation
PARTITION_MAP_HEADER="#
# Partition Map (Auto generated)
#
#	Name	  Description		Start address	Size		RO
set partition_list {"
PARTITION_MAP_FOOTER="}"

#Comma Separator
IFS=","

#Variables
total=1
count=1
id=1
romfs_part_exist=0

#Array to hold partition size of all partitions
part_size[$count]=0

#Array to hold partition start address  of all partitions
part_start[$count]=0

echo -n "Generating partition map ... "

# Add partition map header at the beginning of cfg file
echo ${PARTITION_MAP_HEADER} > ${PARTITION_MAP_CFG}

#Loop partition size list
sum=0
for psize in $partsize_list
do
	while [ "$count" -le "$total" ];
	do
		sum=`expr $sum + $psize`
		let "count += 1"
	done

	part_size[$count-1]=`expr $psize \\* 1024`
	part_start[$count]=`expr $sum \\* 1024`
	let "total += 1"
done

for pname in $partname_list
do
	if [ "$pname" == "bl1" ]; then
		pname_text="BL1"
		ro=0
	elif [ "$pname" == "sssro" ]; then
		pname_text="SSS R/O Key"
		ro=1
	elif [ "$pname" == "bl2" ]; then
		pname_text="BL2"
		ro=0
	elif [ "$pname" == "sssfw" ]; then
		pname_text="SSS F/W"
		ro=0
	elif [ "$pname" == "wlanfw" ]; then
		pname_text="WLAN F/W"
		ro=0
	elif [ "$pname" == "os" ]; then
		pname_text="OS"
		ro=0
	elif [ "$pname" == "factory" ]; then
		pname_text="Factory Reset"
		ro=0
	elif [ "$pname" == "ota" ]; then
		pname_text="OTA download"
		ro=0
	elif [ "$pname" == "user" ]; then
		pname_text="USER R/W"
		ro=0
	elif [ "$pname" == "rom" ]; then
		pname_text="ROM FS"
		ro=0
		romfs_part_exist=1
	elif [ "$pname" == "symtab" ]; then
		pname_text="Symbol Table"
		ro=0
	elif [ "$pname" == "nvram" ]; then
		pname_text="WiFi NVRAM"
		ro=1
	elif [ "$pname" == "sssrw" ]; then
		pname_text="SSS R/W Key"
		ro=1
	fi

	pstart="$(printf 0x"%08X" $((${part_start[$id]} + ${FLASH_BASE})))"
	psize="$(printf 0x"%08X" ${part_size[$id]})"
	format="$(printf \""%-15s" ${pname_text}\")"

	echo "	${pname}	{ ${format}	${pstart}	${psize}" \
		"	${ro} }" >> ${PARTITION_MAP_CFG}

	let "id += 1"
done

# add termination text
echo ${PARTITION_MAP_FOOTER} >> ${PARTITION_MAP_CFG}

echo "Done"
