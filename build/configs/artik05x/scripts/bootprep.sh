#!/bin/bash
###########################################################################
#
# Copyright 2019 Samsung Electronics All Rights Reserved.
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

# This script reads partition size and name list from .config or from Kconfig
# and generates a file bootprep.c with parition information.
# This script can be used standalone or can be invoked from other script.

# TODO: Binary header information to be added

CURRENT_PATH=`test -d ${0%/*} && cd ${0%/*}; pwd`
# When location of this script is changed, only OS_DIR_PATH should be changed together!!!
OS_DIR_PATH=${CURRENT_PATH}/../../../../os

source ${OS_DIR_PATH}/.config

PARTITION_KCONFIG=${OS_DIR_PATH}/board/common/Kconfig
FILENAME=bootprep.c
OUTFILE=${CURRENT_PATH}/${FILENAME}

echo "***Gonna Create bootprep.c file***"

# Read Partition Sizes
sizes=`grep -A 2 'config FLASH_PART_LIST' ${PARTITION_KCONFIG} | sed -n 's/\tdefault "\(.*\)".*/\1/p'`
partition_sizes=${CONFIG_FLASH_PART_LIST:=${sizes}}
echo "Partition sizes: $partition_sizes"

# Read Partition Names
names=`grep -A 2 'config FLASH_PART_NAME' ${PARTITION_KCONFIG} | sed -n 's/\tdefault "\(.*\)".*/\1/p'`
partition_names=${CONFIG_FLASH_PART_NAME:=${names}}
echo "Partiion Names: $partition_names"

# Read Partition Types
types=`grep -A 2 'config FLASH_PART_TYPE' ${PARTITION_KCONFIG} | sed -n 's/\tdefault "\(.*\)".*/\1/p'`
partition_types=${CONFIG_FLASH_PART_TYPE:=${types}}
echo "Partiion Types: $partition_types"

AUTOGENERATED_INFO="
/*Auto Generated file.Dont edit here */"
INCLUDE_STDINT="#include <stdint.h>"
PARTITION_TYPEDEF="partition_entry_t"
PARTITION_DECLARATION="typedef struct __attribute__((packed))
{
	uint8_t bin_name[16];	/* Binary Name */
	uint32_t offset;	/* Location address */
	uint32_t size;		/* Size in bytes */
	uint8_t type[16];	/* Partition type - smartfs, none, ftl etc */
	uint8_t subtype[8];	/* Subtype - data, code, etc */
	uint8_t flags;		/* Reserved for now */
} $PARTITION_TYPEDEF;"

# Generate bootprep.c
# Add autogeneration notice
echo -e "${AUTOGENERATED_INFO}\n" > ${OUTFILE}
# Include header files
echo -e "${INCLUDE_STDINT}\n" >> ${OUTFILE}
# Declare partition
echo -e "${PARTITION_DECLARATION}\n" >> ${OUTFILE}

# Create partition array
num_partitions=`echo $partition_sizes | awk -F, '{print NF-1}'`
echo "$PARTITION_TYPEDEF partitions[$num_partitions]={"	>> ${OUTFILE}

#TODO: addr below should be read from config in future
addr=0x04000000
offset=0x0

for run in $(seq 1 1 $num_partitions)
do
	printf -v addr '%#x' "$((addr + offset))"

	part=$(echo "$partition_names" | awk -v i=$run -F, '{print $i}')
	size=$(echo "$partition_sizes" | awk -v i=$run -F, '{print $i}')
	size=$(expr $size \* 1024)
	parttype=$(echo "$partition_types" | awk -v i=$run -F, '{print $i}')
	echo -e "\t{"\"$part\",$addr,$size,\"$parttype\""}," >>  ${OUTFILE}
	offset=$size
done

echo "}" >> ${OUTFILE}




