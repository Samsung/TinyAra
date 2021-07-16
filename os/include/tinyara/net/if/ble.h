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

#include <stdint.h>
#include <stdbool.h>

#define TRBLE_BD_ADDR_MAX_LEN 6
#define TRBLE_ADV_RAW_DATA_MAX_LEN 31
#define TRBLE_ADV_RESP_DATA_MAX_LEN 31
#define TRBLE_MAX_CONNECTION_COUNT 3

/****************************************************************************
 * BLE Variables
 ****************************************************************************/

/*** Common ***/
struct bledev;

typedef uint16_t trble_conn_handle;
typedef uint16_t trble_attr_handle;

typedef struct _trble_data {
	uint8_t *data;
	uint16_t length;
} trble_data;

typedef enum {
	// Common
	LWNL_REQ_BLE_INIT,
	LWNL_REQ_BLE_DEINIT,
	LWNL_REQ_BLE_GET_MAC,
	LWNL_REQ_BLE_DISCONNECT,
	LWNL_REQ_BLE_DEL_BOND,
	LWNL_REQ_BLE_DEL_BOND_ALL,
	LWNL_REQ_BLE_CONN_IS_ACTIVE,
	LWNL_REQ_BLE_CONN_IS_ANY_ACTIVE,
	
	// Client
	LWNL_REQ_BLE_START_SCAN,
	LWNL_REQ_BLE_STOP_SCAN,
	LWNL_REQ_BLE_CONNECT,
	LWNL_REQ_BLE_DISCONNECT_ALL,
	LWNL_REQ_BLE_CONNECTED_DEV_LIST,
	LWNL_REQ_BLE_CONNECTED_INFO,
	LWNL_REQ_BLE_OP_ENABLE_NOTI,
	LWNL_REQ_BLE_OP_READ,
	LWNL_REQ_BLE_OP_WRITE,
	LWNL_REQ_BLE_OP_WRITE_NO_RESP,

	// Server
	LWNL_REQ_BLE_GET_PROFILE_COUNT,
	LWNL_REQ_BLE_CHARACT_NOTI,
	LWNL_REQ_BLE_ATTR_SET_DATA,
	LWNL_REQ_BLE_ATTR_GET_DATA,
	LWNL_REQ_BLE_ATTR_REJECT,
	LWNL_REQ_BLE_GET_MAC_BY_CONN,
	LWNL_REQ_BLE_GET_CONN_BY_MAC,
	LWNL_REQ_BLE_SET_ADV_DATA,
	LWNL_REQ_BLE_SET_ADV_RESP,
	LWNL_REQ_BLE_GET_BONDED_DEV,
	LWNL_REQ_BLE_START_ADV,
	LWNL_REQ_BLE_START_ADV_DIRECTED,
	LWNL_REQ_BLE_STOP_ADV,
	LWNL_REQ_BLE_SET_ADV_INTERVAL,
	LWNL_REQ_BLE_UNKNOWN
} lwnl_req_ble;

typedef enum {
	LWNL_EVT_BLE_CONNECTED,
	LWNL_EVT_BLE_DISCONNECTED,
	LWNL_EVT_BLE_SCAN_START,
	LWNL_EVT_BLE_SCAN_STOP,
	LWNL_EVT_BLE_UNKNOWN,
} lwnl_cb_ble;

typedef enum {
	TRBLE_SUCCESS = 0,
	TRBLE_FAIL,
	TRBLE_NOT_FOUND,
	TRBLE_INVALID_STATE,
	TRBLE_INVALID_ARGS,
	TRBLE_TIMEOUT,
	TRBLE_BUSY,
	TRBLE_FILE_ERROR,
	TRBLE_UNSUPPORTED,
	TRBLE_CALLBACK_NOT_REGISTERED,
	TRBLE_UNKNOWN,
} trble_result_e;

typedef enum {
	TRBLE_MODE_SERVER,
	TRBLE_MODE_CLIENT,
	TRBLE_MODE_UNKNOWN,
} trble_mode_e;

/*** Central(Client) ***/
typedef enum {
	TRBLE_ADDR_TYPE_PUBLIC,
	TRBLE_ADDR_TYPE_RANDOM_STATIC,
	TRBLE_ADDR_TYPE_RANDOM_RESOLVABLE,
	TRBLE_ADDR_TYPE_RANDOM_NON_RESOLVABLE,
	TRBLE_ADDR_TYPE_UNKNOWN,
} trble_addr_type_e;

typedef enum {
	TRBLE_SCAN_STARTED,
	TRBLE_SCAN_STOPPED,
} trble_scan_state_e;

typedef enum {
	TRBLE_ADV_TYPE_IND,
	TRBLE_ADV_TYPE_DIRECT,
	TRBLE_ADV_TYPE_SCAN_IND,
	TRBLE_ADV_TYPE_NONCONN_IND,
	TRBLE_ADV_TYPE_SCAN_RSP,
	TRBLE_ADV_TYPE_UNKNOWN,
} trble_adv_type_e;

typedef struct {
	uint8_t bd_addr[TRBLE_BD_ADDR_MAX_LEN];
	trble_addr_type_e addr_type;
	uint16_t conn_interval;
	uint16_t slave_latency;
	uint16_t mtu;
	bool is_secured_connect;
} trble_bd_addr;

typedef struct {
	trble_adv_type_e adv_type;
	int8_t rssi;
	trble_bd_addr addr;
	uint8_t raw_data[TRBLE_ADV_RAW_DATA_MAX_LEN];
	uint8_t raw_data_length;
	uint8_t resp_data[TRBLE_ADV_RESP_DATA_MAX_LEN];
	uint8_t resp_data_length;
} trble_scanned_device;

typedef struct {
	trble_bd_addr addr;
	bool is_bonded;
	trble_conn_handle conn_handle;
} trble_device_connected;

typedef struct {
	trble_conn_handle conn_handle[TRBLE_MAX_CONNECTION_COUNT];
	uint8_t connected_count;
} trble_connected_list;

typedef struct {
	trble_conn_handle conn_handle;
	trble_attr_handle attr_handle;
} trble_operation_handle;

typedef struct {
	uint8_t raw_data[TRBLE_ADV_RAW_DATA_MAX_LEN];
	uint8_t raw_data_length;
	uint16_t scan_duration;
} trble_scan_filter;

typedef struct {
	/* This is a set of callback function for BLE client */
	void (*trble_scan_state_changed_cb)(trble_scan_state_e scan_state);
	void (*trble_device_scanned_cb)(trble_scanned_device *scanned_device);
	void (*trble_device_disconnected_cb)(trble_conn_handle conn_id);
	void (*trble_device_connected_cb)(trble_device_connected *connected_device);
	void (*trble_operation_notification_cb)(trble_operation_handle *handle, trble_data *read_result);
	uint16_t mtu;
} trble_client_init_config;

/*** Peripheral(Server) ***/
typedef enum {
	TRBLE_GATT_SERVICE,
	TRBLE_GATT_CHARACT,
	TRBLE_GATT_DESC,
	TRBLE_GATT_MAX
} trble_gatt_profile_type_e;

typedef enum {
	TRBLE_ATTR_CB_WRITING,
	TRBLE_ATTR_CB_READING,
	TRBLE_ATTR_CB_WRITING_NO_RSP,
} trble_attr_cb_type_e;

typedef void (*trble_server_cb_t)(trble_attr_cb_type_e type, trble_conn_handle con_handle, trble_attr_handle handle, void *arg);

typedef enum {
	TRBLE_ATTR_PROP_NONE = 0x00,
	TRBLE_ATTR_PROP_BCAST = 0x01,
	TRBLE_ATTR_PROP_READ = 0x02,
	TRBLE_ATTR_PROP_WRITE_NO_RSP = 0x04,
	TRBLE_ATTR_PROP_WRITE = 0x08,
	TRBLE_ATTR_PROP_NOTIFY = 0x10,
	TRBLE_ATTR_PROP_INDICATE = 0x20,
	TRBLE_ATTR_PROP_AUTHEN = 0x40,
	TRBLE_ATTR_PROP_EXTENDED = 0x80,
	TRBLE_ATTR_PROP_RWN = 0x1a,
} trble_attr_property_e;

typedef enum {
	TRBLE_ATTR_PERM_R_PERMIT = 0x00,  /* Always permitted, no restrictions*/
	TRBLE_ATTR_PERM_R_AUTHEN = 0x01,  /* Authentication required */
	TRBLE_ATTR_PERM_R_AUTHOR = 0x02,  /* Authorization required */
	TRBLE_ATTR_PERM_R_ENCRYPT = 0x04, /* Can only be accessed in encrypted link*/
	TRBLE_ATTR_PERM_R_BANNED = 0x08,  /* Operation not permitted */

	TRBLE_ATTR_PERM_W_PERMIT = 0x00,  /* Always permitted, no restrictions*/
	TRBLE_ATTR_PERM_W_AUTHEN = 0x10,  /* Authentication required */
	TRBLE_ATTR_PERM_W_AUTHOR = 0x20,  /* Authorization required */
	TRBLE_ATTR_PERM_W_ENCRYPT = 0x40, /* Can only be accessed in encrypted link*/
	TRBLE_ATTR_PERM_W_BANNED = 0x80,  /* Operation not permitted */
} trble_attr_permission_e;

typedef struct {
	trble_gatt_profile_type_e type;
	uint8_t uuid[16];
	uint16_t uuid_length;
	uint8_t property;
	uint8_t permission;
	trble_attr_handle attr_handle;
	trble_server_cb_t cb;
	void *arg;
} trble_gatt_t;

typedef enum {
	TRBLE_SERVER_LL_CONNECTED,
	TRBLE_SERVER_SM_CONNECTED,
	TRBLE_SERVER_DISCONNECTED,
} trble_server_connection_type_e;

typedef void (*trble_server_connected_t)(trble_conn_handle con_handle, trble_server_connection_type_e conn_type, uint8_t mac[TRBLE_BD_ADDR_MAX_LEN]);

typedef struct {
	trble_server_connected_t connected_cb;
	// true : Secure Manager is enabled. Bondable.
	// false : Secure Manager is disabled. Requesting Pairing will be rejected. Non-Bondable.
	bool is_secured_connect_allowed;
	trble_gatt_t *profile;
	uint16_t profile_count;
} trble_server_init_config;

typedef struct trble_bonded_device_list {
	uint8_t bd_addr[TRBLE_BD_ADDR_MAX_LEN];
	struct trble_bonded_device_list *next;
} trble_bonded_device_list_s;
/****************************************************************************
 * BLE Functions
 ****************************************************************************/

/*** Common ***/
typedef trble_result_e (*trble_init)(struct bledev *dev, trble_client_init_config *client, trble_server_init_config *server);
typedef trble_result_e (*trble_deinit)(struct bledev *dev);
typedef trble_result_e (*trble_get_mac_addr)(struct bledev *dev, uint8_t mac[TRBLE_BD_ADDR_MAX_LEN]);
// trble_disconnect can be used in both of server & client.
typedef trble_result_e (*trble_disconnect)(struct bledev *dev, trble_conn_handle con_handle, trble_mode_e mode);
typedef trble_result_e (*trble_delete_bond)(struct bledev *dev, trble_bd_addr *addr, trble_mode_e mode);
typedef trble_result_e (*trble_delete_bond_all)(struct bledev *dev, trble_mode_e mode);
typedef trble_result_e (*trble_conn_is_active)(struct bledev *dev, trble_conn_handle con_handle, bool *is_active);
typedef trble_result_e (*trble_conn_is_any_active)(struct bledev *dev, bool *is_active);

/*** Central(Client) ***/
typedef trble_result_e (*trble_start_scan)(struct bledev *dev, trble_scan_filter *filter);
typedef trble_result_e (*trble_stop_scan)(struct bledev *dev);
typedef trble_result_e (*trble_connect)(struct bledev *dev, trble_bd_addr *addr);
typedef trble_result_e (*trble_disconnect_all)(struct bledev *dev);
typedef trble_result_e (*trble_connected_device_list)(struct bledev *dev, trble_connected_list *out_connected_list);
typedef trble_result_e (*trble_connected_info)(struct bledev *dev, trble_conn_handle conn_handle, trble_device_connected *out_connected_device);
typedef trble_result_e (*trble_operation_enable_notification)(struct bledev *dev, trble_operation_handle *handle);
typedef trble_result_e (*trble_operation_read)(struct bledev *dev, trble_operation_handle *handle, trble_data *out_data);
typedef trble_result_e (*trble_operation_write)(struct bledev *dev, trble_operation_handle *handle, trble_data *in_data);
typedef trble_result_e (*trble_operation_write_no_response)(struct bledev *dev, trble_operation_handle *handle, trble_data *in_data);

/*** Peripheral(Server) ***/
typedef trble_result_e (*trble_get_profile_count)(struct bledev *dev, uint16_t *count);
// API for sending a characteristic value notification to the selected target(s). (notify to all clients conn_handle (notify all = 0x99))
typedef trble_result_e (*trble_charact_notify)(struct bledev *dev, trble_attr_handle attr_handle, trble_conn_handle con_handle, trble_data *data);
typedef trble_result_e (*trble_attr_set_data)(struct bledev *dev, trble_attr_handle attr_handle, trble_data *data);
typedef trble_result_e (*trble_attr_get_data)(struct bledev *dev, trble_attr_handle attr_handle, trble_data *data);
// reject attribute request in callback function and return error code
typedef trble_result_e (*trble_attr_reject)(struct bledev *dev, trble_attr_handle attr_handle, uint8_t app_errorcode);
typedef trble_result_e (*trble_get_mac_addr_by_conn_handle)(struct bledev *dev, trble_conn_handle con_handle, uint8_t bd_addr[TRBLE_BD_ADDR_MAX_LEN]);
typedef trble_result_e (*trble_get_conn_handle_by_addr)(struct bledev *dev, uint8_t bd_addr[TRBLE_BD_ADDR_MAX_LEN], trble_conn_handle *con_handle);
typedef trble_result_e (*trble_set_adv_data)(struct bledev *dev, trble_data *data);
typedef trble_result_e (*trble_set_adv_resp)(struct bledev *dev, trble_data *data);
typedef trble_result_e (*trble_get_bonded_device)(struct bledev *dev, trble_bonded_device_list_s *device_list, uint16_t *device_count);
typedef trble_result_e (*trble_start_adv)(struct bledev *dev);
typedef trble_result_e (*trble_start_adv_directed)(struct bledev *dev, uint8_t bd_addr[TRBLE_BD_ADDR_MAX_LEN]);
typedef trble_result_e (*trble_stop_adv)(struct bledev *dev);
typedef trble_result_e (*trble_set_adv_interval)(struct bledev *dev, uint16_t interval);

struct trble_ops {
	/* Common */
	trble_init init;
	trble_deinit deinit;
	trble_get_mac_addr get_mac;
	trble_disconnect disconnect;
	trble_delete_bond del_bond;
	trble_delete_bond_all del_bond_all;
	trble_conn_is_active conn_is_active;
	trble_conn_is_any_active conn_is_any_active;

	/* Central(Client) */
	trble_start_scan start_scan;
	trble_stop_scan stop_scan;
	trble_connect connect;
	trble_disconnect_all disconnect_all;
	trble_connected_device_list conn_dev_list;
	trble_connected_info conn_info;
	trble_operation_enable_notification op_enable_noti;
	trble_operation_read op_read;
	trble_operation_write op_write;
	trble_operation_write_no_response op_wrtie_no_resp;

	/* Peripheral(Server) */
	trble_get_profile_count get_profile_count;
	trble_charact_notify charact_noti;
	trble_attr_set_data attr_set_data;
	trble_attr_get_data attr_get_data;
	trble_attr_reject attr_reject;
	trble_get_mac_addr_by_conn_handle get_mac_by_conn;
	trble_get_conn_handle_by_addr get_conn_by_mac;
	trble_set_adv_data set_adv_data;
	trble_set_adv_resp set_adv_resp;
	trble_get_bonded_device get_bonded_dev;
	trble_start_adv start_adv;
	trble_start_adv_directed start_adv_dir;
	trble_stop_adv stop_adv;
	trble_set_adv_interval set_adv_interval;
};

int trble_post_event(struct bledev *dev, lwnl_cb_ble evt, void *buffer, uint32_t buf_len);
