/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
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

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <wifi_manager/wifi_manager.h>
#include <stress_tool/st_perf.h>
#include "wm_test.h"
#include "wm_test_utils.h"
#include "wm_test_network.h"
#include "wm_test_mock.h"
#include "wm_test_log.h"

#define WM_TEST_TRIAL 3
#define TAG "[WT]"
//if semaphore operation failed then it'll try it again 10ms later
#define WT_SEM_TRY_WAIT_US 10000
static char *WM_AP_SSID;
static char *WM_AP_PASSWORD;
static wifi_manager_ap_auth_type_e WM_AP_AUTH;
static wifi_manager_ap_crypto_type_e WM_AP_CRYPTO;
static char *WM_SOFTAP_SSID;
static char *WM_SOFTAP_PASSWORD;
static int WM_SOFTAP_CHANNEL;
static sem_t g_wm_sem;

/*
 * callbacks
 */
static void wm_sta_connected(wifi_manager_result_e);
static void wm_sta_disconnected(wifi_manager_disconnect_e);
static void wm_softap_sta_join(void);
static void wm_softap_sta_leave(void);
static void wm_scan_done(wifi_manager_scan_info_s **scan_result, wifi_manager_scan_result_e res);

static wifi_manager_cb_s g_wifi_callbacks = {
	wm_sta_connected,
	wm_sta_disconnected,
	wm_softap_sta_join,
	wm_softap_sta_leave,
	wm_scan_done,
};

#define WM_TEST_SIGNAL							\
	do {										\
		while (sem_post(&g_wm_sem) != 0) {		\
			usleep(WT_SEM_TRY_WAIT_US);			\
		}										\
		WT_LOG(TAG, "send signal");			\
	} while (0)

#define WM_TEST_WAIT									\
	do {												\
		WT_LOG(TAG, "wait signal");					\
		while (sem_wait(&g_wm_sem) != 0) {				\
			if (errno == EINTR) {						\
				continue;								\
			}											\
			WT_LOGE(TAG, "sem wait fail %d", errno);	\
		}												\
	} while (0)

void wm_sta_connected(wifi_manager_result_e res)
{
	WT_LOG(TAG, "--> res(%d)", res);
	WM_TEST_SIGNAL;
}

void wm_sta_disconnected(wifi_manager_disconnect_e disconn)
{
	WT_LOG(TAG, "-->");
	WM_TEST_SIGNAL;
}

void wm_softap_sta_join(void)
{
	WT_LOG(TAG, "-->");
	WM_TEST_SIGNAL;
}

void wm_softap_sta_leave(void)
{
	WT_LOG(TAG, "-->");
	WM_TEST_SIGNAL;
}

void wm_scan_done(wifi_manager_scan_info_s **scan_result, wifi_manager_scan_result_e res)
{
	WT_LOG(TAG, "-->");
	/* Make sure you copy the scan results onto a local data structure.
	 * It will be deleted soon eventually as you exit this function.
	 */
	if (scan_result == NULL) {
		WM_TEST_SIGNAL;
		return;
	}
	wifi_manager_scan_info_s *wifi_scan_iter = *scan_result;
	while (wifi_scan_iter != NULL) {
		WT_LOG(TAG, "WiFi AP SSID: %-20s, WiFi AP BSSID: %-20s, WiFi Rssi: %d, AUTH: %d, CRYPTO: %d",
			   wifi_scan_iter->ssid, wifi_scan_iter->bssid, wifi_scan_iter->rssi,
			   wifi_scan_iter->ap_auth_type, wifi_scan_iter->ap_crypto_type);
		wifi_scan_iter = wifi_scan_iter->next;
	}
	WM_TEST_SIGNAL;
}

static void wm_get_softapinfo(wifi_manager_softap_config_s *ap_config)
{
	strncpy(ap_config->ssid, WM_SOFTAP_SSID, strlen(WM_SOFTAP_SSID) + 1);
	strncpy(ap_config->passphrase, WM_SOFTAP_PASSWORD, strlen(WM_SOFTAP_PASSWORD) + 1);
	ap_config->channel = WM_SOFTAP_CHANNEL;
}

static void wm_get_apinfo(wifi_manager_ap_config_s *apconfig)
{
	strncpy(apconfig->ssid, WM_AP_SSID, strlen(WM_AP_SSID) + 1);
	apconfig->ssid_length = strlen(WM_AP_SSID);
	apconfig->ap_auth_type = WM_AP_AUTH;
	if (WM_AP_AUTH != WIFI_MANAGER_AUTH_OPEN) {
		strncpy(apconfig->passphrase, WM_AP_PASSWORD, strlen(WM_AP_PASSWORD) + 1);
		apconfig->passphrase_length = strlen(WM_AP_PASSWORD);
		apconfig->ap_crypto_type = WM_AP_CRYPTO;
	}
}

static int _run_procedure(void)
{
	wifi_manager_ap_config_s apconfig;
	wm_get_apinfo(&apconfig);

	CONTROL_VDRIVER(VWIFI_CMD_SET, VWIFI_KEY_RESULT, (int)TRWIFI_SUCCESS, 0);

	wifi_manager_result_e wres = WIFI_MANAGER_SUCCESS;
	WT_LOG(TAG, "init wi-fi");
	wres = wifi_manager_init(&g_wifi_callbacks);
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "fail to init %d\n", wres);
		return -1;
	}

	/*  Start softAP */
	WT_LOG(TAG, "start softAP");
	/*  it's connected state so disconnected evt have to be happened */
	//CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT_FUNC , LWNL_EVT_STA_DISCONNECTED, 0, 1000);
	wifi_manager_softap_config_s softap_config;
	wm_get_softapinfo(&softap_config);
	wres = wifi_manager_set_mode(SOFTAP_MODE, &softap_config);
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "fail to start softap %d\n", wres);
		return -1;
	}

	/*  wait join event */
	CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT, LWNL_EVT_SOFTAP_STA_JOINED, 0, 3000);
	CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT, VWIFI_PKT_DHCPS_EVT, 0, 3000);
	WT_LOG(TAG, "wait join event");
	WM_TEST_WAIT;

	/*  scan in softAP mode */
	WT_LOG(TAG, "scan in softAP mode");
	wres = wifi_manager_scan_ap(NULL);
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "fail to scan %d\n", wres);
		return -1;
	}
	/*  wait scan event */
	CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT, LWNL_EVT_SCAN_DONE, 0, 1000);
	WT_LOG(TAG, "wait scan done event");
	WM_TEST_WAIT;

	/*  set STA */
	WT_LOG(TAG, "start STA mode");

	wres = wifi_manager_set_mode(STA_MODE, NULL);
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "start STA fail %d\n", wres);
		return -1;
	}

	/*  scan in STA mode */
	WT_LOG(TAG, "scan in STA mode");
	wres = wifi_manager_scan_ap(NULL);
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "fail to scan %d\n", wres);
		return -1;
	}

	CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT, LWNL_EVT_SCAN_DONE, 0, 1000);
	WT_LOG(TAG, "wait scan done event in STA mode");
	WM_TEST_WAIT; /*  wait scan event */

	/*  connect to AP */
	WT_LOG(TAG, "connect AP");
	wres = wifi_manager_connect_ap(&apconfig);
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "connect AP fail %d\n", wres);
		return -1;
	}
	CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT, LWNL_EVT_STA_CONNECTED, 0, 1000);
	WT_LOG(TAG, "wait connect success event");
	WM_TEST_WAIT;

	/*  scan in connected state */
	WT_LOG(TAG, "scan in connected state of STA mode");
	wres = wifi_manager_scan_ap(NULL);
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "fail to scan %d\n", wres);
		return -1;
	}
	CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT, LWNL_EVT_SCAN_DONE, 0, 1000);
	WT_LOG(TAG, "wait scan done event in connected state of STA mode");
	WM_TEST_WAIT; /*  wait scan event */

	WT_LOG(TAG, "deinit wi-fi");
	CONTROL_VDRIVER(VWIFI_CMD_GEN_EVT_FUNC, LWNL_EVT_STA_DISCONNECTED, 0, 0);
	wres = wifi_manager_deinit();
	if (wres != WIFI_MANAGER_SUCCESS) {
		WT_LOGE(TAG, "fail to deinit %d\n", wres);
		return -1;
	}
	return 0;
}

TEST_F(mode_change)
{
	ST_START_TEST;
	ST_EXPECT_EQ(0, _run_procedure());
	ST_END_TEST;
}

TEST_F(init_n)
{
	ST_START_TEST;
	CONTROL_VDRIVER(VWIFI_CMD_SET, VWIFI_KEY_RESULT, TRWIFI_FAIL, 0);
	ST_EXPECT_EQ(WIFI_MANAGER_FAIL, wifi_manager_init(&g_wifi_callbacks));
	ST_END_TEST;
}

TEST_SETUP(init_p)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_TEARDOWN(init_p)
{
	ST_START_TEST;
	wifi_manager_deinit();
	ST_END_TEST;
}

TEST_F(init_p)
{
	ST_START_TEST;
	CONTROL_VDRIVER(VWIFI_CMD_SET, VWIFI_KEY_RESULT, TRWIFI_SUCCESS, 0);
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_init(&g_wifi_callbacks));
	ST_END_TEST;
}

TEST_F(set_power_p)
{
	ST_START_TEST;
	CONTROL_VDRIVER(VWIFI_CMD_SET, VWIFI_KEY_RESULT, TRWIFI_SUCCESS, 0);
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_init(&g_wifi_callbacks));
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_set_powermode(WIFI_MANAGER_POWERMODE_DISABLE));
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	ST_END_TEST;

}

void wm_run_stress_test4(struct wt_options *opt)
{
	WM_AP_SSID = opt->ssid;
	WM_AP_PASSWORD = opt->password;
	WM_AP_AUTH = opt->auth_type;
	WM_AP_CRYPTO = opt->crypto_type;
	WM_SOFTAP_SSID = opt->softap_ssid;
	WM_SOFTAP_PASSWORD = opt->softap_password;
	WM_SOFTAP_CHANNEL = opt->softap_channel;

	WT_LOG(TAG, "init sem");
	if (0 != sem_init(&g_wm_sem, 0, 0)) {
		WT_LOGE(TAG, "sem init fail %d", errno);
		return;
	}
	ST_SET_PACK(wifi);

	ST_SET_SMOKE1(wifi, WM_TEST_TRIAL, 0, "use case test", mode_change);
	ST_SET_SMOKE1(wifi, 1, 0, "init negative case", init_n);
	ST_SET_SMOKE(wifi, 1, 0, "init positive case", init_p);
	ST_SET_SMOKE1(wifi, 1, 0, "set power", set_power_p);

	ST_RUN_TEST(wifi);
	ST_RESULT_TEST(wifi);

	WT_LOG(TAG, "deinit sem");
	sem_destroy(&g_wm_sem);
}

