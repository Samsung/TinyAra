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

OSDIR="../os"
METAFILE=".appSpec"
DOTCONFIG=".config"

function write_config(){
	file_path=`find $OSDIR -name "$METAFILE"`
	config_path=`find $OSDIR -name "$DOTCONFIG"`
	# update .config's user entry point from metafile;

	UPDATED_ENTRYPOINT=`sed -n '/USER_ENTRYPOINT/p' $file_path | sed -n 's/.*=//p'`
	sed -i "s/CONFIG_USER_ENTRYPOINT.*/CONFIG_USER_ENTRYPOINT=$UPDATED_ENTRYPOINT/" $config_path

	# update .config' use config from metafile;
	# if OLD_CONFIG_USE is not updated => maintain origin CONFIG_USE;
	# else OLD_CONFIG and NEW_CONFIG are not equal => change CONFIG_USE;

	UPDATED_APP_LIST=`sed -n '/{.*}/p' $file_path | sed -n 's/ //pg'`
	for UPDATED_APP in $UPDATED_APP_LIST
	do
		CONFIG_NAME=`echo "$UPDATED_APP" | awk 'BEGIN {FS=","} {print$3}'`
		OLD_CONFIG=`sed -n "/# $CONFIG_NAME is not set/p" $config_path`
		if [ "$OLD_CONFIG" = "" ]
			then OLD_USE_CONFIG="y"
		else
			OLD_USE_CONFIG="n"
		fi
		NEW_USE_CONFIG=`echo "$UPDATED_APP" | awk 'BEGIN {FS=","} {print$4}'`
		if [ "$OLD_USE_CONFIG" != "$NEW_USE_CONFIG" ]
			then if [ "$NEW_USE_CONFIG" = "y" ]
				 	then sed -i "s/# $CONFIG_NAME is not set/$CONFIG_NAME=y/" $config_path
				 else
					sed -i "s/$CONFIG_NAME=./# $CONFIG_NAME is not set/" $config_path
			fi
		fi
	done
}

function set_entrypoint(){
	# if appname for updating is omitted => set_entrypoint() is not excuted;
	if [ "$1" = "" ]
		then return
	fi
	file_path=`find $OSDIR -name "$METAFILE"`
	SELECTED_APP=`sed -n "/$1,/p" $file_path`
	SELECTED_APP_ENTRY=`echo "$SELECTED_APP" | awk 'BEGIN {FS=","} {print$2}' | sed -n 's/ //p'`
	sed -i "s/USER_ENTRYPOINT=.*/USER_ENTRYPOINT=$SELECTED_APP_ENTRY/" $file_path
}

while getopts ws opt
do
	case $opt in
		w) write_config;
			;;
		s) set_entrypoint $2;
			;;
	esac
done
