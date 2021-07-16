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

#pragma once

typedef enum {
	// Common
	BLE_EVT_CMD_INIT,	
	BLE_EVT_CMD_DEINIT,
	BLE_EVT_CMD_GET_MAC,
	BLE_EVT_CMD_DISCONNECT,
	BLE_EVT_CMD_DEL_BOND,
	BLE_EVT_CMD_DEL_BOND_ALL,
	BLE_EVT_CMD_CONN_IS_ACTIVE,
	BLE_EVT_CMD_CONN_IS_ANY_ACTIVE,

	// Client
	BLE_EVT_CMD_START_SCAN,
	BLE_EVT_CMD_STOP_SCAN,
	BLE_EVT_CMD_CONNECT,
	BLE_EVT_CMD_DISCONNECT_ALL,
	BLE_EVT_CMD_CONNECTED_DEV_LIST,
	BLE_EVT_CMD_CONNECTED_INFO,
	BLE_EVT_CMD_OP_ENABLE_NOTI,
	BLE_EVT_CMD_OP_READ,
	BLE_EVT_CMD_OP_WRITE,
	BLE_EVT_CMD_OP_WRITE_NO_RESP,

	// Server
	BLE_EVT_CMD_GET_PROFILE_COUNT,
	BLE_EVT_CMD_CHARACT_NOTI,
	BLE_EVT_CMD_ATTR_SET_DATA,
	BLE_EVT_CMD_ATTR_GET_DATA,
	BLE_EVT_CMD_ATTR_REJECT,
	BLE_EVT_CMD_GET_MAC_BY_CONN,
	BLE_EVT_CMD_GET_CONN_BY_MAC,
	BLE_EVT_CMD_SET_ADV_DATA,
	BLE_EVT_CMD_SET_ADV_RESP,
	BLE_EVT_CMD_GET_BONDED_DEV,
	BLE_EVT_CMD_START_ADV,
	BLE_EVT_CMD_START_ADV_DIRECTED,
	BLE_EVT_CMD_STOP_ADV,
	BLE_EVT_CMD_SET_ADV_INTERVAL,
	
	// Callback Event
	BLE_EVT_CONNECTED,
	BLE_EVT_DISCONNECTED,
	BLE_EVT_SCAN_STARTED,
	BLE_EVT_SCAN_STOPPED,
	BLE_EVT_BONDED,
	BLE_EVT_NONE,
} blemgr_req_e;
