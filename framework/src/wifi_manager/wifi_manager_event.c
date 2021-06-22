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
#include <wifi_manager/wifi_manager.h>
#include <tinyara/wifi/wifi_common.h>
#include "wifi_manager_dhcp.h"
#include "wifi_manager_utils.h"
#include "wifi_manager_event.h"
#include "wifi_manager_msghandler.h"

/*  wifimgr_evt_str should be mapped to enum _wifimgr_evt  */
static char *wifimgr_evt_str[] = {
	"EVT_INIT_CMD",				// Command to initialize WiFi Manager
	"EVT_DEINIT_CMD",			// Command to Deinit WiFi Manager
	"EVT_SET_SOFTAP_CMD",
	"EVT_SET_STA_CMD",
	"EVT_CONNECT_CMD",
	"EVT_DISCONNECT_CMD",
	"EVT_SCAN_CMD",
	"EVT_GETINFO_CMD",
	"EVT_GETSTATS_CMD",
	"EVT_RECONNECT_CMD",
	"EVT_STA_CONNECTED",
	"EVT_STA_CONNECT_FAILED",
	"EVT_STA_DISCONNECTED",
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
	"EVT_DHCPS_ASSIGN_IP",
#endif
	"EVT_JOINED",
	"EVT_LEFT",
	"EVT_SCAN_DONE",
	"EVT_NONE",
};

/*
 * wifi utils callbacks registered
 */
static void _wifi_utils_connect_event(wifi_utils_result_e result, void *arg);
static void _wifi_utils_disconnect_event(void *arg);
static void _wifi_utils_join_event(void *arg); // it needs arg which contains sta infomation joined
static void _wifi_utils_leave_event(void *arg); // it needs arg which contains a sta info left
static void _wifi_utils_scan_done(wifi_utils_result_e result, wifi_utils_scan_list_s *slist, void *arg);
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
static void _wifi_dhcps_event(dhcp_evt_type_e type, void *data);
#endif

#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
void _wifi_dhcps_event(dhcp_evt_type_e type, void *data)
{
	WM_ENTER;
	if (type == DHCP_ACK_EVT) {
		wifimgr_msg_s msg = {EVT_DHCPS_ASSIGN_IP, WIFI_MANAGER_FAIL, data, NULL};
		WIFIMGR_CHECK_RESULT_NORET(wifimgr_post_message(&msg), "[WM] handle dhcpd event fail\n");
	}
	return;
}
#endif

void _wifi_utils_connect_event(wifi_utils_result_e result, void *arg)
{
	WM_ENTER;
	wifimgr_msg_s msg = {EVT_STA_CONNECTED, WIFI_MANAGER_FAIL, NULL, NULL};
	if (result == WIFI_UTILS_FAIL) {
		msg.event = EVT_STA_CONNECT_FAILED;
	}
	WIFIMGR_CHECK_RESULT_NORET(wifimgr_post_message(&msg), "[WM] handle connect_event fail\n");
}


void _wifi_utils_disconnect_event(void *arg)
{
	WM_ENTER;
	wifimgr_msg_s msg = {EVT_STA_DISCONNECTED, WIFI_MANAGER_FAIL, NULL, NULL};
	WIFIMGR_CHECK_RESULT_NORET(wifimgr_post_message(&msg), "[WM] handle disconnect event fail\n");
}


void _wifi_utils_join_event(void *arg)
{
	WM_ENTER;
	wifimgr_msg_s msg = {EVT_JOINED, WIFI_MANAGER_FAIL, NULL, NULL};
	WIFIMGR_CHECK_RESULT_NORET(wifimgr_post_message(&msg), "[WM] handle join event fail\n");
	return;
}


void _wifi_utils_leave_event(void *arg)
{
	WM_ENTER;
	wifimgr_msg_s msg = {EVT_LEFT, WIFI_MANAGER_FAIL, NULL, NULL};
	WIFIMGR_CHECK_RESULT_NORET(wifimgr_post_message(&msg), "[WM] handle leave event fail\n");
}


void _wifi_utils_scan_done(wifi_utils_result_e result, wifi_utils_scan_list_s *slist, void *arg)
{
	WM_ENTER;
	wifimgr_msg_s msg = {EVT_SCAN_DONE, WIFI_MANAGER_FAIL, (void *)slist, NULL};
	WIFIMGR_CHECK_RESULT_NORET(wifimgr_post_message(&msg), "[WM] handle scan done event fail\n");
}


/*
* Public functions
*/

int wifimgr_get_evthandler(wifimgr_evt_handler_s *evt)
{
	if (!evt) {
		return -1;
	}

	evt->wifi_evt.sta_connected = _wifi_utils_connect_event;
	evt->wifi_evt.sta_disconnected = _wifi_utils_disconnect_event;
	evt->wifi_evt.softap_sta_joined = _wifi_utils_join_event;
	evt->wifi_evt.softap_sta_left = _wifi_utils_leave_event;
	evt->wifi_evt.scan_done = _wifi_utils_scan_done;
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
	evt->dhcps_sta_joined = _wifi_dhcps_event;
#endif
	return 0;
}


char *wifimgr_get_evt_str(wifimgr_evt_e evt)
{
	return wifimgr_evt_str[evt];
}
