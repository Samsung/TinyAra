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

#ifndef _WIFI_MANAGER_EVENT_H__
#define _WIFI_MANAGER_EVENT_H__

#define WIFIMGR_EVENT_INITIALIZER {NULL, {NULL, NULL, NULL, NULL, NULL} }

enum _wifimgr_evt {
	EVT_INIT_CMD,				// Command to initialize WiFi Manager
	EVT_DEINIT_CMD,				// Command to Deinit WiFi Manager
	EVT_SET_SOFTAP_CMD,			// Command to set SoftAP
	EVT_SET_STA_CMD,			// Command to set STA mode
	EVT_CONNECT_CMD,			// Command to connect to a WiFi AP
	EVT_DISCONNECT_CMD,			// Command to Disconnect from a connected WiFi AP
	EVT_SCAN_CMD,				// Command to perform WiFi Scanning over WLAN channels
	EVT_GETINFO_CMD,			// Command to get WiFi Manager information
	EVT_GETSTATS_CMD,			// Command to get WiFi driver stats
	EVT_RECONNECT_CMD,			// Command to reconnect to WiFi AP. If we use EVT_CONNECT_CMD, then we can't distinguish it's from application request
	EVT_STA_CONNECTED,			// Event that STA is connected
	EVT_STA_CONNECT_FAILED, // Event that STA connect failed
	EVT_STA_DISCONNECTED,	// Event that external STA disconnected from WiFi AP
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
	EVT_DHCPS_ASSIGN_IP,	// Event that SoftAP got IP address
#endif
	EVT_JOINED,				// Event that new STA joined softAP
	EVT_LEFT,				// Event that external STA device left softAP
	EVT_SCAN_DONE,			// Event that WiFi scanning over WLAN channels is done
	EVT_NONE,
};
typedef enum _wifimgr_evt wifimgr_evt_e;

struct wifimgr_evt_handler {
	dhcp_sta_joined_cb dhcps_sta_joined;
	wifi_utils_cb_s wifi_evt;
};
typedef struct wifimgr_evt_handler wifimgr_evt_handler_s;

int wifimgr_get_evthandler(wifimgr_evt_handler_s *evt);
char *wifimgr_get_evt_str(wifimgr_evt_e evt);

#endif // _WIFI_MANAGER_EVENT_H__
