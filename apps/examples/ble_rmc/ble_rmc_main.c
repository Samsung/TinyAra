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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <tinyara/clock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ble_manager/ble_manager.h>
#include <semaphore.h>
#include <errno.h>

#define RMC_TAG "[RMC]"
// #define RMC_LOG(tag, fmt, args...) printf("\033[0;32m"tag fmt"\033[0m", ##args)
#define RMC_LOG(tag, fmt, args...) printf(tag fmt, ##args)

static sem_t g_conn_sem = { 0, };
static ble_conn_handle g_conn = 0;
static int g_scan_done = -1;
static uint8_t g_target[BLE_BD_ADDR_MAX_LEN] = { 0, };

static void ble_scan_state_changed_cb(ble_client_scan_state_e scan_state)
{
	RMC_LOG(RMC_TAG, "'%s' is called[%d]\n", __FUNCTION__, scan_state);
	return;
}

static void ble_device_scanned_cb(ble_client_scanned_device *scanned_device)
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	printf("scan mac : %02x:%02x:%02x:%02x:%02x:%02x\n", 
		scanned_device->addr.bd_addr[0],
		scanned_device->addr.bd_addr[1],
		scanned_device->addr.bd_addr[2],
		scanned_device->addr.bd_addr[3],
		scanned_device->addr.bd_addr[4],
		scanned_device->addr.bd_addr[5]
	);
	if (g_scan_done == 0) {
		g_target[0] = scanned_device->addr.bd_addr[5];
		g_target[1] = scanned_device->addr.bd_addr[4];
		g_target[2] = scanned_device->addr.bd_addr[3];
		g_target[3] = scanned_device->addr.bd_addr[2];
		g_target[4] = scanned_device->addr.bd_addr[1];
		g_target[5] = scanned_device->addr.bd_addr[0];
		g_scan_done = 1;
	}
	
	return;
}

static void ble_device_disconnected_cb(ble_conn_handle conn_id)
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	return;
}

static void ble_device_connected_cb(ble_client_device_connected *dev)
{
	int ret;
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	ret = sem_post(&g_conn_sem);

	g_conn = dev->conn_handle;
	RMC_LOG(RMC_TAG, "Conn Handle : %d\n", g_conn);
	return;
}

static void ble_operation_notification_cb(ble_client_operation_handle *handle, ble_data *read_result)
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	printf("conn : %d // attr : %x // len : %d\n", handle->conn_handle, handle->attr_handle, read_result->length);
	if (read_result->length > 0) {
		printf("read : ");
		int i;
		for (i = 0; i < read_result->length; i++) {
			printf("%02x ", read_result->data[i]);
		}
		printf("\n");
	}
	return;
}

static void ble_server_connected_cb(ble_conn_handle con_handle, ble_server_connection_type_e conn_type, uint8_t mac[BLE_BD_ADDR_MAX_LEN])
{
	RMC_LOG(RMC_TAG, "'%s' is called\n", __FUNCTION__);
	printf("conn : %d / conn_type : %d\n", con_handle, conn_type);
	printf("conn mac : %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return;
}

static void utc_cb_charact_a_1(ble_server_attr_cb_type_e type, ble_conn_handle linkindex, ble_attr_handle handle, void *arg)
{
	char *arg_str = "None";
	if (arg != NULL) {
		arg_str = (char *)arg;
	}
	printf("-- [RMC CHAR] %s [ type : %d / handle : %d / attr : %d ] --\n", arg_str, type, linkindex, handle);
}

static void utc_cb_desc_b_1(ble_server_attr_cb_type_e type, ble_conn_handle linkindex, ble_attr_handle handle, void *arg)
{
	char *arg_str = "None";
	if (arg != NULL) {
		arg_str = (char *)arg;
	}
	printf("-- [RMC DESC] %s [ type : %d / handle : %d / attr : %d ] --\n", arg_str, type, linkindex, handle);
}

static ble_server_gatt_t gatt_profile[] = {
	{
		.type = BLE_SERVER_GATT_SERVICE,
		.uuid = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01},
		.uuid_length = 16,
		.attr_handle = 0x006a,
	},

	{
		.type = BLE_SERVER_GATT_CHARACT, 
		.uuid = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02}, 
		.uuid_length = 16, .property = BLE_ATTR_PROP_RWN | BLE_ATTR_PROP_WRITE_NO_RSP, 
		.permission = BLE_ATTR_PERM_R_PERMIT | BLE_ATTR_PERM_W_PERMIT, 
		.attr_handle = 0x006b, 
		.cb = utc_cb_charact_a_1, 
		.arg = NULL
	},

	{
		.type = BLE_SERVER_GATT_DESC, 
		.uuid = {0x02, 0x29}, 
		.uuid_length = 2, 
		.permission = BLE_ATTR_PERM_R_PERMIT | BLE_ATTR_PERM_W_PERMIT, 
		.attr_handle = 0x006c, 
		.cb = utc_cb_desc_b_1, 
		.arg = NULL,
	},
};

static uint8_t ble_filter[] = { 0x02, 0x01, 0x05, 0x03, 0x19, 0x80, 0x01, 0x05, 0x03, 0x12, 0x18, 0x0f, 0x18 };

/****************************************************************************
 * ble_rmc_main
 ****************************************************************************/
int ble_rmc_main(int argc, char *argv[])
{
	RMC_LOG(RMC_TAG, "test!!\n");

	ble_result_e ret;

	ble_client_init_config client_config = {
		ble_scan_state_changed_cb,
		ble_device_scanned_cb,
		ble_device_disconnected_cb,
		ble_device_connected_cb,
		ble_operation_notification_cb,
		240};

	ble_server_init_config server_config = {
		ble_server_connected_cb,
		true,
		gatt_profile, sizeof(gatt_profile) / sizeof(ble_server_gatt_t)};

	if (argc < 2) {
		return 0;
	}

	RMC_LOG(RMC_TAG, "cmd : %s\n", argv[1]);

	if (strncmp(argv[1], "init", 5) == 0) {
		ret = ble_manager_init(&client_config, &server_config);
		RMC_LOG(RMC_TAG, "init done[%d]\n", ret);

		sem_init(&g_conn_sem, 0, 0);
	}

	if (strncmp(argv[1], "deinit", 7) == 0) {
		ret = ble_manager_deinit();
		RMC_LOG(RMC_TAG, "deinit done[%d]\n", ret);

		sem_destroy(&g_conn_sem);
	}

	if (strncmp(argv[1], "mac", 4) == 0) {
		uint8_t mac[BLE_BD_ADDR_MAX_LEN];
		int i;

		ret = ble_manager_get_mac_addr(mac);

		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "get mac fail[%d]\n", ret);
			return 0;
		}

		RMC_LOG(RMC_TAG, "BLE mac : %02x", mac[0]);
		for (i = 1; i < BLE_BD_ADDR_MAX_LEN; i++) {
			printf(":%02x", mac[i]);
		}
		printf("\n");
	}

	if (strncmp(argv[1], "scan", 5) == 0) {
		if (argc == 3 && argv[2][0] == '1') {
			printf("Start !\n");
			ret = ble_client_start_scan(NULL);

			if (ret != BLE_MANAGER_SUCCESS) {
				RMC_LOG(RMC_TAG, "scan start fail[%d]\n", ret);
				return 0;
			}
		} else if (argc == 3 && argv[2][0] == '2') {
			printf("Start with filter!\n");

			ble_client_scan_filter filter = { 0, };
			memcpy(&(filter.raw_data), ble_filter, sizeof(ble_filter));
			filter.raw_data_length = sizeof(ble_filter);
			filter.scan_duration = 1500;
			ret = ble_client_start_scan(&filter);

			if (ret != BLE_MANAGER_SUCCESS) {
				RMC_LOG(RMC_TAG, "scan start fail[%d]\n", ret);
				return 0;
			}
		} else {
			printf("stop !\n");
			ret = ble_client_stop_scan();

			if (ret != BLE_MANAGER_SUCCESS) {
				RMC_LOG(RMC_TAG, "scan stop fail[%d]\n", ret);
				return 0;
			}
		}
	}

	if (strncmp(argv[1], "check", 6) == 0) {
		bool chk = false;
		ret = ble_manager_conn_is_active(g_conn, &chk);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "Fail to get status[%d]\n", ret);
		} else {
			if (chk == true) {
				RMC_LOG(RMC_TAG, "Connected!\n");
			} else {
				RMC_LOG(RMC_TAG, "Disonnected!\n");
			}
		}
	}

	if (strncmp(argv[1], "connect", 8) == 0) {
		struct timespec abstime;

		ret = ble_client_delete_bond_all();
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "fail to del bonded dev[%d]\n", ret);
			return 0;
		}

		ble_client_scan_filter filter = { 0, };
		memcpy(&(filter.raw_data), ble_filter, sizeof(ble_filter));
		filter.raw_data_length = sizeof(ble_filter);
		filter.scan_duration = 1500;
		g_scan_done = 0;
		ret = ble_client_start_scan(&filter);

		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "scan start fail[%d]\n", ret);
			return 0;
		}

		sleep(2);

		if (g_scan_done != 1) {
			RMC_LOG(RMC_TAG, "scan fail to get adv packet\n", ret);
			return 0;
		}

		printf("Try to connect! [%02x:%02x:%02x:%02x:%02x:%02x]\n", 
			g_target[0],
			g_target[1],
			g_target[2],
			g_target[3],
			g_target[4],
			g_target[5]
		);

		ble_client_bd_addr addr = { 0, };
		memcpy(addr.bd_addr, g_target, BLE_BD_ADDR_MAX_LEN);
		addr.addr_type = BLE_CLIENT_ADDR_TYPE_PUBLIC;
		addr.conn_interval = 8;
		addr.slave_latency = 128;
		addr.mtu = 240;
		addr.is_secured_connect = true;

		ret = ble_client_connect(&addr);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "connect fail[%d]\n", ret);
		}

		ret = clock_gettime(CLOCK_REALTIME, &abstime);

		abstime.tv_sec = abstime.tv_sec + 10;
		abstime.tv_nsec = 0;

		int status = sem_timedwait(&g_conn_sem, &abstime);
		int errcode = errno;
		if (status < 0) {
			RMC_LOG(RMC_TAG, "ERROR: sem_timedwait failed with: %d\n", errcode);
		} else {
			RMC_LOG(RMC_TAG, "PASS: sem_timedwait succeeded\n");
		}
		
		ble_client_operation_handle handle[2] = { 0, };
		
		handle[0].conn_handle = g_conn;
		handle[0].attr_handle = 0xff03;

		handle[1].conn_handle = g_conn;
		handle[1].attr_handle = 0x006e;

		ret = ble_client_operation_enable_notification(&handle[0]);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "Fail to enable noti handle1[%d]\n", ret);
		} else {
			RMC_LOG(RMC_TAG, "Success to enable noti handle1.\n");
		}
		ret = ble_client_operation_enable_notification(&handle[1]);
		if (ret != BLE_MANAGER_SUCCESS) {
			RMC_LOG(RMC_TAG, "Fail to enable noti handle2[%d]\n", ret);
		} else {
			RMC_LOG(RMC_TAG, "Success to enable noti handle2.\n");
		}
	}

	RMC_LOG(RMC_TAG, "done\n");
	return 0;
}
