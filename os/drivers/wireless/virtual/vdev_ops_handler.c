/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
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
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>
#include <time.h>
#include <tinyara/net/if/wifi.h>
#include <tinyara/lwnl/lwnl.h>
#include "vdev_handler.h"
#include "vdev_command.h"

#define WM_NSOFTAP_SSID "no_sta_0101" // for auto test

typedef struct {
	int pending;
	int key;
	int value;
	int wait;
} vdev_evt_s;

static trwifi_result_e vwifi_handle_init(void *req);
static trwifi_result_e vwifi_handle_deinit(void *req);
static trwifi_result_e vwifi_handle_scanap(void *req);
static trwifi_result_e vwifi_handle_connectap(void *req);
static trwifi_result_e vwifi_handle_disconnectap(void *req);
static trwifi_result_e vwifi_handle_getinfo(void *req);
static trwifi_result_e vwifi_handle_startsta(void *req);
static trwifi_result_e vwifi_handle_startsoftap(void *req);
static trwifi_result_e vwifi_handle_stopsoftap(void *req);
static trwifi_result_e vwifi_handle_setautoconnect(void *req);
static int _handle_set_command(trwifi_msg_s *msg);
static int _handle_evt_command(trwifi_msg_s *msg);

static int g_vdev_ops_result = TRWIFI_SUCCESS;
static vdev_evt_s g_vdev_evt = {0, 0, 0, 0};
static const vdev_evt_s VDEV_EVT_INIT = {0, 0, 0, 0};

int _handle_set_command(trwifi_msg_s *msg)
{
	vwifi_ioctl_msg_s *vmsg = (vwifi_ioctl_msg_s *)msg->data;
	if (!vmsg) {
		assert(0);
		return -1;
	}
	switch (vmsg->key) {
	case VWIFI_KEY_RESULT:
		g_vdev_ops_result = vmsg->value;
		break;
	default:
		assert(0);
	}
	return 0;
}

int _handle_evt_command(trwifi_msg_s *msg)
{
	vwifi_ioctl_msg_s *vmsg = (vwifi_ioctl_msg_s *)msg->data;
	if (!vmsg) {
		assert(0);
		return -1;
	}
	if (msg->cmd == VWIFI_CMD_GEN_EVT_FUNC) {
		g_vdev_evt.pending = 1;
		g_vdev_evt.key = vmsg->key;
		g_vdev_evt.value = vmsg->value;
		g_vdev_evt.wait = vmsg->wait;
	} else if (msg->cmd == VWIFI_CMD_GEN_EVT) {
		vdvdbg("key %d wait %d value %d\n", vmsg->key, vmsg->wait, vmsg->value);
		vwifi_create_event(vmsg->key, vmsg->value, vmsg->wait);
	}
	return 0;
}

int _handle_power_command(trwifi_msg_s *msg)
{
	if (msg->cmd == TRWIFI_MSG_SET_POWERMODE) {
		int *mode = (int *)msg->data;
		if (*mode == TRWIFI_POWERMODE_ON) {
			vdvdbg("set power mode on\n");
			return TRWIFI_SUCCESS;
		} else if (*mode == TRWIFI_POWERMODE_OFF) {
			vdvdbg("set power mode off\n");
			return TRWIFI_SUCCESS;
		}
	} else if (msg->cmd == TRWIFI_MSG_GET_POWERMODE) {
		vdvdbg("get power mode\n");
		return TRWIFI_SUCCESS;
	}
	return TRWIFI_FAIL;
}
static inline void _check_pending_event(void)
{
	if (!g_vdev_evt.pending) {
		return;
	}
	vwifi_create_event(g_vdev_evt.key, g_vdev_evt.value, g_vdev_evt.wait);
	g_vdev_evt = VDEV_EVT_INIT;
}

trwifi_result_e vwifi_handle_init(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_deinit(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_scanap(void *req)
{
	VWIFI_ENTRY;
	struct vwifi_req *vreq = (struct vwifi_req *)req;
	trwifi_scan_config_s *config = (trwifi_scan_config_s *)(vreq->arg);
	if (config) {
		vdvdbg("ssid length %d\n", config->ssid_length);
		vdvdbg("channel %d\n", config->channel);
	}
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_connectap(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_disconnectap(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_getinfo(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_startsta(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_startsoftap(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_stopsoftap(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_setautoconnect(void *req)
{
	VWIFI_ENTRY;
	_check_pending_event();
	return g_vdev_ops_result;
}

trwifi_result_e vwifi_handle_ioctl(void *req)
{
	VWIFI_ENTRY;
	struct vwifi_req *vreq = (struct vwifi_req *)req;
	trwifi_msg_s *msg = (trwifi_msg_s *)(vreq->arg);
	if (!msg) {
		return TRWIFI_INVALID_ARGS;
	}

	switch (msg->cmd) {
	case VWIFI_CMD_SET:
		_handle_set_command(msg);
		break;
	case VWIFI_CMD_GEN_EVT:
	case VWIFI_CMD_GEN_EVT_FUNC:
		_handle_evt_command(msg);
		break;
	case TRWIFI_MSG_GET_POWERMODE:
	case TRWIFI_MSG_SET_POWERMODE:
		_handle_power_command(msg);
		break;
	default:
		return TRWIFI_NOT_SUPPORTED;
	}
	return g_vdev_ops_result;
}

static struct vwifi_ops g_trwifi_ops = {
	vwifi_handle_init,
	vwifi_handle_deinit,
	vwifi_handle_scanap,
	vwifi_handle_connectap,
	vwifi_handle_disconnectap,
	vwifi_handle_getinfo,
	vwifi_handle_startsta,
	vwifi_handle_startsoftap,
	vwifi_handle_stopsoftap,
	vwifi_handle_setautoconnect,
	vwifi_handle_ioctl,
};

struct vwifi_ops *get_vdev_ops(void)
{
	return &g_trwifi_ops;
}
