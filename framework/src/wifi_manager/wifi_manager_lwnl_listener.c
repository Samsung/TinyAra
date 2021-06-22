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
#include <tinyara/config.h>
#include <debug.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/wifi/wifi_utils.h>
#include "wifi_manager_lwnl_listener.h"
#include "wifi_manager_dhcp.h"
#include "wifi_manager_event.h"
#include "wifi_manager_msghandler.h"
#include "wifi_manager_log.h"
#include "wifi_manager_message.h"

static int _lwnl_convert_scan(wifi_utils_scan_list_s **scan_list, void *input, int len)
{
	WM_LOG_VERBOSE("[WU] T%d %s len(%d)\n", getpid(), __FUNCTION__, len);
	int remain = len;
	wifi_utils_scan_list_s *prev = NULL;

	while (remain > 0) {
		wifi_utils_scan_list_s *item = (wifi_utils_scan_list_s *)malloc(sizeof(wifi_utils_scan_list_s));
		if (!item) {
			// To Do
			return -1;
		}
		// definition of wifi_utils_scan_list_s and lwnl80211_scan_list shoud be same
		memcpy(&item->ap_info, input, sizeof(wifi_utils_ap_scan_info_s));
		item->next = NULL;

		remain -= sizeof(wifi_utils_ap_scan_info_s);
		input = input + sizeof(wifi_utils_ap_scan_info_s);
		if (!prev) {
			prev = item;
			*scan_list = item;
			continue;
		}
		prev->next = item;
		prev = item;
	}

	return 0;
}

static wifi_utils_scan_list_s *_lwnl_handle_scan(int fd, int len)
{
	char *buf = (char *)malloc(len);
	if (!buf) {
		return NULL;
	}

	int res = read(fd, buf, len);
	if (res != len) {
		WM_LOG_ERROR("read error\n");
		free(buf);
		return NULL;
	}

	wifi_utils_scan_list_s *scan_list = NULL;
	res = _lwnl_convert_scan(&scan_list, buf, len);
	free(buf);
	if (res < 0) {
		return NULL;
	}
	return scan_list;
}

/*  only wifi msg handler thread can use this variable */
static wifimgr_msg_s g_msg = {EVT_NONE, WIFI_MANAGER_FAIL, NULL, NULL};

static inline void LWNL_SET_MSG(wifimgr_msg_s *msg, wifimgr_evt_e event,
								wifi_manager_result_e result, void *param, sem_t *signal)
{
	msg->event = event;
	msg->result = result;
	msg->param = param;
	msg->signal = signal;
}

static int _lwnl_call_event(int fd, lwnl_cb_status status, int len)
{
	switch (status) {
	case LWNL_STA_CONNECTED:
		LWNL_SET_MSG(&g_msg, EVT_STA_CONNECTED, WIFI_MANAGER_FAIL, NULL, NULL);
		break;
	case LWNL_STA_CONNECT_FAILED:
		LWNL_SET_MSG(&g_msg, EVT_STA_CONNECT_FAILED, WIFI_MANAGER_FAIL, NULL, NULL);
		break;
	case LWNL_STA_DISCONNECTED:
		LWNL_SET_MSG(&g_msg, EVT_STA_DISCONNECTED, WIFI_MANAGER_FAIL, NULL, NULL);
		break;
	case LWNL_SOFTAP_STA_JOINED:
		LWNL_SET_MSG(&g_msg, EVT_JOINED, WIFI_MANAGER_FAIL, NULL, NULL);
		break;
	case LWNL_SOFTAP_STA_LEFT:
		LWNL_SET_MSG(&g_msg, EVT_LEFT, WIFI_MANAGER_FAIL, NULL, NULL);
		break;
	case LWNL_SCAN_FAILED:
		LWNL_SET_MSG(&g_msg, EVT_SCAN_DONE, WIFI_MANAGER_FAIL, NULL, NULL);
		break;
	case LWNL_SCAN_DONE:
	{
		wifi_utils_scan_list_s *scan_list = _lwnl_handle_scan(fd, len);
		if (scan_list) {
			LWNL_SET_MSG(&g_msg, EVT_SCAN_DONE, WIFI_MANAGER_FAIL, scan_list, NULL);
		} else {
			LWNL_SET_MSG(&g_msg, EVT_SCAN_DONE, WIFI_MANAGER_FAIL, NULL, NULL);
		}
		break;
	}
	default:
		WM_LOG_ERROR("Bad status received (%d)\n", status);
		WM_ERR;
		return -1;
	}
	return 0;
}

/**
 * Public
 */
int lwnl_fetch_event(int fd, void *buf, int buflen)
{
	lwnl_cb_status status;
	uint32_t len;
	char type_buf[8] = {0,};
	handler_msg *hmsg = (handler_msg *)buf;

	int nbytes = read(fd, (char *)type_buf, 8);
	if (nbytes < 0) {
		WM_LOG_ERROR("Failed to receive (nbytes=%d)\n", nbytes);
		WM_ERR;
		return -1;
	}

	memcpy(&status, type_buf, sizeof(lwnl_cb_status));
	memcpy(&len, type_buf + sizeof(lwnl_cb_status), sizeof(uint32_t));

	WM_LOG_VERBOSE("scan state(%d) length(%d)\n", status, len);
	(void)_lwnl_call_event(fd, status, len);

	hmsg->signal = NULL;
	hmsg->msg = &g_msg;

	return 0;
}
