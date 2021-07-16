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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/net/if/ble.h>
#include <tinyara/ble/ble_manager.h>

#define TRBLE_DRV_CALL(res, dev, method, param)		\
	do {										\
		if (dev->ops->method) {			\
			res = (dev->ops->method)param;	\
		}										\
	} while (0)

int bledev_handle(struct bledev *dev, lwnl_req cmd, void *data, uint32_t data_len)
{
	// To Do
	trble_result_e ret = TRBLE_FAIL;

	BLE_LOGV(TRBLE_TAG, "cmd(%d) data(%p) len(%d)\n", cmd.type, data, data_len);

	switch (cmd.type) {
	case LWNL_REQ_BLE_INIT:
	{
		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		}
		trble_client_init_config *t_client = (trble_client_init_config *)param.param[0];
		trble_server_init_config *t_server = (trble_server_init_config *)param.param[1];
		trble_client_init_config client = { 0, };
		trble_server_init_config server = { 0, };

#if defined(CONFIG_AMEBAD_BLE_SCATTERNET) || defined(CONFIG_AMEBAD_BLE_CENTRAL)
		if (t_client != NULL) {
			memcpy(&client, t_client, sizeof(trble_client_init_config));
		}
#endif
#if defined(CONFIG_AMEBAD_BLE_SCATTERNET) || defined(CONFIG_AMEBAD_BLE_PERIPHERAL)
		if (t_server != NULL) {
			memcpy(&server, t_server, sizeof(trble_server_init_config));
		}
#endif
		TRBLE_DRV_CALL(ret, dev, init, (dev, &client, &server));
	}
	break;
	case LWNL_REQ_BLE_DEINIT:
	{
		TRBLE_DRV_CALL(ret, dev, deinit, (dev));
	}
	break;
	case LWNL_REQ_BLE_GET_MAC:
	{
		uint8_t *mac = (uint8_t *)data;
		if (mac != NULL) {
			TRBLE_DRV_CALL(ret, dev, get_mac, (dev, mac));
		} else {
			ret = TRBLE_INVALID_ARGS;
		}
	}
	break;
	case LWNL_REQ_BLE_DISCONNECT:
	{
		trble_conn_handle con_handle = 0;
		trble_mode_e mode;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		con_handle = *(trble_conn_handle *)param.param[0];
		mode = *(trble_mode_e *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, disconnect, (dev, con_handle, mode));
	}
	break;
	case LWNL_REQ_BLE_DEL_BOND:
	{
		trble_bd_addr *addr = NULL;
		trble_mode_e mode;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		addr = (trble_bd_addr *)param.param[0];
		mode = *(trble_mode_e *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, del_bond, (dev, addr, mode));
	}
	break;
	case LWNL_REQ_BLE_DEL_BOND_ALL:
	{
		trble_mode_e mode;
		if (data != NULL) {
			mode = *(trble_mode_e *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}
		TRBLE_DRV_CALL(ret, dev, del_bond_all, (dev, mode));
	}
	break;
	case LWNL_REQ_BLE_CONN_IS_ACTIVE:
	{
		bool *is_active = NULL;
		trble_conn_handle con_handle = 0;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		con_handle = *(trble_conn_handle *)param.param[0];
		is_active = (bool *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, conn_is_active, (dev, con_handle, is_active));
	}
	break;
	case LWNL_REQ_BLE_CONN_IS_ANY_ACTIVE:
	{
		bool *is_active = NULL;
		if (data != NULL) {
			is_active = (bool *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}
		TRBLE_DRV_CALL(ret, dev, conn_is_any_active, (dev, is_active));
	}
	break;
	
	// Client
	case LWNL_REQ_BLE_START_SCAN:
	{
		/* filter can be NULL */
		trble_scan_filter *filter = (trble_scan_filter *)data;
		TRBLE_DRV_CALL(ret, dev, start_scan, (dev, filter));
	}
	break;
	case LWNL_REQ_BLE_STOP_SCAN:
	{
		TRBLE_DRV_CALL(ret, dev, stop_scan, (dev));
	}
	break;
	case LWNL_REQ_BLE_CONNECT:
	{
		trble_bd_addr *addr = NULL;
		if (data != NULL) {
			addr = (trble_bd_addr *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}
		TRBLE_DRV_CALL(ret, dev, connect, (dev, addr));
	}
	break;
	case LWNL_REQ_BLE_DISCONNECT_ALL:
	{
		TRBLE_DRV_CALL(ret, dev, disconnect_all, (dev));
	}
	break;
	case LWNL_REQ_BLE_CONNECTED_DEV_LIST:
	{
		trble_connected_list *list = NULL;
		if (data != NULL) {
			list = (trble_connected_list *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}
		TRBLE_DRV_CALL(ret, dev, conn_dev_list, (dev, list));
	}
	break;
	case LWNL_REQ_BLE_CONNECTED_INFO:
	{
		trble_device_connected *device = NULL;
		trble_conn_handle handle;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		handle = *(trble_conn_handle *)param.param[0];
		device = (trble_device_connected *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, conn_info, (dev, handle, device));
	}
	break;
	case LWNL_REQ_BLE_OP_ENABLE_NOTI:
	{
		trble_operation_handle *handle = NULL;
		if (data != NULL) {
			handle = (trble_operation_handle *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}

		TRBLE_DRV_CALL(ret, dev, op_enable_noti, (dev, handle));
	}
	break;
	case LWNL_REQ_BLE_OP_READ:
	{
		trble_operation_handle *handle = NULL;
		trble_data *buf = NULL;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		handle = (trble_operation_handle *)param.param[0];
		buf = (trble_data *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, op_read, (dev, handle, buf));
	}
	break;
	case LWNL_REQ_BLE_OP_WRITE:
	{
		trble_operation_handle *handle = NULL;
		trble_data *buf = NULL;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		handle = (trble_operation_handle *)param.param[0];
		buf = (trble_data *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, op_write, (dev, handle, buf));
	}
	break;
	case LWNL_REQ_BLE_OP_WRITE_NO_RESP:
	{
		trble_operation_handle *handle = NULL;
		trble_data *buf = NULL;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		handle = (trble_operation_handle *)param.param[0];
		buf = (trble_data *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, op_wrtie_no_resp, (dev, handle, buf));
	}
	break;
	
	//Server
	case LWNL_REQ_BLE_GET_PROFILE_COUNT:
	{
		uint16_t *count = NULL;
		if (data != NULL) {
			count = (uint16_t *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}
		TRBLE_DRV_CALL(ret, dev, get_profile_count, (dev, count));
	}
	break;
	case LWNL_REQ_BLE_CHARACT_NOTI:
	{
		trble_attr_handle attr_handle = 0;
		trble_conn_handle con_handle = 0;
		trble_data *buf = NULL;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		attr_handle = *(trble_attr_handle *)param.param[0];
		con_handle = *(trble_conn_handle *)param.param[1];
		buf = (trble_data *)param.param[2];

		TRBLE_DRV_CALL(ret, dev, charact_noti, (dev, attr_handle, con_handle, buf));
	}
	break;
	case LWNL_REQ_BLE_ATTR_SET_DATA:
	{
		trble_attr_handle attr_handle = 0;
		trble_data *buf = NULL;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		attr_handle = *(trble_attr_handle *)param.param[0];
		buf = (trble_data *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, attr_set_data, (dev, attr_handle, buf));
	}
	break;
	case LWNL_REQ_BLE_ATTR_GET_DATA:
	{
		trble_attr_handle attr_handle = 0;
		trble_data *buf = NULL;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		attr_handle = *(trble_attr_handle *)param.param[0];
		buf = (trble_data *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, attr_get_data, (dev, attr_handle, buf));
	}
	break;
	case LWNL_REQ_BLE_ATTR_REJECT:
	{
		trble_attr_handle attr_handle = 0;
		uint8_t err_code = 0;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		attr_handle = *(trble_attr_handle *)param.param[0];
		err_code = *(uint8_t *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, attr_reject, (dev, attr_handle, err_code));
	}
	break;
	case LWNL_REQ_BLE_GET_MAC_BY_CONN:
	{
		trble_conn_handle con_handle = 0;
		uint8_t *bd_addr;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		con_handle = *(trble_conn_handle *)param.param[0];
		bd_addr = (uint8_t *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, get_mac_by_conn, (dev, con_handle, bd_addr));
	}
	break;
	case LWNL_REQ_BLE_GET_CONN_BY_MAC:
	{
		uint8_t *bd_addr = NULL;
		trble_conn_handle *con_handle;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		bd_addr = (uint8_t *)param.param[0];
		con_handle = (trble_conn_handle *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, get_conn_by_mac, (dev, bd_addr, con_handle));
	}
	break;
	case LWNL_REQ_BLE_SET_ADV_DATA:
	{
		trble_data *buf = NULL;
		if (data != NULL) {
			buf = (trble_data *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}
		TRBLE_DRV_CALL(ret, dev, set_adv_data, (dev, buf));
	}
	break;
	case LWNL_REQ_BLE_SET_ADV_RESP:
	{
		trble_data *buf = NULL;
		if (data != NULL) {
			buf = (trble_data *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}
		TRBLE_DRV_CALL(ret, dev, set_adv_resp, (dev, buf));
	}
	break;
	case LWNL_REQ_BLE_GET_BONDED_DEV:
	{
		trble_bonded_device_list_s *device_list = NULL;
		uint16_t *device_count = NULL;

		lwnl_msg_params param = { 0, };
		if (data != NULL) {
			memcpy(&param, data, data_len);
		} else {
			return TRBLE_INVALID_ARGS;
		}
		device_list = (trble_bonded_device_list_s *)param.param[0];
		device_count = (uint16_t *)param.param[1];

		TRBLE_DRV_CALL(ret, dev, get_bonded_dev, (dev, device_list, device_count));
	}
	break;
	case LWNL_REQ_BLE_START_ADV:
	{
		TRBLE_DRV_CALL(ret, dev, start_adv, (dev));
	}
	break;
	case LWNL_REQ_BLE_START_ADV_DIRECTED:
	{
		uint8_t *bd_addr = { 0, };

		if (data != NULL) {
			bd_addr = (uint8_t *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}

		TRBLE_DRV_CALL(ret, dev, start_adv_dir, (dev, bd_addr));
	}
	break;
	case LWNL_REQ_BLE_STOP_ADV:
	{
		TRBLE_DRV_CALL(ret, dev, stop_adv, (dev));
	}
	break;
	case LWNL_REQ_BLE_SET_ADV_INTERVAL:
	{
		uint16_t interval = 0;

		if (data != NULL) {
			interval = *(uint16_t *)data;
		} else {
			return TRBLE_INVALID_ARGS;
		}

		TRBLE_DRV_CALL(ret, dev, set_adv_interval, (dev, interval));
	}
	break;
	default:
		ret = TRBLE_UNKNOWN;
		break;
	}

	BLE_LOGV(TRBLE_TAG, "trble drv result : %d\n", ret);
	return ret;
}

// uint8_t t[3] = { 3, 2, 1 };
// trble_post_event(dev, LWNL_EVT_BLE_CONNECTED, t, 3);
int trble_post_event(struct bledev *dev, lwnl_cb_ble evt, void *buffer, uint32_t buf_len)
{
	BLE_LOGV(TRBLE_TAG, "trble post event : %d\n", evt);
	return lwnl_postmsg(LWNL_DEV_BLE, (int)evt, buffer, buf_len);
}
