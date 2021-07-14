/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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

//***************************************************************************
// Included Files
//***************************************************************************

/* */

#ifndef MPLINUX

#include <tinyara/config.h>
#include <tinyara/init.h>
#include <tinyara/signal.h>

#include <wifi_manager/wifi_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <protocols/ntpclient.h>

#include "AraPlayer.h"
#include "AudioCommon.h"

#define SSID "DENT_5G"
#define PASSWORD "jonbeo1#"
#define AUTH_TYPE WIFI_MANAGER_AUTH_WPA2_PSK
#define CRYPTO_TYPE WIFI_MANAGER_CRYPTO_TKIP_AND_AES

const char *radio_url = "http://111.119.28.213/stream/bbcmedia_radio2_mf_q?s=1555372806&e=1555387206&h=345be035221c0f70351b9c1f66617dfa";
const char *local_url = "/rom/over_16000.mp3";
const pid_t appPid = 1;

/*
 *Callback
 */
/* */ static bool g_wifi_connected = false;

void __wm_sta_connected(wifi_manager_result_e res)
{
	printf(" T%d --> %s res(%d)\n", getpid(), __FUNCTION__, res);

	g_wifi_connected = true;
}

void __wm_sta_disconnected(wifi_manager_disconnect_e disconn)
{
	printf(" T%d --> %s\n", getpid(), __FUNCTION__);

	g_wifi_connected = false;
}

void __wm_softap_sta_join(void)
{
	printf(" T%d --> %s\n", getpid(), __FUNCTION__);
}

void __wm_softap_sta_leave(void)
{
	printf(" T%d --> %s\n", getpid(), __FUNCTION__);
}

static wifi_manager_cb_s multiroom_wifi_callbacks = {
	__wm_sta_connected,
	__wm_sta_disconnected,
	__wm_softap_sta_join,
	__wm_softap_sta_leave,
	NULL,
};

void __wm_connect(void)
{
	wifi_manager_ap_config_s apconfig;
	strncpy(apconfig.ssid, SSID, sizeof(SSID));
	apconfig.ssid_length = strlen(apconfig.ssid);
	apconfig.ap_auth_type = AUTH_TYPE;
	if (apconfig.ap_auth_type != WIFI_MANAGER_AUTH_OPEN) {
		strncpy(apconfig.passphrase, PASSWORD, sizeof(PASSWORD));
		apconfig.passphrase_length = strlen(apconfig.passphrase);
		apconfig.ap_crypto_type = CRYPTO_TYPE;
	}

	wifi_manager_result_e res = wifi_manager_connect_ap(&apconfig);
	if (res != WIFI_MANAGER_SUCCESS) {
		printf(" AP connect failed\n");
		return;
	}
	/* Wait for DHCP connection */
	printf(" wait join done [%s]\n", apconfig.ssid);
}

void __wm_sta_start(void)
{
	wifi_manager_result_e res = WIFI_MANAGER_SUCCESS;

	res = wifi_manager_set_mode(STA_MODE, NULL);
	if (res != WIFI_MANAGER_SUCCESS) {
		printf(" Set STA mode Fail\n");
		return;
	}
	printf(" Connecting to AP\n");
}

void wifi_start(void)
{
	wifi_manager_result_e res;

	res = wifi_manager_init(&multiroom_wifi_callbacks);
	if (res != WIFI_MANAGER_SUCCESS) {
		printf(" wifi_manager_init fail\n");
	}
	__wm_sta_start();
	__wm_connect();
	while (g_wifi_connected == false) {
		WAITMS(1000);
	}

	return;
}

void menu()
{
	std::cout << "=== BBC Radio ===" << std::endl;
	std::cout << "1. play Radio" << std::endl;
	std::cout << "2. pause Radio" << std::endl;
	std::cout << "3. resume Radio" << std::endl;
	std::cout << "4. stop Radio" << std::endl;

	std::cout << "=== Local File ===" << std::endl;
	std::cout << "5. play Local" << std::endl;
	std::cout << "6. pause Local" << std::endl;
	std::cout << "7. resume Local" << std::endl;
	std::cout << "8. stop Local" << std::endl;

	std::cout << "=== SBS Live Radio ===" << std::endl;
	std::cout << "9. play Radio" << std::endl;
	std::cout << "10. stop Radio" << std::endl;
}

int userInput(int min, int max)
{
	assert(min <= max);
	int input = 0;

	std::cin >> input;
	std::cout << std::endl;

	if (!std::cin.fail()) {
		if (min <= input && input <= max) {
			std::cout << "return input" << std::endl;
			return input;
		}
	}

	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	std::cout << "Invalid Input, please try again" << std::endl;

	return input;
}

extern "C" {
int AraPlayerService_main(int argc, char *argv[])
{
	int input = 0;
	wifi_start();

	auto instance = std::make_shared<AraPlayer>();
	instance->init();
	//TO-DO will change (example -> service(use binary manager))
	while (1) {
		menu();
		input = userInput(1, 8);
		printf("input = %d\n", input);
		ara_player_msg_t *msg = new ara_player_msg_t;
		switch (input) {
		case 1: {
			memset(msg, 0x00, sizeof(ara_player_msg_t));
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_START;
			msg->streamType = STREAM_HTTP_PROGRESSIVE;
			memcpy(msg->url, radio_url, strlen(radio_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 2: {
			memset(msg, 0x00, sizeof(ara_player_msg_t));
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_PAUSE;
			msg->streamType = STREAM_HTTP_PROGRESSIVE;
			memcpy(msg->url, radio_url, strlen(radio_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 3: {
			memset(msg, 0x00, sizeof(ara_player_msg_t));
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_RESUME;
			msg->streamType = STREAM_HTTP_PROGRESSIVE;
			memcpy(msg->url, radio_url, strlen(radio_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 4: {
			memset(msg, 0x00, sizeof(ara_player_msg_t));
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_STOP;
			msg->streamType = STREAM_HTTP_PROGRESSIVE;
			memcpy(msg->url, radio_url, strlen(radio_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 5: {
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_START;
			msg->streamType = STREAM_LOCAL;
			memcpy(msg->url, local_url, strlen(local_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 6: {
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_PAUSE;
			msg->streamType = STREAM_LOCAL;
			memcpy(msg->url, local_url, strlen(local_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 7: {
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_RESUME;
			msg->streamType = STREAM_LOCAL;
			memcpy(msg->url, local_url, strlen(local_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 8: {
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_STOP;
			msg->streamType = STREAM_LOCAL;
			memcpy(msg->url, local_url, strlen(local_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 9: {
			memset(msg, 0x00, sizeof(ara_player_msg_t));
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_START;
			msg->streamType = STREAM_HTTP_HLS;
			memcpy(msg->url, radio_url, strlen(radio_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		case 10: {
			memset(msg, 0x00, sizeof(ara_player_msg_t));
			msg->priority = PRIORITY_NORMAL;
			msg->cmdType = CMD_STOP;
			msg->streamType = STREAM_HTTP_HLS;
			memcpy(msg->url, radio_url, strlen(radio_url) + 1);
			instance->execute(appPid, msg);
		}
		break;
		default:
			break;
		}
	}

	printf("exit\n");
	return 0;
}
}
#endif
