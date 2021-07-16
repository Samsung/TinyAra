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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/kmalloc.h>
#include <tinyara/arch.h>
#include <tinyara/fs/fs.h>
#include <tinyara/sched.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/net/if/wifi.h>
#include <tinyara/wifi/slsi/slsi_wifi_api.h>

#define DHCP_RETRY_COUNT           1
#define SLSI_DRV_SCAN_DEBUG        0

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define SLSIDRV_TAG "[SLSIDRV]"

#define SLSIDRV_ERR                                                         \
	do {                                                                    \
		vddbg(SLSIDRV_TAG"[ERR] %s: %d line err(%s)\n",                     \
				  __FILE__, __LINE__, strerror(errno));                     \
	} while (0)

#define SLSIDRV_ENTER                                                       \
	do {                                                                    \
		vddbg(SLSIDRV_TAG"%s:%d\n", __FILE__, __LINE__);                    \
	} while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct _wifi_scan_filter_result_ {
	int scan_flag;
	sem_t scan_sem;
	uint8_t scan_ssid[TRWIFI_SSID_LEN + 1];
	trwifi_scan_list_s *result_list;
};
typedef struct _wifi_scan_filter_result_ scan_filter_result_t;

static trwifi_result_e slsidrv_init(struct netdev *dev);
static trwifi_result_e slsidrv_deinit(struct netdev *dev);
static trwifi_result_e slsidrv_scan_ap(struct netdev *dev, trwifi_ap_config_s *config);
static trwifi_result_e slsidrv_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg);
static trwifi_result_e slsidrv_disconnect_ap(struct netdev *dev, void *arg);
static trwifi_result_e slsidrv_get_info(struct netdev *dev, trwifi_info *wifi_info);
static trwifi_result_e slsidrv_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config);
static trwifi_result_e slsidrv_start_sta(struct netdev *dev);
static trwifi_result_e slsidrv_stop_softap(struct netdev *dev);
static trwifi_result_e slsidrv_set_autoconnect(struct netdev *dev, uint8_t check);
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
static scan_filter_result_t scan_filter_result;
static WiFi_InterFace_ID_t g_mode;
static struct trwifi_ops g_trwifi_drv_ops = {
	slsidrv_init,                   /* init */
	slsidrv_deinit,                 /* deinit */
	slsidrv_scan_ap,                /* scan_ap */
	slsidrv_connect_ap,             /* connect_ap */
	slsidrv_disconnect_ap,          /* disconnect_ap */
	slsidrv_get_info,               /* get_info */
	slsidrv_start_sta,              /* start_sta */
	slsidrv_start_softap,           /* start_softap */
	slsidrv_stop_softap,            /* stop_softap */
	slsidrv_set_autoconnect,        /* set_autoconnect */
	NULL                              /* drv_ioctl */
};

/*
 * DRIVER SPECIFIC
 */

static void
get_security_type(slsi_security_config_t *sec_modes, uint8_t num_sec_modes,
				   trwifi_ap_auth_type_e *auth,
				   trwifi_ap_crypto_type_e *crypto)
{
	if (!sec_modes) {
		*auth = TRWIFI_AUTH_OPEN;
		*crypto = TRWIFI_CRYPTO_NONE;
	} else {
		if (sec_modes->secmode == SLSI_SEC_MODE_WEP ||
			sec_modes->secmode == SLSI_SEC_MODE_WEP_SHARED ||
			sec_modes->secmode == (SLSI_SEC_MODE_WEP | SLSI_SEC_MODE_WEP_SHARED)) {
			*auth = TRWIFI_AUTH_WEP_SHARED;
			*crypto = TRWIFI_CRYPTO_WEP_64;
		} else if (num_sec_modes == 2 &&
				   sec_modes[0].secmode == SLSI_SEC_MODE_WPA_MIXED &&
				   sec_modes[1].secmode == SLSI_SEC_MODE_WPA2_MIXED) {
			*auth = TRWIFI_AUTH_WPA_AND_WPA2_PSK;
			*crypto = TRWIFI_CRYPTO_TKIP_AND_AES;
		} else if (sec_modes->secmode == SLSI_SEC_MODE_WPA2_MIXED) {
			*auth = TRWIFI_AUTH_WPA2_PSK;
			*crypto = TRWIFI_CRYPTO_TKIP_AND_AES;
		} else if (sec_modes->secmode == SLSI_SEC_MODE_WPA2_CCMP) {
			*auth = TRWIFI_AUTH_WPA2_PSK;
			*crypto = TRWIFI_CRYPTO_AES;
		} else if (sec_modes->secmode == SLSI_SEC_MODE_WPA2_TKIP) {
			*auth = TRWIFI_AUTH_WPA2_PSK;
			*crypto = TRWIFI_CRYPTO_TKIP;
		} else if (sec_modes->secmode == SLSI_SEC_MODE_WPA_MIXED) {
			*auth = TRWIFI_AUTH_WPA_PSK;
			*crypto = TRWIFI_CRYPTO_TKIP_AND_AES;
		} else if (sec_modes->secmode == SLSI_SEC_MODE_WPA_CCMP) {
			*auth = TRWIFI_AUTH_WPA_PSK;
			*crypto = TRWIFI_CRYPTO_AES;
		} else if (sec_modes->secmode == SLSI_SEC_MODE_WPA_TKIP) {
			*auth = TRWIFI_AUTH_WPA_PSK;
			*crypto = TRWIFI_CRYPTO_TKIP;
		} else {
			*auth = TRWIFI_AUTH_UNKNOWN;
			*crypto = TRWIFI_CRYPTO_UNKNOWN;
		}
	}
}

static void
free_scan_results(trwifi_scan_list_s *scan_list)
{
	trwifi_scan_list_s *cur = scan_list, *prev = NULL;
	while (cur) {
		prev = cur;
		cur = cur->next;
		kmm_free(prev);
	}
	scan_list = NULL;
}

static trwifi_result_e
fetch_scan_results(trwifi_scan_list_s **scan_list, slsi_scan_info_t **slsi_scan_info, const char *ssid)
{
	trwifi_result_e result = TRWIFI_FAIL;
	trwifi_scan_list_s *cur = NULL, *prev = NULL;
	slsi_scan_info_t *wifi_scan_iter = NULL;
	int cnt = 0;
	if (*slsi_scan_info) {
	/* Initialize pointer */
		wifi_scan_iter = *slsi_scan_info;
		while (wifi_scan_iter) {
#if SLSI_DRV_SCAN_DEBUG
			/* Debug */
			vddbg("SSID (");
			int i = 0;
			for (; i < wifi_scan_iter->ssid_len; i++) {
				vddbg("%c", wifi_scan_iter->ssid[i]);
			}
			vddbg(")\n");
			vddbg("rssi(%d)\n", wifi_scan_iter->rssi);
			vddbg("beacon_period(%d)\n", wifi_scan_iter->beacon_period);
			vddbg("channel(%d)\n", wifi_scan_iter->channel);
			vddbg("physical mode(%d)\n", wifi_scan_iter->phy_mode);
			vddbg("bss type(%d)\n", wifi_scan_iter->bss_type);
			vddbg("wps support(%d)\n", wifi_scan_iter->wps_support);
			vddbg("num_sec modes(%d)\n", wifi_scan_iter->num_sec_modes);
			vddbg("-----------------------------------------------\n");
#endif
			if (ssid != NULL && strlen(ssid) > 0 && strncmp(ssid, (const char *)wifi_scan_iter->ssid, SLSI_SSID_LEN + 1) != 0) {
				wifi_scan_iter = wifi_scan_iter->next;
				continue;
			}

			cur = (trwifi_scan_list_s *)kmm_malloc(sizeof(trwifi_scan_list_s));
			if (!cur) {
				free_scan_results(*scan_list);
				return result;
			}
			cur->next = NULL;

			memset(&cur->ap_info, 0x0, sizeof(trwifi_ap_scan_info_s));
			cur->ap_info.rssi = wifi_scan_iter->rssi;
			cur->ap_info.channel = wifi_scan_iter->channel;
			if (wifi_scan_iter->phy_mode == 1) {
				cur->ap_info.phy_mode = TRWIFI_IEEE_80211_N;
			} else {
				cur->ap_info.phy_mode = TRWIFI_IEEE_80211_LEGACY;
			}
			get_security_type(wifi_scan_iter->sec_modes, wifi_scan_iter->num_sec_modes,
			&cur->ap_info.ap_auth_type, &cur->ap_info.ap_crypto_type);
			strncpy(cur->ap_info.ssid, (const char *)wifi_scan_iter->ssid, wifi_scan_iter->ssid_len);
			cur->ap_info.ssid_length = (unsigned int)wifi_scan_iter->ssid_len;
			strncpy((char *)cur->ap_info.bssid, (const char *)wifi_scan_iter->bssid, TRWIFI_MACADDR_STR_LEN);
			cur->ap_info.bssid[TRWIFI_MACADDR_STR_LEN] = '\0';

			if (!prev) {
				*scan_list = cur;
			} else {
				prev->next = cur;
			}
			prev = cur;
			wifi_scan_iter = wifi_scan_iter->next;
			cnt++;
		}
		vddbg("%d records scanned\n", cnt);
		result = TRWIFI_SUCCESS;
	} else {
		vddbg("Scanning result is null...\n");
	}
	return result;
}

/*
 * Callback
 */
static int slsi_drv_callback_handler(void *arg)
{
	int *type = (int*)(arg);

	vddbg("Got callback from SLSI drv (%d)\n", *type);
	switch (*type) {
	case 1:
		trwifi_post_event(LWNL_EVT_STA_CONNECTED, NULL, 0);
		break;
	case 2:
		trwifi_post_event(LWNL_EVT_STA_CONNECT_FAILED, NULL, 0);
		break;
	case 3:
		trwifi_post_event(LWNL_EVT_SOFTAP_STA_JOINED, NULL, 0);
		break;
	case 4:
		trwifi_post_event(LWNL_EVT_STA_DISCONNECTED, NULL, 0);
		break;
	case 5:
		trwifi_post_event(LWNL_EVT_SOFTAP_STA_LEFT, NULL, 0);
		break;
	default:
		trwifi_post_event(LWNL_EVT_UNKNOWN, NULL, 0);
		break;
	}

	kmm_free(type);

	return 0;
}

static void linkup_handler(slsi_reason_t *reason)
{
	int *type = (int *)kmm_malloc(sizeof(int));
	if (type == NULL) {
		vddbg("malloc error\n");
		return;
	}

	if (g_mode == SLSI_WIFI_STATION_IF) {
		if (reason->reason_code == SLSI_STATUS_SUCCESS) {
			*type = 1;
		} else {
			*type = 2;
		}
	} else if (g_mode == SLSI_WIFI_SOFT_AP_IF) {
		*type = 3;
	}

	/*  If driver sends event to lwnl80211 directly it will generate platform watchdog */
	pthread_t tid;
	int ret = pthread_create(&tid, NULL, (pthread_startroutine_t)slsi_drv_callback_handler, (void *)type);
	if (ret != 0) {
		vddbg("pthread create fail(%d)\n", errno);
		kmm_free(type);
		return;
	}
	pthread_setname_np(tid, "trwifi_cbk_handler");
	pthread_detach(tid);
}


static void linkdown_handler(slsi_reason_t *reason)
{
	int *type = (int *)kmm_malloc(sizeof(int));
	if (type == NULL) {
		vddbg("malloc error linkdown\n");
		return;
	}
	*type = 4;
	if (g_mode == SLSI_WIFI_STATION_IF) {
		*type = 4;
	} else if (g_mode == SLSI_WIFI_SOFT_AP_IF) {
		*type = 5;
	}
	pthread_t tid;
	int ret = pthread_create(&tid, NULL, (pthread_startroutine_t)slsi_drv_callback_handler, (void *)type);
	if (ret != 0) {
		vddbg("pthread create fail(%d)\n", errno);
		kmm_free(type);
		return;
	}
	pthread_setname_np(tid, "trwifi_cbk_handler");
	pthread_detach(tid);
}


static int8_t slsi_drv_scan_callback_handler(slsi_reason_t *reason)
{
	int8_t result;
	trwifi_scan_list_s *scan_list = NULL;

	if (reason->reason_code != SLSI_STATUS_SUCCESS) {
		vddbg("Scan failed %d\n");
		trwifi_post_event(LWNL_EVT_SCAN_FAILED, NULL, 0);
		result = SLSI_STATUS_ERROR;
		goto return_result;
	}
	slsi_scan_info_t *wifi_scan_result;
	int8_t res = WiFiGetScanResults(&wifi_scan_result);
	if (res != SLSI_STATUS_SUCCESS) {
		result = SLSI_STATUS_ERROR;
		goto return_result;
	}

	if (scan_filter_result.scan_flag) {
		fetch_scan_results(&scan_filter_result.result_list, &wifi_scan_result, (const char *)scan_filter_result.scan_ssid);
		TRWIFI_POST_SCANEVENT(LWNL_EVT_SCAN_DONE, (void *)scan_filter_result.result_list);
		sem_post(&scan_filter_result.scan_sem);
	} else {
		if (fetch_scan_results(&scan_list, &wifi_scan_result, NULL) == TRWIFI_SUCCESS) {
			TRWIFI_POST_SCANEVENT(LWNL_EVT_SCAN_DONE, (void *)scan_list);
		} else {
			trwifi_post_event(LWNL_EVT_SCAN_FAILED, NULL, 0);
		}
		free_scan_results(scan_list);
	}

	WiFiFreeScanResults(&wifi_scan_result);

	return SLSI_STATUS_SUCCESS;

return_result:
	if (scan_filter_result.scan_flag) {
		sem_post(&scan_filter_result.scan_sem);
	}
	return result;
}


/*
 * Interface API
*/
trwifi_result_e slsidrv_init(struct netdev *dev)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	if (g_mode == SLSI_WIFI_NONE) {
		int ret = SLSI_STATUS_SUCCESS;
		ret = WiFiStart(SLSI_WIFI_STATION_IF, NULL);
		if (ret != SLSI_STATUS_SUCCESS) {
			vddbg("Failed to start STA mode\n");
			return result;
		}
		g_mode = SLSI_WIFI_STATION_IF;

		ret = WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler);
		if (ret != SLSI_STATUS_SUCCESS) {
			vddbg("Link callback handles: register failed !\n");
			return result;
		} else {
			vdvdbg("Link callback handles: registered\n");
		}

		ret = WiFiRegisterScanCallback(&slsi_drv_scan_callback_handler);
		if (ret != SLSI_STATUS_SUCCESS) {
			vddbg("[ERR] Register Scan Callback(%d)\n", ret);
			return result;
		}
		memset((void *)&scan_filter_result, 0, sizeof(scan_filter_result_t));
		sem_init(&scan_filter_result.scan_sem, 0, 0);
		scan_filter_result.result_list = NULL;
		scan_filter_result.scan_flag = 0;
		result = TRWIFI_SUCCESS;
	} else {
		vddbg("Already %d\n", g_mode);
	}
	return result;
}

trwifi_result_e slsidrv_deinit(struct netdev *dev)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	int ret = WiFiStop();
	if (ret == SLSI_STATUS_SUCCESS) {
		g_mode = SLSI_WIFI_NONE;
		result = TRWIFI_SUCCESS;
	} else {
		vddbg("Failed to stop STA mode\n");
	}
	return result;
}

trwifi_result_e slsidrv_scan_ap(struct netdev *dev, trwifi_scan_config_s *config)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	int8_t ret = WiFiRegisterScanCallback(&slsi_drv_scan_callback_handler);
	if (ret != SLSI_STATUS_SUCCESS) {
		vddbg("[ERR] Register Scan Callback(%d)\n", ret);
		return result;
	}

	scan_filter_result.scan_flag = 0;
	memset(scan_filter_result.scan_ssid, 0, SLSI_SSID_LEN + 1);
	if (config != NULL && config->ssid_length > 0) {
		scan_filter_result.scan_flag = 1;
		if (scan_filter_result.result_list != NULL) {
			free_scan_results(scan_filter_result.result_list);
			scan_filter_result.result_list = NULL;
		}
		strncpy((char *)scan_filter_result.scan_ssid, (const char *)config->ssid, config->ssid_length + 1);
		ret = WiFiScanNetwork((char *)scan_filter_result.scan_ssid);
	} else {
		ret = WiFiScanNetwork(NULL);
	}

	if (ret != SLSI_STATUS_SUCCESS) {
		vddbg("[ERR] WiFi scan fail(%d)\n", ret);
		return result;
	}

	if (scan_filter_result.scan_flag > 0) {
		struct timespec abstime;
		(void)clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec += (SLSI_SCAN_INTERVAL + 1);
		ret = sem_timedwait(&scan_filter_result.scan_sem, &abstime);
		if (ret != OK) {
			vddbg("WIFi scan filtering timeout!\n");
			scan_filter_result.scan_flag = 0;
			return TRWIFI_TIMEOUT;
		}
		vdvdbg("WIFi scan filtering succeed!\n");
		scan_filter_result.scan_flag = 0;
	}

	result = TRWIFI_SUCCESS;
	vddbg("WIFi Scan success\n");
	return result;
}

trwifi_result_e slsidrv_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_INVALID_ARGS;
	if (!ap_connect_config) {
		return result;
	}

	int ret;
	result = TRWIFI_FAIL;
	slsi_security_config_t *config = NULL;

	if (ap_connect_config->passphrase_length > 0) {
		// scan to get the security config if it is unknown
		if (ap_connect_config->ap_auth_type == TRWIFI_AUTH_UNKNOWN ||
			ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_UNKNOWN) {
			result = slsidrv_scan_ap(dev, ap_connect_config);
			if (result != TRWIFI_SUCCESS) {
				vddbg("slsidrv_scan_ap failed! ssid:%s\n", ap_connect_config->ssid);
				goto connect_ap_fail;
			}

			if (scan_filter_result.result_list != NULL) {
				// Choose AP with the highest rssi value
				trwifi_scan_list_s *best = NULL;
				trwifi_scan_list_s *iter;
				for (iter = scan_filter_result.result_list; iter != NULL; iter = iter->next) {
					vdvdbg("Filter result - rssi:%d channel:%d\n", iter->ap_info.rssi, iter->ap_info.channel);
					if (best == NULL || iter->ap_info.rssi > best->ap_info.rssi) {
						best = iter;
					}
				}
				// Use the auth_type/crypto_type value of best one.
				vdvdbg("Choose result - rssi:%d channel:%d auth:%d crypto:%d\n",
					best->ap_info.rssi, best->ap_info.channel, best->ap_info.ap_auth_type, best->ap_info.ap_crypto_type);
				ap_connect_config->ap_auth_type = best->ap_info.ap_auth_type;
				ap_connect_config->ap_crypto_type = best->ap_info.ap_crypto_type;
				// Free the result_list.
				free_scan_results(scan_filter_result.result_list);
				scan_filter_result.result_list = NULL;
			} else {
				vddbg("slsidrv_scan_ap finish, but result is empty! ssid:%s\n", ap_connect_config->ssid);
				result = TRWIFI_FAIL;
				goto connect_ap_fail;
			}
		}

		config = (slsi_security_config_t *)kmm_zalloc(sizeof(slsi_security_config_t));
		if (!config) {
			vddbg("Memory allocation failed!\n");
			goto connect_ap_fail;
		}

		if ((ap_connect_config->ap_auth_type == TRWIFI_AUTH_WEP_SHARED) &&
			(ap_connect_config->passphrase_length == 5 || ap_connect_config->passphrase_length == 13)) {
			config->passphrase[0] = '"';
			memcpy(&config->passphrase[1], ap_connect_config->passphrase,
				   ap_connect_config->passphrase_length);
			config->passphrase[ap_connect_config->passphrase_length + 1] = '"';
			config->passphrase[ap_connect_config->passphrase_length + 2] = '\0';
		} else {
			memcpy(config->passphrase, ap_connect_config->passphrase, ap_connect_config->passphrase_length);
		}

		if (ap_connect_config->ap_auth_type == TRWIFI_AUTH_WEP_SHARED) {
			config->secmode = SLSI_SEC_MODE_WEP_SHARED;
		} else if (ap_connect_config->ap_auth_type == TRWIFI_AUTH_WPA_PSK) {
			if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_AES) {
				config->secmode = SLSI_SEC_MODE_WPA_CCMP;
			} else if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP) {
				config->secmode = SLSI_SEC_MODE_WPA_TKIP;
			} else if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP_AND_AES) {
				config->secmode = SLSI_SEC_MODE_WPA_MIXED;
			}
		} else if (ap_connect_config->ap_auth_type == TRWIFI_AUTH_WPA2_PSK) {
			if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_AES) {
				config->secmode = SLSI_SEC_MODE_WPA2_CCMP;
			} else if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP) {
				config->secmode = SLSI_SEC_MODE_WPA2_TKIP;
			} else if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP_AND_AES) {
				config->secmode = SLSI_SEC_MODE_WPA2_MIXED;
			}
		} else if (ap_connect_config->ap_auth_type == TRWIFI_AUTH_WPA_AND_WPA2_PSK) {
			if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_AES) {
				config->secmode = (SLSI_SEC_MODE_WPA_CCMP | SLSI_SEC_MODE_WPA2_CCMP);
			} else if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP) {
				config->secmode = (SLSI_SEC_MODE_WPA_TKIP | SLSI_SEC_MODE_WPA2_TKIP);
			} else if (ap_connect_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP_AND_AES) {
				config->secmode = (SLSI_SEC_MODE_WPA_MIXED | SLSI_SEC_MODE_WPA2_MIXED);
			}
		} else {
			/* wrong security type */
			vddbg("Wrong security type\n");
			goto connect_ap_fail;
		}
	} else if (ap_connect_config->ap_auth_type !=  TRWIFI_AUTH_OPEN ||
				ap_connect_config->ap_crypto_type != TRWIFI_CRYPTO_NONE) {
		vddbg("No passphrase!\n");
		goto connect_ap_fail;
	}

	ret = WiFiNetworkJoin((uint8_t *)ap_connect_config->ssid, ap_connect_config->ssid_length, NULL, config);
	if (ret != SLSI_STATUS_SUCCESS) {
		if (ret == SLSI_STATUS_ALREADY_CONNECTED) {
			vdvdbg("WiFiNetworkJoin already connected\n");
			result = TRWIFI_ALREADY_CONNECTED;
		} else {
			vddbg("WiFiNetworkJoin failed: %d, %s\n", ret, ap_connect_config->ssid);
			goto connect_ap_fail;
		}
	} else {
		result = TRWIFI_SUCCESS;
		vdvdbg("Successfully joined the network: %s(%d)\n", ap_connect_config->ssid,
			  ap_connect_config->ssid_length);
	}

connect_ap_fail:
	if (config) {
		kmm_free(config);
		config = NULL;
	}

	return result;
}

trwifi_result_e slsidrv_disconnect_ap(struct netdev *dev, void *arg)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	int ret = WiFiNetworkLeave();
	if (ret == SLSI_STATUS_SUCCESS) {
		vddbg("WiFiNetworkLeave success\n");
		result = TRWIFI_SUCCESS;
	} else {
		vddbg("WiFiNetworkLeave fail because of %d\n", ret);
	}

	return result;
}

trwifi_result_e slsidrv_get_info(struct netdev *dev, trwifi_info *wifi_info)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_INVALID_ARGS;
	if (wifi_info) {
		result = TRWIFI_FAIL;
		if (g_mode != SLSI_WIFI_NONE) {
			int ret = WiFiGetMac(wifi_info->mac_address);
			if (ret == SLSI_STATUS_SUCCESS) {
				wifi_info->rssi = (int)0;
				if (g_mode == SLSI_WIFI_SOFT_AP_IF) {
					wifi_info->wifi_status = TRWIFI_SOFTAP_MODE;
				} else if (g_mode == SLSI_WIFI_STATION_IF) {
					uint8_t isConnected;
					if (WiFiIsConnected(&isConnected, NULL) == SLSI_STATUS_SUCCESS) {
						int8_t rssi;
						wifi_info->wifi_status = TRWIFI_CONNECTED;
						if (WiFiGetRssi(&rssi) == SLSI_STATUS_SUCCESS) {
							wifi_info->rssi = (int)rssi;
						}
					} else {
						wifi_info->wifi_status = TRWIFI_DISCONNECTED;
					}
				}
				result = TRWIFI_SUCCESS;
			} else {
				vddbg("no MAC exists\n");
			}
		} else {
			vddbg("need to init... get info fail\n");
		}
	}
	return result;
}

trwifi_result_e slsidrv_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config)
{
	SLSIDRV_ENTER;
	if (!softap_config) {
		return TRWIFI_INVALID_ARGS;
	}

	trwifi_result_e ret = TRWIFI_FAIL;
	slsi_ap_config_t *ap_config = NULL;
	slsi_security_config_t *security_config = NULL;

	ap_config = (slsi_ap_config_t *)kmm_zalloc(sizeof(slsi_ap_config_t));
	if (!ap_config) {
		vddbg("Memory allocation failed!\n");
		return TRWIFI_FAIL;
	}

	/* add initialization code as slsi_app */
	ap_config->beacon_period = 100;
	ap_config->DTIM = 1;
	ap_config->phy_mode = 1; //1 for 11n, 0 for legacy

	if (softap_config->channel > 14 || softap_config->channel < 1) {
		vddbg("Channel needs to be between 1 and 14" " (highest channel depends on regulatory of countries)\n");
		goto start_soft_ap_fail;
	} else {
		ap_config->channel = softap_config->channel;
	}

	if (softap_config->ssid_length < 1) {
		goto start_soft_ap_fail;
	} else {
		memcpy(&ap_config->ssid, softap_config->ssid, softap_config->ssid_length);
		ap_config->ssid_len = softap_config->ssid_length;
	}

	if (softap_config->passphrase_length < 1) {
		goto start_soft_ap_fail;
	} else {
		security_config = (slsi_security_config_t *)kmm_zalloc(sizeof(slsi_security_config_t));
		if (!security_config) {
			vddbg("Memory allocation failed!\n");
			goto start_soft_ap_fail;
		}
		memcpy(security_config->passphrase, softap_config->passphrase, softap_config->passphrase_length);
	}

	if ((softap_config->ap_auth_type == TRWIFI_AUTH_WPA_PSK) &&
		(softap_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP)) {
		security_config->secmode = SLSI_SEC_MODE_WPA_TKIP;
	} else if ((softap_config->ap_auth_type == TRWIFI_AUTH_WPA2_PSK) &&
			   (softap_config->ap_crypto_type == TRWIFI_CRYPTO_AES)) {
		security_config->secmode = SLSI_SEC_MODE_WPA2_CCMP;
	} else if ((softap_config->ap_auth_type == TRWIFI_AUTH_WPA_AND_WPA2_PSK) &&
			   (softap_config->ap_crypto_type == TRWIFI_CRYPTO_TKIP_AND_AES)) {
		security_config->secmode = (SLSI_SEC_MODE_WPA_MIXED | SLSI_SEC_MODE_WPA2_MIXED);
	} else {
		// if not WPA-TKIP, WPA2-AES, WPA/WPA2 TKIP/AES/MIXED, return fail.
		vddbg("Wrong security config. Match proper auth and crypto.\n");
		goto start_soft_ap_fail;
	}
	ap_config->security = security_config;

	if (WiFiStart(SLSI_WIFI_SOFT_AP_IF, ap_config) != SLSI_STATUS_SUCCESS) {
		vddbg("Failed to start AP mode\n");
		goto start_soft_ap_fail;
	}
	g_mode = SLSI_WIFI_SOFT_AP_IF;
	vdvdbg("SoftAP with SSID: %s has successfully started!\n", softap_config->ssid);

	ret = WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler);
	if (ret != SLSI_STATUS_SUCCESS) {
		vddbg("Link callback handles: register failed !\n");
		return TRWIFI_FAIL;
	} else {
		vdvdbg("Link callback handles: registered\n");
	}

	ret = WiFiRegisterScanCallback(&slsi_drv_scan_callback_handler);
	if (ret != SLSI_STATUS_SUCCESS) {
		vddbg("[ERR] Register Scan Callback(%d)\n", ret);
		return TRWIFI_FAIL;
	}

	ret = TRWIFI_SUCCESS;

start_soft_ap_fail:
	if (ap_config) {
		kmm_free(ap_config);
		ap_config = NULL;
	}
	if (security_config) {
		kmm_free(security_config);
		security_config = NULL;
	}
	return ret;
}

trwifi_result_e slsidrv_start_sta(struct netdev *dev)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	int ret = SLSI_STATUS_SUCCESS;
	ret = WiFiStart(SLSI_WIFI_STATION_IF, NULL);
	if (ret == SLSI_STATUS_SUCCESS) {
		g_mode = SLSI_WIFI_STATION_IF;
		ret = WiFiRegisterLinkCallback(&linkup_handler, &linkdown_handler);
		if (ret == SLSI_STATUS_SUCCESS) {
			vdvdbg("Link callback handles: registered\n");
			ret = WiFiRegisterScanCallback(&slsi_drv_scan_callback_handler);
			if (ret == SLSI_STATUS_SUCCESS) {
				vdvdbg("Scan callback handles: registered\n");
				result = TRWIFI_SUCCESS;
			} else {
				vddbg("[ERR] Register Scan Callback(%d)\n", ret);
			}
		} else {
			vddbg("[ERR] Register Link Callback(%d)\n", ret);
		}
	} else {
		vddbg("Failed to start STA mode\n");
	}
	return result;
}

trwifi_result_e slsidrv_stop_softap(struct netdev *dev)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	int ret;
	if (g_mode == SLSI_WIFI_SOFT_AP_IF) {
		ret = WiFiStop();
		if (ret == SLSI_STATUS_SUCCESS) {
			result = TRWIFI_SUCCESS;
			vddbg("Stop AP mode successfully\n");
		} else {
			vddbg("Stop AP mode fail\n");
		}
	} else {
		vddbg("Mode is not AP mode\n");
	}
	return result;
}

trwifi_result_e slsidrv_set_autoconnect(struct netdev *dev, uint8_t check)
{
	SLSIDRV_ENTER;
	trwifi_result_e result = TRWIFI_FAIL;
	int ret = WiFiSetAutoconnect(check);
	if (ret == SLSI_STATUS_SUCCESS) {
		result = TRWIFI_SUCCESS;
		vddbg("External Autoconnect set to %d\n", check);
	} else {
		vddbg("External Autoconnect failed to set %d", check);
	}
	return result;
}

int slsi_drv_initialize(void)
{
	SLSIDRV_ENTER;
	struct netdev *dev = NULL;
	dev = (struct netdev *)kmm_malloc(sizeof(struct netdev));
	if (!dev) {
		return -1;
	}
	dev->ifname[0] = 'w';
	dev->ifname[1] = 'l';
	dev->ifname[2] = '1';

	dev->type = NM_WIFI;
	dev->ops = (void *)&g_trwifi_drv_ops;

	int res = lwnl_register_dev(dev);
	if (res < 0) {
		vddbg("register dev to lwnl fail\n");
		kmm_free(dev);
		return -1;
	}
	return 0;
}
