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
 * @defgroup BLE_Common
 * @ingroup BLE_Common
 * @brief Provides APIs for BLE Common
 * @{
 */

/**
 * @file ble_manager/ble_client.h
 * @brief Provides APIs for BLE Common
 */
#pragma once

#include <stdint.h>

/* Length defines */
#define BLE_BD_ADDR_MAX_LEN 6
#define BLE_BD_ADDR_STR_LEN 17
#define BLE_ADV_RAW_DATA_MAX_LEN 31
#define BLE_ADV_RESP_DATA_MAX_LEN 31
#define BLE_MAX_CONNECTION_COUNT 3

typedef struct _ble_data {
	uint8_t *data;
	uint16_t length;
} ble_data;

/**
 * @brief Result types of BLE Manager APIs such as FAIL, SUCCESS, or INVALID ARGS
 */
typedef enum {
	BLE_MANAGER_SUCCESS = 0,
	BLE_MANAGER_FAIL,
	BLE_MANAGER_NOT_FOUND,
	BLE_MANAGER_INVALID_STATE,
	BLE_MANAGER_INVALID_ARGS,
	BLE_MANAGER_TIMEOUT,
	BLE_MANAGER_BUSY,
	BLE_MANAGER_FILE_ERROR,
	BLE_MANAGER_UNSUPPORTED,
	BLE_MANAGER_CALLBACK_NOT_REGISTERED,
	BLE_MANAGER_UNKNOWN,
} ble_result_e;

typedef uint16_t ble_conn_handle;
typedef uint16_t ble_attr_handle;
