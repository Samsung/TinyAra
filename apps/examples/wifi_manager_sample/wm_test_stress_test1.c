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

#include <sys/types.h>
#include <stdio.h>
#include <wifi_manager/wifi_manager.h>
#include <stress_tool/st_perf.h>
#include "wm_test.h"
#include "wm_test_log.h"

/* Macro*/
#define WM_TEST_TRIAL	   CONFIG_WIFIMANAGER_TEST_TRIAL
#define TAG "[WTS1]"

static char *WM_AP_SSID;
static char *WM_AP_PASSWORD;
static wifi_manager_ap_auth_type_e WM_AP_AUTH;
static wifi_manager_ap_crypto_type_e WM_AP_CRYPTO;

#define WM_TEST_SIGNAL										\
	do {													\
		pthread_mutex_lock(&g_wm_mutex);					\
		WT_LOG(TAG, "send signal");	\
		pthread_cond_signal(&g_wm_cond);					\
		pthread_mutex_unlock(&g_wm_mutex);					\
	} while (0)

#define WM_TEST_WAIT								\
	do {											\
		pthread_mutex_lock(&g_wm_mutex);			\
		WT_LOG(TAG, "wait signal");		\
		pthread_cond_wait(&g_wm_cond, &g_wm_mutex);	\
		pthread_mutex_unlock(&g_wm_mutex);			\
	} while (0)

/* callbacks*/
static void wm_sta_connected(wifi_manager_result_e);
static void wm_sta_disconnected(wifi_manager_disconnect_e);
static void wm_softap_sta_joined(void);
static void wm_softap_sta_left(void);
static void wm_scan_ap_done(wifi_manager_scan_info_s **, wifi_manager_scan_result_e);

/* Global*/
static wifi_manager_cb_s g_wifi_callbacks = {
	wm_sta_connected,
	wm_sta_disconnected,
	wm_softap_sta_joined,
	wm_softap_sta_left,
	wm_scan_ap_done
};

static pthread_mutex_t g_wm_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_wm_cond = PTHREAD_COND_INITIALIZER;

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

void wm_softap_sta_joined(void)
{
	WT_LOG(TAG, "-->");
}

void wm_softap_sta_left(void)
{
	WT_LOG(TAG, "-->");
}

void wm_scan_ap_done(wifi_manager_scan_info_s **info, wifi_manager_scan_result_e res)
{
	WT_LOG(TAG, "-->");
	WT_LOG(TAG, " result (%d) info pointer(%p)", res, *info);
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

/*
 * station join test
 */
TEST_SETUP(init)
{
	ST_START_TEST;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_init(&g_wifi_callbacks));
	ST_END_TEST;
}

TEST_TEARDOWN(init)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_F(init)
{
	ST_START_TEST;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	ST_END_TEST;
}

/**
 * Description: call deinit() before receiving connect done event.
 */
TEST_SETUP(connect)
{
	ST_START_TEST;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_init(&g_wifi_callbacks));
	wifi_manager_ap_config_s apconfig;
	wm_get_apinfo(&apconfig);
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_connect_ap(&apconfig));
	ST_END_TEST;
}

TEST_TEARDOWN(connect)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_F(connect)
{
	ST_START_TEST;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	ST_END_TEST;
}

/**
 * Description: call deinit() after receiving connect done event.
 */
TEST_SETUP(connect_done)
{
	ST_START_TEST;

	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_init(&g_wifi_callbacks));
	wifi_manager_ap_config_s apconfig;
	wm_get_apinfo(&apconfig);

	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_connect_ap(&apconfig));
	WM_TEST_WAIT;

	ST_END_TEST;
}

TEST_TEARDOWN(connect_done)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_F(connect_done)
{
	ST_START_TEST;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	ST_END_TEST;
}

/**
 * Description: call deinit() after disconnect() is executed
 */
TEST_SETUP(disconnect)
{
	ST_START_TEST;

	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_init(&g_wifi_callbacks));
	wifi_manager_ap_config_s apconfig;
	wm_get_apinfo(&apconfig);
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_connect_ap(&apconfig));
	WM_TEST_WAIT;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_disconnect_ap());
	ST_END_TEST;
}

TEST_TEARDOWN(disconnect)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_F(disconnect)
{
	ST_START_TEST;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	ST_END_TEST;
}

/**
 * Description: call deinit() after scan() is executed
 */
TEST_SETUP(scan)
{
	ST_START_TEST;

	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_init(&g_wifi_callbacks));
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_scan_ap(NULL));

	ST_END_TEST;
}

TEST_TEARDOWN(scan)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_F(scan)
{
	ST_START_TEST;
	ST_EXPECT_EQ(WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	ST_END_TEST;
}

void wm_run_stress_test1(struct wt_options *opt)
{
	WM_AP_SSID = opt->ssid;
	WM_AP_PASSWORD = opt->password;
	WM_AP_AUTH = opt->auth_type;
	WM_AP_CRYPTO = opt->crypto_type;

	ST_SET_PACK(wifi);

	ST_SET_SMOKE(wifi, WM_TEST_TRIAL, 10000000, "init", init);
	ST_SET_SMOKE(wifi, WM_TEST_TRIAL, 2000000, "connect", connect);
	ST_SET_SMOKE(wifi, WM_TEST_TRIAL, 10000000, "connect_done", connect_done);
	ST_SET_SMOKE(wifi, WM_TEST_TRIAL, 5000000, "disconnect", disconnect);
	ST_SET_SMOKE(wifi, WM_TEST_TRIAL, 5000000, "scan", scan);

	ST_RUN_TEST(wifi);
	ST_RESULT_TEST(wifi);
}
