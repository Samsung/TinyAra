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
#include <tinyara/config.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <tinyara/ble/ble_manager.h>
#include <ble_manager/ble_manager.h>
#include "ble_manager_event.h"
#include "ble_manager_msghandler.h"
#include "ble_manager_state.h"
#include "ble_manager_log.h"

struct _blemgr_state_handle {
	blemgr_state_e state;
	blemgr_server_state_e server_state;
	blemgr_server_adv_state_e adv_state;
	blemgr_client_state_e client_state[BLE_MAX_CONNECTION_COUNT];
	blemgr_client_scan_state_e scan_state;
	blemgr_mode_e mode;
};

typedef struct _blemgr_state_handle blemgr_state_handle_s;
static blemgr_state_handle_s g_state_handle = {
	BLEMGR_UNINITIALIZED,
	BLEMGR_SERVER_NONE,
	BLEMGR_SERVER_ADV_NONE,
	{
		0,
	},
	BLEMGR_CLIENT_SCAN_NONE,
	BLEMGR_UNKNOWN_MODE,
};

#define BLE_STATE_CHECK                                   \
	do {                                                  \
		if (g_state_handle.state != BLEMGR_INITIALIZED) { \
			BLE_LOG_INFO("[BLEMGR] is not running\n");    \
			ret = TRBLE_INVALID_STATE;                    \
			goto handle_req_done;                         \
		}                                                 \
	} while (0)

/*
 * public
 */
ble_result_e blemgr_handle_request(blemgr_msg_s *msg)
{
	trble_result_e ret = TRBLE_FAIL;

	BLE_LOG_INFO("[BLEMGR] T%d --> _handle_request[%d]\n", getpid(), msg->event);

	switch (msg->event) {
	case BLE_EVT_CMD_INIT: {
		if (g_state_handle.state != BLEMGR_UNINITIALIZED) {
			BLE_LOG_INFO("[BLEMGR] already running[mode : %d]\n", g_state_handle.mode);
			break;
		}
		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_client_init_config *client = (trble_client_init_config *)(param->param[0]);
		trble_server_init_config *server = (trble_server_init_config *)(param->param[1]);

		ret = ble_drv_init(client, server);
		if (ret != TRBLE_SUCCESS) {
			BLE_LOG_ERROR("[BLEMGR] init fail[%d]\n", ret);
			break;
		}
		g_state_handle.state = BLEMGR_INITIALIZED;
#if defined(CONFIG_AMEBAD_BLE_SCATTERNET)
		g_state_handle.mode = BLEMGR_COMBO_MODE;
#elif defined(CONFIG_AMEBAD_BLE_CENTRAL)
		g_state_handle.mode = BLEMGR_CLIENT_MODE;
#elif defined(CONFIG_AMEBAD_BLE_PERIPHERAL)
		g_state_handle.mode = BLEMGR_SERVER_MODE;
#endif
	} break;

	case BLE_EVT_CMD_DEINIT: {
		BLE_STATE_CHECK;

		ret = ble_drv_deinit();
		if (ret != TRBLE_SUCCESS) {
			BLE_LOG_ERROR("[BLEMGR] deinit fail[%d]\n", ret);
			break;
		}
		g_state_handle.state = BLEMGR_UNINITIALIZED;
		g_state_handle.mode = BLEMGR_UNKNOWN_MODE;
	} break;

	case BLE_EVT_CMD_GET_MAC: {
		BLE_STATE_CHECK;

		uint8_t *mac = (uint8_t *)msg->param;
		ret = ble_drv_get_mac_addr(mac);
	} break;
	case BLE_EVT_CMD_DISCONNECT: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_conn_handle con_handle = *(trble_conn_handle *)param->param[0];
		trble_mode_e mode = *(trble_mode_e *)param->param[1];

		ret = ble_drv_disconnect(con_handle, mode);
	} break;

	// Client
	case BLE_EVT_CMD_DEL_BOND: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_bd_addr *addr = (trble_bd_addr *)param->param[0];
		trble_mode_e mode = *(trble_mode_e *)param->param[1];

		ret = ble_drv_delete_bond(addr, mode);
	} break;

	case BLE_EVT_CMD_DEL_BOND_ALL: {
		BLE_STATE_CHECK;

		trble_mode_e mode = *(trble_mode_e *)msg->param;

		ret = ble_drv_delete_bond_all(mode);
	} break;
	case BLE_EVT_CMD_CONN_IS_ACTIVE: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_conn_handle con_handle = *(trble_conn_handle *)param->param[0];
		bool *is_active = (bool *)param->param[1];
		ret = ble_drv_conn_is_active(con_handle, is_active);
	} break;

	case BLE_EVT_CMD_CONN_IS_ANY_ACTIVE: {
		BLE_STATE_CHECK;

		bool *is_active = (bool *)msg->param;
		ret = ble_drv_conn_is_any_active(is_active);
	} break;
	case BLE_EVT_CMD_START_SCAN: {
		BLE_STATE_CHECK;

		trble_scan_filter *filter = (trble_scan_filter *)msg->param;
		ret = ble_drv_start_scan(filter);
	} break;

	case BLE_EVT_CMD_STOP_SCAN: {
		BLE_STATE_CHECK;

		ret = ble_drv_stop_scan();
	} break;

	case BLE_EVT_CMD_CONNECT: {
		BLE_STATE_CHECK;

		trble_bd_addr *addr = (trble_bd_addr *)msg->param;
		ret = ble_drv_connect(addr);
	} break;

	case BLE_EVT_CMD_DISCONNECT_ALL: {
		BLE_STATE_CHECK;

		ret = ble_drv_disconnect_all();
	} break;

	case BLE_EVT_CMD_CONNECTED_DEV_LIST: {
		BLE_STATE_CHECK;

		trble_connected_list *list = (trble_connected_list *)msg->param;
		ret = ble_drv_connected_device_list(list);
	} break;

	case BLE_EVT_CMD_CONNECTED_INFO: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_conn_handle conn_handle = *(trble_conn_handle *)param->param[0];
		trble_device_connected *device = (trble_device_connected *)param->param[1];

		ret = ble_drv_connected_info(conn_handle, device);
	} break;

	case BLE_EVT_CMD_OP_ENABLE_NOTI: {
		BLE_STATE_CHECK;

		trble_operation_handle *handle = (trble_operation_handle *)msg->param;
		ret = ble_drv_operation_enable_notification(handle);
	} break;

	case BLE_EVT_CMD_OP_READ: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_operation_handle *handle = (trble_operation_handle *)param->param[0];
		trble_data *data = (trble_data *)param->param[1];

		ret = ble_drv_operation_read(handle, data);
	} break;

	case BLE_EVT_CMD_OP_WRITE: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_operation_handle *handle = (trble_operation_handle *)param->param[0];
		trble_data *data = (trble_data *)param->param[1];

		ret = ble_drv_operation_write(handle, data);
	} break;

	case BLE_EVT_CMD_OP_WRITE_NO_RESP: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_operation_handle *handle = (trble_operation_handle *)param->param[0];
		trble_data *data = (trble_data *)param->param[1];

		ret = ble_drv_operation_write_no_response(handle, data);
	} break;

	// Server
	case BLE_EVT_CMD_GET_PROFILE_COUNT: {
		BLE_STATE_CHECK;

		uint16_t *count = (uint16_t *)msg->param;
		ret = ble_drv_get_profile_count(count);
	} break;

	case BLE_EVT_CMD_CHARACT_NOTI: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_attr_handle attr_handle = *(trble_attr_handle *)param->param[0];
		trble_conn_handle con_handle = *(trble_conn_handle *)param->param[1];
		trble_data *data = (trble_data *)param->param[2];

		ret = ble_drv_charact_notify(attr_handle, con_handle, data);
	} break;

	case BLE_EVT_CMD_ATTR_SET_DATA: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_attr_handle attr_handle = *(trble_attr_handle *)param->param[0];
		trble_data *data = (trble_data *)param->param[1];

		ret = ble_drv_attr_set_data(attr_handle, data);
	} break;

	case BLE_EVT_CMD_ATTR_GET_DATA: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_attr_handle attr_handle = *(trble_attr_handle *)param->param[0];
		trble_data *data = (trble_data *)param->param[1];

		ret = ble_drv_attr_get_data(attr_handle, data);
	} break;

	case BLE_EVT_CMD_ATTR_REJECT: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_attr_handle attr_handle = *(trble_attr_handle *)param->param[0];
		uint8_t app_errorcode = *(uint8_t *)param->param[1];

		ret = ble_drv_attr_reject(attr_handle, app_errorcode);
	} break;

	case BLE_EVT_CMD_GET_MAC_BY_CONN: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_conn_handle con_handle = *(trble_conn_handle *)param->param[0];
		uint8_t *bd_addr = (uint8_t *)param->param[1];

		ret = ble_drv_get_mac_addr_by_conn_handle(con_handle, bd_addr);
	} break;

	case BLE_EVT_CMD_GET_CONN_BY_MAC: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		uint8_t *bd_addr = (uint8_t *)param->param[0];
		trble_conn_handle *con_handle = (trble_conn_handle *)param->param[1];

		ret = ble_drv_get_conn_handle_by_addr(bd_addr, con_handle);
	} break;

	case BLE_EVT_CMD_SET_ADV_DATA: {
		BLE_STATE_CHECK;

		trble_data *data = (trble_data *)msg->param;
		ret = ble_drv_set_adv_data(data);
	} break;

	case BLE_EVT_CMD_SET_ADV_RESP: {
		BLE_STATE_CHECK;

		trble_data *data = (trble_data *)msg->param;
		ret = ble_drv_set_adv_resp(data);
	} break;

	case BLE_EVT_CMD_GET_BONDED_DEV: {
		BLE_STATE_CHECK;

		blemgr_msg_params *param = (blemgr_msg_params *)msg->param;
		trble_bonded_device_list_s *list = (trble_bonded_device_list_s *)param->param[0];
		uint16_t *device_count = (uint16_t *)param->param[1];

		ret = ble_drv_get_bonded_device(list, device_count);
	} break;

	case BLE_EVT_CMD_START_ADV: {
		BLE_STATE_CHECK;

		ret = ble_drv_start_adv();
	} break;

	case BLE_EVT_CMD_START_ADV_DIRECTED: {
		BLE_STATE_CHECK;

		uint8_t *bd_addr = (uint8_t *)msg->param;
		ret = ble_drv_start_adv_directed(bd_addr);
	} break;

	case BLE_EVT_CMD_STOP_ADV: {
		BLE_STATE_CHECK;

		ret = ble_drv_stop_adv();
	} break;

	case BLE_EVT_CMD_SET_ADV_INTERVAL: {
		BLE_STATE_CHECK;

		uint16_t interval = *(uint16_t *)msg->param;
		ret = ble_drv_set_adv_interval(interval);
	} break;

	default:
		ret = BLE_MANAGER_UNKNOWN;
		break;
	}

handle_req_done:
	BLE_LOG_INFO("[BLE] T%d <-- _handle_request\n", getpid());
	return ret;
}
