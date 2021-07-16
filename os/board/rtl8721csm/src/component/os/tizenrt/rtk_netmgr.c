/******************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/
#include <pthread.h>

#include <tinyara/wifi/wifi_utils.h>
#include "wifi_conf.h"
#include "rtk_wifi_utils.h"
#include <wifi_manager/wifi_manager.h>
#include <debug.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/net/if/wifi.h>
#include <tinyara/netmgr/netdev_mgr.h>
/* WLAN CONFIG ---------------------------------------------------------------*/
#define RTK_OK          0		/*!< RTK_err_t value indicating success (no error) */
#define RTK_FAIL        -1		/*!< Generic RTK_err_t code indicating failure */

#define RTK_ERR_NO_MEM          0x101	/*!< Out of memory */
#define RTK_ERR_INVALID_ARG     0x102	/*!< Invalid argument */
#define RTK_ERR_INVALID_STATE   0x103	/*!< Invalid state */
#define RTK_ERR_INVALID_SIZE    0x104	/*!< Invalid size */
#define RTK_ERR_NOT_FOUND       0x105	/*!< Requested resource not found */
#define RTK_ERR_NOT_SUPPORTED   0x106	/*!< Operation or feature not supported */
#define RTK_ERR_TIMEOUT         0x107	/*!< Operation timed out */
#define RTK_ERR_INVALID_RRTKONSE    0x108	/*!< Received rRTKonse was invalid */
#define RTK_ERR_INVALID_CRC     0x109	/*!< CRC or checksum was invalid */
#define RTK_ERR_INVALID_VERSION     0x10A	/*!< Version was invalid */
#define RTK_ERR_INVALID_MAC     0x10B	/*!< MAC address was invalid */

#define RTK_ERR_WIFI_BASE       0x3000	/*!< Starting number of WiFi error codes */
#define RTK_ERR_MESH_BASE       0x4000	/*!< Starting number of MESH error codes */

#define vTaskDelay(t)   usleep(t*1000)

/* Private define ------------------------------------------------------------*/

//typedef unsigned char    bool;

static WiFi_InterFace_ID_t g_mode = RTK_WIFI_NONE;
#include <tinyara/net/if/wifi.h>

trwifi_result_e wifi_netmgr_utils_init(struct netdev *dev);
trwifi_result_e wifi_netmgr_utils_deinit(struct netdev *dev);
trwifi_result_e wifi_netmgr_utils_scan_ap(struct netdev *dev, trwifi_scan_config_s *config);
trwifi_result_e wifi_netmgr_utils_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg);
trwifi_result_e wifi_netmgr_utils_disconnect_ap(struct netdev *dev, void *arg);
trwifi_result_e wifi_netmgr_utils_get_info(struct netdev *dev, trwifi_info *wifi_info);
trwifi_result_e wifi_netmgr_utils_start_sta(struct netdev *dev);
trwifi_result_e wifi_netmgr_utils_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config);
trwifi_result_e wifi_netmgr_utils_stop_softap(struct netdev *dev);
trwifi_result_e wifi_netmgr_utils_set_autoconnect(struct netdev *dev, uint8_t check);
trwifi_result_e wifi_netmgr_utils_ioctl(struct netdev *dev, trwifi_msg_s *msg);
struct trwifi_ops g_trwifi_drv_ops = {
	wifi_netmgr_utils_init,			/* init */
	wifi_netmgr_utils_deinit,			/* deinit */
	wifi_netmgr_utils_scan_ap,			/* scan_ap */
	wifi_netmgr_utils_connect_ap,		/* connect_ap */
	wifi_netmgr_utils_disconnect_ap,   /* disconnect_ap */
	wifi_netmgr_utils_get_info,		/* get_info */
	wifi_netmgr_utils_start_sta,		/* start_sta */
	wifi_netmgr_utils_start_softap,	/* start_softap */
	wifi_netmgr_utils_stop_softap,		/* stop_softap */
	wifi_netmgr_utils_set_autoconnect, /* set_autoconnect */
	wifi_netmgr_utils_ioctl,					/* drv_ioctl */
};

static wifi_utils_scan_list_s *g_scan_list;
static int g_scan_num;
extern struct netdev *ameba_nm_dev_wlan0;

static void _free_scanlist(void)
{
	while (g_scan_list) {
		wifi_utils_scan_list_s *cur = g_scan_list;
		g_scan_list = g_scan_list->next;
		rtw_mfree(cur, sizeof(wifi_utils_scan_list_s));
	}
	g_scan_num = 0;
}
rtw_result_t app_scan_result_handler(rtw_scan_handler_result_t *malloced_scan_result)
{
	wifi_utils_scan_list_s *scan_list;

	if (malloced_scan_result->scan_complete != RTW_TRUE) {
		scan_list = (wifi_utils_scan_list_s *)rtw_zmalloc(sizeof(wifi_utils_scan_list_s));
		if (scan_list == NULL) {
			ndbg("\r\n[app_scan_result_handler]:Fail to malloc scan_list\r\n");
			return RTW_ERROR;
		}
		rtw_scan_result_t *record = &malloced_scan_result->ap_details;
		record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */
		scan_list->ap_info.channel = record->channel;
		strncpy(scan_list->ap_info.ssid, (const char *)record->SSID.val, record->SSID.len);
		scan_list->ap_info.ssid_length = record->SSID.len;
		snprintf((char *)scan_list->ap_info.bssid, WIFI_UTILS_MACADDR_STR_LEN + 1, "%02x:%02x:%02x:%02x:%02x:%02x", record->BSSID.octet[0], record->BSSID.octet[1], record->BSSID.octet[2], record->BSSID.octet[3], record->BSSID.octet[4], record->BSSID.octet[5]);
		scan_list->ap_info.max_rate = 0;
		scan_list->ap_info.rssi = record->signal_strength;
		scan_list->ap_info.phy_mode = 0x00000004; //bit2 is set to 1 if support 11n
		switch (record->security) {
		case RTW_SECURITY_OPEN:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_OPEN;
			break;
		case RTW_SECURITY_WEP_SHARED:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WEP_SHARED;
			break;
		case RTW_SECURITY_WEP_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WEP_SHARED;
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA_PSK;
			break;
		case RTW_SECURITY_WPA_AES_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA_PSK;
			break;
		case RTW_SECURITY_WPA_MIXED_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA_PSK;
			break;
		case RTW_SECURITY_WPA2_AES_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA2_PSK;
			break;
		case RTW_SECURITY_WPA2_TKIP_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA2_PSK;
			break;
		case RTW_SECURITY_WPA2_MIXED_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA2_PSK;
			break;
		case RTW_SECURITY_WPA_WPA2_TKIP_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA_AND_WPA2_PSK;
			break;
		case RTW_SECURITY_WPA_WPA2_AES_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA_AND_WPA2_PSK;
			break;
		case RTW_SECURITY_WPA_WPA2_MIXED_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA_AND_WPA2_PSK;
			break;
		case RTW_SECURITY_WPA2_AES_CMAC:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA2_PSK;
			break;
		case RTW_SECURITY_WPA2_ENTERPRISE:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA2_PSK_ENT;
			break;
		case RTW_SECURITY_WPA_WPA2_ENTERPRISE:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA_AND_WPA2_PSK_ENT;
			break;
		case RTW_SECURITY_WPS_OPEN:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPS;
			break;
		case RTW_SECURITY_WPS_SECURE:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPS;
			break;
		case RTW_SECURITY_WPA3_AES_PSK:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_WPA3_PSK;
			break;
		default:
			scan_list->ap_info.ap_auth_type = WIFI_UTILS_AUTH_UNKNOWN;
			break;
		}

		switch (record->security) {
		case RTW_SECURITY_OPEN:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_NONE;
			break;
		case RTW_SECURITY_WEP_SHARED:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_WEP_64;
			break;
		case RTW_SECURITY_WEP_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_WEP_128;
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_TKIP;
			break;
		case RTW_SECURITY_WPA_AES_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_AES;
			break;
		case RTW_SECURITY_WPA_MIXED_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_TKIP_AND_AES;
			break;
		case RTW_SECURITY_WPA2_AES_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_AES;
			break;
		case RTW_SECURITY_WPA2_TKIP_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_TKIP;
			break;
		case RTW_SECURITY_WPA2_MIXED_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_TKIP_AND_AES;
			break;
		case RTW_SECURITY_WPA_WPA2_TKIP_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_TKIP;
			break;
		case RTW_SECURITY_WPA_WPA2_AES_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_AES;
			break;
		case RTW_SECURITY_WPA_WPA2_MIXED_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_TKIP_AND_AES;
			break;
		case RTW_SECURITY_WPA2_AES_CMAC:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_UNKNOWN;
			break;
		case RTW_SECURITY_WPA2_ENTERPRISE:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_AES_ENT;
			break;
		case RTW_SECURITY_WPA_WPA2_ENTERPRISE:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_AES_ENT;
			break;
		case RTW_SECURITY_WPS_OPEN:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_NONE;
			break;
		case RTW_SECURITY_WPS_SECURE:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_AES;
			break;
		case RTW_SECURITY_WPA3_AES_PSK:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_AES;
			break;
		default:
			scan_list->ap_info.ap_crypto_type = WIFI_UTILS_CRYPTO_UNKNOWN;
			break;
		}
		if (g_scan_list == NULL) {
			g_scan_list = scan_list;
			g_scan_num = 1;
		} else {
			wifi_utils_scan_list_s *cur = g_scan_list;
			int idx;
			for (idx = 1; idx < g_scan_num; idx++) {
				cur = cur->next;
			}
			cur->next = scan_list;
			scan_list->next = NULL;
			g_scan_num++;
		}
	} else {
		nvdbg("SCAN DONE: Calling wifi_scan_result_callback\r\n");
		TRWIFI_POST_SCANEVENT(ameba_nm_dev_wlan0, LWNL_EVT_SCAN_DONE, (void *)g_scan_list);
		_free_scanlist();
		if (g_scan_list) {
			ndbg("SCAN list is not initialized\n");
			return RTW_ERROR;
		}
	}
	return RTW_SUCCESS;
}

int parse_scan_with_ssid_res(char*buf, int buflen, char *target_ssid, void *user_data)
{
	if (rltk_wlan_scan_with_ssid_by_extended_security_is_enable()) {
			int plen = 0, scan_cnt = 0;
			rtw_scan_handler_result_t scan_result_report;
			scan_result_report.scan_complete = RTW_FALSE;
			scan_result_report.user_data = user_data;
			while (plen < buflen) {
				int len;
				rtw_memset(&scan_result_report.ap_details, 0, sizeof(scan_result_report.ap_details));
				len = (int)*(buf + plen);
				if (len == 0) break;
				scan_result_report.ap_details.SSID.len = len - 1 - 6 - 4 - 4 - 1 - 1;
				rtw_memcpy(&scan_result_report.ap_details.SSID.val, (buf + plen + 1 + 6 + 4 + 4 + 1 + 1), scan_result_report.ap_details.SSID.len);
				rtw_memcpy(&scan_result_report.ap_details.BSSID.octet, buf + plen + 1, 6);
				rtw_memcpy(&scan_result_report.ap_details.signal_strength, buf + plen + 1 + 6, 4);
				rtw_memcpy(&scan_result_report.ap_details.security, buf + plen + 1 + 6 + 4, 4);
				scan_result_report.ap_details.wps_type = (int)*(buf + plen + 1 + 6 + 4 + 4);
				scan_result_report.ap_details.channel = (int)*(buf + plen + 1 + 6 + 4 + 4 + 1);
				app_scan_result_handler(&scan_result_report);
				plen += len;
				scan_cnt++;
			}
			//RTW_API_INFO("\n\rwifi_scan: scan count = %d, scan_cnt);
			rltk_wlan_enable_scan_with_ssid_by_extended_security(0);
			scan_result_report.scan_complete = RTW_TRUE;
			app_scan_result_handler(&scan_result_report);
			return RTW_SUCCESS;
		} else {
			RTW_API_INFO("scan_with_ssid_by_extended_security is not enabled\n");
			return RTW_ERROR;
		}
}

/*
 * Callback
 */
static int rtk_drv_callback_handler(int argc, char *argv[])
{
	//RTKDRV_ENTER;
	int type = (int)(argv[1][0] - '0');

	switch (type) {
	case 1:
		trwifi_post_event(ameba_nm_dev_wlan0, LWNL_EVT_STA_CONNECTED, NULL, 0);
		break;
	case 2:
		trwifi_post_event(ameba_nm_dev_wlan0, LWNL_EVT_STA_CONNECT_FAILED, NULL, 0);
		break;
	case 3:
		trwifi_post_event(ameba_nm_dev_wlan0, LWNL_EVT_SOFTAP_STA_JOINED, NULL, 0);
		break;
	case 4:
		trwifi_post_event(ameba_nm_dev_wlan0, LWNL_EVT_STA_DISCONNECTED, NULL, 0);
		break;
	case 5:
		trwifi_post_event(ameba_nm_dev_wlan0, LWNL_EVT_SOFTAP_STA_LEFT, NULL, 0);
		break;
	default:
		trwifi_post_event(ameba_nm_dev_wlan0, LWNL_EVT_UNKNOWN, NULL, 0);
		break;
	}

	return 0;
}

void linkup_handler(rtk_reason_t *reason)
{
	//RTKDRV_ENTER;
	pid_t pid;
	char *argv[2];
	argv[1] = NULL;
	char data[2] = {0, 0};

	if (g_mode == RTK_WIFI_STATION_IF) {
		if (reason->reason_code == RTK_STATUS_SUCCESS) {
			data[0] = '1';
		} else {
			data[0] = '2';
		}
	} else if (g_mode == RTK_WIFI_SOFT_AP_IF) {
		data[0] = '3';
	}
	argv[0] = data;

	pid = kernel_thread("lwnl80211_cbk_handler", 100, 2048, (main_t)rtk_drv_callback_handler, argv);
	if (pid < 0) {
		vddbg("kernel thread create fail(%d)\n", errno);
		return;
	}
}

void linkdown_handler(rtk_reason_t *reason)
{
	//RTKDRV_ENTER;
	pid_t pid;
	char *argv[2];
	argv[1] = NULL;
	char data[2] = {0, 0};

	data[0] = '4';
	if (g_mode == RTK_WIFI_STATION_IF) {
		data[0] = '4';
	} else if (g_mode == RTK_WIFI_SOFT_AP_IF) {
		data[0] = '5';
	}
	argv[0] = data;

	pid = kernel_thread("lwnl80211_cbk_handler", 100, 2048, (main_t)rtk_drv_callback_handler, argv);
	if (pid < 0) {
		vddbg("pthread create fail(%d)\n", errno);
		return;
	}
}


//
// Interface API
//
trwifi_result_e wifi_netmgr_utils_init(struct netdev *dev)
{
	trwifi_result_e wuret = TRWIFI_FAIL;
	if (g_mode == RTK_WIFI_NONE) {
		int ret = RTK_STATUS_SUCCESS;
		
		ret = WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler);
		if (ret != RTK_STATUS_SUCCESS) {
			ndbg("[RTK] Link callback handles: register failed !\n");
			return wuret;
		} else {
			nvdbg("[RTK] Link callback handles: registered\n");
		}

		ret = cmd_wifi_on(RTK_WIFI_STATION_IF);
		if (ret != RTK_STATUS_SUCCESS) {
			ndbg("[RTK] Failed to start STA mode\n");
			return wuret;
		}
		g_mode = RTK_WIFI_STATION_IF;
		wuret = TRWIFI_SUCCESS;
	} else {
		ndbg("Already %d\n", g_mode);
	}
	return wuret;
	//return TRWIFI_FAIL;
}

trwifi_result_e wifi_netmgr_utils_deinit(struct netdev *dev)
{
	trwifi_result_e wuret = TRWIFI_FAIL;
	int ret = cmd_wifi_off();
	if (ret == RTK_STATUS_SUCCESS) {
		g_mode = RTK_WIFI_NONE;
		wuret = TRWIFI_SUCCESS;
	} else {
		ndbg("[RTK] Failed to stop STA mode\n");
	}
	return wuret;
}

trwifi_result_e wifi_netmgr_utils_scan_ap(struct netdev *dev, trwifi_scan_config_s *config)
{
	if (config) {
		if (config->ssid_length > 0) {
			int scan_buf_len = 500;
			rltk_wlan_enable_scan_with_ssid_by_extended_security(1);
			if (wifi_scan_networks_with_ssid(parse_scan_with_ssid_res, NULL,
											 scan_buf_len, config->ssid, config->ssid_length) != RTW_SUCCESS) {
				return TRWIFI_FAIL;
			}
		} else {
			// scan specified channel.
			return TRWIFI_NOT_SUPPORTED;
		}
	} else {
		if (wifi_scan_networks(app_scan_result_handler, NULL ) != RTW_SUCCESS) {
			//ndbg("[RTK] [ERR] WiFi scan fail(%d)\n", ret);
			return TRWIFI_FAIL;
		}
	}
	nvdbg("[RTK] WIFi Scan success\n");
	return TRWIFI_SUCCESS;
}

trwifi_result_e wifi_netmgr_utils_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg)
{
	trwifi_result_e wuret = TRWIFI_INVALID_ARGS;
	if (!ap_connect_config) {
		return wuret;
	}

	int ret;
	wuret = TRWIFI_FAIL;

	if (g_mode == RTK_WIFI_SOFT_AP_IF) {
		if (wifi_netmgr_utils_deinit(dev)) {
			ndbg("[RTK] Failed to stop AP mode\n");
			return TRWIFI_FAIL;
		}
		vTaskDelay(20);
		if (wifi_netmgr_utils_init(dev) < 0) {
			ndbg("\n\rERROR: Wifi on failed!");
			return TRWIFI_FAIL;
		}
	}

	ret = cmd_wifi_connect(ap_connect_config, arg);
	if (ret != RTK_STATUS_SUCCESS) {
		ndbg("[RTK] WiFiNetworkJoin failed: %d, %s\n", ret, ap_connect_config->ssid);
		return wuret;
	} else {
		wuret = TRWIFI_SUCCESS;
		nvdbg("[RTK] Successfully joined the network: %s(%d)\n", ap_connect_config->ssid,
			  ap_connect_config->ssid_length);
	}

	return wuret;
}

trwifi_result_e wifi_netmgr_utils_disconnect_ap(struct netdev *dev, void *arg)
{
	trwifi_result_e wuret = TRWIFI_FAIL;
	int ret = cmd_wifi_disconnect();
	if (ret == RTK_STATUS_SUCCESS) {
		nvdbg("[RTK] WiFiNetworkLeave success\n");
		wuret = TRWIFI_SUCCESS;
	} else {
		ndbg("[RTK] WiFiNetworkLeave fail because of %d\n", ret);
	}

	return wuret;
}

trwifi_result_e wifi_netmgr_utils_get_info(struct netdev *dev, trwifi_info *wifi_info)
{
	trwifi_result_e wuret = TRWIFI_INVALID_ARGS;
	if (wifi_info) {
		wuret = TRWIFI_FAIL;
		if (g_mode != RTK_WIFI_NONE) {
			wifi_info->rssi = (int)0;
			if (g_mode == RTK_WIFI_SOFT_AP_IF) {
				wifi_info->wifi_status = WIFI_UTILS_SOFTAP_MODE;
			} else if (g_mode == RTK_WIFI_STATION_IF) {
				if (wifi_is_connected_to_ap() == RTK_STATUS_SUCCESS) {
					int rssi;
					wifi_info->wifi_status = WIFI_UTILS_CONNECTED;
					if (wifi_get_rssi(&rssi) == RTK_STATUS_SUCCESS) {
						wifi_info->rssi = (int)rssi;
					}
				} else {
					wifi_info->wifi_status = WIFI_UTILS_DISCONNECTED;
				}
			}
			wuret = TRWIFI_SUCCESS;
		} else {
			ndbg("[RTK] need to init... get info fail\n");
		}
	}

	return wuret;
}

trwifi_result_e wifi_netmgr_utils_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config)
{
	if (!softap_config) {
		return TRWIFI_INVALID_ARGS;
	}

	if (g_mode == RTK_WIFI_SOFT_AP_IF)
		ndbg("[RTK] softap is already running!\n");

	trwifi_result_e ret = TRWIFI_FAIL;

	ret = WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler);
	if (ret != RTK_STATUS_SUCCESS) {
		ndbg("[RTK] Link callback handles: register failed !\n");
		return TRWIFI_FAIL;
	} else {
		nvdbg("[RTK] Link callback handles: registered\n");
	}

	if (cmd_wifi_ap(softap_config) != RTK_STATUS_SUCCESS) {
		ndbg("[RTK] Failed to start AP mode\n");
		return ret;
	}
	g_mode = RTK_WIFI_SOFT_AP_IF;
	nvdbg("[RTK] SoftAP with SSID: %s has successfully started!\n", softap_config->ssid);

	ret = TRWIFI_SUCCESS;

	return ret;
}

trwifi_result_e wifi_netmgr_utils_start_sta(struct netdev *dev)
{
	trwifi_result_e wuret = TRWIFI_FAIL;
	int ret = RTK_STATUS_SUCCESS;

	if (g_mode == RTK_WIFI_STATION_IF)
		ndbg("[RTK] station is already running!\n");

	ret = WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler);
	if (ret != RTK_STATUS_SUCCESS) {
		ndbg("[RTK] Link callback handles: register failed !\n");
		return wuret;
	} else {
		nvdbg("[RTK] Link callback handles: registered\n");
	}

	cmd_wifi_off();
	vTaskDelay(20);
	ret = cmd_wifi_on(RTK_WIFI_STATION_IF);
	if (ret == RTK_STATUS_SUCCESS) {
		g_mode = RTK_WIFI_STATION_IF;
		wuret = TRWIFI_SUCCESS;
	} else {
		ndbg("[RTK] Failed to start STA mode\n");
	}
	return wuret;
}

trwifi_result_e wifi_netmgr_utils_stop_softap(struct netdev *dev)
{
	trwifi_result_e wuret = TRWIFI_FAIL;
	int ret;
	if (g_mode == RTK_WIFI_SOFT_AP_IF) {
		ret = cmd_wifi_off();
		if (ret == RTK_STATUS_SUCCESS) {
			g_mode = RTK_WIFI_NONE;
			wuret = TRWIFI_SUCCESS;
			nvdbg("[RTK] Stop AP mode successfully\n");
		} else {
			ndbg("[RTK] Stop AP mode fail\n");
		}
	} else {
		ndbg("[RTK] Mode is not AP mode\n");
	}
	return wuret;
}

trwifi_result_e wifi_netmgr_utils_set_autoconnect(struct netdev *dev, uint8_t check)
{
	trwifi_result_e wuret = TRWIFI_FAIL;
	int ret = RTK_FAIL;
	ret = wifi_set_autoreconnect(check);
	if (ret == RTK_STATUS_SUCCESS) {
		wuret = TRWIFI_SUCCESS;
		nvdbg("[RTK] External Autoconnect set to %d\n", check);
	} else {
		ndbg("[RTK] External Autoconnect failed to set %d", check);
	}
	return wuret;
}

trwifi_result_e wifi_netmgr_utils_ioctl(struct netdev *dev, trwifi_msg_s *msg)
{
	if (msg->cmd == TRWIFI_MSG_SET_POWERMODE) {
		int *mode = (int *)msg->data;
		if (*mode == TRWIFI_POWERMODE_ON) {
			ndbg("[RTK] set power mode on\n");
			wifi_enable_powersave();
			return TRWIFI_SUCCESS;
		} else if (*mode == TRWIFI_POWERMODE_OFF) {
			ndbg("[RTK] set power mode off\n");
			wifi_disable_powersave();
			return TRWIFI_SUCCESS;
		}
	}
	return TRWIFI_NOT_SUPPORTED;
}
