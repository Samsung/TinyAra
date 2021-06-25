/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
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

/*  str, type, description */
WIFIMGR_EVT_TABLE("INIT_CMD", EVT_INIT_CMD, "Command to initialize WiFi Manager")
WIFIMGR_EVT_TABLE("DEINIT_CMD", EVT_DEINIT_CMD, "Command to Deinit WiFi Manager")
WIFIMGR_EVT_TABLE("SET_SOFTAP_CMD", EVT_SET_SOFTAP_CMD, "Command to set SoftAP")
WIFIMGR_EVT_TABLE("SET_STA_CMD", EVT_SET_STA_CMD, "Command to set STA mode")
WIFIMGR_EVT_TABLE("CONNECT_CMD", EVT_CONNECT_CMD,	"Command to connect to a WiFi AP")
WIFIMGR_EVT_TABLE("DISCONNECT_CMD", EVT_DISCONNECT_CMD, "Command to Disconnect from a connected WiFi AP")
WIFIMGR_EVT_TABLE("SCAN_CMD", EVT_SCAN_CMD, "Command to perform WiFi Scanning over WLAN channels")
WIFIMGR_EVT_TABLE("GETINFO_CMD", EVT_GETINFO_CMD, "Command to get WiFi Manager information")
WIFIMGR_EVT_TABLE("GETSTATS_CMD", EVT_GETSTATS_CMD, "Command to get WiFi driver stats")
WIFIMGR_EVT_TABLE("EVT_STA_CONNECTED", EVT_STA_CONNECTED, "Event that STA is connected")
WIFIMGR_EVT_TABLE("EVT_STA_CONNECT_FAILED", EVT_STA_CONNECT_FAILED, "Event that STA connect failed")
WIFIMGR_EVT_TABLE("EVT_STA_DISCONNECTED", EVT_STA_DISCONNECTED, "Event that external STA disconnected from WiFi AP")
WIFIMGR_EVT_TABLE("EVT_DHCPS_ASSIGN_IP", EVT_DHCPS_ASSIGN_IP, "Event that SoftAP got IP address")
WIFIMGR_EVT_TABLE("EVT_JOINED", EVT_JOINED, "Event that new STA joined softAP")
WIFIMGR_EVT_TABLE("EVT_LEFT", EVT_LEFT, "Event that external STA device left softAP")
WIFIMGR_EVT_TABLE("EVT_SCAN_DONE", EVT_SCAN_DONE, "Event that WiFi scanning over WLAN channels is done")
