/****************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License\n");
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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>
#include <wifi_manager/wifi_manager.h>
#include "wm_test.h"
#include "wm_test_utils.h"

/*
 * Macro
 */
#define WIFIMGR_SSID ""
#define WIFIMGR_PWD ""
#define WIFIMGR_AUTH WIFI_MANAGER_AUTH_WPA2_PSK
#define WIFIMGR_CRYPTO WIFI_MANAGER_CRYPTO_AES

#define WO_CONN_FAIL 1
#define WO_CONN_SUCCESS 2
#define WO_INTERVAL 10
#define WO_GOAL_CNT 1000

/*
 * callbacks
 */
static void wm_sta_connected(wifi_manager_result_e);
static void wm_sta_disconnected(wifi_manager_disconnect_e);

/*
 * State
 */
static int run_init(void *arg);
static int run_connecting(wifi_manager_ap_config_s *ap_config);
static int run_connected(void);

/*
 * Global
 */
static wifi_manager_cb_s g_wifi_callbacks = {
	wm_sta_connected,
	wm_sta_disconnected,
	NULL, NULL, NULL,
};
static struct wo_queue *g_wo_queue = NULL;
/*
 * Callback
 */
void wm_sta_connected(wifi_manager_result_e res)
{
	int conn = 0;
	printf("[WO] T%d --> %s res(%d)\n", getpid(), __FUNCTION__, res);
	if (WIFI_MANAGER_SUCCESS == res) {
		conn = WO_CONN_SUCCESS;
	} else {
		conn = WO_CONN_FAIL;
	}
	WO_TEST_SIGNAL(conn, g_wo_queue);
}

void wm_sta_disconnected(wifi_manager_disconnect_e disconn)
{
	printf("[WO] T%d --> %s %d\n", getpid(), __FUNCTION__, disconn);
	WO_TEST_SIGNAL(WO_CONN_FAIL, g_wo_queue);
}

static void print_wifi_ap_profile(wifi_manager_ap_config_s *config, char *title)
{
	printf("====================================\n");
	if (title) {
		printf("[WO] %s\n", title);
	}
	printf("------------------------------------\n");
	printf("SSID: %s\n", config->ssid);
	printf("security type (%d) (%d)\n", config->ap_auth_type, config->ap_crypto_type);
	printf("====================================\n");
}

static void wm_get_info(wifi_manager_ap_config_s *arg)
{
	printf("[WO] T%d -->%s\n", getpid(), __FUNCTION__);
	wifi_manager_ap_config_s apconfig;
	wifi_manager_result_e res = wifi_manager_get_config(&apconfig);
	if (res != WIFI_MANAGER_SUCCESS) {
		printf("[WO] Get AP configuration failed\n");
		return;
	}
	print_wifi_ap_profile(&apconfig, "Stored Wi-Fi Information");
}

static int run_init(void *arg)
{
	wifi_manager_result_e res = wifi_manager_init(&g_wifi_callbacks);
	if (res != WIFI_MANAGER_SUCCESS) {
		WO_ERROR(res);
		return -1;
	}

	int cnt_auto_connect = 0;

	/* Set AP Configuration */
	struct options *ap_info = (struct options *)arg;
	wifi_manager_ap_config_s apconfig;
	strncpy(apconfig.ssid, ap_info->ssid, WIFIMGR_SSID_LEN);
	apconfig.ssid_length = strlen(ap_info->ssid);
	apconfig.ssid[WIFIMGR_SSID_LEN] = '\0';
	apconfig.ap_auth_type = ap_info->auth_type;
	if (ap_info->auth_type != WIFI_MANAGER_AUTH_OPEN) {
		strncpy(apconfig.passphrase, ap_info->password, WIFIMGR_PASSPHRASE_LEN);
		apconfig.passphrase[WIFIMGR_PASSPHRASE_LEN] = '\0';
		apconfig.passphrase_length = strlen(ap_info->password);
		apconfig.ap_crypto_type = ap_info->crypto_type;
	} else {
		apconfig.passphrase[0] = '\0';
		apconfig.passphrase_length = 0;
		apconfig.ap_crypto_type = ap_info->crypto_type;
	}

	print_wifi_ap_profile(&apconfig, "Connecting AP Info");

	/*  Run Auto Test */
	int state = 1;
	while (1) {
		if (state == 1) {
			state = run_connecting(&apconfig);
		} else if (state == 2) {
			cnt_auto_connect++;
			printf("\n\n\n[WO] connection count %d\n\n\n\n", cnt_auto_connect);
			if (cnt_auto_connect > WO_GOAL_CNT) {
				break;
			}
			state = run_connected();
		} else {
			printf("[WO] Unknown state (%d)\n", state);
			assert(0);
		}
	}

	printf("[WO] terminate program total (%d)\n", cnt_auto_connect);

	return 0;
}

static int run_connecting(wifi_manager_ap_config_s *ap_config)
{
	printf("[WO] -->%s\n", __FUNCTION__);

	wifi_manager_result_e res = wifi_manager_connect_ap(ap_config);
	if (res != WIFI_MANAGER_SUCCESS) {
		WO_ERROR(res);
		goto connect_fail;
	}

	int conn = WO_CONN_FAIL;
	WO_TEST_WAIT(conn, g_wo_queue);

	if (conn == WO_CONN_FAIL) {
		// does it need to get info from wi-fi wm_get_info(ap_config);
		goto connect_fail;
	} else if (conn == WO_CONN_SUCCESS) {
		return 2; // connected, wait disconnect message
	} else {
		printf("[WO] program is corrupted %s\n", __FUNCTION__);
		assert(0);
	}
	return 0;

connect_fail:
	printf("[WO] wait %d second\n", WO_INTERVAL);
	sleep(WO_INTERVAL);
	return 1;
}

static int run_connected(void)
{
	printf("[WO] -->%s\n", __FUNCTION__);
	int conn = WO_CONN_FAIL;
	WO_TEST_WAIT(conn, g_wo_queue);
	if (conn == WO_CONN_FAIL) {
		return 1;
	} else {
		printf("[WO] program is corrupted %s\n", __FUNCTION__);
		assert(0);
	}
	return 0;
}

void wm_test_on_off(void *arg)
{
	g_wo_queue = wo_create_queue();
	if (!g_wo_queue) {
		printf("[WO] create queue fail\n");
		return;
	}

	run_init(arg);

	wo_destroy_queue(g_wo_queue);
}
