/****************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
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

#include <stdio.h>
#include <debug.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#include <wifi_manager/wifi_manager.h>
#include "wifi_manager_dhcp.h"
#include "wifi_manager_event.h"
#include "wifi_manager_msghandler.h"
#include "wifi_manager_message.h"
#include <tinyara/net/netlog.h>

#define WIFIMGR_MSG_QUEUE_NAME "/dev/wifimgr_msg"
#define TAG "[WM]"

static inline int _send_message(int fd, void *buf, int buflen)
{
	int sent = 0;
	while (1) {
		int res = write(fd, (void *)buf + sent, buflen - sent);
		if (res < 0) {
			int err_no = get_errno();
			if (err_no == EAGAIN || err_no == EINTR) {
				continue;
			}
			NET_LOGE(TAG, "write error %d\n", err_no);
			return -1;
		}
		sent += res;
		if (sent == buflen) {
			break;
		}
	}
	return 0;
}

static inline int _recv_message(int fd, void *buf, int buflen)
{
	int received = 0;
	while (1) {
		int res = read(fd, buf + received, buflen - received);
		if (res < 0) {
			int err_no = get_errno();
			if (err_no == EAGAIN || err_no == EINTR) {
				continue;
			}
			NET_LOGE(TAG, "read error %d\n", err_no);
			return -1;
		}
		received += res;
		if (received == buflen) {
			break;
		}
	}
	return 0;
}

/*
 * Public functions
 */
int wifimgr_message_in(handler_msg *msg, handler_queue *queue)
{
	int fd = open(WIFIMGR_MSG_QUEUE_NAME, O_WRONLY);
	if (fd < 0) {
		NET_LOGE(TAG, "open error %d\n", errno);
		return -1;
	}

	int res = _send_message(fd, (void *)msg, sizeof(handler_msg));
	close(fd);
	if (res < 0) {
		return -1;
	}
	return 0;
}


// it's not thread-safe. in other words only one thread should call
// this function
int wifimgr_message_out(handler_msg *msg, handler_queue *queue)
{
	fd_set rfds = queue->rfds;
	int res = select(queue->max + 1, &rfds, NULL, NULL, NULL);
	if (res <= 0) {
		int err_no = get_errno();
		if (err_no == EINTR) {
			return 1;
		}
		NET_LOGE(TAG, "select error(%d)\n", err_no);
		return -1;
	}
	if (FD_ISSET(queue->fd, &rfds)) {
		res = _recv_message(queue->fd, (void *)msg, sizeof(handler_msg));
		if (res < 0) {
			NET_LOGE(TAG, "critical error\n");
		} else {
			wifimgr_msg_s *wmsg = msg->msg;
			wmsg->result = wifimgr_handle_request(wmsg);
			if (msg->signal) {
				sem_post(msg->signal);
			}
		}
	}
#ifdef CONFIG_LWNL80211
	if (FD_ISSET(queue->nd, &rfds)) {
		res = lwnl_fetch_event(queue->nd, (void *)msg, sizeof(handler_msg));
		if (res < 0) {
			NET_LOGE(TAG, "critical error\n");
		} else {
			wifimgr_msg_s *wmsg = msg->msg;
			if (wmsg) {
				wmsg->result = wifimgr_handle_request(wmsg);
			}
			if (msg->signal) {
				sem_post(msg->signal);
			}
		}
	}
#endif
	return 0;
}

int wifimgr_create_msgqueue(handler_queue *queue)
{
	queue->fd = queue->nd = queue->max = 0;
	FD_ZERO(&queue->rfds);

	int res = mkfifo(WIFIMGR_MSG_QUEUE_NAME, 0666);
	if (res < 0 && res != -EEXIST) {
		NET_LOGE(TAG, "create wifimgr msg queue fail %d\n", errno);
		return -1;
	}

	queue->fd = open(WIFIMGR_MSG_QUEUE_NAME, O_RDWR);
	if (queue->fd < 0) {
		NET_LOGE(TAG, "open wifimgr msg queue fail %d\n", errno);
		unlink(WIFIMGR_MSG_QUEUE_NAME);
		return -1;
	}
	FD_SET(queue->fd, &queue->rfds);

#ifdef CONFIG_LWNL80211
	queue->nd = socket(AF_LWNL, SOCK_RAW, LWNL_ROUTE);
	if (queue->nd < 0) {
		close(queue->fd);
		NET_LOGE(TAG, "socket open fail %d\n", errno);
		return -1;
	}

	struct sockaddr_lwnl addr = {LWNL_DEV_WIFI};
	res = bind(queue->nd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_lwnl));
	if (res < 0) {
		close(queue->fd);
		close(queue->nd);
		NET_LOGE(TAG, "bind fail %d\n", errno);
		return -1;
	}
	FD_SET(queue->nd, &queue->rfds);
#endif
	queue->max = queue->fd > queue->nd ? queue->fd : queue->nd;

	return 0;
}
