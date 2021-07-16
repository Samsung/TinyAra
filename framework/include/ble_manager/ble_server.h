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

/**
 * @defgroup BLE_Server
 * @ingroup BLE_Server
 * @brief Provides APIs for BLE Server
 * @{
 */

/**
 * @file ble_manager/ble_client.h
 * @brief Provides APIs for BLE Server
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ble_common.h"

typedef enum  {
	BLE_SERVER_GATT_SERVICE,
	BLE_SERVER_GATT_CHARACT,
	BLE_SERVER_GATT_DESC,
	BLE_SERVER_GATT_MAX
} ble_server_gatt_profile_type_e;

typedef enum {
	BLE_SERVER_ATTR_CB_WRITING,
	BLE_SERVER_ATTR_CB_READING,
	BLE_SERVER_ATTR_CB_WRITING_NO_RSP,
} ble_server_attr_cb_type_e;

typedef void (*ble_server_cb_t)(ble_server_attr_cb_type_e type, ble_conn_handle con_handle, ble_attr_handle handle, void* arg);

typedef enum  {
	BLE_ATTR_PROP_NONE          = 0x00,
	BLE_ATTR_PROP_BCAST         = 0x01,
	BLE_ATTR_PROP_READ          = 0x02,
	BLE_ATTR_PROP_WRITE_NO_RSP  = 0x04,
	BLE_ATTR_PROP_WRITE         = 0x08,
	BLE_ATTR_PROP_NOTIFY        = 0x10,
	BLE_ATTR_PROP_INDICATE      = 0x20,
	BLE_ATTR_PROP_AUTHEN        = 0x40,
	BLE_ATTR_PROP_EXTENDED      = 0x80,
	BLE_ATTR_PROP_RWN           = 0x1a,
} ble_attr_property_e;

typedef enum {
	BLE_ATTR_PERM_R_PERMIT    = 0x00,         /* Always permitted, no restrictions*/
	BLE_ATTR_PERM_R_AUTHEN    = 0x01,         /* Authentication required */
	BLE_ATTR_PERM_R_AUTHOR    = 0x02,         /* Authorization required */
	BLE_ATTR_PERM_R_ENCRYPT   = 0x04,         /* Can only be accessed in encrypted link*/
	BLE_ATTR_PERM_R_BANNED    = 0x08,         /* Operation not permitted */

	BLE_ATTR_PERM_W_PERMIT    = 0x00,         /* Always permitted, no restrictions*/
	BLE_ATTR_PERM_W_AUTHEN    = 0x10,         /* Authentication required */
	BLE_ATTR_PERM_W_AUTHOR    = 0x20,         /* Authorization required */
	BLE_ATTR_PERM_W_ENCRYPT   = 0x40,         /* Can only be accessed in encrypted link*/
	BLE_ATTR_PERM_W_BANNED    = 0x80,         /* Operation not permitted */
} ble_attr_permission_e;

typedef enum {
	BLE_SERVER_LL_CONNECTED,
	BLE_SERVER_SM_CONNECTED,
	BLE_SERVER_DISCONNECTED,
} ble_server_connection_type_e;

typedef struct {
	ble_server_gatt_profile_type_e type;
	uint8_t uuid[16];
	uint16_t uuid_length;
	uint8_t property;
	uint8_t permission;
	ble_attr_handle attr_handle;
	ble_server_cb_t cb;
	void* arg;
} ble_server_gatt_t;

typedef void (*ble_server_connected_t)(ble_conn_handle con_handle, ble_server_connection_type_e conn_type, uint8_t mac[BLE_BD_ADDR_MAX_LEN]);

typedef struct {
	ble_server_connected_t connected_cb;
	// true : Secure Manager is enabled. Bondable.
	// false : Secure Manager is disabled. Requesting Pairing will be rejected. Non-Bondable.
	bool is_secured_connect_allowed; 
	ble_server_gatt_t *profile;
	uint16_t profile_count;
} ble_server_init_config;

typedef struct ble_server_bonded_device_list {
	uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN];
	struct ble_server_bonded_device_list *next;
} ble_server_bonded_device_list_s;

ble_result_e ble_server_get_profile_count(uint16_t *count);

// API for sending a characteristic value notification to the selected target(s). (notify to all clients conn_handle (notify all = 0x99))
ble_result_e ble_server_charact_notify(ble_attr_handle attr_handle, ble_conn_handle con_handle, ble_data *data);

// set data of attribute value
ble_result_e ble_server_attr_set_data(ble_attr_handle attr_handle, ble_data *data);

// get data of attribute value
ble_result_e ble_server_attr_get_data(ble_attr_handle attr_handle, ble_data *data);

// reject attribute request in callback function and return error code
ble_result_e ble_server_reject(ble_attr_handle attr_handle, uint8_t app_errorcode);

ble_result_e ble_server_get_mac_addr_by_conn_handle(ble_conn_handle con_handle, uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN]);
ble_result_e ble_server_get_conn_handle_by_addr(uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN], ble_conn_handle *con_handle);

// Set Advertisement Data 
ble_result_e ble_server_set_adv_data(ble_data *data);

// Set Scan Response Data 
ble_result_e ble_server_set_adv_resp(ble_data *data);

// Disconnect client. The client with secured connection would be required pairing again. 
ble_result_e ble_server_disconnect(ble_conn_handle con_handle);

ble_result_e ble_server_start_adv(void);
ble_result_e ble_server_stop_adv(void);

ble_result_e ble_server_get_bonded_device(ble_server_bonded_device_list_s* device_list, uint16_t* device_count);
ble_result_e ble_server_delete_bonded_device(uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN]);
ble_result_e ble_server_delete_bonded_device_all(void);

ble_result_e ble_server_start_adv_directed(uint8_t bd_addr[BLE_BD_ADDR_MAX_LEN]);

/*
You can set the fixed interval from 20ms to 10.24 seconds, in steps of 0.625ms. 
The random delay is a pseudo-random value from 0ms to 10ms that is automatically added. 
This randomness helps reduce the possibility of collisions between advertisements of different devices
*/
ble_result_e ble_server_set_adv_interval(unsigned int interval);
