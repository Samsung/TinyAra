/****************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <tinyara/wifi/wifi_common.h>
#include <tinyara/net/netlog.h>
#include <wifi_manager/wifi_manager.h>
#include "wifi_manager_state.h"
#include "wifi_manager_info.h"

#define TAG "[WM]"

struct _wifimgr_info {
	char ssid[WIFIMGR_SSID_LEN + 1];             // SSID of Connected AP if mode is a station
	char softap_ssid[WIFIMGR_SSID_LEN + 1];      // SoftAP SSID if mode is a soft ap
	int rssi;                  // It is only used for a station mode
	wifimgr_state_e state;
	wifimgr_state_e prev_state;
};
typedef struct _wifimgr_info _wifimgr_info_s;

static _wifimgr_info_s g_manager_info = {{0}, {0}, 0, WIFIMGR_NONE, WIFIMGR_NONE};

int wifimgr_get_info(int flag, wifimgr_info_msg_s *info)
{
	if ((flag & WIFIMGR_SSID) != 0) {
		strncpy(info->ssid, g_manager_info.ssid, WIFIMGR_SSID_LEN);
		info->ssid[WIFIMGR_SSID_LEN] = '\0';
	}

	if ((flag & WIFIMGR_SOFTAP_SSID) != 0) {
		strncpy(info->softap_ssid, g_manager_info.softap_ssid, WIFIMGR_SSID_LEN);
		info->softap_ssid[WIFIMGR_SSID_LEN] = '\0';
	}

	if ((flag & WIFIMGR_RSSI) != 0) {
		wifi_utils_info_s info_utils;
		wifi_utils_result_e wres = wifi_utils_get_info(&info_utils);
		if (wres == WIFI_UTILS_FAIL) {
			return -1;
		}
		info->rssi = info_utils.rssi;
	}

	if ((flag & WIFIMGR_STATE) != 0) {
		if (g_manager_info.state == WIFIMGR_SCANNING) {
			info->state = g_manager_info.prev_state;
		} else {
			info->state = g_manager_info.state;
		}
	}

	return 0;
}


int wifimgr_set_info(int flag, wifimgr_info_msg_s *info)
{
	if ((flag & WIFIMGR_SSID) != 0) {
		strncpy(g_manager_info.ssid, info->ssid, WIFIMGR_SSID_LEN);
		g_manager_info.ssid[WIFIMGR_SSID_LEN] = '\0';
	}

	if ((flag & WIFIMGR_SOFTAP_SSID) != 0) {
		strncpy(g_manager_info.softap_ssid, info->softap_ssid, WIFIMGR_SSID_LEN);
		g_manager_info.softap_ssid[WIFIMGR_SSID_LEN] = '\0';
	}

	if ((flag & WIFIMGR_RSSI) != 0) {
		g_manager_info.rssi = info->rssi;
	}

	if ((flag & WIFIMGR_STATE) != 0) {
		g_manager_info.prev_state = g_manager_info.state;
		g_manager_info.state = info->state;
	}
	return 0;
}

