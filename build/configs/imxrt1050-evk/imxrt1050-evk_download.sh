#!/usr/bin/env bash
###########################################################################
#
# Copyright 2019 NXP Semiconductors All Rights Reserved.
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
# imxrt1050-evk_download.sh

THIS_PATH=`test -d ${0%/*} && cd ${0%/*}; pwd`

#echo $THIS_PATH

# When location of this script is changed, only OS_DIR_PATH should be changed together!!!
OS_DIR_PATH=$THIS_PATH/../../../os

IMXRT1050_USB_STRING=`mount | grep RT1050-EVK`
FIND_WORD="RT1050-EVK"

#echo $IMXRT1050_USB_STRING

for ELEMENT in $IMXRT1050_USB_STRING; do 
	#echo $ELEMENT
	if [[ "$ELEMENT" =~ "$FIND_WORD" ]]; then
		IMXRT1050_USB_PATH=$ELEMENT
	fi 
done;

#echo $IMXRT1050_USB_PATH
#echo $OS_DIR_PATH

CONFIG=${OS_DIR_PATH}/.config
if [ ! -f ${CONFIG} ]; then
        echo "No .config file"
        exit 1
fi

source ${CONFIG}

if [[ "${CONFIG_BUILD_PROTECTED}" == "y" ]]; then
sed '$d' $OS_DIR_PATH/../build/output/bin/tinyara.hex > $OS_DIR_PATH/../build/output/bin/tinyara_temp.hex
cat $OS_DIR_PATH/../build/output/bin/tinyara_temp.hex $OS_DIR_PATH/../build/output/bin/tinyara_user.hex > $OS_DIR_PATH/../build/output/bin/tinyara_prot.hex
echo "Downloading tinyara_prot.hex for Protected build"
cp $OS_DIR_PATH/../build/output/bin/tinyara_prot.hex $IMXRT1050_USB_PATH
else
echo "Downloading tinyara.hex for Flat build"
cp $OS_DIR_PATH/../build/output/bin/tinyara.hex $IMXRT1050_USB_PATH
fi
