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
#include <tinyara/config.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <wifi_manager/wifi_manager.h>
#include <tinyara/net/netlog.h>
#include <tinyara/wifi/wifi_common.h>
#include "wifi_manager_utils.h"
#include "wifi_manager_profile.h"
#include "wifi_manager_dhcp.h"
#include "wifi_manager_stats.h"
#include "wifi_manager_event.h"
#include "wifi_manager_msghandler.h"
#include "wifi_manager_error.h"
#include "wifi_manager_cb.h"
#include "wifi_manager_state.h"
#include "wifi_manager_info.h"

/*  Setting MACRO */
static inline void WIFIMGR_SET_SSID(char *s)
{
	wifimgr_info_msg_s winfo;
	winfo.ssid = s;
	wifimgr_set_info(WIFIMGR_SSID, &winfo);
}

static inline void WIFIMGR_SET_SOFTAP_SSID(char *s)
{
	wifimgr_info_msg_s winfo;
	winfo.softap_ssid = s;
	wifimgr_set_info(WIFIMGR_SOFTAP_SSID, &winfo);
}

/*  Copy MACRO */
#define WIFIMGR_COPY_SOFTAP_CONFIG(dest, src)							\
	do {																\
		(dest).channel = (src)->channel;								\
		strncpy((dest).ssid, (src)->ssid, WIFIMGR_SSID_LEN);			\
		(dest).ssid[WIFIMGR_SSID_LEN] = '\0';							\
		strncpy((dest).passphrase, (src)->passphrase, WIFIMGR_PASSPHRASE_LEN); \
		(dest).passphrase[WIFIMGR_PASSPHRASE_LEN] = '\0';				\
	} while (0)

#define WIFIMGR_COPY_AP_INFO(dest, src)									\
	do {																\
		(dest).ssid_length = (src).ssid_length;							\
		(dest).passphrase_length = (src).passphrase_length;				\
		strncpy((dest).ssid, (src).ssid, WIFIMGR_SSID_LEN);				\
		(dest).ssid[WIFIMGR_SSID_LEN] = '\0';							\
		strncpy((dest).passphrase, (src).passphrase, WIFIMGR_PASSPHRASE_LEN); \
		(dest).passphrase[WIFIMGR_PASSPHRASE_LEN] = '\0';				\
		(dest).ap_auth_type = (src).ap_auth_type;						\
		(dest).ap_crypto_type = (src).ap_crypto_type;					\
	} while (0)

/*  Initialize MACRO */
#define WM_APINFO_INITIALIZER {{0,}, 0, {0,}, 0, WIFI_MANAGER_AUTH_UNKNOWN, WIFI_MANAGER_CRYPTO_UNKNOWN}
#define WIFIMGR_SOTFAP_CONFIG {{0,}, {0,}, 1}

struct _wifimgr_state_handle {
	wifimgr_state_e state;
	wifimgr_state_e prev_state; // it is for returning to previous sta state after scanning is done
	// event
	wifimgr_evt_handler_s cbk;
	// substate
	_wifimgr_disconn_substate_e disconn_substate;
	sem_t *api_sig;
	wifi_manager_softap_config_s softap_config;
};
typedef struct _wifimgr_state_handle _wifimgr_state_handle_s;

/* global variables*/
static _wifimgr_state_handle_s g_manager_info = {
	WIFIMGR_UNINITIALIZED, WIFIMGR_UNINITIALIZED, // state, prev_state
	// event
	WIFIMGR_EVENT_INITIALIZER,
	// substate
	WIFIMGR_DISCONN_NONE, // _wifimgr_disconn_substate_e
	NULL,
	WIFIMGR_SOTFAP_CONFIG};

/* Internal functions*/
static wifi_manager_result_e _wifimgr_deinit(void);
static wifi_manager_result_e _wifimgr_run_sta(void);
static wifi_manager_result_e _wifimgr_connect_ap(wifi_manager_ap_config_s *config);
static wifi_manager_result_e _wifimgr_save_connected_config(wifi_manager_ap_config_s *config);
static wifi_manager_result_e _wifimgr_disconnect_ap(void);
static wifi_manager_result_e _wifimgr_run_softap(wifi_manager_softap_config_s *config);
static wifi_manager_result_e _wifimgr_stop_softap(void);
static wifi_manager_result_e _wifimgr_scan(wifi_manager_scan_config_s *config);

/* functions managing a state machine*/
#undef WIFIMGR_STATE_TABLE
#define WIFIMGR_STATE_TABLE(state, handler, str)				\
	static wifi_manager_result_e handler(wifimgr_msg_s *msg);
#include "wifi_manager_state_table.h"
static wifi_manager_result_e _handle_request(wifimgr_msg_s *msg);
typedef wifi_manager_result_e (*wifimgr_handler)(wifimgr_msg_s *msg);

/* g_handler should be matched to _wifimgr_state*/
static const wifimgr_handler g_handler[] = {
#undef WIFIMGR_STATE_TABLE
#define WIFIMGR_STATE_TABLE(state, handler, str) handler,
#include "wifi_manager_state_table.h"
};

static char *wifimgr_state_str[] = {
#undef WIFIMGR_STATE_TABLE
#define WIFIMGR_STATE_TABLE(state, handler, str) str,
#include "wifi_manager_state_table.h"
	"WIFIMGR_NONE",
	"WIFIMGR_STATE_MAX",
};

/*  State MACRO */
#define WIFIMGR_CHECK_STATE(s) ((s) != g_manager_info.state)
#define WIFIMGR_IS_STATE(s) ((s) == g_manager_info.state)
#define WIFIMGR_GET_STATE g_manager_info.state
#define WIFIMGR_GET_PREVSTATE g_manager_info.prev_state
#define WIFIMGR_STORE_PREV_STATE (g_manager_info.prev_state = g_manager_info.state)
#define WIFIMGR_RESTORE_STATE								\
	do {													\
		g_manager_info.state = g_manager_info.prev_state;	\
		g_manager_info.prev_state = WIFIMGR_NONE;			\
	} while (0)

#define TAG "[WM]"

static inline void WIFIMGR_SET_STATE(wifimgr_state_e s)
{
	g_manager_info.state = s;
	wifimgr_info_msg_s wmsg;
	wmsg.state = s;
	wifimgr_set_info(WIFIMGR_STATE, &wmsg);
}

static inline char *wifimgr_get_state_str(int state)
{
	return wifimgr_state_str[state];
}

/*  Substate MACRO */
static inline void WIFIMGR_SET_SUBSTATE(_wifimgr_disconn_substate_e state, sem_t *signal)
{
	g_manager_info.disconn_substate = state;
	g_manager_info.api_sig = signal;
}

static inline void WIFIMGR_RESET_SUBSTATE(void)
{
	g_manager_info.disconn_substate = WIFIMGR_DISCONN_NONE;
	g_manager_info.api_sig = NULL;
}

static inline void WIFIMGR_SEND_API_SIGNAL(sem_t *api_sig)
{
	/*	send signal to wifi_manager_api to notify request is done*/
	if (api_sig) {
		sem_post(api_sig);
	}
}

static void _free_scan_list(wifi_utils_scan_list_s *scan_list)
{
	wifi_utils_scan_list_s *iter = scan_list, *prev = NULL;
	while (iter) {
		prev = iter;
		iter = iter->next;
		free(prev);
	}
}

wifi_manager_result_e _wifimgr_deinit(void)
{
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_deinit(), TAG, "wifi_utils_deinit fail", WIFI_MANAGER_FAIL);
	wifimgr_unregister_all();

	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _wifimgr_run_sta(void)
{
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_start_sta(), TAG, "Starting STA failed.", WIFI_MANAGER_FAIL);
#ifdef CONFIG_DISABLE_EXTERNAL_AUTOCONNECT
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_set_autoconnect(0), TAG, "Set Autoconnect failed", WIFI_MANAGER_FAIL);
#else
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_set_autoconnect(1), TAG, "Set Autoconnect failed", WIFI_MANAGER_FAIL);
#endif
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _wifimgr_save_connected_config(wifi_manager_ap_config_s *config)
{
#ifdef CONFIG_WIFI_MANAGER_SAVE_CONFIG
	wifi_utils_result_e ret = wifi_profile_write(config, 1);
	if (ret != WIFI_UTILS_SUCCESS) {
		NET_LOGE(TAG, "Failed to save the connected AP configuration in file system\n");
		return WIFI_MANAGER_FAIL;
	}
#endif
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _wifimgr_connect_ap(wifi_manager_ap_config_s *config)
{
	wifi_utils_ap_config_s util_config;
	strncpy(util_config.ssid, config->ssid, WIFIMGR_SSID_LEN);
	util_config.ssid[WIFIMGR_SSID_LEN] = '\0';
	util_config.ssid_length = config->ssid_length;
	strncpy(util_config.passphrase, config->passphrase, WIFIMGR_PASSPHRASE_LEN);
	util_config.passphrase[WIFIMGR_PASSPHRASE_LEN] = '\0';
	util_config.passphrase_length = config->passphrase_length;
	util_config.ap_auth_type = config->ap_auth_type;
	util_config.ap_crypto_type = config->ap_crypto_type;

	wifi_utils_result_e wres = wifi_utils_connect_ap(&util_config, NULL);
	if (wres == WIFI_UTILS_ALREADY_CONNECTED) {
		return WIFI_MANAGER_ALREADY_CONNECTED;
	} else if (wres != WIFI_UTILS_SUCCESS) {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_CONNECT_FAIL);
		return WIFI_MANAGER_FAIL;
	}
	WIFIMGR_SET_SSID(config->ssid);
	wifi_manager_result_e wret = _wifimgr_save_connected_config(config);
	if (wret != WIFI_MANAGER_SUCCESS) {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INTERNAL_FAIL);
		return wret;
	}

	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _wifimgr_disconnect_ap(void)
{
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_disconnect_ap(NULL), TAG, "disconnect to ap fail", WIFI_MANAGER_FAIL);
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _wifimgr_run_softap(wifi_manager_softap_config_s *config)
{
	if (strlen(config->ssid) > WIFIMGR_SSID_LEN || strlen(config->passphrase) > WIFIMGR_PASSPHRASE_LEN) {
		NET_LOGE(TAG, "SSID or PASSPHRASE length invalid\n");
		return WIFI_MANAGER_FAIL;
	}
	wifi_utils_softap_config_s softap_config;

	softap_config.channel = config->channel;
	softap_config.ap_crypto_type = WIFI_UTILS_CRYPTO_AES;
	softap_config.ap_auth_type = WIFI_UTILS_AUTH_WPA2_PSK;
	softap_config.ssid_length = strlen(config->ssid);
	softap_config.passphrase_length = strlen(config->passphrase);
	strncpy(softap_config.ssid, config->ssid, WIFIMGR_SSID_LEN);
	softap_config.ssid[WIFIMGR_SSID_LEN] = '\0';
	strncpy(softap_config.passphrase, config->passphrase, WIFIMGR_PASSPHRASE_LEN);
	softap_config.passphrase[WIFIMGR_PASSPHRASE_LEN] = '\0';

	WIFIMGR_CHECK_UTILRESULT(wifi_utils_start_softap(&softap_config),
							 TAG, "Starting softap mode failed", WIFI_MANAGER_FAIL);
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
	WIFIMGR_CHECK_RESULT(wm_dhcps_start((dhcp_sta_joined_cb)g_manager_info.cbk.dhcps_sta_joined),
						 (TAG, "Starting DHCP server failed\n"), WIFI_MANAGER_FAIL);
#endif
	/* update wifi_manager_info */
	WIFIMGR_SET_SOFTAP_SSID(config->ssid);
	dhcps_reset_num();

	/* For tracking softap stats, the LAST value is used */
	WIFIMGR_STATS_INC(CB_SOFTAP_DONE);
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _wifimgr_stop_softap(void)
{
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_stop_softap(), TAG, "Stoping softap failed", WIFI_MANAGER_FAIL);
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
	WIFIMGR_CHECK_RESULT(wm_dhcps_stop(), (TAG, "Stoping softap DHCP server failed.\n"), WIFI_MANAGER_FAIL);
#endif
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _wifimgr_scan(wifi_manager_scan_config_s *config)
{
	trwifi_scan_config_s uconf = {0, {0,}, 0};
	if (!config) {
		WIFIMGR_CHECK_UTILRESULT(wifi_utils_scan_ap(NULL), TAG,
								 "request full scan is fail", WIFI_MANAGER_FAIL);
		return WIFI_MANAGER_SUCCESS;
	}
	if (config->ssid_length > 0 && config->channel != 0) {
		/*  both channel and ssid are set, it's invalid usage */
		return WIFI_MANAGER_INVALID_ARGS;
	}
	if (config->ssid_length > WIFIMGR_SSID_LEN) {
		return WIFI_MANAGER_INVALID_ARGS;
	}
	if (config->ssid_length > 0) {
		strncpy(uconf.ssid, config->ssid, config->ssid_length + 1);
		uconf.ssid_length = config->ssid_length;
	} else if (config->channel != 0) {
		uconf.channel = config->channel;
	}

	WIFIMGR_CHECK_UTILRESULT(wifi_utils_scan_ap((void *)&uconf), TAG,
							 "request scan is fail", WIFI_MANAGER_FAIL);
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _handler_on_uninitialized_state(wifimgr_msg_s *msg)
{
	wifimgr_evt_e evt = msg->event;
	if (evt != WIFIMGR_CMD_INIT) {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
		return WIFI_MANAGER_FAIL;
	}

	WIFIMGR_CHECK_UTILRESULT(wifi_utils_init(), TAG, "wifi_utils_init fail", WIFI_MANAGER_FAIL);

#ifdef CONFIG_WIFI_MANAGER_SAVE_CONFIG
	WIFIMGR_CHECK_UTILRESULT(wifi_profile_init(), TAG, "wifi_profile init fail", WIFI_MANAGER_FAIL);
#endif
	/*  get event function pointers from event handler */
	int res = wifimgr_get_evthandler(&g_manager_info.cbk);
	if (res < 0) {
		NET_LOGE(TAG, "WIFIMGR GET EVENT\n");
	}
	wifi_utils_register_callback(&g_manager_info.cbk.wifi_evt);

	/*  register default callback to callback handler */
	wifi_manager_cb_s *cb = (wifi_manager_cb_s *)msg->param;
	res = wifimgr_register_cb(cb);
	if (res < 0) {
		NET_LOGE(TAG, "WIFIMGR REGISTER CB\n");
	}
#ifdef CONFIG_DISABLE_EXTERNAL_AUTOCONNECT
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_set_autoconnect(0), TAG, "Set Autoconnect failed", WIFI_MANAGER_FAIL);
#else
	WIFIMGR_CHECK_UTILRESULT(wifi_utils_set_autoconnect(1), TAG, "Set Autoconnect failed", WIFI_MANAGER_FAIL);
#endif

	WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTED);

	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _handler_on_disconnected_state(wifimgr_msg_s *msg)
{
	if (msg->event == WIFIMGR_CMD_CONNECT) {
		wifi_manager_ap_config_s *apinfo = (wifi_manager_ap_config_s *)msg->param;
		WIFIMGR_CHECK_RESULT(_wifimgr_connect_ap(apinfo), (TAG, "connect ap fail\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SET_STATE(WIFIMGR_STA_CONNECTING);
	} else if (msg->event == WIFIMGR_CMD_DEINIT) {
		WIFIMGR_CHECK_RESULT(_wifimgr_deinit(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SEND_API_SIGNAL(msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_UNINITIALIZED);
	} else if (msg->event == WIFIMGR_CMD_SET_SOFTAP) {
		WIFIMGR_CHECK_RESULT(_wifimgr_run_softap((wifi_manager_softap_config_s *)msg->param),
							 (TAG, "run_softap fail\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SEND_API_SIGNAL(msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_SOFTAP);
	} else if (msg->event == WIFIMGR_CMD_SCAN) {
		WIFIMGR_CHECK_RESULT(_wifimgr_scan((wifi_manager_scan_config_s *)msg->param), (TAG, "fail scan\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_STORE_PREV_STATE;
		WIFIMGR_SET_STATE(WIFIMGR_SCANNING);
	} else {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
		return WIFI_MANAGER_FAIL;
	}
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _handler_on_disconnecting_state(wifimgr_msg_s *msg)
{
	if (msg->event == WIFIMGR_CMD_DEINIT) {
		WIFIMGR_SET_SUBSTATE(WIFIMGR_DISCONN_DEINIT, msg->signal);
		return WIFI_MANAGER_SUCCESS;
	}

	if (msg->event != WIFIMGR_EVT_STA_DISCONNECTED
		&& msg->event != WIFIMGR_EVT_STA_CONNECTED
		&& msg->event != WIFIMGR_EVT_STA_CONNECT_FAILED
		&& msg->event != WIFIMGR_EVT_SCAN_DONE) {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
		NET_LOGE(TAG, "invalid param\n");
		return WIFI_MANAGER_BUSY;
	}

	if (msg->event == WIFIMGR_EVT_SCAN_DONE) {
		_free_scan_list((wifi_utils_scan_list_s *)msg->param);
	}

	/* it handles disconnecting state differently by substate
	 * for example, if it enters disconnecting state because dhcpc fails
	 * then it should not call disconnect callback to applications */
	switch (g_manager_info.disconn_substate) {
	case WIFIMGR_DISCONN_DEINIT:
		WIFIMGR_CHECK_RESULT(_wifimgr_deinit(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SEND_API_SIGNAL(g_manager_info.api_sig);
		WIFIMGR_SET_STATE(WIFIMGR_UNINITIALIZED);
		break;
	case WIFIMGR_DISCONN_SOFTAP:
		WIFIMGR_CHECK_RESULT(_wifimgr_run_softap(
								 (wifi_manager_softap_config_s *)&g_manager_info.softap_config),
							 (TAG, "run_softap fail\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SEND_API_SIGNAL(g_manager_info.api_sig);
		WIFIMGR_SET_STATE(WIFIMGR_SOFTAP);
		break;
	case WIFIMGR_DISCONN_INTERNAL_ERROR:
		wifimgr_call_cb(CB_STA_CONNECT_FAILED, NULL);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTED);
		break;
	case WIFIMGR_DISCONN_NONE:
		wifimgr_call_cb(CB_STA_DISCONNECTED, NULL);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTED);
		break;
	default:
		NET_LOGE(TAG, "invalid argument\n");
		break;
	}

	WIFIMGR_RESET_SUBSTATE();

	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _handler_on_connecting_state(wifimgr_msg_s *msg)
{
	if (msg->event == WIFIMGR_EVT_STA_CONNECTED) {
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPC
		wifi_manager_result_e wret;
		wret = dhcpc_get_ipaddr();
		if (wret != WIFI_MANAGER_SUCCESS) {
			WIFIMGR_CHECK_RESULT(_wifimgr_disconnect_ap(), (TAG, "critical error: DHCP failure\n"), WIFI_MANAGER_FAIL);
			WIFIMGR_SET_SUBSTATE(WIFIMGR_DISCONN_INTERNAL_ERROR, NULL);
			WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTING);
			return wret;
		}
#endif
		wifimgr_call_cb(CB_STA_CONNECTED, NULL);
		WIFIMGR_SET_STATE(WIFIMGR_STA_CONNECTED);
	} else if (msg->event == WIFIMGR_EVT_STA_CONNECT_FAILED) {
		wifimgr_call_cb(CB_STA_CONNECT_FAILED, NULL);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTED);
	} else if (msg->event == WIFIMGR_CMD_DEINIT) {
		WIFIMGR_SET_SUBSTATE(WIFIMGR_DISCONN_DEINIT, msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTING);
	} else {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
		return WIFI_MANAGER_BUSY;
	}

	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _handler_on_connected_state(wifimgr_msg_s *msg)
{
	if (msg->event == WIFIMGR_CMD_DISCONNECT) {
		dhcpc_close_ipaddr();
		WIFIMGR_CHECK_RESULT(_wifimgr_disconnect_ap(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTING);
	} else if (msg->event == WIFIMGR_EVT_STA_DISCONNECTED) {
		dhcpc_close_ipaddr();
		wifimgr_call_cb(CB_STA_DISCONNECTED, NULL);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTED);
	} else if (msg->event == WIFIMGR_CMD_SET_SOFTAP) {
		dhcpc_close_ipaddr();
		WIFIMGR_COPY_SOFTAP_CONFIG(g_manager_info.softap_config, (wifi_manager_softap_config_s *)msg->param);
		WIFIMGR_CHECK_RESULT(_wifimgr_disconnect_ap(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SET_SUBSTATE(WIFIMGR_DISCONN_SOFTAP, msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTING);
	} else if (msg->event == WIFIMGR_CMD_DEINIT) {
		dhcpc_close_ipaddr();
		WIFIMGR_CHECK_RESULT(_wifimgr_disconnect_ap(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SET_SUBSTATE(WIFIMGR_DISCONN_DEINIT, msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTING);
	} else if (msg->event == WIFIMGR_CMD_SCAN) {
		WIFIMGR_CHECK_RESULT(_wifimgr_scan((wifi_manager_scan_config_s *)msg->param), (TAG, "fail scan\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_STORE_PREV_STATE;
		WIFIMGR_SET_STATE(WIFIMGR_SCANNING);
	} else if (msg->event == WIFIMGR_CMD_CONNECT) {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
		return WIFI_MANAGER_ALREADY_CONNECTED;
	} else {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
		return WIFI_MANAGER_FAIL;
	}
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _handler_on_softap_state(wifimgr_msg_s *msg)
{
	if (msg->event == WIFIMGR_CMD_SET_STA) {
		WIFIMGR_CHECK_RESULT(_wifimgr_stop_softap(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_CHECK_RESULT(_wifimgr_run_sta(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SEND_API_SIGNAL(msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTED);
	} else if (msg->event == WIFIMGR_CMD_SCAN) {
		WIFIMGR_CHECK_RESULT(_wifimgr_scan((wifi_manager_scan_config_s *)msg->param), (TAG, "fail scan\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_STORE_PREV_STATE;
		WIFIMGR_SET_STATE(WIFIMGR_SCANNING);
#ifdef CONFIG_WIFIMGR_DISABLE_DHCPS
	} else if (msg->event == WIFIMGR_EVT_JOINED) {
#else
		/* wifi manager passes the callback after the dhcp server gives a station an IP address*/
	} else if (msg->event == WIFIMGR_EVT_DHCPS_ASSIGN_IP) {
		if (dhcps_add_node((dhcp_node_s *)msg->param) == DHCP_EXIST) {
			return WIFI_MANAGER_SUCCESS;
		}
#endif
		dhcps_inc_num();
		wifimgr_call_cb(CB_STA_JOINED, NULL);
	} else if (msg->event == WIFIMGR_EVT_LEFT) {
#ifndef CONFIG_WIFIMGR_DISABLE_DHCPS
		dhcps_del_node();
#endif
		dhcps_dec_num();
		wifimgr_call_cb(CB_STA_LEFT, NULL);
	} else if (msg->event == WIFIMGR_CMD_DEINIT) {
		WIFIMGR_CHECK_RESULT(_wifimgr_stop_softap(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_CHECK_RESULT(_wifimgr_deinit(), (TAG, "critical error\n"), WIFI_MANAGER_FAIL);
		WIFIMGR_SEND_API_SIGNAL(msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_UNINITIALIZED);
	} else {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
		return WIFI_MANAGER_FAIL;
	}

	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e _handler_on_scanning_state(wifimgr_msg_s *msg)
{
	wifi_manager_result_e wret = WIFI_MANAGER_FAIL;
	if (msg->event == WIFIMGR_EVT_SCAN_DONE) {
		wifimgr_call_cb(CB_SCAN_DONE, msg->param);
		_free_scan_list((wifi_utils_scan_list_s *)msg->param);
		WIFIMGR_RESTORE_STATE;
		wret = WIFI_MANAGER_SUCCESS;
	} else if (msg->event == WIFIMGR_CMD_DEINIT) {
		WIFIMGR_SET_SUBSTATE(WIFIMGR_DISCONN_DEINIT, msg->signal);
		WIFIMGR_SET_STATE(WIFIMGR_STA_DISCONNECTING);
		wret = WIFI_MANAGER_SUCCESS;
	} else {
		WIFIADD_ERR_RECORD(ERR_WIFIMGR_INVALID_EVENT);
	}
	return wret;
}

wifi_manager_result_e _handler_get_stats(wifimgr_msg_s *msg)
{
	wifi_manager_result_e wret = WIFI_MANAGER_SUCCESS;
	trwifi_msg_stats_s stats;
	stats.cmd = TRWIFI_MSG_GET_STATS;
	wifi_utils_result_e res = wifi_utils_ioctl(&stats);
	if (res != WIFI_UTILS_SUCCESS) {
		wret = WIFI_MANAGER_FAIL;
	} else {
		/* update msg
		 * msg->param was checked in wifi_manager_get_stats()
		 * So doesn't need to check msg->param in here.
		 */
		wifi_manager_stats_s *wstats = (wifi_manager_stats_s *)msg->param;
		wstats->start = stats.start;
		wstats->end = stats.end;
		wstats->tx_retransmit = stats.tx_retransmit;
		wstats->tx_drop = stats.tx_drop;
		wstats->rx_drop = stats.rx_drop;
		wstats->tx_success_cnt = stats.tx_success_cnt;
		wstats->tx_success_bytes = stats.tx_success_bytes;
		wstats->rx_cnt = stats.rx_cnt;
		wstats->rx_bytes = stats.rx_bytes;
		wstats->tx_try = stats.tx_try;
		wstats->rssi_avg = stats.rssi_avg;
		wstats->rssi_min = stats.rssi_min;
		wstats->rssi_max = stats.rssi_max;
		wstats->beacon_miss_cnt = stats.beacon_miss_cnt;
	}
	return wret;
}

wifi_manager_result_e _handler_set_powermode(wifimgr_msg_s *msg)
{
	wifi_manager_powermode_e mode = *((wifi_manager_powermode_e *)(msg->param));
	int imode = TRWIFI_POWERMODE_OFF;
	if (mode == WIFI_MANAGER_POWERMODE_ENABLE) {
		imode = TRWIFI_POWERMODE_ON;
	}
	trwifi_msg_s tmsg = {TRWIFI_MSG_SET_POWERMODE, (void *)(&imode)};
	wifi_utils_result_e res = wifi_utils_ioctl(&tmsg);
	if (res != WIFI_UTILS_SUCCESS) {
		NET_LOGE(TAG, "set power mode fail %d\n", res);
		return WIFI_MANAGER_FAIL;
	}
	return WIFI_MANAGER_SUCCESS;
}

wifi_manager_result_e wifimgr_handle_request(wifimgr_msg_s *msg)
{
	wifi_manager_result_e res = WIFI_MANAGER_FAIL;

	NET_LOGI(TAG, "handle request state(%s) evt(%s)\n",
			 wifimgr_get_state_str(WIFIMGR_GET_STATE),
			 wifimgr_get_evt_str(msg->event));
	if (msg->event == WIFIMGR_CMD_GETSTATS) {
		res = _handler_get_stats(msg);
	} else if (msg->event == WIFIMGR_CMD_SETPOWER) {
		res = _handler_set_powermode(msg);
	} else {
		res = g_handler[WIFIMGR_GET_STATE](msg);
	}
#ifdef CONFIG_WIFIMGR_ERROR_REPORT
	_set_error_code(res);
#endif
	NET_LOGV(TAG, "<-- _handle_request\n");
	return res;
}
