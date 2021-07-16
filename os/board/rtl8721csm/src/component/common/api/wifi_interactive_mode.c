/******************************************************************************
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include <tinyara/config.h>
#include <tinyara/wifi/wifi_common.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <debug.h>
#include <lwip/netif.h>

#include "osdep_service.h"
#include "wifi_conf.h"
#include "wifi_util.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcpip.h"
#include "rtk_wifi_utils.h"
#include <tinyara/net/if/wifi.h>

#define vTaskDelay(t) usleep(t*1000)

#ifndef CONFIG_INTERACTIVE_EXT
#define CONFIG_INTERACTIVE_EXT 0
#endif
#ifndef CONFIG_SSL_CLIENT
#define CONFIG_SSL_CLIENT 0
#endif

#ifndef CONFIG_GOOGLENEST
#define CONFIG_GOOGLENEST 0
#endif
#ifndef CONFIG_OTA_UPDATE
#define CONFIG_OTA_UPDATE 0
#endif
#ifndef CONFIG_BSD_TCP
#define CONFIG_BSD_TCP 0
#endif
#define CONFIG_JD_SMART 0
#ifndef CONFIG_ENABLE_P2P
#define CONFIG_ENABLE_P2P 0
#endif
#define SCAN_WITH_SSID 1

#ifdef CONFIG_WPS
#define STACKSIZE 1280
#else
#define STACKSIZE 1024
#endif

#ifndef WLAN0_NAME
#define WLAN0_NAME "wlan0"
#endif
#ifndef WLAN1_NAME
#define WLAN1_NAME "wlan1"
#endif
/* Give default value if not defined */
#ifndef NET_IF_NUM
#ifdef CONFIG_CONCURRENT_MODE
#define NET_IF_NUM 2
#else
#define NET_IF_NUM 1
#endif
#endif

/*Static IP ADDRESS*/
#ifndef IP_ADDR0
#define IP_ADDR0 192
#define IP_ADDR1 168
#define IP_ADDR2 1
#define IP_ADDR3 80
#endif

/*NETMASK*/
#ifndef NETMASK_ADDR0
#define NETMASK_ADDR0 255
#define NETMASK_ADDR1 255
#define NETMASK_ADDR2 255
#define NETMASK_ADDR3 0
#endif

/*Gateway Address*/
#ifndef GW_ADDR0
#define GW_ADDR0 192
#define GW_ADDR1 168
#define GW_ADDR2 1
#define GW_ADDR3 1
#endif

/*Static IP ADDRESS*/
#ifndef AP_IP_ADDR0
#define AP_IP_ADDR0 192
#define AP_IP_ADDR1 168
#define AP_IP_ADDR2 43
#define AP_IP_ADDR3 1
#endif

/*NETMASK*/
#ifndef AP_NETMASK_ADDR0
#define AP_NETMASK_ADDR0 255
#define AP_NETMASK_ADDR1 255
#define AP_NETMASK_ADDR2 255
#define AP_NETMASK_ADDR3 0
#endif

/*Gateway Address*/
#ifndef AP_GW_ADDR0
#define AP_GW_ADDR0 192
#define AP_GW_ADDR1 168
#define AP_GW_ADDR2 43
#define AP_GW_ADDR3 1
#endif

static void cmd_help(int argc, char **argv);
#if CONFIG_SSL_CLIENT
extern void cmd_ssl_client(int argc, char **argv);
#endif

#if CONFIG_GOOGLENEST
extern void cmd_googlenest(int argc, char **argv);
#endif
#if CONFIG_JD_SMART
extern void cmd_jd_smart(int argc, char **argv);
#endif

#if CONFIG_BSD_TCP
extern void cmd_tcp(int argc, char **argv);
extern void cmd_udp(int argc, char **argv);
#endif

#if CONFIG_WLAN
int8_t cmd_wifi_on(WiFi_InterFace_ID_t interface_id);
int8_t cmd_wifi_off(void);
int8_t cmd_wifi_disconnect(void);
#ifdef CONFIG_PROMISC
extern void cmd_promisc(int argc, char **argv);
#endif
extern void cmd_simple_config(int argc, char **argv);

#if CONFIG_OTA_UPDATE
extern void cmd_update(int argc, char **argv);
#endif

#if CONFIG_WEBSERVER
extern void start_web_server(void);
extern void stop_web_server(void);
#endif
extern void cmd_app(int argc, char **argv);
#if CONFIG_ENABLE_WPS
extern void cmd_wps(int argc, char **argv);
#endif
#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
extern void cmd_ap_wps(int argc, char **argv);
extern int wpas_wps_dev_config(u8 *dev_addr, u8 bregistrar);
#endif //CONFIG_ENABLE_WPS_AP
#if CONFIG_ENABLE_P2P
extern void cmd_wifi_p2p_start(int argc, char **argv);
extern void cmd_wifi_p2p_stop(int argc, char **argv);
extern void cmd_p2p_listen(int argc, char **argv);
extern void cmd_p2p_find(int argc, char **argv);
extern void cmd_p2p_peers(int argc, char **argv);
extern void cmd_p2p_info(int argc, char **argv);
extern void cmd_p2p_disconnect(int argc, char **argv);
extern void cmd_p2p_connect(int argc, char **argv);
#endif //CONFIG_ENABLE_P2P
#if defined(CONFIG_RTL8195A) || defined(CONFIG_RTL8711B) || defined(CONFIG_RTL8721D)
extern u32 CmdDumpWord(IN u16 argc, IN u8 *argv[]);
extern u32 CmdWriteWord(IN u16 argc, IN u8 *argv[]);
#endif
#ifdef CONFIG_CONCURRENT_MODE
static void cmd_wifi_sta_and_ap(int argc, char **argv)
{
	int timeout = 20; //, mode;
	int channel;

	if ((argc != 3) && (argc != 4)) {
		ndbg("\n\rUsage: wifi_ap SSID CHANNEL [PASSWORD]");
		return;
	}

	if (atoi((const char *)argv[2]) > 14) {
		ndbg("\n\r bad channel!Usage: wifi_ap SSID CHANNEL [PASSWORD]");
		return;
	}
        
#if 0
	//Check mode
	wext_get_mode(WLAN0_NAME, &mode);

	switch(mode) {
		case IW_MODE_MASTER:	//In AP mode
			cmd_wifi_off(0, NULL);
			cmd_wifi_on(0, NULL);
			break;
		case IW_MODE_INFRA:		//In STA mode
			if(wext_get_ssid(WLAN0_NAME, ssid) > 0)
				cmd_wifi_disconnect(0, NULL);
	}
#endif
	wifi_off();

#if defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
	//TODO
	//delay 20 Ticks
#else
	rtw_msleep_os(20);
#endif
	if (wifi_on(RTW_MODE_STA_AP) < 0) {
		ndbg("\n\rERROR: Wifi on failed!");
		return;
	}

	nvdbg("\n\rStarting AP ...");
	channel = atoi((const char *)argv[2]);
	if (channel > 13) {
		ndbg("\n\rChannel is from 1 to 13. Set channel 1 as default!\n");
		channel = 1;
	}

	if (argc == 4) {
		if (wifi_start_ap(argv[1],
						  RTW_SECURITY_WPA2_AES_PSK,
						  argv[3],
						  strlen((const char *)argv[1]),
						  strlen((const char *)argv[3]),
						  channel) != RTW_SUCCESS) {
			ndbg("\n\rERROR: Operation failed!");
			return;
		}
	} else {
		if (wifi_start_ap(argv[1],
						  RTW_SECURITY_OPEN,
						  NULL,
						  strlen((const char *)argv[1]),
						  0,
						  channel) != RTW_SUCCESS) {
			ndbg("\n\rERROR: Operation failed!");
			return;
		}
	}

	while (1) {
		char essid[33];

		if (wext_get_ssid(WLAN1_NAME, (unsigned char *)essid) > 0) {
			if (strncmp((const char *)essid, (const char *)argv[1], sizeof(essid)) == 0) {
				nvdbg("\n\r%s started", argv[1]);
				break;
			}
		}

		if (timeout == 0) {
			ndbg("\n\rERROR: Start AP timeout!");
			break;
		}

#if defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
	//TODO
	//Delay 1s
#else
		rtw_msleep_os(1);
#endif
		timeout--;
	}
}
#endif

static int _find_ap_from_scan_buf(char*buf, int buflen, char *target_ssid, void *user_data)
{
	rtw_wifi_setting_t *pwifi = (rtw_wifi_setting_t *)user_data;
	int plen = 0;
	while(plen < buflen){
		u8 len, ssid_len, security_mode;
		char *ssid;

		// len offset = 0
		len = (int)*(buf + plen);
		// check end
		if (len == 0) break;
		// ssid offset = 14
		ssid_len = len - 14;
		ssid = buf + plen + 14 ;
		if ((ssid_len == strlen(target_ssid))
			&& (!memcmp(ssid, target_ssid, ssid_len)))
		{
			strcpy((char*)pwifi->ssid, target_ssid);
			// channel offset = 13
			pwifi->channel = *(buf + plen + 13);
			// security_mode offset = 11
			security_mode = (u8)*(buf + plen + 11);
			if (security_mode == IW_ENCODE_ALG_NONE)
				pwifi->security_type = RTW_SECURITY_OPEN;
			else if (security_mode == IW_ENCODE_ALG_WEP)
				pwifi->security_type = RTW_SECURITY_WEP_PSK;
			else if (security_mode == IW_ENCODE_ALG_CCMP)
				pwifi->security_type = RTW_SECURITY_WPA2_AES_PSK;
			break;
		}
		plen += len;
	}
	return 0;
}

static int _get_ap_security_mode(IN char * ssid)
{
	rtw_wifi_setting_t wifi;
	u32 scan_buflen = 1000;

	memset(&wifi, 0, sizeof(wifi));
	if (wifi_scan_networks_with_ssid(_find_ap_from_scan_buf, (void*)&wifi, scan_buflen, ssid, strlen(ssid)) != RTW_SUCCESS) {
		ndbg("Wifi active scan failed!\n");
		return RTW_SECURITY_UNKNOWN;
	}
	if (strncmp((char *)wifi.ssid, ssid, sizeof(wifi.ssid)) == 0) {
		//*security_mode = wifi.security_type;
		//*channel = wifi.channel;
		return wifi.security_type;
	}
	return RTW_SECURITY_UNKNOWN;
}


void dhcpd_event(void)
{
	return;
}

int8_t cmd_wifi_ap(trwifi_softap_config_s *softap_config)
{
	int ret = 0;
	rtw_security_t security_type;
	char *password;

	if (rltk_wlan_running(WLAN0_IDX)) {
		if (wifi_set_mode(RTW_MODE_AP) < 0) {
			ndbg("\n\rERROR: Wifi Set Mode to SoftAP failed!");
			return -1;
		}
	} else {
		wifi_off();
#if defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
		//TODO
		//Delay 20 Ticks
#else
		rtw_msleep_os(20);
#endif
		if (wifi_on(RTW_MODE_AP) < 0) {
			ndbg("\n\rERROR: Wifi on failed!");
			return -1;
		}
	}
	nvdbg("\n\rStarting AP ...");

	switch (softap_config->ap_auth_type) {
	case WIFI_UTILS_AUTH_OPEN:
		security_type = RTW_SECURITY_OPEN;
		password = NULL;
		break;
	case WIFI_UTILS_AUTH_WPA2_PSK:
		security_type = RTW_SECURITY_WPA2_AES_PSK;
		password = softap_config->passphrase;
		break;
	case WIFI_UTILS_AUTH_WPA3_PSK:
		security_type = RTW_SECURITY_WPA3_AES_PSK;
		password = softap_config->passphrase;
		break;
	case WIFI_UTILS_AUTH_UNKNOWN:
	default:
		ndbg("\r\nAP AUTH type is unknown %d;\n", softap_config->ap_auth_type);
		return WIFI_UTILS_INVALID_ARGS;
	}

	if (softap_config->channel > 14 || softap_config->channel < 1) {
		ndbg("\r\nAP channel is wrong: %d;\n", softap_config->channel);
		return WIFI_UTILS_INVALID_ARGS;
	}

	if (softap_config->ssid_length < 1) {
		ndbg("\r\nAP ssid_length is wrong: %s,	%d;\n", softap_config->ssid, softap_config->ssid_length);
		return WIFI_UTILS_INVALID_ARGS;
	}

	if (softap_config->ap_auth_type != TRWIFI_AUTH_OPEN && softap_config->passphrase_length < 1) {
		ndbg("\r\nAP passphrase_length is wrong: %s,  %d;\n", softap_config->passphrase, softap_config->passphrase_length);
		return WIFI_UTILS_INVALID_ARGS;
	}

	if (wifi_start_ap(softap_config->ssid,
					  security_type,
					  password,
					  softap_config->ssid_length,
					  softap_config->passphrase_length,
					  softap_config->channel) != RTW_SUCCESS) {
		ndbg("\n\rERROR: Operation failed!");
		return -1;
	}

	nvdbg("\r\nap start");

	return ret;
}

int8_t cmd_wifi_connect(trwifi_ap_config_s *ap_connect_config, void *arg)
{
	int ret;

	char *ssid;
	rtw_security_t security_type = -1;
	char *password;
	int ssid_len;
	int password_len;
	int key_id = 0;
	void *semaphore;
	int security_retry_count = 0;

	wifi_utils_ap_auth_type_e auth = ap_connect_config->ap_auth_type;
	wifi_utils_ap_crypto_type_e crypto = ap_connect_config->ap_crypto_type;
	ssid = ap_connect_config->ssid;
	switch (auth) {
	case WIFI_UTILS_AUTH_OPEN:
		security_type = RTW_SECURITY_OPEN;
		password = NULL;
		ssid_len = strlen((const char *)ssid);
		password_len = 0;
		semaphore = NULL;
		break;
	case WIFI_UTILS_AUTH_WEP_SHARED:
		security_type = RTW_SECURITY_WEP_PSK;
		password = ap_connect_config->passphrase;
		ssid_len = strlen((const char *)ssid);
		password_len = ap_connect_config->passphrase_length;
		if ((password_len != 5) && (password_len != 13) && (password_len != 10) && (password_len != 26)) {
			ndbg("\n\rWrong WEP key length. Must be 5 or 13 ASCII characters, or 10 or 26 for HEX password.");
			return -1;
		}
		semaphore = NULL;
		break;
	case WIFI_UTILS_AUTH_WPA2_PSK:
		security_type = RTW_SECURITY_WPA2_AES_PSK;
		password = ap_connect_config->passphrase;
		ssid_len = strlen((const char *)ssid);
		password_len = ap_connect_config->passphrase_length;
		semaphore = NULL;
		break;
	case WIFI_UTILS_AUTH_WPA3_PSK:
		security_type = RTW_SECURITY_WPA3_AES_PSK;
		password = ap_connect_config->passphrase;
		ssid_len = strlen((const char *)ssid);
		password_len = ap_connect_config->passphrase_length;
		semaphore = NULL;
		break;
	default:
		while (1) {
			security_type = (_get_ap_security_mode((char*)ssid));
			if (security_type >= 0)	
				break;

			security_retry_count++;
			if (security_retry_count >= 3) {
				ndbg("Can't get AP security mode.\n");
				break;
			}
		}
		password = ap_connect_config->passphrase;
		ssid_len = strlen((const char *)ssid);
		password_len = ap_connect_config->passphrase_length;
		semaphore = NULL;
		break;
	}

	if (wifi_on(RTW_MODE_STA) < 0) {
		ndbg("\n\rERROR: Wifi on failed!");
		return -1;
	}

	ret = wifi_connect(ssid,
					   security_type,
					   password,
					   ssid_len,
					   password_len,
					   key_id,
					   semaphore);

	if (ret) {
		ndbg("\n\rrtk_wifi_connect failed\n");
		return -1;
	}

	return 0;
}

int8_t cmd_wifi_connect_bssid(int argc, char **argv)
{
	int ret = RTW_ERROR;
	//unsigned long tick1 = xTaskGetTickCount();
	//unsigned long tick2, tick3;
	int mode;
	unsigned char bssid[ETH_ALEN];
	char *ssid = NULL;
	rtw_security_t security_type;
	char *password;
	int bssid_len;
	int ssid_len = 0;
	int password_len;
	int key_id;
	void *semaphore;
	u32 mac[ETH_ALEN];
	u32 i;
	u32 index = 0;

	if ((argc != 3) && (argc != 4) && (argc != 5) && (argc != 6)) {
		ndbg("\n\rUsage: wifi_connect_bssid 0/1 [SSID] BSSID / xx:xx:xx:xx:xx:xx [WPA PASSWORD / (5 or 13) ASCII WEP KEY] [WEP KEY ID 0/1/2/3]");
		return -1;
	}

	//Check if in AP mode
	wext_get_mode(WLAN0_NAME, &mode);

	if (mode == IW_MODE_MASTER) {
		wifi_off();
		vTaskDelay(20);
		if (wifi_on(RTW_MODE_STA) < 0) {
			ndbg("\n\rERROR: Wifi on failed!");
			return -1;
		}
	}
	//check ssid
	if (memcmp(argv[1], "0", 1)) {
		index = 1;
		ssid_len = strlen((const char *)argv[2]);
		if ((ssid_len <= 0) || (ssid_len > 32)) {
			ndbg("\n\rWrong ssid. Length must be less than 32.");
			return -1;
		}
		ssid = argv[2];
	}
	sscanf(argv[2 + index], MAC_FMT, mac, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5);
	for (i = 0; i < ETH_ALEN; i++)
		bssid[i] = (u8)mac[i] & 0xFF;

	if (argc == 3 + index) {
		security_type = RTW_SECURITY_OPEN;
		password = NULL;
		bssid_len = ETH_ALEN;
		password_len = 0;
		key_id = 0;
		semaphore = NULL;
	} else if (argc == 4 + index) {
		security_type = RTW_SECURITY_WPA2_AES_PSK;
		password = argv[3 + index];
		bssid_len = ETH_ALEN;
		password_len = strlen((const char *)argv[3 + index]);
		key_id = 0;
		semaphore = NULL;
	} else {
		security_type = RTW_SECURITY_WEP_PSK;
		password = argv[3 + index];
		bssid_len = ETH_ALEN;
		password_len = strlen((const char *)argv[3 + index]);
		key_id = atoi(argv[4 + index]);
		if ((password_len != 5) && (password_len != 13)) {
			ndbg("\n\rWrong WEP key length. Must be 5 or 13 ASCII characters.");
			return -1;
		}
		if ((key_id < 0) || (key_id > 3)) {
			ndbg("\n\rWrong WEP key id. Must be one of 0,1,2, or 3.");
			return -1;
		}
		semaphore = NULL;
	}

	ret = wifi_connect_bssid(bssid,
							 ssid,
							 security_type,
							 password,
							 bssid_len,
							 ssid_len,
							 password_len,
							 key_id,
							 semaphore);

	if (ret != RTW_SUCCESS) {
		ndbg("\n\rERROR: Operation failed!");
		return -1;
	} else {
		nvdbg("\r\nConnected\n");
	}

	return 0;
}

int8_t cmd_wifi_disconnect(void)
{
	int timeout = 20;
	char essid[33];

	nvdbg("\n\rDeassociating AP ...");

	if (wext_get_ssid(WLAN0_NAME, (unsigned char *)essid) < 0) {
		nvdbg("\n\rWIFI disconnected");
		return 0;
	}

	if (wifi_disconnect() < 0) {
		ndbg("\n\rERROR: Operation failed!");
		return -1;
	}

	while (1) {
		if (wext_get_ssid(WLAN0_NAME, (unsigned char *)essid) < 0) {
			nvdbg("\n\rWIFI disconnected");
			return 0;
		}

		if (timeout == 0) {
			ndbg("\n\rERROR: Deassoc timeout!");
			return -1;
		}

#if defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
		//TODO
		//Delay 1 Tick
#else
		rtw_msleep_os(1);
#endif
		timeout--;
	}
}

void cmd_wifi_info(int argc, char **argv)
{
	int i = 0;
	u8 *ifname[2] = {(unsigned char*)WLAN0_NAME, (unsigned char*)WLAN1_NAME};
#ifdef CONFIG_MEM_MONITOR
	extern int min_free_heap_size;
#endif

	rtw_wifi_setting_t setting;
	for (i = 0; i < NET_IF_NUM; i++) {
		if (rltk_wlan_running(i)) {
			nvdbg("\n\r\nWIFI %s Status: Running", ifname[i]);
			nvdbg("\n\r==============================");

			rltk_wlan_statistic(i);

			wifi_get_setting((const char *)ifname[i], &setting);
			wifi_show_setting((const char *)ifname[i], &setting);

			if (setting.mode == RTW_MODE_AP || i == 1) {
				int client_number;
				struct {
					int count;
					rtw_mac_t mac_list[AP_STA_NUM];
				} client_info;

				client_info.count = AP_STA_NUM;
				wifi_get_associated_client_list(&client_info, sizeof(client_info));

				nvdbg("\n\rAssociated Client List:");
				nvdbg("\n\r==============================");

				if (client_info.count == 0)
					nvdbg("\n\rClient Num: 0\n\r");
				else {
					nvdbg("\n\rClient Num: %d", client_info.count);
					for (client_number = 0; client_number < client_info.count; client_number++) {
						nvdbg("\n\rClient [%d]:", client_number);
						nvdbg("\n\r\tMAC => " MAC_FMT "",
							   MAC_ARG(client_info.mac_list[client_number].octet));
					}
					ndbg("\n\r");
				}
			}

			{
				int error = wifi_get_last_error();
				ndbg("\n\rLast Link Error");
				ndbg("\n\r==============================");
				switch (error) {
				case RTW_NO_ERROR:
					ndbg("\n\r\tNo Error");
					break;
				case RTW_NONE_NETWORK:
					ndbg("\n\r\tTarget AP Not Found");
					break;
				case RTW_CONNECT_FAIL:
					ndbg("\n\r\tAssociation Failed");
					break;
				case RTW_WRONG_PASSWORD:
					ndbg("\n\r\tWrong Password");
					break;
				case RTW_DHCP_FAIL:
					ndbg("\n\r\tDHCP Failed");
					break;
				default:
					ndbg("\n\r\tUnknown Error(%d)", error);
				}
				ndbg("\n\r");
			}
		}
	}

#if defined(configUSE_TRACE_FACILITY) && (configUSE_TRACE_FACILITY == 1)
	{
		signed char pcWriteBuffer[1024];
		vTaskList((char *)pcWriteBuffer);
		nvdbg("\n\rTask List: \n%s", pcWriteBuffer);
	}
#endif
#ifdef CONFIG_MEM_MONITOR
	nvdbg("\n\rMemory Usage");
	nvdbg("\n\r==============================");
	nvdbg("\r\nMin Free Heap Size:  %d", min_free_heap_size);
#endif
}

int8_t cmd_wifi_on(WiFi_InterFace_ID_t interface_id)
{
	int ret;

	if (rltk_wlan_running(WLAN0_IDX)) {
		if (wifi_set_mode(RTW_MODE_STA) < 0) {
			ndbg("\n\rERROR: Wifi Set Mode to STA failed!\n");
			return RTK_STATUS_ERROR;
		}
	} else {
		/* Kill init thread after all init tasks done */
		ret = wifi_on(RTW_MODE_STA);
		if (ret != RTW_SUCCESS) {
			ndbg("\n\rrtk_wifi_start failed\n");
			return RTK_STATUS_ERROR;
		}
		nvdbg("\r\n===============>>wifi_on success!!\r\n");
	}

#if RTW_AUTO_RECONNECT
	//setup reconnection flag
	wifi_set_autoreconnect(1);
#endif

	rtw_wifi_setting_t setting;
	wifi_get_setting(WLAN0_NAME, &setting);
	wifi_show_setting(WLAN0_NAME, &setting);

	nvdbg("\r\n===============>>Finish wifi_on!!\r\n");
	return RTK_STATUS_SUCCESS;
}

int8_t cmd_wifi_off(void)
{
	int mode;

#if CONFIG_WEBSERVER
	stop_web_server();
#endif
#if CONFIG_ENABLE_P2P
	cmd_wifi_p2p_stop(0, NULL);
#else
	if ((!rltk_wlan_running(WLAN0_IDX)) &&
		(!rltk_wlan_running(WLAN1_IDX))) {
		RTW_API_INFO("\n\rWIFI is not running");
		return 0;
	}
	wext_get_mode(WLAN0_NAME, &mode);
	if (mode == IW_MODE_MASTER) {
		RTW_API_INFO("\n\rWIFI Mode Change: AP, disable beacon\r\n");
		wext_set_mode(WLAN0_NAME, IW_MODE_INFRA);
		rtw_msleep_os(50);
	} else if (mode == IW_MODE_INFRA) {
		unsigned char ssid[33];
		if (wext_get_ssid(WLAN0_NAME, ssid) > 0){
			RTW_API_INFO("\n\rWIFI Mode Change: STA, disconnecting\r\n");
			wifi_disconnect();
		}
	}
	RTW_API_INFO("\n\rWIFI Mode Change instead of WIFI reload\r\n");
	return 0;
#endif
}

static void print_scan_result(rtw_scan_result_t *record)
{
	RTW_API_INFO("%s\t ", (record->bss_type == RTW_BSS_TYPE_ADHOC) ? "Adhoc" : "Infra");
	RTW_API_INFO(MAC_FMT, MAC_ARG(record->BSSID.octet));
	RTW_API_INFO(" %d\t ", record->signal_strength);
	RTW_API_INFO(" %d\t  ", record->channel);
	RTW_API_INFO(" %d\t  ", record->wps_type);
	RTW_API_INFO("%s\t\t ", (record->security == RTW_SECURITY_OPEN) ? "Open" :
				 (record->security == RTW_SECURITY_WEP_PSK) ? "WEP" :
				 (record->security == RTW_SECURITY_WPA_TKIP_PSK) ? "WPA TKIP" :
				 (record->security == RTW_SECURITY_WPA_AES_PSK) ? "WPA AES" :
				 ( record->security == RTW_SECURITY_WPA_MIXED_PSK ) ? "WPA Mixed" :
				 (record->security == RTW_SECURITY_WPA2_AES_PSK) ? "WPA2 AES" :
				 (record->security == RTW_SECURITY_WPA2_TKIP_PSK) ? "WPA2 TKIP" :
				 (record->security == RTW_SECURITY_WPA2_MIXED_PSK) ? "WPA2 Mixed" :
				 ( record->security == RTW_SECURITY_WPA_WPA2_TKIP_PSK) ? "WPA/WPA2 TKIP" :
				 ( record->security == RTW_SECURITY_WPA_WPA2_AES_PSK) ? "WPA/WPA2 AES" :
				 ( record->security == RTW_SECURITY_WPA_WPA2_MIXED_PSK) ? "WPA/WPA2 Mixed" :
				  "Unknown");

	RTW_API_INFO(" %s ", record->SSID.val);
	RTW_API_INFO("\r\n");
}

/* Scanning operation should be blocked mode */
wifi_utils_scan_list_s *g_scan_list;
int g_scan_num;
static void _free_scanlist(void)
{
	while (g_scan_list) {
		wifi_utils_scan_list_s *cur = g_scan_list;
		g_scan_list = g_scan_list->next;
		rtw_mfree((unsigned char *)cur, sizeof(wifi_utils_scan_list_s));
	}
	g_scan_num = 0;
}

extern int8_t wifi_scan_result_callback(wifi_utils_scan_list_s *scan_list, int scan_num);



#if SCAN_WITH_SSID
#define SCAN_BUF_LEN 1000
int8_t cmd_wifi_scan_with_ssid(trwifi_scan_config_s *scan_config)
{

	u8 *channel_list = NULL;
	u8 *pscan_config = NULL;

	char *ssid = NULL;
	int ssid_len = 0;
	unsigned char mode = 0;
	int num_channel = 0;

	int scan_buf_len = SCAN_BUF_LEN;
	if (!scan_config) {
		return;
	}
	mode = scan_config->scan_mode;

	if (0 == mode) {	//full scan
		if (!scan_config->ssid) {
			ssid = scan_config->ssid;
			ssid_len = strlen((const char *)ssid);
			if ((ssid_len <= 0) || (ssid_len > 32)) {
				RTW_API_INFO("\n\rWrong ssid. Length must be less than 32.");
				return;
			}
		}
		if (scan_buf_len < 36) {
			RTW_API_INFO("\n\rBUFFER_LENGTH too short\n\r");
			return;
		}
	} else if (1 == mode) {	// partial scan
		if (!scan_config->ssid) {
			ssid = scan_config->ssid;
			ssid_len = strlen((const char *)ssid);
			if ((ssid_len <= 0) || (ssid_len > 32)) {
				RTW_API_INFO("\n\rWrong ssid. Length must be less than 32.");
				goto exit;
			}
		}
		num_channel = scan_config->channel_2g_len;
		channel_list = (u8 *)rtw_malloc(num_channel);
		if (!channel_list) {
			RTW_API_INFO("\n\r ERROR: Can't malloc memory for channel list");
			goto exit;
		}
		rtw_memcpy(channel_list, scan_config->channel_2g, num_channel);
		pscan_config = (u8 *)rtw_malloc(num_channel);
		if (!pscan_config) {
			RTW_API_INFO("\n\r ERROR: Can't malloc memory for pscan_config");
			goto exit;
		}
		rtw_memset(pscan_config, PSCAN_ENABLE, num_channel);
		if (wifi_set_pscan_chan(channel_list, pscan_config, num_channel) < 0) {
			RTW_API_INFO("\n\rERROR: wifi set partial scan channel fail");
			goto exit;
		}
	} else {
		RTW_API_INFO("\n\r For Scan all channel Usage: wifi_scan_with_ssid ssid");
		RTW_API_INFO("\n\r For Scan partial channel Usage: wifi_scan_with_ssid ssid num_channels channel_num1 ...");
		return -1;
	}

	if (wifi_scan_networks_with_ssid(NULL, NULL, scan_buf_len, ssid, ssid_len) != RTW_SUCCESS) {
		RTW_API_INFO("\n\rERROR: wifi scan failed");
		goto exit;
	}
exit:
	if ((mode == 1) && channel_list)
		rtw_mfree(channel_list, num_channel);
	if ((mode == 1) && pscan_config)
		rtw_mfree(pscan_config, num_channel);
	return 0;
}
#endif
void cmd_wifi_scan(int argc, char **argv)
{

	u8 *channel_list = NULL;
	u8 *pscan_config = NULL;

	if (argc > 2) {
		int i = 0;
		int num_channel = atoi(argv[1]);

		channel_list = (u8 *)kmm_zalloc(num_channel);
		if (!channel_list) {
			ndbg("\n\r ERROR: Can't malloc memory for channel list");
			goto exit;
		}
		pscan_config = (u8 *)kmm_zalloc(num_channel);
		if (!pscan_config) {
			ndbg("\n\r ERROR: Can't malloc memory for pscan_config");
			goto exit;
		}
		//parse command channel list
		for (i = 2; i <= argc - 1; i++) {
			*(channel_list + i - 2) = (u8)atoi(argv[i]);
			*(pscan_config + i - 2) = PSCAN_ENABLE;
		}

		if (wifi_set_pscan_chan(channel_list, pscan_config, num_channel) < 0) {
			ndbg("\n\rERROR: wifi set partial scan channel fail");
			goto exit;
		}
	}

	if (wifi_scan_networks(app_scan_result_handler, NULL) != RTW_SUCCESS) {
		ndbg("\n\rERROR: wifi scan failed");
		goto exit;
	}
exit:
	if (argc > 2 && channel_list)
		kmm_free(channel_list);
	if (argc > 2 && pscan_config)
		kmm_free(pscan_config);
}

#if CONFIG_WEBSERVER
static void cmd_wifi_start_webserver(int argc, char **argv)
{
	start_web_server();
}
#endif

static void cmd_wifi_iwpriv(int argc, char **argv)
{
	if (argc == 2 && argv[1]) {
		wext_private_command(WLAN0_NAME, argv[1], 1);
	} else {
		ndbg("\n\rUsage: iwpriv COMMAND PARAMETERS");
	}
}
#endif //#if CONFIG_WLAN

static void cmd_ping(int argc, char **argv)
{
	ndbg("\n\r NOT SUPPORTED");
}
#ifdef CONFIG_GENERATE_RUN_TIME_STATS
static char cBuffer[512];
static void cmd_cpustat(int argc, char **argv)
{
	vTaskGetRunTimeStats((char *)cBuffer);
	ndbg(cBuffer);
}
#endif
#if 0//defined(CONFIG_RTL8195A) || defined(CONFIG_RTL8711B) || defined(CONFIG_RTL8721D)
static void cmd_dump_reg(int argc, char **argv)
{
	CmdDumpWord(argc - 1, (u8 **)(argv + 1));
}
static void cmd_edit_reg(int argc, char **argv)
{
	CmdWriteWord(argc - 1, (u8 **)(argv + 1));
}
#endif
static void cmd_exit(int argc, char **argv)
{
	nvdbg("\n\rLeave INTERACTIVE MODE");
	task_delete(0);
}

static void cmd_debug(int argc, char **argv)
{
	if (strncmp(argv[1], "ready_trx", strlen("ready_trx")) == 0) {
		nvdbg("\r\n%d", wifi_is_ready_to_transceive((rtw_interface_t)rtw_atoi((u8 *)argv[2])));
	} else if (strncmp(argv[1], "is_up", strlen("is_up")) == 0) {
		nvdbg("\r\n%d", wifi_is_up((rtw_interface_t)rtw_atoi((u8 *)argv[2])));
	} else if (strncmp(argv[1], "set_mac", strlen("set_mac")) == 0) {
		nvdbg("\r\n%d", wifi_set_mac_address(argv[2]));
	} else if (strncmp(argv[1], "get_mac", strlen("get_mac")) == 0) {
		u8 mac[18] = {0};
		wifi_get_mac_address((char *)mac);
		nvdbg("\r\n%s", mac);
	} else if (strncmp(argv[1], "ps_on", strlen("ps_on")) == 0) {
		nvdbg("\r\n%d", wifi_enable_powersave());
	} else if (strncmp(argv[1], "ps_off", strlen("ps_off")) == 0) {
		nvdbg("\r\n%d", wifi_disable_powersave());
	} else if (strncmp(argv[1], "get_clientlist", strlen("get_clientlist")) == 0) {
		int client_number;
		struct {
			int count;
			rtw_mac_t mac_list[3];
		} client_info;

		client_info.count = 3;

		nvdbg("\r\n%d\r\n", wifi_get_associated_client_list(&client_info, sizeof(client_info)));

		if (client_info.count == 0) {
			RTW_API_INFO(("Clients connected 0..\r\n"));
		} else {
			RTW_API_INFO("Clients connected %d..\r\n", client_info.count);
			for (client_number = 0; client_number < client_info.count; client_number++) {
				RTW_API_INFO(("------------------------------------\r\n"));
				RTW_API_INFO("| %d | " MAC_FMT " |\r\n",
							 client_number,
							 MAC_ARG(client_info.mac_list[client_number].octet));
			}
			RTW_API_INFO(("------------------------------------\r\n"));
		}
	} else if (strncmp(argv[1], "get_apinfo", strlen("get_apinfo")) == 0) {
		rtw_bss_info_t ap_info;
		rtw_security_t sec;
		if (wifi_get_ap_info(&ap_info, &sec) == RTW_SUCCESS) {
			RTW_API_INFO("\r\nSSID  : %s\r\n", (char *)ap_info.SSID);
			RTW_API_INFO("BSSID : " MAC_FMT "\r\n", MAC_ARG(ap_info.BSSID.octet));
			RTW_API_INFO("RSSI  : %d\r\n", ap_info.RSSI);
			RTW_API_INFO("Beacon period : %d\r\n", ap_info.beacon_period);
			RTW_API_INFO("Security : %s\r\n", (sec == RTW_SECURITY_OPEN) ? "Open" :
								(sec == RTW_SECURITY_WEP_PSK) ? "WEP" :
								(sec == RTW_SECURITY_WPA_TKIP_PSK) ? "WPA TKIP" :
								(sec == RTW_SECURITY_WPA_AES_PSK) ? "WPA AES" :
								(sec == RTW_SECURITY_WPA_MIXED_PSK)  ? "WPA Mixed" :
								(sec == RTW_SECURITY_WPA2_AES_PSK) ? "WPA2 AES" :
								(sec == RTW_SECURITY_WPA2_TKIP_PSK) ? "WPA2 TKIP" :
								(sec == RTW_SECURITY_WPA2_MIXED_PSK) ? "WPA2 Mixed" :
								(sec == RTW_SECURITY_WPA_WPA2_TKIP_PSK) ? "WPA/WPA2 TKIP" :
								(sec == RTW_SECURITY_WPA_WPA2_AES_PSK) ? "WPA/WPA2 AES" :
								(sec == RTW_SECURITY_WPA_WPA2_MIXED_PSK) ? "WPA/WPA2 Mixed" :
								"Unknown");
		}
	} else if (strncmp(argv[1], "reg_mc", strlen("reg_mc")) == 0) {
		rtw_mac_t mac;
		sscanf(argv[2], MAC_FMT, (int *)(mac.octet + 0), (int *)(mac.octet + 1), (int *)(mac.octet + 2), (int *)(mac.octet + 3), (int *)(mac.octet + 4), (int *)(mac.octet + 5));
		nvdbg("\r\n%d", wifi_register_multicast_address(&mac));
	} else if (strncmp(argv[1], "unreg_mc", strlen("unreg_mc")) == 0) {
		rtw_mac_t mac;
		sscanf(argv[2], MAC_FMT, (int *)(mac.octet + 0), (int *)(mac.octet + 1), (int *)(mac.octet + 2), (int *)(mac.octet + 3), (int *)(mac.octet + 4), (int *)(mac.octet + 5));
		nvdbg("\r\n%d", wifi_unregister_multicast_address(&mac));
	} else if (strncmp(argv[1], "get_rssi", strlen("get_rssi")) == 0) {
		int rssi = 0;
		wifi_get_rssi(&rssi);
		nvdbg("\n\rwifi_get_rssi: rssi = %d", rssi);
	} else if (strncmp(argv[1], "dbg", strlen("dbg")) == 0) {
		char buf[32] = {0};
		char *copy = buf;
		int i = 0;
		int len = 0;
		for (i = 1; i < argc; i++) {
			strcpy(&buf[len], argv[i]);
			len = strlen(copy);
			buf[len++] = ' ';
			buf[len] = '\0';
		}
		wext_private_command(WLAN0_NAME, copy, 1);
#ifdef CONFIG_IEEE80211W
	} else if (strncmp(argv[1], "11w_sa", strlen("11w_sa")) == 0) {
		rltk_wlan_tx_sa_query(atoi((const char *)argv[2]));
	} else if (strncmp(argv[1], "11w_deauth", strlen("11w_deauth")) == 0) {
		rltk_wlan_tx_deauth(atoi((const char *)argv[2]), atoi((const char *)argv[3]));
	} else if (strncmp(argv[1], "11w_auth", strlen("11w_auth")) == 0) {
		rltk_wlan_tx_auth();
#endif
	} else {
		ndbg("\r\nUnknown CMD\r\n");
	}
}
