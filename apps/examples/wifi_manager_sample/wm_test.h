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
#pragma once

struct wt_options;
typedef void (*test_func)(void *arg);

struct wt_options {
	test_func func;
	uint16_t channel;
	char *ssid;
	char *bad_ssid;
	char *password;
	char *bad_password;
	wifi_manager_ap_auth_type_e    auth_type;
	wifi_manager_ap_crypto_type_e  crypto_type;
	char *softap_ssid;
	char *softap_password;
	int softap_channel;
	int scan_specific;
	uint32_t stress_tc_idx;
	uint8_t power_mode;
};
