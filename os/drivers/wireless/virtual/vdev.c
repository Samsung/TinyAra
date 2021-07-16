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

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include <debug.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/kthread.h>
#include <tinyara/netmgr/netdev_mgr.h>
#include <tinyara/net/if/wifi.h>
#include "vdev_handler.h"

#define VWIFI_MSG_QUEUE_NAME "/dev/vwifi"

/****************************************************************************
 * Private Types
 ****************************************************************************/
static trwifi_result_e vdev_init(struct netdev *dev);
static trwifi_result_e vdev_deinit(struct netdev *dev);
static trwifi_result_e vdev_scan_ap(struct netdev *dev, trwifi_scan_config_s *config);
static trwifi_result_e vdev_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg);
static trwifi_result_e vdev_disconnect_ap(struct netdev *dev, void *arg);
static trwifi_result_e vdev_get_info(struct netdev *dev, trwifi_info *wifi_info);
static trwifi_result_e vdev_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config);
static trwifi_result_e vdev_start_sta(struct netdev *dev);
static trwifi_result_e vdev_stop_softap(struct netdev *dev);
static trwifi_result_e vdev_set_autoconnect(struct netdev *dev, uint8_t check);
static trwifi_result_e vdev_drv_ioctl(struct netdev *dev, trwifi_msg_s *msg);

static int vdev_linkoutput(struct netdev *dev, void *buf, uint16_t dlen);
static int vdev_set_multicast_list(struct netdev *dev, const struct in_addr *group, netdev_mac_filter_action action);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
static struct trwifi_ops g_trwifi_drv_ops = {
	vdev_init,                   /* init */
	vdev_deinit,                 /* deinit */
	vdev_scan_ap,                /* scan_ap */
	vdev_connect_ap,             /* connect_ap */
	vdev_disconnect_ap,          /* disconnect_ap */
	vdev_get_info,               /* get_info */
	vdev_start_sta,              /* start_sta */
	vdev_start_softap,           /* start_softap */
	vdev_stop_softap,            /* stop_softap */
	vdev_set_autoconnect,        /* set_autoconnect */
	vdev_drv_ioctl                   /* drv_ioctl */
};

struct netdev *g_vwifi_dev = NULL;
static uint8_t g_hwaddr[IFHWADDRLEN] = {0x0e, 0x04, 0x96, 0x1d, 0xb3, 0xb0};

extern void vwifi_handle_packet(uint8_t *buf, uint32_t len);
extern void vwifi_initialize_scan(void);

/*
 * Callback
 */
static inline int _create_message(struct vwifi_msg **vmsg, struct vwifi_req *req)
{
	struct vwifi_msg *tmsg = (struct vwifi_msg *)kmm_malloc(sizeof(struct vwifi_msg));
	if (!tmsg) {
		VWIFI_ERROR(0);
		return -1;
	}
	sem_t *signal = (sem_t *)kmm_malloc(sizeof(sem_t));
	if (!signal) {
		kmm_free(tmsg);
		VWIFI_ERROR(0);
		return -1;
	}
	sem_init(signal, 0, 0);

	tmsg->req = req;
	tmsg->signal = signal;

	*vmsg = tmsg;

	return 0;
}

static inline int _destroy_message(struct vwifi_msg *msg)
{
	if (msg) {
		if (msg->signal) {
			sem_destroy(msg->signal);
			kmm_free(msg->signal);
		}
		kmm_free(msg);
	}

	return 0;
}

static inline void _wait_message(struct vwifi_msg *msg)
{
	int res = sem_wait(msg->signal);
	if (res < 0) {
		VWIFI_ERROR(res);
	}
}

static inline void _send_signal(struct vwifi_msg *msg)
{
	int res = sem_post(msg->signal);
	if (res < 0) {
		VWIFI_ERROR(res);
	}
}

static inline int _recv_message(int fd, char *buf, int buflen)
{
	int received = 0;
	while (1) {
		int res = read(fd, (void *)(buf + received), buflen - received);
		if (res < 0) {
			VWIFI_ERROR(res);
			return -1;
		}
		received += res;
		if (received == buflen) {
			break;
		}
	}
	return 0;
}

static inline int _send_message(int fd, char *buf, int buflen)
{
	int sent = 0;
	while (1) {
		int res = write(fd, (void *)(buf + sent), buflen - sent);
		if (res < 0) {
			VWIFI_ERROR(res);
			return -1;
		}
		sent += res;
		if (sent == buflen) {
			break;
		}
	}
	return 0;
}

static inline int _progress_message(struct vwifi_req *req)
{
	int fd = open(VWIFI_MSG_QUEUE_NAME, O_WRONLY);
	if (fd < 0) {
		VWIFI_ERROR(0);
		return -1;
	}

	struct vwifi_msg *msg = NULL;
	int res = _create_message(&msg, req);
	if (res < 0) {
		VWIFI_ERROR(res);
		close(fd);
		return -1;
	}

	res = _send_message(fd, (char *)msg, sizeof(struct vwifi_msg));
	close(fd);
	if (res < 0) {
		VWIFI_ERROR(res);
		_destroy_message(msg);
		return -1;
	}

	_wait_message(msg);
	_destroy_message(msg);

	return 0;
}

int _vwifi_create_msgqueue(int *fd)
{
	int res = mkfifo(VWIFI_MSG_QUEUE_NAME, 0666);
	if (res < 0 && res != -EEXIST) {
		VWIFI_ERROR(0);
		return -1;
	}

	*fd = open(VWIFI_MSG_QUEUE_NAME, O_RDWR);
	if (*fd < 0) {
		VWIFI_ERROR(0);
		unlink(VWIFI_MSG_QUEUE_NAME);
		return -1;
	}

	return 0;
}

/*
 * Internal Function
 */
static struct netdev* vdev_register_dev(void)
{
	struct nic_io_ops nops = {vdev_linkoutput, vdev_set_multicast_list};
	struct netdev_config nconfig;
	nconfig.ops = &nops;
	nconfig.flag = NM_FLAG_ETHARP | NM_FLAG_ETHERNET | NM_FLAG_BROADCAST | NM_FLAG_IGMP;
	nconfig.mtu = CONFIG_NET_ETH_MTU; // is it right that vendor decides MTU size??
	nconfig.hwaddr_len = IFHWADDRLEN;
	nconfig.is_default = 1;

	nconfig.type = NM_WIFI;
	nconfig.t_ops.wl = &g_trwifi_drv_ops;
	nconfig.priv = NULL;

	return netdev_register(&nconfig);
}

/*
 * Handler
 */
static void vdev_run(int argc, char *argv[])
{
	int fd;
	int res = _vwifi_create_msgqueue(&fd);
	if (res < 0) {
		VWIFI_ERROR(0);
		return;
	}

	fd_set rfds, tfds;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	while (1) {
		tfds = rfds;
		res = select(fd + 1, &tfds, NULL, NULL, NULL);
		if (res <= 0) {
			if (errno == EINTR) {
				continue;
			}
			VWIFI_ERROR(res);
			break;
		}
		struct vwifi_msg msg;
		res = _recv_message(fd, (char *)&msg, sizeof(struct vwifi_msg));
		if (res < 0) {
			VWIFI_ERROR(res);
			break;
		}

		res = vwifi_handle_message(msg.req);
		if (res < 0) {
			VWIFI_ERROR(res);
			break;
		}
		_send_signal(&msg);
	}
	return;
}

void vwifi_start(void)
{
	vwifi_initialize_scan();
	g_vwifi_dev = vdev_register_dev();
	if (!g_vwifi_dev) {
		VWIFI_ERROR(0);
		return;
	}
	netdev_set_hwaddr(g_vwifi_dev, g_hwaddr, IFHWADDRLEN);

	int new_thread = kernel_thread("wifi mocking driver", 100, 2048,
								   (main_t)vdev_run, (char *const *)NULL);
	if (new_thread < 0) {
		VWIFI_ERROR(new_thread);
	}
	return;
}

void vwifi_send_packet(uint8_t *buf, uint32_t len)
{
	VWIFI_LOG("send packet %d\n", len);
	if (!g_vwifi_dev) {
		VWIFI_ERROR(0);
		return;
	}
	netdev_input(g_vwifi_dev, buf, len);
}

void up_netinitialize(void)
{
	return;
}

/*
 * Interface API
 */
trwifi_result_e vdev_init(struct netdev *dev)
{
	VWIFI_ENTRY;

	struct vwifi_req req = {VWIFI_MSG_INIT, NULL, 0};
	int res = _progress_message(&req);
	if (res < 0) {
		VWIFI_ERROR(0);
		return TRWIFI_FAIL;
	}
	return req.res;
}

trwifi_result_e vdev_deinit(struct netdev *dev)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_DEINIT, NULL, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}
	return tres;
}

trwifi_result_e vdev_scan_ap(struct netdev *dev, trwifi_scan_config_s *config)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_SCANAP, (void *)config, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}
	return tres;
}

trwifi_result_e vdev_connect_ap(struct netdev *dev, trwifi_ap_config_s *ap_connect_config, void *arg)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_CONNECTAP, NULL, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}
	return tres;
}

trwifi_result_e vdev_disconnect_ap(struct netdev *dev, void *arg)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_DISCONENCTAP, NULL, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}
	return tres;
}

trwifi_result_e vdev_get_info(struct netdev *dev, trwifi_info *wifi_info)
{
	VWIFI_ENTRY;

	wifi_info->rssi = 30;
	wifi_info->wifi_status = TRWIFI_DISCONNECTED;

	return TRWIFI_SUCCESS;
}

trwifi_result_e vdev_start_softap(struct netdev *dev, trwifi_softap_config_s *softap_config)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_STARTSOFTAP, NULL, tres};
	uint8_t *ssid = (uint8_t *)kmm_zalloc(softap_config->ssid_length + 1);
	if (!ssid) {
		return TRWIFI_FAIL;
	}
	strncpy((char *)ssid, softap_config->ssid, softap_config->ssid_length);
	req.arg = ssid;
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}
	return tres;
}

trwifi_result_e vdev_start_sta(struct netdev *dev)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_STARTSTA, NULL, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}
	return tres;
}

trwifi_result_e vdev_stop_softap(struct netdev *dev)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_STOPSOFTAP, NULL, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}

	return tres;
}

trwifi_result_e vdev_set_autoconnect(struct netdev *dev, uint8_t check)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_SETAUTOCONNECT, NULL, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}

	return tres;
}

trwifi_result_e vdev_drv_ioctl(struct netdev *dev, trwifi_msg_s *msg)
{
	VWIFI_ENTRY;
	trwifi_result_e tres = TRWIFI_SUCCESS;
	struct vwifi_req req = {VWIFI_MSG_IOCTL, msg, tres};
	int res = _progress_message(&req);
	if (res < 0) {
		return TRWIFI_FAIL;
	}

	return tres;
}

int vdev_linkoutput(struct netdev *dev, void *buf, uint16_t dlen)
{
	VWIFI_ENTRY;
	vwifi_handle_packet(buf, dlen);
	return 0;
}

int vdev_set_multicast_list(struct netdev *dev, const struct in_addr *group,
							netdev_mac_filter_action action)
{
	VWIFI_ENTRY;
	return 0;
}
