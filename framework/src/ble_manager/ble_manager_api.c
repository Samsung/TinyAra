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

#include <stdio.h>
#include <stdint.h>
#include <ble_manager/ble_manager.h>
#include "ble_manager_event.h"
#include "ble_manager_msghandler.h"

#define RETURN_RESULT(res, msg)      \
	do {                             \
		if (res < 0) {               \
			return BLE_MANAGER_FAIL; \
		} else {                     \
			return msg.result;       \
		}                            \
	} while (0)

ble_result_e ble_manager_init(ble_client_init_config *client_config, ble_server_init_config *server_config)
{
	blemgr_msg_params param = { 2, {(void *)client_config, (void *)server_config} };
	blemgr_msg_s msg = {BLE_EVT_CMD_INIT, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_manager_deinit(void)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_DEINIT, BLE_MANAGER_FAIL, NULL, NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_manager_get_mac_addr(uint8_t mac[BLE_BD_ADDR_MAX_LEN])
{
	blemgr_msg_s msg = {BLE_EVT_CMD_GET_MAC, BLE_MANAGER_FAIL, (void *)(mac), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_manager_conn_is_active(ble_conn_handle con_handle, bool *is_active)
{
	blemgr_msg_params param = { 2, {(void *)&con_handle, (void *)is_active} };
	blemgr_msg_s msg = {BLE_EVT_CMD_CONN_IS_ACTIVE, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_manager_conn_is_any_active(bool *is_active)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_CONN_IS_ANY_ACTIVE, BLE_MANAGER_FAIL, (void *)(is_active), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

/* Client */
ble_result_e ble_client_disconnect(ble_conn_handle conn_handle)
{
	blemgr_mode_e mode = BLEMGR_CLIENT_MODE;
	blemgr_msg_params param = { 2, {(void *)&conn_handle, (void *)&mode} };
	blemgr_msg_s msg = {BLE_EVT_CMD_DISCONNECT, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_delete_bond(ble_client_bd_addr *addr)
{
	blemgr_mode_e mode = BLEMGR_CLIENT_MODE;
	blemgr_msg_params param = { 2, {(void *)addr, (void *)&mode} };
	blemgr_msg_s msg = {BLE_EVT_CMD_DEL_BOND, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_delete_bond_all(void)
{
	blemgr_mode_e mode = BLEMGR_CLIENT_MODE;
	blemgr_msg_s msg = {BLE_EVT_CMD_DEL_BOND_ALL, BLE_MANAGER_FAIL, (void *)(&mode), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_start_scan(ble_client_scan_filter *filter)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_START_SCAN, BLE_MANAGER_FAIL, (void *)(filter), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_stop_scan(void)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_STOP_SCAN, BLE_MANAGER_FAIL, NULL, NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_connect(ble_client_bd_addr *addr)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_CONNECT, BLE_MANAGER_FAIL, (void *)(addr), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_disconnect_all(void)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_DISCONNECT_ALL, BLE_MANAGER_FAIL, NULL, NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_connected_device_list(ble_client_connected_list *out_connected_list)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_CONNECTED_DEV_LIST, BLE_MANAGER_FAIL, (void *)(out_connected_list), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_connected_info(ble_conn_handle conn_handle, ble_client_device_connected *out_connected_device)
{
	blemgr_msg_params param = { 2, {(void *)&conn_handle, (void *)out_connected_device} };
	blemgr_msg_s msg = {BLE_EVT_CMD_CONNECTED_INFO, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_operation_enable_notification(ble_client_operation_handle *handle)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_OP_ENABLE_NOTI, BLE_MANAGER_FAIL, (void *)(handle), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_operation_read(ble_client_operation_handle *handle, ble_data *data)
{
	blemgr_msg_params param = { 2, {(void *)handle, (void *)data} };
	blemgr_msg_s msg = {BLE_EVT_CMD_OP_READ, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_operation_write(ble_client_operation_handle *handle, ble_data *data)
{
	blemgr_msg_params param = { 2, {(void *)handle, (void *)data} };
	blemgr_msg_s msg = {BLE_EVT_CMD_OP_WRITE, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_client_operation_write_no_response(ble_client_operation_handle *handle, ble_data *data)
{
	blemgr_msg_params param = { 2, {(void *)handle, (void *)data} };
	blemgr_msg_s msg = {BLE_EVT_CMD_OP_WRITE_NO_RESP, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

/* Server */
ble_result_e ble_server_disconnect(ble_conn_handle con_handle)
{
	blemgr_mode_e mode = BLEMGR_SERVER_MODE;
	blemgr_msg_params param = { 2, {(void *)&mode, (void *)&con_handle} };
	blemgr_msg_s msg = {BLE_EVT_CMD_DISCONNECT, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_delete_bonded_device(uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN])
{
	blemgr_mode_e mode = BLEMGR_SERVER_MODE;
	blemgr_msg_params param = { 2, {(void *)&mode, (void *)bd_addr} };
	blemgr_msg_s msg = {BLE_EVT_CMD_DEL_BOND, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_delete_bonded_device_all(void)
{
	blemgr_mode_e mode = BLEMGR_SERVER_MODE;
	blemgr_msg_s msg = {BLE_EVT_CMD_DEL_BOND_ALL, BLE_MANAGER_FAIL, (void *)(&mode), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_get_profile_count(uint16_t *count)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_GET_PROFILE_COUNT, BLE_MANAGER_FAIL, (void *)(count), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_charact_notify(ble_attr_handle attr_handle, ble_conn_handle con_handle, ble_data *data)
{
	blemgr_msg_params param = { 3, {(void *)&attr_handle, (void *)&con_handle, (void *)data} };
	blemgr_msg_s msg = {BLE_EVT_CMD_CHARACT_NOTI, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_attr_set_data(ble_attr_handle attr_handle, ble_data *data)
{
	blemgr_msg_params param = { 2, {(void *)&attr_handle, (void *)data} };
	blemgr_msg_s msg = {BLE_EVT_CMD_ATTR_SET_DATA, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_attr_get_data(ble_attr_handle attr_handle, ble_data *data)
{
	blemgr_msg_params param = { 2, {(void *)&attr_handle, (void *)data} };
	blemgr_msg_s msg = {BLE_EVT_CMD_ATTR_GET_DATA, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_reject(ble_attr_handle attr_handle, uint8_t app_errorcode)
{
	blemgr_msg_params param = { 2, {(void *)&attr_handle, (void *)&app_errorcode} };
	blemgr_msg_s msg = {BLE_EVT_CMD_ATTR_REJECT, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_get_mac_addr_by_conn_handle(ble_conn_handle con_handle, uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN])
{
	blemgr_msg_params param = { 2, {(void *)&con_handle, (void *)bd_addr} };
	blemgr_msg_s msg = {BLE_EVT_CMD_GET_MAC_BY_CONN, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_get_conn_handle_by_addr(uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN], ble_conn_handle *con_handle)
{
	blemgr_msg_params param = { 2, {(void *)bd_addr, (void *)con_handle} };
	blemgr_msg_s msg = {BLE_EVT_CMD_GET_CONN_BY_MAC, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_set_adv_data(ble_data *data)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_SET_ADV_DATA, BLE_MANAGER_FAIL, (void *)(data), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_set_adv_resp(ble_data *data)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_SET_ADV_RESP, BLE_MANAGER_FAIL, (void *)(data), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_start_adv(void)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_START_ADV, BLE_MANAGER_FAIL, NULL, NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_stop_adv(void)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_STOP_ADV, BLE_MANAGER_FAIL, NULL, NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_start_adv_directed(uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN])
{
	blemgr_msg_s msg = {BLE_EVT_CMD_START_ADV_DIRECTED, BLE_MANAGER_FAIL, (void *)(bd_addr), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_set_adv_interval(unsigned int interval)
{
	blemgr_msg_s msg = {BLE_EVT_CMD_SET_ADV_INTERVAL, BLE_MANAGER_FAIL, (void *)(&interval), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}

ble_result_e ble_server_get_bonded_device(ble_server_bonded_device_list_s *device_list, uint16_t *device_count)
{
	blemgr_msg_params param = { 2, {(void *)device_list, (void *)device_count} };
	blemgr_msg_s msg = {BLE_EVT_CMD_GET_BONDED_DEV, BLE_MANAGER_FAIL, (void *)(&param), NULL};
	int res = blemgr_post_message(&msg);

	RETURN_RESULT(res, msg);
}
