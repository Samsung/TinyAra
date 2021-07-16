/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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

#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "logging/things_logger.h"
#include "utils/things_malloc.h"
#include "utils/things_string.h"
#include "utils/things_rtos_util.h"
#include "things_api.h"
#include "things_network.h"
#include "framework/things_data_manager.h"

#include "easy-setup/resource_handler.h"
#include "easy-setup/easysetup_manager.h"
#include "cloud/cloud_manager.h"
#include "things_ping.h"

#ifdef CONFIG_NETUTILS_NTPCLIENT
#include <protocols/ntpclient.h>
#endif

#define TAG "[things_network]"

#define MAX_WIFI_CONNECT_RETRY_COUNT 3
unsigned short g_retry_connect_cnt = 0;

typedef void *(*pthread_func_type)(void *);

static char *app_ap_name = NULL;
static char *app_ip_addr = NULL;
volatile static bool is_connected_target_ap = false;
extern bool b_reset_continue_flag;

static pthread_mutex_t g_ap_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static wifi_manager_ap_config_s g_homeap_info;
static access_point_info_s *g_wifi_scan_info;
static int g_wifi_count = 0;

static pthread_t h_thread_things_wifi_join = PTHREAD_ONCE_INIT;

#define IP_NULL_VAL     "0.0.0.0"

#ifdef CONFIG_NETUTILS_NTPCLIENT
static void ntp_link_error(void)
{
	THINGS_LOG_D(TAG, "ntp_link_error() callback is called");
}

static void sync_time_from_ntp(int guarantee_secs)
{
	THINGS_LOG_V(TAG, "T%d --> %s", getpid(), __FUNCTION__);

	if (ntpc_get_status() != NTP_RUNNING) {
		struct timespec pre_tp;
		clock_gettime(CLOCK_REALTIME, &pre_tp);

		struct timespec init_tp;
		init_tp.tv_sec = 0;
		init_tp.tv_nsec = 0;
		clock_settime(CLOCK_REALTIME, &init_tp);

		if (ntpc_start(NULL, 0, 0, ntp_link_error) < 0) {
			THINGS_LOG_E(TAG, "ntpc_start() failed");
		} else {
			THINGS_LOG_V(TAG, "ntpc_start() OK");
			int time_sync = 0;

			while (1) {
				struct timespec sync_tp;
				clock_gettime(CLOCK_REALTIME, &sync_tp);
				if ((init_tp.tv_sec + 1000) < sync_tp.tv_sec) {
					time_sync = 1;
					break;
				} else if ((init_tp.tv_sec + guarantee_secs) < sync_tp.tv_sec) {
					break;
				}
				usleep(100000);
			}
			if (time_sync) {
				THINGS_LOG_V(TAG, "ntpc_time sync done");
			} else {
				THINGS_LOG_E(TAG, "ntpc_time sync fail");
				clock_settime(CLOCK_REALTIME, &pre_tp);
			}
		}
	} else {
		THINGS_LOG_V(TAG, "ntpc already running");
	}
}
#endif

wifi_manager_ap_config_s *things_network_get_homeap_config(void)
{
	wifi_manager_result_e res = wifi_manager_get_config(&g_homeap_info);

	if (res != WIFI_MANAGER_SUCCESS) {
		THINGS_LOG_E(TAG, "Get AP configuration failed [error code : %d]", res);
	} else {
		THINGS_LOG_D(TAG, "[WIFI] Saved SSID : %s", g_homeap_info.ssid);
	}
	return &g_homeap_info;
}

bool things_network_connect_home_ap(void)
{
	wifi_manager_result_e result = WIFI_MANAGER_SUCCESS;
	wifi_manager_ap_config_s *connect_config = things_network_get_homeap_config();

	if (connect_config == NULL || connect_config->ssid == NULL) {
		return false;
	}

	THINGS_LOG_V(TAG, "Try_connect_home_ap [ssid : %s]", connect_config->ssid);

	result = wifi_manager_connect_ap(connect_config);
	if (result != WIFI_MANAGER_SUCCESS) {
		THINGS_LOG_E(TAG, "Failed to connect WiFi [Error Code : %d]", result);
		return false;
	}

	return true;
}

bool things_network_turn_on_soft_ap(void)
{
	wifi_manager_info_s info;
	wifi_manager_get_info(&info);
	wifi_manager_remove_config();
	if (info.mode != SOFTAP_MODE) {
		wifi_manager_softap_config_s *ap_config = dm_get_softap_wifi_config();
		if (wifi_manager_set_mode(SOFTAP_MODE, ap_config) != WIFI_MANAGER_SUCCESS) {
			THINGS_LOG_E(TAG, "Failed to change to SOFTAP mode");
			return false;
		}
	}
	pthread_mutex_init(&g_ap_list_mutex, NULL);
	THINGS_LOG_V(TAG, "In SOFTAP mode");
	return true;
}

bool things_handle_stop_soft_ap(wifi_manager_ap_config_s *connect_config)
{
	wifi_manager_result_e result = WIFI_MANAGER_SUCCESS;
	wifi_manager_info_s info;
	wifi_manager_get_info(&info);

	if (info.mode == SOFTAP_MODE) {
		if (wifi_manager_set_mode(STA_MODE, NULL) != WIFI_MANAGER_SUCCESS) {
			THINGS_LOG_E(TAG, "Failed to change to STA mode)");
			return false;
		}
		usleep(500000);
	}
	g_retry_connect_cnt = 0;
	result = wifi_manager_connect_ap(connect_config);
	if (result != WIFI_MANAGER_SUCCESS) {
		THINGS_LOG_E(TAG, "Failed to connect WiFi");
		return false;
	}
	return true;
}

int things_set_ap_connection(access_point_info_s *p_info)
{
	THINGS_LOG_D(TAG, "T%d --> %s", getpid(), __FUNCTION__);

	if (p_info == NULL) {
		THINGS_LOG_E(TAG, "Invalid params");
		return -1;
	}

	wifi_manager_ap_config_s connect_config;

	strncpy(connect_config.ssid, p_info->e_ssid, sizeof(connect_config.ssid));
	connect_config.ssid_length = strlen(connect_config.ssid);
	strncpy(connect_config.passphrase, p_info->security_key, sizeof(connect_config.passphrase));

	connect_config.passphrase_length = strlen(connect_config.passphrase);

	THINGS_LOG_V(TAG, "[%s] ssid : %s", __FUNCTION__, connect_config.ssid);

	// set sec type
	if (strncmp(p_info->sec_type, SEC_TYPE_WEP, strlen(SEC_TYPE_WEP)) == 0) {
		connect_config.ap_auth_type = WIFI_MANAGER_AUTH_WEP_SHARED;
	} else if (strncmp(p_info->sec_type, SEC_TYPE_WPA_PSK, strlen(SEC_TYPE_WPA_PSK)) == 0) {
		connect_config.ap_auth_type = WIFI_MANAGER_AUTH_WPA_PSK;
	} else if (strncmp(p_info->sec_type, SEC_TYPE_WPA2_PSK, strlen(SEC_TYPE_WPA2_PSK)) == 0) {
		connect_config.ap_auth_type = WIFI_MANAGER_AUTH_WPA2_PSK;
	}
	// set encryption crypto type
	if (strncmp(p_info->enc_type, ENC_TYPE_WEP_64, strlen(ENC_TYPE_WEP_64)) == 0) {
		connect_config.ap_crypto_type = WIFI_MANAGER_CRYPTO_WEP_64;
	} else if (strncmp(p_info->enc_type, ENC_TYPE_WEP_128, strlen(ENC_TYPE_WEP_128)) == 0) {
		connect_config.ap_crypto_type = WIFI_MANAGER_CRYPTO_WEP_128;
	} else if (strncmp(p_info->enc_type, ENC_TYPE_TKIP, strlen(ENC_TYPE_TKIP)) == 0) {
		connect_config.ap_crypto_type = WIFI_MANAGER_CRYPTO_TKIP;
	} else if (strncmp(p_info->enc_type, ENC_TYPE_AES, strlen(ENC_TYPE_AES)) == 0) {
		connect_config.ap_crypto_type = WIFI_MANAGER_CRYPTO_AES;
	} else if (strncmp(p_info->enc_type, ENC_TYPE_TKIP_AES, strlen(ENC_TYPE_TKIP_AES)) == 0) {
		connect_config.ap_crypto_type = WIFI_MANAGER_CRYPTO_TKIP_AND_AES;
	}

	wifi_manager_result_e res = wifi_manager_save_config(&connect_config);

	if (res != WIFI_MANAGER_SUCCESS) {
		THINGS_LOG_E(TAG, "Failed to save AP configuration : [%d]", res);
		return -1;
	} else {
		THINGS_LOG_V(TAG, "Success to save AP configuration");
	}

	if (!things_handle_stop_soft_ap(&connect_config)) {
		return -1;
	}

	return 1;
}

int things_get_ap_list(access_point_info_s** p_info, int* p_count)
{
	if (p_info == NULL || p_count == NULL) {
		THINGS_LOG_E(TAG, "Can't Call GetAPSearchList(p_info=0x%X, p_count=0x%X).", p_info, p_count);
		return 0;
	}
	pthread_mutex_lock(&g_ap_list_mutex);
	*p_count = g_wifi_count;
	access_point_info_s *wifi_scan_iter = g_wifi_scan_info;
	access_point_info_s *pinfo = NULL;
	access_point_info_s *p_last_info = NULL;
	while (wifi_scan_iter != NULL) {
		if (wifi_scan_iter->e_ssid[0] != NULL) {
			pinfo = (access_point_info_s*)things_malloc(sizeof(access_point_info_s));
			if (pinfo != NULL) {
				pinfo->next = NULL;
				snprintf(pinfo->e_ssid, WIFIMGR_SSID_LEN, "%s", wifi_scan_iter->e_ssid);
				snprintf(pinfo->bss_id, WIFIMGR_MACADDR_STR_LEN, "%s", wifi_scan_iter->bss_id);
				snprintf(pinfo->signal_level, MAX_LEVEL_SIGNAL, "%d", wifi_scan_iter->signal_level);
				snprintf(pinfo->sec_type, MAX_TYPE_SEC, "%s", wifi_scan_iter->sec_type);
				snprintf(pinfo->enc_type, MAX_TYPE_ENC, "%s", wifi_scan_iter->enc_type);
				if (*p_info == NULL) {
					*p_info = pinfo;
				} else {
					if (p_last_info != NULL) {
						p_last_info->next = pinfo;
					}
				}
				p_last_info = pinfo;
			}
		}
		wifi_scan_iter = wifi_scan_iter->next;
	}
	pthread_mutex_unlock(&g_ap_list_mutex);
	return 1;
}

int things_wifi_changed_call_func(int state, char *ap_name, char *ip_addr)
{
	return things_wifi_state_changed_cb(state, ap_name, ip_addr);;
}

static void *__attribute__((optimize("O0"))) t_things_wifi_join_loop(void *args)
{
	bool* is_wifi_retry_connect = (bool*)args;

	if ((*is_wifi_retry_connect) == true) {
		THINGS_LOG_E(TAG, "Retry to connect to home ap.");

		if (things_network_connect_home_ap() == false) {
			THINGS_LOG_E(TAG, "Failed to connect home ap.");
		}
	} else {
#ifdef CONFIG_NETUTILS_NTPCLIENT
		sync_time_from_ntp(10);
#else
		THINGS_LOG_V(TAG, "CONFIG_NETUTILS_NTPCLIENT is not set");
#endif
		wifi_manager_info_s wifi_info;
		wifi_manager_get_info(&wifi_info);

		things_wifi_changed_call_func(1, wifi_info.ssid, wifi_info.ip4_address);
	}

	return NULL;
}

void things_wifi_sta_connected(wifi_manager_result_e res)
{
	static bool is_wifi_retry_connect;
	is_wifi_retry_connect = false;
	if (res == WIFI_MANAGER_FAIL) {
		THINGS_LOG_E(TAG, "Failed to connect to the AP");

		if (!dm_is_es_complete()) {
			g_retry_connect_cnt++;

			if (g_retry_connect_cnt <= MAX_WIFI_CONNECT_RETRY_COUNT) {
				is_wifi_retry_connect = true;
				THINGS_LOG_E(TAG, "(%d) Retry to connect ap during easy-setup.", g_retry_connect_cnt);
			} else {
				return;
			}
		} else {
			is_wifi_retry_connect = true;
			THINGS_LOG_V(TAG, "Retry to connect ap during normal time.");
		}
	} else {
		is_wifi_retry_connect = false;
		THINGS_LOG_V(TAG, "Stop retry to connect ap.");
	}

	THINGS_LOG_V(TAG, "T%d --> %s", getpid(), __FUNCTION__);

	if (things_is_things_module_initialized() == 1) {
		pthread_create_rtos(&h_thread_things_wifi_join, NULL, (pthread_func_type)t_things_wifi_join_loop, (void *)&is_wifi_retry_connect, THINGS_STACK_WIFI_JOIN_THREAD);
	} else {
		THINGS_LOG_E(TAG, "things module was not initialized.");
	}
}

void things_wifi_sta_disconnected(wifi_manager_disconnect_e disconn)
{
	THINGS_LOG_V(TAG, "T%d --> %s", getpid(), __FUNCTION__);
	things_wifi_changed_call_func(0, NULL, NULL);
}

void things_wifi_soft_ap_sta_joined(void)
{
	THINGS_LOG_V(TAG, "T%d --> %s", getpid(), __FUNCTION__);

}

void things_wifi_soft_ap_sta_left(void)
{
	THINGS_LOG_V(TAG, "T%d --> %s", getpid(), __FUNCTION__);
}

void things_wifi_list_cleanup(void)
{
	if (g_wifi_scan_info != NULL) {
		// free old wifi list
		access_point_info_s* piter;
		access_point_info_s* pdel;
		piter = pdel = g_wifi_scan_info;
		while (piter != NULL) {
			pdel = piter;
			piter = piter->next;
			things_free(pdel);
		}
		g_wifi_scan_info = NULL;
	}
	g_wifi_count = 0;
}

void things_wifi_scan_done(wifi_manager_scan_info_s **scan_result, wifi_manager_scan_result_e res)
{
	THINGS_LOG_V(TAG, "T%d --> %s", getpid(), __FUNCTION__);
	/* Make sure you copy the scan results onto a local data structure.
	 * It will be deleted soon eventually as you exit this function.
	 */
	THINGS_LOG_V(TAG, "WiFi Scan done.");
	if (scan_result == NULL) {
		THINGS_LOG_E(TAG, "\tWiFi scan result is NULL.");
		return;
	}
	pthread_mutex_lock(&g_ap_list_mutex);
	things_wifi_list_cleanup();
	wifi_manager_scan_info_s *wifi_scan_iter = *scan_result;
	access_point_info_s *pinfo = NULL;
	access_point_info_s *p_last_info = NULL;
	while (wifi_scan_iter != NULL) {
		if ((wifi_scan_iter->ssid[0] != NULL) && (strlen(wifi_scan_iter->ssid)) != 0) {
			pinfo = (access_point_info_s*)things_malloc(sizeof(access_point_info_s));
			if (pinfo == NULL) {
				break;
			}
			pinfo->next = NULL;
			snprintf(pinfo->e_ssid, WIFIMGR_SSID_LEN, "%s", wifi_scan_iter->ssid);
			snprintf(pinfo->bss_id, WIFIMGR_MACADDR_STR_LEN, "%s", wifi_scan_iter->bssid);
			snprintf(pinfo->signal_level, MAX_LEVEL_SIGNAL, "%d", wifi_scan_iter->rssi);
			// Set auth type
			const char *sec_type;
			if (wifi_scan_iter->ap_auth_type == WIFI_MANAGER_AUTH_WEP_SHARED) {
				sec_type = SEC_TYPE_WEP;
			} else if (wifi_scan_iter->ap_auth_type == WIFI_MANAGER_AUTH_WPA_PSK) {
				sec_type = SEC_TYPE_WPA_PSK;
			} else if (wifi_scan_iter->ap_auth_type == WIFI_MANAGER_AUTH_WPA2_PSK) {
				sec_type = SEC_TYPE_WPA2_PSK;
			} else {
				sec_type = SEC_TYPE_NONE;
			}
			snprintf(pinfo->sec_type, MAX_TYPE_SEC, "%s", sec_type);

			// Set encryption type
			const char *enc_type;
			if (wifi_scan_iter->ap_crypto_type == WIFI_MANAGER_CRYPTO_WEP_64) {
				enc_type = ENC_TYPE_WEP_64;
			} else if (wifi_scan_iter->ap_crypto_type == WIFI_MANAGER_CRYPTO_WEP_128) {
				enc_type = ENC_TYPE_WEP_128;
			} else if (wifi_scan_iter->ap_crypto_type == WIFI_MANAGER_CRYPTO_TKIP) {
				enc_type = ENC_TYPE_TKIP;
			} else if (wifi_scan_iter->ap_crypto_type == WIFI_MANAGER_CRYPTO_AES) {
				enc_type = ENC_TYPE_AES;
			} else if (wifi_scan_iter->ap_crypto_type == WIFI_MANAGER_CRYPTO_TKIP_AND_AES) {
				enc_type = ENC_TYPE_TKIP_AES;
			} else {
				enc_type = ENC_TYPE_NONE;
			}
			snprintf(pinfo->enc_type, MAX_TYPE_ENC, "%s", enc_type);
			if (g_wifi_scan_info == NULL) {
				g_wifi_scan_info = pinfo;
			} else {
				p_last_info->next = pinfo;
			}
			p_last_info = pinfo;
			g_wifi_count++;
		}
		wifi_scan_iter = wifi_scan_iter->next;
	}
	THINGS_LOG_V(TAG, "\tWiFi list count is %d.", g_wifi_count);
	pthread_mutex_unlock(&g_ap_list_mutex);
	return;
}

int things_wifi_scan_ap(void)
{
	if (wifi_manager_scan_ap(NULL) != WIFI_MANAGER_SUCCESS) {
		THINGS_LOG_E(TAG, "Wifi manager scan ap failed");
		return 0;
	}
	return 1;
}

static const wifi_manager_cb_s wifi_callbacks = {
	things_wifi_sta_connected,
	things_wifi_sta_disconnected,
	things_wifi_soft_ap_sta_joined,
	things_wifi_soft_ap_sta_left,
	things_wifi_scan_done,
};

int things_network_initialize(void)
{
	if (wifi_manager_init((wifi_manager_cb_s *) &wifi_callbacks) != WIFI_MANAGER_SUCCESS) {
		THINGS_LOG_E(TAG, "Failed to initialize WiFi manager");
		return 0;
	}
	return 1;
}


/*****************************************************************************
 *
 * It's called from st_things Application when WiFi state changed.
 *
 * state : 1 -> Connected to WiFi AP.
 *         0 -> Disconnected from WiFi AP.
 * ap_name : AP SSID.
 * ip_addr : IP address.
 *
 *****************************************************************************/
int things_wifi_state_changed_cb(int state, char *ap_name, char *ip_addr)
{
	bool Retry1Stop0 = false;

	THINGS_LOG_D(TAG, "state=%d, ap_name=%s, ip_name=%s", state, ap_name, ip_addr);

	// Process.
	if (state == 1) {			// [Info] Always, when Soft-AP enabling, Call-Back State maintain disconnected. So, Soft-AP Check is not need.
		THINGS_LOG_D(TAG, "AP CONNECTED.");
		is_connected_target_ap = true;

		set_ssid_in_wifi_resource(app_ap_name);
		Retry1Stop0 = true;		// Re-Try with Last failed Access info.
		esm_wifi_prov_check_cb(state, app_ap_name, app_ip_addr);
	} else {
		THINGS_LOG_D(TAG, "AP DISCONNECTED.");
		is_connected_target_ap = false;
		set_ssid_in_wifi_resource(NULL);
	}

	// Call Retry or forceStop Connection
	ci_retry_stop_by_wifi_cb(Retry1Stop0);

	return 1;
}

/*****************************************************************************
 *
 * It's called from Iotivity Framework when Adaptor state changed.
 *
 * adapter : Adapter Type.
 * enabled  : true -> connected.
 *            false-> disconnected.
 *
 *****************************************************************************/
void things_adapter_state_cb(CATransportAdapter_t adapter, bool enabled)
{
	(void)adapter;

	THINGS_LOG_D(TAG, "IN");

	if (enabled) {
		THINGS_LOG_D(TAG, "CONNECTED but Not Guarantee to be Communication-Available");
	} else {
		THINGS_LOG_D(TAG, "AP DISCONNECTED.");
		is_connected_target_ap = false;
		set_ssid_in_wifi_resource(NULL);
		ci_retry_stop_by_wifi_cb(false);
	}

	THINGS_LOG_D(TAG, "OUT");
}

/*****************************************************************************
 *
 * It's called from Iotivity Framework when TCP session state changed.
 *
 * info : information about remote-peer.
 * connected  : true -> connected
 *              false-> disconnected
 *
 *****************************************************************************/
void things_tcp_session_state_cb(const CAEndpoint_t *info, bool connected)
{
	THINGS_LOG_D(TAG, "Enter.");

	if (b_reset_continue_flag == true) {
		return;
	}

	if (info == NULL) {
		THINGS_LOG_E(TAG, "[IoTivity Error] invalid call-back parameter.(info is null.)");
		return;
	}

	if (info->adapter != CA_ADAPTER_TCP) {
		THINGS_LOG_D(TAG, "Don't support adapter type.(%d)", info->adapter);
		return;
	}

	if (strlen(info->addr) == 0 || info->port == 0) {
		THINGS_LOG_E(TAG, "[IoTivity Error] invalid call-back parameter.(addrIP=%s, port=%d)", info->addr, info->port);
		return;
	}

	THINGS_LOG_D(TAG, "IP = %s, port = %d", info->addr, info->port);

	if (connected == true) {
		THINGS_LOG_D(TAG, "CONNECTED");
		things_ping_set_mask(info->addr, info->port, PING_ST_TCPCONNECT);
		return;
	}

	THINGS_LOG_D(TAG, "DISCONNECTED");
	things_ping_unset_mask(info->addr, PING_ST_SIGNIN | PING_ST_TCPCONNECT);

	if (ci_retry_stop_by_tcp_cb(info->addr, info->port) == -1) {
		THINGS_LOG_E(TAG, "[Error] System Error.");
		return;
	}

	THINGS_LOG_D(TAG, "Exit.");
}


/*****************************************************************************
 *
 * Ready Check All of Network APIs.
 *
 * return : 0 --> Not Ready.
 *          1 --> Initialize Success.
 *         -1 --> Initialize Failed.
 *
 *****************************************************************************/

bool things_is_connected_ap(void)
{
	return is_connected_target_ap;
}
