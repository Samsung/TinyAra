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
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <wifi_manager/wifi_manager.h>
#include "tc_common.h"

static pthread_mutex_t g_wifi_manager_test_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_wifi_manager_test_cond;

#define TEST_SSID CONFIG_EXAMPLES_TESTCASE_WIFI_MANAGER_ITC_SSID
#define TEST_PASSWORD CONFIG_EXAMPLES_TESTCASE_WIFI_MANAGER_ITC_PASSPHRASE
#define TEST_AUTH_TYPE CONFIG_EXAMPLES_TESTCASE_WIFI_MANAGER_ITC_AUTHENTICATION
#define TEST_CRYPTO_TYPE CONFIG_EXAMPLES_TESTCASE_WIFI_MANAGER_ITC_CRYPTO
#define TEST_SOFTAP_SSID CONFIG_EXAMPLES_TESTCASE_WIFI_MANAGER_ITC_SOFTAP_SSID
#define TEST_SOFTAP_CHANNEL CONFIG_EXAMPLES_TESTCASE_WIFI_MANAGER_ITC_SOFTAP_CHANNEL
#define TEST_SOFTAP_PASSWORD CONFIG_EXAMPLES_TESTCASE_WIFI_MANAGER_ITC_SOFTAP_PASSPHRASE
#define SEC_5 5
#define LOOP_SIZE 3
#define SSID_MAX_LEN 32
#define PASSPHRASE_MAX_LEN 64
#define WIFITEST_SIGNAL\
	do {\
		pthread_mutex_lock(&g_wifi_manager_test_mutex);\
		pthread_cond_signal(&g_wifi_manager_test_cond);\
		pthread_mutex_unlock(&g_wifi_manager_test_mutex);\
	} while (0)

#define WIFITEST_WAIT\
	do {\
		pthread_mutex_lock(&g_wifi_manager_test_mutex);\
		pthread_cond_wait(&g_wifi_manager_test_cond, &g_wifi_manager_test_mutex);\
		pthread_mutex_unlock(&g_wifi_manager_test_mutex);\
	} while (0)

static void wifi_sta_connected_cb(wifi_manager_result_e res); // in station mode, connected to ap
static void wifi_sta_disconnected_cb(wifi_manager_disconnect_e disconn); // in station mode, disconnected from ap
static void wifi_scan_ap_done_cb(wifi_manager_scan_info_s **scan_info, wifi_manager_scan_result_e res); // called when scanning ap is done

static wifi_manager_cb_s wifi_callbacks = {
	wifi_sta_connected_cb,
	wifi_sta_disconnected_cb,
	NULL,
	NULL,
	wifi_scan_ap_done_cb, // this callback function is called when scanning ap is done.
};

static void wifi_sta_connected_cb(wifi_manager_result_e res)
{
	printf("wifi_sta_connected: send signal!!! \n");
	WIFITEST_SIGNAL;
}

static void wifi_sta_disconnected_cb(wifi_manager_disconnect_e disconn)
{
	printf("wifi_sta_disconnected: send signal!!! \n");
	WIFITEST_SIGNAL;
}

static void wifi_scan_ap_done_cb(wifi_manager_scan_info_s **scan_info, wifi_manager_scan_result_e res)
{
	/******************************************************************
	 * Make sure you copy the scan results onto a local data structure.
	 * It will be deleted soon eventually as you exit this function.
	 ******************************************************************/
	if (res == WIFI_SCAN_FAIL) {
		printf("WiFi scan failed\n");
		WIFITEST_SIGNAL;
		return;
	}
	wifi_manager_scan_info_s *wifi_scan_iter = *scan_info;
	while (wifi_scan_iter != NULL) {
		printf("SSID: %-20s, BSSID: %-20s, RSSI: %d, CH: %d, Phy_type: %d\n", \
				wifi_scan_iter->ssid, wifi_scan_iter->bssid, wifi_scan_iter->rssi, \
				wifi_scan_iter->channel, wifi_scan_iter->phy_mode);
		wifi_scan_iter = wifi_scan_iter->next;
	}
	WIFITEST_SIGNAL;
}

static int wifi_test_signal_init(void)
{
	int res = pthread_mutex_init(&g_wifi_manager_test_mutex, NULL);
	if (res != 0) {
		printf("wifi test signal init fail(%d) (%d)\n", res, errno);
		return -1;
	}
	res = pthread_cond_init(&g_wifi_manager_test_cond, NULL);
	if (res != 0) {
		printf("wifi test condition init fail(%d) (%d)\n", res, errno);
		res = pthread_mutex_destroy(&g_wifi_manager_test_mutex);
		if (res != 0) {
			printf("wifi test signal init fail(%d) (%d)\n", res, errno);
		}
		return -1;
	}
	return 0;
}

static void wifi_test_signal_deinit(void)
{
	int res = pthread_mutex_destroy(&g_wifi_manager_test_mutex);
	if (res != 0) {
		printf("wifi test signal deinit fail(%d) (%d)\n", res, errno);
	}
	res = pthread_cond_destroy(&g_wifi_manager_test_cond);
	if (res != 0) {
		printf("wifi test signal deinit fail(%d) (%d)\n", res, errno);
	}
}

static double diff_time(struct timeval *x, struct timeval *y)
{
	double x_ms;
	double y_ms;
	double diff;
	x_ms = (double)x->tv_sec * 1000000 + (double)x->tv_usec;
	y_ms = (double)y->tv_sec * 1000000 + (double)y->tv_usec;
	diff = (double)y_ms - (double)x_ms;
	return diff;
}

/*********************************************************************
 * Test Cases
 *********************************************************************/

/**
 * @testcase         itc_wifimanager_init_deinit_p
 * @brief            to check initialize and de-initalization of wifi manager
 * @scenario         check initialize and de-initalization of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_init_deinit_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_set_mode_p
 * @brief            to check get info operation of wifi manager
 * @scenario         check get info operation of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_get_info, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_set_mode_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_softap_config_s ap_config;

	strncpy(ap_config.ssid, TEST_SOFTAP_SSID,
			strlen(TEST_SOFTAP_SSID) + 1);

	ap_config.channel = TEST_SOFTAP_CHANNEL;
	strncpy(ap_config.passphrase, TEST_SOFTAP_PASSWORD,
			strlen(TEST_SOFTAP_PASSWORD) + 1);

	printf("SoftAP config: %s(%d), %s(%d), %d\n", ap_config.ssid, strlen(TEST_SOFTAP_SSID),
			ap_config.passphrase, strlen(TEST_SOFTAP_PASSWORD), ap_config.channel);

	ret = wifi_manager_set_mode(SOFTAP_MODE, &ap_config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_set_mode", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_set_mode(STA_MODE, NULL);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_set_mode", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_get_mode_p
 * @brief            to check get info operation of wifi manager
 * @scenario         check set mode operation of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_get_info, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_get_mode_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	wifi_manager_info_s info;
	int ret_cmp = 0;
	int i;

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	ret = wifi_manager_get_info(&info);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_get_info", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	TC_ASSERT_EQ_CLEANUP("wifi_manager_get_info", info.mode, STA_MODE, wifi_manager_deinit());

	wifi_manager_softap_config_s ap_config;
	strncpy(ap_config.ssid, TEST_SOFTAP_SSID,
			strlen(TEST_SOFTAP_SSID) + 1);

	ap_config.channel = TEST_SOFTAP_CHANNEL;
	strncpy(ap_config.passphrase, TEST_SOFTAP_PASSWORD,
			strlen(TEST_SOFTAP_PASSWORD) + 1);

	ret = wifi_manager_set_mode(SOFTAP_MODE, &ap_config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_set_mode", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_get_info(&info);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_get_info", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	TC_ASSERT_EQ_CLEANUP("wifi_manager_get_info", info.mode, SOFTAP_MODE, wifi_manager_deinit());

	printf("\nMAC Address: ");

	for (i = 0; i < 5; i++) {
		printf("%x:", info.mac_address[i]);
	}
	printf("%x\n", info.mac_address[5]);

	ret_cmp = strncmp(info.ssid, ap_config.ssid, strlen(TEST_SOFTAP_SSID));
	TC_ASSERT_EQ_CLEANUP("string cmp between SSID ", ret_cmp, 0, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_connect_disconnect_ap_p
 * @brief            check initialize, connect, disconnect, de-initalization operations of wifi manager
 * @scenario         check initialize, connect, disconnect, de-initalization operations of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_disconnect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_connect_disconnect_ap_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret = wifi_manager_connect_ap(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_disconnect_ap();
	WIFITEST_WAIT;
	TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_connect_ap_config_p
 * @brief            check initialize, connect, de-initalization operations of wifi manager
 * @scenario         check initialize, connect, de-initalization operations of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_connect_ap_config, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_connect_ap_config_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	reconfig.interval = 10;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret =  wifi_manager_connect_ap(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_disconnect_ap();
	TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_connect_ap_config_n
 * @brief            check initialize, connect, de-initalization operations of wifi manager
 * @scenario         check initialize, connect, de-initalization operations of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_connect_ap_config, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_connect_ap_config_n(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret =  wifi_manager_connect_ap(&config);
	TC_ASSERT_NEQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_disconnect_ap();
	TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_FAIL, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_save_get_config_p
 * @brief            check initialize, save ap config, get ap config, de-initalization operations of wifi manager
 * @scenario         check initialize, save ap config, get ap config, de-initalization operations of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_save_config, wifi_manager_get_config,, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_save_get_config_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	wifi_manager_ap_config_s getconfig;

	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret =  wifi_manager_save_config(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_save_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret =  wifi_manager_get_config(&getconfig);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_get_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = strncmp(config.ssid, getconfig.ssid, strlen(TEST_SSID));
	TC_ASSERT_EQ_CLEANUP("string cmp between SSID ", ret, 0, wifi_manager_deinit());

	ret = strncmp(config.passphrase, getconfig.passphrase, strlen(TEST_PASSWORD));
	TC_ASSERT_EQ_CLEANUP("string cmp between PASSWORD ", ret, 0, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_get_config_n
 * @brief            check initialize, get ap config, de-initalization operations of wifi manager
 * @scenario         check initialize, get ap config, de-initalization operations of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_save_config, wifi_manager_get_config,, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_get_config_n(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	wifi_manager_ap_config_s gconfig;

	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret =  wifi_manager_get_config(&gconfig);
	TC_ASSERT_NEQ_CLEANUP("wifi_manager_get_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}


/**
 * @testcase         itc_wifimanager_remove_get_config_p
 * @brief            check initialize, save ap config, get ap config, remove ap config, de-initalization operations of wifi manager
 * @scenario         check initialize, save ap config, get ap config, remove ap config, de-initalization operations of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_save_config, wifi_manager_get_config, wifi_manager_remove_config wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_remove_get_config_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	wifi_manager_ap_config_s gconfig;

	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret =  wifi_manager_save_config(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_save_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret =  wifi_manager_get_config(&gconfig);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_get_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret =  wifi_manager_remove_config();
	TC_ASSERT_EQ_CLEANUP("wifi_manager_remove_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret =  wifi_manager_get_config(&gconfig);
	TC_ASSERT_NEQ_CLEANUP("wifi_manager_get_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_reremove_n
 * @brief            check initialize, save ap config, get ap config, remove ap config, de-initalization operations of wifi manager
 * @scenario         check initialize, save ap config, get ap config, remove ap config, de-initalization operations of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_save_config, wifi_manager_get_config, wifi_manager_remove_config wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_reremove_n(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	wifi_manager_ap_config_s gconfig;

	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret =  wifi_manager_save_config(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_save_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret =  wifi_manager_get_config(&gconfig);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_get_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret =  wifi_manager_remove_config();
	TC_ASSERT_EQ_CLEANUP("wifi_manager_remove_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret =  wifi_manager_remove_config();
	TC_ASSERT_NEQ_CLEANUP("wifi_manager_remove_config", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}


/**
 * @testcase         itc_wifimanager_scan_ap_p
 * @brief            check initialize, scan,and de-initalization of wifi manager
 * @scenario         check initialize, scan,and de-initalization of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_scan_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_scan_ap_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	ret = wifi_manager_scan_ap(NULL);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_scan_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_reinit_p
 * @brief            check re-initialize and de-initalization of wifi manager
 * @scenario         check re-initialize and de-initalization of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_reinit_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_LT_CLEANUP("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_init_redeinit_p
 * @brief            check initialization and multi time de-initalization of wifi manager
 * @scenario         check initialization and multi time de-initalization of wifi manager
 * @apicovered       wifi_manager_init, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_redeinit_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	ret = wifi_manager_deinit();
	TC_ASSERT_LT("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_reconnect_p
 * @brief            check reconnecting AP when already connected
 * @scenario         check reconnecting operation of AP when already connected
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_disconnect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_reconnect_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret = wifi_manager_connect_ap(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_connect_ap(&config);
	TC_ASSERT_NEQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_disconnect_ap(); wifi_manager_deinit());

	ret = wifi_manager_disconnect_ap();
	TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_connect_long_ssid_p
 * @brief            check connection with very long SSID
 * @scenario         check connection with very long SSID
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_connect_long_ssid_p(void)
{
	const char ssid[] = "qwertyuiopalokasdasdjasdkajshdukekukzhjdshdkjsdksjdsdadsasdasdasd";
	const char passphrase[] = "ashfdkjasfhkjsdhksahfkjdhsjfkhsdjahfkjashdfkjhasjkdhfkfdhksjafhsdaashdkjfksdhfkjashdjfhsajkdfh";

	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(ssid);
	config.passphrase_length = strlen(passphrase);
	strncpy(config.ssid, ssid, sizeof(config.ssid));
	strncpy(config.passphrase, passphrase, sizeof(config.passphrase));
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret = wifi_manager_connect_ap(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_INVALID_ARGS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_redisconnect_p
 * @brief            check disconnect operation if AP already disconnected
 * @scenario         check disconnect operation if AP already disconnected
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_disconnect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_redisconnect_p(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret = wifi_manager_connect_ap(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_disconnect_ap();
	TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
	WIFITEST_WAIT;

	ret = wifi_manager_disconnect_ap();
	TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_FAIL, wifi_manager_deinit());
	// wifitest_wait is not required here, as it is already disconnected

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_average_joining_ap
 * @brief            to check average time taken while connecting to AP
 * @scenario         check average time taken while connecting to AP
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_disconnect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_average_joining_ap(void)
{
	int i = 0;
	double average = 0.0f;
	struct timeval start_t;
	struct timeval end_t;
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	for (i = 1; i <= LOOP_SIZE; i++) {
		gettimeofday(&start_t, NULL);
		ret = wifi_manager_connect_ap(&config);
		TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
		WIFITEST_WAIT;
		gettimeofday(&end_t, NULL);
		average += diff_time(&start_t, &end_t);

		ret = wifi_manager_disconnect_ap();
		TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
		WIFITEST_WAIT;
	}

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	printf("\n[Average connect time: %.2f ms]\n", average / (LOOP_SIZE * 1000));
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_average_leaving_ap
 * @brief            to check average time taken while disconnecting to AP
 * @scenario         check average time taken while disconnecting to AP
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_disconnect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_average_leaving_ap(void)
{
	int i = 0;
	double average = 0.0f;
	struct timeval start_t;
	struct timeval end_t;
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	for (i = 1; i <= LOOP_SIZE; i++) {
		ret = wifi_manager_connect_ap(&config);
		TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
		WIFITEST_WAIT;

		gettimeofday(&start_t, NULL);
		ret = wifi_manager_disconnect_ap();
		TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
		WIFITEST_WAIT;
		gettimeofday(&end_t, NULL);
		average += diff_time(&start_t, &end_t);
	}

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	printf("\n[Average disconnect time: %.2f ms]\n", average / (LOOP_SIZE * 1000));
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_success_ratio_ap
 * @brief            to check success ratio of operations(init, join, leave, deinit)
 * @scenario         check success ratio of operations(init, join, leave, deinit)
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_disconnect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_success_ratio_ap(void)
{
	float init_cnt = 0.0f;
	float s_init_cnt = 0.0f;
	float join_cnt = 0.0f;
	float s_join_cnt = 0.0f;
	float leave_cnt = 0.0f;
	float s_leave_cnt = 0.0f;
	float deinit_cnt = 0.0f;
	float s_deinit_cnt = 0.0f;
	int i = 0;

	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	for (i = 0; i < LOOP_SIZE; i++) {
		init_cnt++;
		ret = wifi_manager_init(&wifi_callbacks);
		if (ret == WIFI_MANAGER_SUCCESS) {
			s_init_cnt++;
		} else {
			continue;
		}

		join_cnt++;
		ret = wifi_manager_connect_ap(&config);
		if (ret == WIFI_MANAGER_SUCCESS) {
			s_join_cnt++;
			WIFITEST_WAIT;

			leave_cnt++;
			ret = wifi_manager_disconnect_ap();
			if (ret == WIFI_MANAGER_SUCCESS) {
				s_leave_cnt++;
				WIFITEST_WAIT;
			}
		}

		deinit_cnt++;
		ret = wifi_manager_deinit();
		if (ret == WIFI_MANAGER_SUCCESS) {
			s_deinit_cnt++;
		}
	}

	printf("\nSuccess Ratio \n");
	if (init_cnt > 0) {
		printf("[Initialization Wifi] Success Ratio = %.2f%% \n", (s_init_cnt / init_cnt) * 100);
	}
	if (join_cnt > 0) {
		printf("[Connect Wifi] Success Ratio = %.2f%% \n", (s_join_cnt / join_cnt) * 100);
	}
	if (leave_cnt > 0) {
		printf("[Disconnect Wifi] Success Ratio = %.2f%% \n", (s_leave_cnt / leave_cnt) * 100);
	}
	if (deinit_cnt > 0) {
		printf("[Deinitialization Wifi] Success Ratio = %.2f%% \n", (s_deinit_cnt / deinit_cnt) * 100);
	}
	TC_ASSERT_EQ("itc_wifimanager_success_ratio_ap", s_init_cnt, init_cnt);
	TC_ASSERT_EQ("itc_wifimanager_success_ratio_ap", s_join_cnt, join_cnt);
	TC_ASSERT_EQ("itc_wifimanager_success_ratio_ap", s_leave_cnt, leave_cnt);
	TC_ASSERT_EQ("itc_wifimanager_success_ratio_ap", s_deinit_cnt, deinit_cnt);

	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_average_stoping_ap
 * @brief            to check average time taken while deinit wifi
 * @scenario         to check average time taken while deinit wifi
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_disconnect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_average_stoping_ap(void)
{
	int i = 0;
	double average = 0.0f;
	struct timeval start_t;
	struct timeval end_t;
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(TEST_SSID);
	config.passphrase_length = strlen(TEST_PASSWORD);
	strncpy(config.ssid, TEST_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, TEST_PASSWORD, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	for (i = 1; i <= LOOP_SIZE; i++) {
		ret = wifi_manager_init(&wifi_callbacks);
		TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

		ret = wifi_manager_connect_ap(&config);
		TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
		WIFITEST_WAIT;

		ret = wifi_manager_disconnect_ap();
		TC_ASSERT_EQ_CLEANUP("wifi_manager_disconnect_ap", ret, WIFI_MANAGER_SUCCESS, wifi_manager_deinit());
		WIFITEST_WAIT;

		gettimeofday(&start_t, NULL);
		ret = wifi_manager_deinit();
		TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
		gettimeofday(&end_t, NULL);
		average += diff_time(&start_t, &end_t);
	}

	printf("\n[Average wifi deinitialization time: %.2f ms]\n", average / (LOOP_SIZE * 1000));
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_init_deinit_n
 * @brief            initialize and de-initalization of wifi manager
 * @scenario         initialize and de-initalization of wifi manager
 * @apicovered       wifi_manager_init and wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_init_deinit_n(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(NULL);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_INVALID_ARGS);

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_FAIL);
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_connect_ap_n
 * @brief            connecting with invalid configuration parameters
 * @scenario         connecting with invalid configuration parameters
 * @apicovered       wifi_manager_init, wifi_manager_set_mode, wifi_manager_connect_ap, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_connect_ap_n(void)
{
	char ssid[] = "";
	char passphrase[] = "";

	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	wifi_manager_ap_config_s config;
	config.ssid_length = strlen(ssid);
	config.passphrase_length = strlen(passphrase);
	strncpy(config.ssid, ssid, config.ssid_length + 1);
	strncpy(config.passphrase, passphrase, config.passphrase_length + 1);
	config.ap_auth_type = (wifi_manager_ap_auth_type_e)TEST_AUTH_TYPE;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)TEST_CRYPTO_TYPE;
	printf("AP config: %s(%d), %s(%d), %d %d\n", config.ssid, config.ssid_length, config.passphrase, \
			config.passphrase_length, config.ap_auth_type, config.ap_crypto_type);

	ret = wifi_manager_connect_ap(&config);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_connect_ap", ret, WIFI_MANAGER_FAIL, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}

/**
 * @testcase         itc_wifimanager_connect_set_mode_n
 * @brief            set mode with NULL config
 * @scenario         set mode with NULL config
 * @apicovered       wifi_manager_set_mode, wifi_manager_init, wifi_manager_deinit
 * @precondition     none
 * @postcondition    none
 */
static void itc_wifimanager_connect_set_mode_n(void)
{
	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&wifi_callbacks);
	TC_ASSERT_EQ("wifi_manager_init", ret, WIFI_MANAGER_SUCCESS);

	ret = wifi_manager_set_mode(SOFTAP_MODE, NULL);
	TC_ASSERT_EQ_CLEANUP("wifi_manager_set_mode", ret, WIFI_MANAGER_INVALID_ARGS, wifi_manager_deinit());

	ret = wifi_manager_deinit();
	TC_ASSERT_EQ("wifi_manager_deinit", ret, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}
/**
 * @testcase         itc_wifimanager_get_stats_p
 * @brief            get net config information
 * @scenario         getting network config information
 * @apicovered       wifi_manager_get_stats
 * @precondition     none
 * @postcondition    none
 */

static void itc_wifimanager_get_stats_p(void)
{
	wifi_manager_result_e nRet = WIFI_MANAGER_INVALID_ARGS;
	wifi_manager_stats_s stats;
	nRet = wifi_manager_get_stats(&stats);
	if (nRet == WIFI_MANAGER_SUCCESS) {
		printf("===	====================================================================\n");
		printf("CONN    CONNFAIL    DISCONN    RECONN    SCAN    SOFTAP    JOIN    LEFT\n");
		printf("%-8d%-12d%-11d%-10d", stats.connect, stats.connectfail, stats.disconnect, stats.reconnect);
		printf("%-8d%-10d%-8d%-8d\n", stats.scan, stats.softap, stats.joined, stats.left);
		printf("=======================================================================\n");
	}
	TC_ASSERT_EQ("wifi_manager_get_stats_p", nRet, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}
/**
 * @testcase         itc_wifimanager_get_stats_n
 * @brief            get net config information for NULL ssid
 * @scenario         getting network config information for NULL ssid
 * @apicovered       wifi_manager_get_stats
 * @precondition     none
 * @postcondition    none
 */

static void itc_wifimanager_get_stats_n(void)
{
	wifi_manager_result_e nRet = WIFI_MANAGER_FAIL;
	nRet = wifi_manager_get_stats(NULL);
	TC_ASSERT_EQ("wifi_manager_get_stats", nRet, WIFI_MANAGER_INVALID_ARGS);
	TC_SUCCESS_RESULT();
}
/**
 * @testcase         itc_wifimanager_get_connected_config_p
 * @brief            get connected config information
 * @scenario         To get connected config information.
 * @apicovered       wifi_manager_get_connected_config
 * @precondition     none
 * @postcondition    none
 */

static void itc_wifimanager_get_connected_config_p(void)
{
	wifi_manager_result_e nRet = WIFI_MANAGER_FAIL;
	wifi_manager_ap_config_s apconfig;
	nRet = wifi_manager_get_connected_config(&apconfig);
	if (nRet == WIFI_MANAGER_SUCCESS) {
		printf("====================================\n");
		printf("SSID: %s\n", apconfig.ssid);
		printf("SECURITY TYPE: %d\n", apconfig.ap_auth_type);
		printf("CYPTO TYPE: %d\n", apconfig.ap_crypto_type);
		printf("====================================\n");
	}
	if (((strncmp(apconfig.ssid, TEST_SSID, strnlen(TEST_SSID)) != 0)) 
			|| (apconfig.ap_auth_type !=  TEST_AUTH_TYPE)
			|| (apconfig.ap_crypto_type != TEST_CRYPTO_TYPE)) {
		printf("\nConfig Mismatch nRet = %d \n", nRet);
		return;
	}

	TC_ASSERT_EQ("wifi_manager_get_connected_config", nRet, WIFI_MANAGER_SUCCESS);
	TC_SUCCESS_RESULT();
}
/**
 * @testcase         itc_wifimanager_get_connected_config_n
 * @brief            get connected config information when config file not present
 * @scenario         To get connected config information when there is no
 *                   connection.
 * @apicovered       wifi_manager_get_connected_config
 * @precondition     none
 * @postcondition    none
 */

static void itc_wifimanager_get_connected_config_n(void)
{
	wifi_manager_result_e nRet = WIFI_MANAGER_FAIL;
	wifi_manager_ap_config_s apconfig;

	nRet = unlink("/mnt/wifi_connected.conf");
	printf("\nThe return after unlink  is =%d \n", nRet);
	if (nRet != 0) {
		wifi_manager_deinit();
		return;
	}

	nRet = wifi_manager_get_connected_config(&apconfig);
	if (nRet == WIFI_MANAGER_SUCCESS) {
		printf("====================================\n");
		printf("SSID: %s\n", apconfig.ssid);
		printf("SECURITY TYPE: %d\n", apconfig.ap_auth_type);
		printf("CYPTO TYPE: %d\n", apconfig.ap_crypto_type);
		printf("====================================\n");
	}
	TC_ASSERT_EQ("wifi_manager_get_connected_config", nRet, WIFI_MANAGER_FAIL);
	TC_SUCCESS_RESULT();
}

/*******************************************************************************************************************/

int wifi_manager_itc(int argc, FAR char *argv[])
{
	if (testcase_state_handler(TC_START, "WiFiManager ITC") == ERROR) {
		return ERROR;
	}

	itc_wifimanager_init_deinit_p();

	itc_wifimanager_set_mode_p();

	itc_wifimanager_get_mode_p(); // set softap mode inside this function
	itc_wifimanager_connect_disconnect_ap_p(); // change to station mode and try to connect to ap

	itc_wifimanager_scan_ap_p(); // Reinitialized wifi manager with the callback hander for scan results

	itc_wifimanager_reinit_p();

	itc_wifimanager_redeinit_p();

	itc_wifimanager_redisconnect_p();

	itc_wifimanager_connect_long_ssid_p();
	itc_wifimanager_average_joining_ap(); // Disable DoS preventation from Access Point

	itc_wifimanager_average_leaving_ap(); // Disable DoS preventation from Access Point

	itc_wifimanager_average_stoping_ap(); // Disable DoS preventation from Access Point

	itc_wifimanager_success_ratio_ap();
	itc_wifimanager_init_deinit_n();

	itc_wifimanager_connect_ap_n();

	itc_wifimanager_connect_set_mode_n();

	itc_wifimanager_connect_ap_config_p();

	itc_wifimanager_save_get_config_p();

	itc_wifimanager_remove_get_config_p();

	itc_wifimanager_get_config_n();

	itc_wifimanager_connect_ap_config_n();

	itc_wifimanager_reremove_n();

	itc_wifimanager_get_connected_config_n();
	itc_wifimanager_connect_ap_config_p();//To make correct connection
	itc_wifimanager_get_stats_p();
	itc_wifimanager_get_stats_n();
	itc_wifimanager_get_connected_config_p();
	//itc_wifimanager_get_connected_config_n();



	itc_wifimanager_reconnect_p(); // System is crashing tested manually

	(void)testcase_state_handler(TC_END, "WiFiManager ITC");

	return 0;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int itc_wifimanager_main(int argc, char *argv[])
#endif
{
	int ssid_len = strlen(TEST_SSID);
	int passphrase_len = strlen(TEST_PASSWORD);
	int softap_ssid_len = strlen(TEST_SOFTAP_SSID);
	int softap_passphrase_len = strlen(TEST_SOFTAP_PASSWORD);

	if ((ssid_len >= SSID_MAX_LEN) || (softap_ssid_len >= SSID_MAX_LEN) || (passphrase_len >= PASSPHRASE_MAX_LEN) || (softap_passphrase_len >= PASSPHRASE_MAX_LEN)) {
		printf("AP or SoftAP configuration fails: too long ssid or passphrase\n");
		printf("Make sure that length of SSID < SSID_MAX_LEN and length of passphrase < PASSPHRASE_MAX_LEN\n");
		return -1;
	}

	int res = wifi_test_signal_init();
	if (res < 0) {
		return -1;
	}

	wifi_manager_itc(argc, argv);

	wifi_test_signal_deinit();
	return 0;
}
