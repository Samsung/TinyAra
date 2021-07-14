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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#ifdef CONFIG_NET_NETMGR
#include <tinyara/netmgr/netdev_mgr.h>
#endif
#include <tinyara/net/if/wifi.h>
#include "wifi_conf.h"

#define vTaskDelay(t) usleep(t)
#define MACADDR_LEN 6

#define RTKDRV_TAG "[RTKDRV]"

#define RTKDRV_ERR                                      \
	do {                                                \
		vddbg(RTKDRV_TAG "[ERR] %s: %d line err(%s)\n", \
			  __FILE__, __LINE__, strerror(errno));     \
	} while (0)

#define RTKDRV_ENTER                                     \
	do {                                                 \
		vddbg(RTKDRV_TAG "%s:%d\n", __FILE__, __LINE__); \
	} while (0)

struct netdev *g_rtk_netdev;
/****************************************************************************
 * Private Types
 ****************************************************************************/
static trwifi_result_e rtkdrv_init(struct netdev *dev);
static trwifi_result_e rtkdrv_deinit(struct netdev *dev);
static trwifi_result_e rtkdrv_scan_ap(struct netdev *dev, trwifi_scan_config_s *config);
static trwifi_result_e rtkdrv_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg);
static trwifi_result_e rtkdrv_disconnect_ap(struct netdev *dev, void *arg);
static trwifi_result_e rtkdrv_get_info(struct netdev *dev, trwifi_info *wifi_info);
static trwifi_result_e rtkdrv_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config);
static trwifi_result_e rtkdrv_start_sta(struct netdev *dev);
static trwifi_result_e rtkdrv_stop_softap(struct netdev *dev);
static trwifi_result_e rtkdrv_set_autoconnect(struct netdev *dev, uint8_t check);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
static WiFi_InterFace_ID_t g_mode = RTK_WIFI_NONE;

static struct trwifi_ops g_trwifi_drv_ops = {
	rtkdrv_init,			/* init */
	rtkdrv_deinit,			/* deinit */
	rtkdrv_scan_ap,			/* scan_ap */
	rtkdrv_connect_ap,		/* connect_ap */
	rtkdrv_disconnect_ap,   /* disconnect_ap */
	rtkdrv_get_info,		/* get_info */
	rtkdrv_start_sta,		/* start_sta */
	rtkdrv_start_softap,	/* start_softap */
	rtkdrv_stop_softap,		/* stop_softap */
	rtkdrv_set_autoconnect, /* set_autoconnect */
	NULL					/* drv_ioctl */
};

/*
 * Callback
 */
static int rtk_drv_callback_handler(int argc, char *argv[])
{
	RTKDRV_ENTER;
	int type = (int)(argv[1][0] - '0');

	switch (type) {
	case 1:
		trwifi_post_event(g_rtk_netdev, LWNL_EVT_STA_CONNECTED, NULL, 0);
		break;
	case 2:
		trwifi_post_event(g_rtk_netdev, LWNL_EVT_STA_CONNECT_FAILED, NULL, 0);
		break;
	case 3:
		trwifi_post_event(g_rtk_netdev, LWNL_EVT_SOFTAP_STA_JOINED, NULL, 0);
		break;
	case 4:
		trwifi_post_event(g_rtk_netdev, LWNL_EVT_STA_DISCONNECTED, NULL, 0);
		break;
	case 5:
		trwifi_post_event(g_rtk_netdev, LWNL_EVT_SOFTAP_STA_LEFT, NULL, 0);
		break;
	default:
		trwifi_post_event(g_rtk_netdev, LWNL_EVT_UNKNOWN, NULL, 0);
		break;
	}

	return 0;
}

static void linkup_handler(rtk_reason_t *reason)
{
	RTKDRV_ENTER;
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

	pid = kernel_thread("lwnl80211_cbk_handler", 100, 1024, (main_t)rtk_drv_callback_handler, argv);
	if (pid < 0) {
		vddbg("pthread create fail(%d)\n", errno);
		return;
	}
}

static void linkdown_handler(rtk_reason_t *reason)
{
	RTKDRV_ENTER;
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

	pid = kernel_thread("lwnl80211_cbk_handler", 100, 1024, (main_t)rtk_drv_callback_handler, argv);
	if (pid < 0) {
		vddbg("pthread create fail(%d)\n", errno);
		return;
	}
}


int8_t wifi_scan_result_callback(wifi_utils_scan_list_s *utils_scan_input, int scan_num)
{
	trwifi_scan_list_s *scan_list = (trwifi_scan_list_s *)utils_scan_input;

	if (scan_list) {
		TRWIFI_POST_SCANEVENT(g_rtk_netdev, LWNL_EVT_SCAN_DONE, (void *)scan_list);
	} else {
		trwifi_post_event(g_rtk_netdev, LWNL_EVT_SCAN_FAILED, NULL, 0);
	}

	return RTK_STATUS_SUCCESS;
}

/*
 * Interface API
*/
trwifi_result_e rtkdrv_init(struct netdev *dev)
{
	RTKDRV_ENTER;
	trwifi_result_e result = LWNL_FAIL;
	if (g_mode == RTK_WIFI_NONE) {
		if (WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler) != RTK_STATUS_SUCCESS) {
			vddbg("[RTK] Link callback handles: register failed !\n");
			return result;
		}
		vdvdbg("[RTK] Link callback handles: registered\n");

		if (cmd_wifi_on(RTK_WIFI_STATION_IF) != RTK_STATUS_SUCCESS) {
			vddbg("[RTK] Failed to start STA mode\n");
			return result;
		}
		vdvdbg("[RTK] Start STA mode\n");

		g_mode = RTK_WIFI_STATION_IF;
		result = LWNL_SUCCESS;
		return result;
	} else {
		vddbg("[RTK] Already %d\n", g_mode);
	}

	return result;
}

trwifi_result_e rtkdrv_deinit(struct netdev *dev)
{
	RTKDRV_ENTER;
	trwifi_result_e result = LWNL_FAIL;
	int ret = cmd_wifi_off();
	if (ret == RTK_STATUS_SUCCESS) {
		g_mode = RTK_WIFI_NONE;
		result = LWNL_SUCCESS;
	} else {
		vddbg("[RTK] Failed to stop STA mode\n");
	}
	return result;
}

trwifi_result_e rtkdrv_scan_ap(struct netdev *dev, trwifi_scan_config_s *config)
{
	RTKDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	if (wifi_scan_networks(app_scan_result_handler, NULL) != RTW_SUCCESS) {
		vddbg("[RTK] WiFi scan fail\n");
		return result;
	}
	result = LWNL_SUCCESS;
	vddbg("[RTK] WIFi Scan success\n");
	return result;
}

trwifi_result_e rtkdrv_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg)
{
	RTKDRV_ENTER;
	if (!ap_connect_config) {
		return LWNL_INVALID_ARGS;
	}

	if (g_mode == RTK_WIFI_SOFT_AP_IF) {
		vddbg("[RTK] SoftAP is running\n");
		return LWNL_FAIL;
	}
	int ret = cmd_wifi_connect((wifi_utils_ap_config_s *)ap_connect_config, arg);
	if (ret != RTK_STATUS_SUCCESS) {
		if (ret == RTK_STATUS_ALREADY_CONNECTED) {
			vdvdbg("[RTK] Connect failed (alreay connected: %d)\n", ret);
			return LWNL_ALREADY_CONNECTED;
		} else {
			vddbg("[RTK] Connect failed: %d, %s\n", ret, ap_connect_config->ssid);
			return LWNL_FAIL;
		}
	}
	vddbg("[RTK] Successfully joined the network: %s(%d)\n", ap_connect_config->ssid, ap_connect_config->ssid_length);

	return LWNL_SUCCESS;
}

trwifi_result_e rtkdrv_disconnect_ap(struct netdev *dev, void *arg)
{
	RTKDRV_ENTER;
	int ret = cmd_wifi_disconnect();
	if (ret == RTK_STATUS_SUCCESS) {
		vddbg("[RTK] Disconnect success\n");
		return LWNL_SUCCESS;
	}
	vddbg("[RTK] WiFiNetworkLeave fail because of %d\n", ret);

	return LWNL_FAIL;
}

trwifi_result_e rtkdrv_get_info(struct netdev *dev, trwifi_info *wifi_info)
{
	RTKDRV_ENTER;
	if (!wifi_info) {
		vddbg("[RTK] Get failed\n");
		return LWNL_INVALID_ARGS;
	}

	if (g_mode == RTK_WIFI_NONE) {
		vddbg("[RTK] WiFi driver is not initialized\n");
		return LWNL_INVALID_ARGS;
	}

	char mac_str[18] = {
		0,
	};

	wifi_info->rssi = (int)0;
	if (g_mode == RTK_WIFI_SOFT_AP_IF) {
		wifi_info->wifi_status = TRWIFI_SOFTAP_MODE;
	} else {
		if (wifi_is_connected_to_ap() == RTW_SUCCESS) {
			wifi_info->wifi_status = TRWIFI_CONNECTED;
			int rssi_tmp;
			if (wifi_get_rssi(&rssi_tmp) == RTW_SUCCESS) {
				wifi_info->rssi = rssi_tmp;
			} else {
				vddbg("[RTK] Failed to get rssi\n");
				return LWNL_FAIL;
			}
		} else {
			wifi_info->wifi_status = TRWIFI_DISCONNECTED;
		}
	}

	return LWNL_SUCCESS;
}

trwifi_result_e rtkdrv_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config)
{
	RTKDRV_ENTER;
	if (!softap_config) {
		return LWNL_INVALID_ARGS;
	}

	if (g_mode == RTK_WIFI_SOFT_AP_IF) {
		vddbg("[RTK] softap is already running\n");
		return LWNL_FAIL;
	}

	if (cmd_wifi_ap((wifi_utils_softap_config_s *)softap_config) != RTK_STATUS_SUCCESS) {
		vddbg("[RTK] Failed to start AP mode\n");
		return LWNL_FAIL;
	}

	vdvdbg("[RTK] Start SoftAp mode\n");
	if (WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler) != RTK_STATUS_SUCCESS) {
			vddbg("[RTK] link callback handles: register failed !\n");
			return LWNL_FAIL;
		}
	vdvdbg("[RTK] link callback handles: registered\n");
	g_mode = RTK_WIFI_SOFT_AP_IF;

	return LWNL_SUCCESS;
}

trwifi_result_e rtkdrv_start_sta(struct netdev *dev)
{
	RTKDRV_ENTER;
	if (g_mode == RTK_WIFI_STATION_IF) {
		vddbg("[RTK] Station mode is already running\n");
		return LWNL_FAIL;
	}

	if (WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler) != RTK_STATUS_SUCCESS) {
		vddbg("[RTK] link callback handles: register failed !\n");
		return LWNL_FAIL;
	}
	vdvdbg("[RTK] link callback handles: registered\n");

	if (cmd_wifi_off() == RTK_STATUS_SUCCESS) {
		g_mode = RTK_WIFI_NONE;
		vTaskDelay(50);
	} else {
		vddbg("[RTK] Failed to stop SoftAP mode\n");
	}

	if (cmd_wifi_on(RTK_WIFI_STATION_IF) != RTK_STATUS_SUCCESS) {
		vddbg("[RTK] Failed to strate station\n");
		return LWNL_FAIL;
	}

	vdvdbg("[RTK] Start station mode\n");
	g_mode = RTK_WIFI_STATION_IF;

	return LWNL_SUCCESS;
}

trwifi_result_e rtkdrv_stop_softap(struct netdev *dev)
{
	RTKDRV_ENTER;

	if (g_mode == RTK_WIFI_SOFT_AP_IF) {

		if (cmd_wifi_off() == RTK_STATUS_SUCCESS) {
			g_mode = RTK_WIFI_NONE;
			vdvdbg("[RTK] Stop AP mode successfully\n");
		} else {
			vddbg("[RTK] Stop AP mode fail\n");
			return LWNL_FAIL;
		}
	} else {
		vddbg("[RTK] Mode is not AP mode\n");
		return LWNL_FAIL;
	}
	return LWNL_SUCCESS;
}

trwifi_result_e rtkdrv_set_autoconnect(struct netdev *dev, uint8_t check)
{
	RTKDRV_ENTER;
	if (wifi_set_autoreconnect(check) != RTK_STATUS_SUCCESS) {
		vddbg("External Autoconnect failed to set %d", check);
		return LWNL_FAIL;
	}

	vdvdbg("[RTK] External Autoconnect set to %d\n", check);
	return LWNL_SUCCESS;
}

int rtkdrv_linkoutput(struct netdev *dev, void *buf, uint16_t dlen)
{
	// To Do
	return 0;
}

int rtk_drv_initialize(void)
{
	RTKDRV_ENTER;

	struct nic_io_ops nops = {rtkdrv_linkoutput, NULL};
	struct netdev_config nconfig;
	nconfig.ops = &nops;
	nconfig.flag = NM_FLAG_ETHARP | NM_FLAG_ETHERNET | NM_FLAG_BROADCAST | NM_FLAG_IGMP;
	nconfig.mtu = CONFIG_NET_ETH_MTU; // is it right that vendor decides MTU size??
	nconfig.hwaddr_len = IFHWADDRLEN;

	nconfig.is_default = 1;

	nconfig.type = NM_WIFI;
	nconfig.t_ops.wl = &g_trwifi_drv_ops;
	g_rtk_netdev = netdev_register(&nconfig);
	return (g_rtk_netdev != NULL ? 0 : -1);
}
