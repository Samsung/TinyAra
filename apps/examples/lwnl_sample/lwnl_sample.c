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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/net/if/ble.h>
#include <tinyara/net/if/wifi.h>
#define UNKNWON_LISTENER -1
#define WIFI_LISTENER 0
#define BLE_LISTENER 1

static char g_buf[512] = {1,};

static void print_usage(void)
{
	printf("lwnl_sample [type] [option]");
	printf("type:\n");
	printf("\twifi: generate wi-fi event listener\n");
	printf("\tble: generate BLE event listener\n");
	printf("option: the number of event listener\n");
	printf("\tevent: generate random events\n");
}

/*
 * it's test code so it doesn't perform error handling strictly.
 */
static void *_wifi_event_listener(void *arg)
{
	int res = 0;
	int idx = *((int *)arg);
	printf("[WIFI][%d] run event listener\n", idx);
	int fd = socket(AF_LWNL, SOCK_RAW, LWNL_ROUTE);
	if (fd < 0) {
		printf("[WIFI][%d] fail to create socket\n", idx);
		return NULL;
	}

	struct sockaddr_lwnl addr = {LWNL_DEV_WIFI};
	res = bind(fd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_lwnl));
	if (res < 0) {
		printf("[WIFI][%d] bind fail errno(%d)\n", idx, errno);
		close(fd);
		return NULL;
	}

	fd_set rfds, ofds;
	FD_ZERO(&ofds);
	FD_SET(fd, &ofds);

	printf("[WIFI][%d] run event loop\n", idx);
	for (;;) {
		rfds = ofds;
		res = select(fd + 1, &rfds, NULL, NULL, 0);
		if (res <= 0) {
			printf("[WIFI][%d] select error res(%d) errno(%d))\n", idx, res, errno);
			return NULL;
		}

		if (FD_ISSET(fd, &rfds)) {
			#define LWNL_TEST_HEADER_LEN (sizeof(lwnl_cb_status) + sizeof(uint32_t))

			char buf[LWNL_TEST_HEADER_LEN] = {0, };
			lwnl_cb_status status;
			uint32_t len;
			printf("[T%d] -->%s %d\n", getpid(), __FUNCTION__, __LINE__);
			int nbytes = read(fd, (char *)buf, LWNL_TEST_HEADER_LEN);
			printf("[T%d] <--%s %d\n", getpid(), __FUNCTION__, __LINE__);
			if (nbytes < 0) {
				printf("[WIFI][%d] read error bytes(%d) errno(%d))\n", idx, nbytes, errno);
				return NULL;
			}
			memcpy(&status, buf, sizeof(lwnl_cb_status));
			memcpy(&len, buf + sizeof(lwnl_cb_status), sizeof(uint32_t));
			// flush the event queue
			if (len > 0) {
				char *tmp = (char *)malloc(len);
				read(fd, tmp, len);
				free(tmp);
			}
			printf("[WIFI][T%d][%d] dev type (%d) evt (%d) len(%d)\n",
				   getpid(), idx, status.type, status.evt, len);
			if (status.type != LWNL_DEV_WIFI) {
				printf("[WIFI][%d] drop packet\n", idx);
			}
		}
	}
}

static void *_ble_event_listener(void *arg)
{
	int res = 0;
	int idx = *((int *)arg);
	printf("[BLE][%d] run event listener\n", idx);
	int fd = socket(AF_LWNL, SOCK_RAW, LWNL_ROUTE);
	if (fd < 0) {
		printf("[BLE][%d] fail to create socket\n", idx);
		return NULL;
	}

	struct sockaddr_lwnl addr = {LWNL_DEV_BLE};
	res = bind(fd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_lwnl));
	if (res < 0) {
		printf("[BLE][%d] bind fail errno(%d)\n", idx, errno);
		close(fd);
		return NULL;
	}

	fd_set rfds, ofds;
	FD_ZERO(&ofds);
	FD_SET(fd, &ofds);

	printf("[BLE][%d] run event loop\n", idx);
	for (;;) {
		rfds = ofds;
		res = select(fd + 1, &rfds, NULL, NULL, 0);
		if (res <= 0) {
			printf("[BLE][%d] select error res(%d) errno(%d))\n", idx, res, errno);
			return NULL;
		}

		if (FD_ISSET(fd, &rfds)) {
			#define LWNL_TEST_HEADER_LEN (sizeof(lwnl_cb_status) + sizeof(uint32_t))

			char buf[LWNL_TEST_HEADER_LEN] = {0, };
			lwnl_cb_status status;
			uint32_t len;
			printf("[T%d] -->%s %d\n", getpid(), __FUNCTION__, __LINE__);
			int nbytes = read(fd, (char *)buf, LWNL_TEST_HEADER_LEN);
			printf("[T%d] <--%s %d\n", getpid(), __FUNCTION__, __LINE__);
			if (nbytes < 0) {
				printf("[BLE][%d] read error bytes(%d) errno(%d))\n", idx, nbytes, errno);
				return NULL;
			}
			memcpy(&status, buf, sizeof(lwnl_cb_status));
			memcpy(&len, buf + sizeof(lwnl_cb_status), sizeof(uint32_t));
			// flush the event queue
			if (len > 0) {
				char *tmp = (char *)malloc(len);
				read(fd, tmp, len);
				free(tmp);
			}
			printf("[BLE][T%d][%d] dev type (%d) evt (%d) len(%d)\n",
				   getpid(), idx, status.type, status.evt, len);
			if (status.type != LWNL_DEV_BLE) {
				printf("[BLE][%d] drop packet\n", idx);
			}
		}
	}
}

static void *_generate_event(void *arg)
{
	lwnl_postmsg(LWNL_DEV_BLE, LWNL_EVT_BLE_CONNECTED, NULL, 0);
	lwnl_postmsg(LWNL_DEV_WIFI, LWNL_EVT_STA_CONNECTED, NULL, 0);

	lwnl_postmsg(LWNL_DEV_BLE, LWNL_EVT_BLE_DISCONNECTED, g_buf, 256);
	lwnl_postmsg(LWNL_DEV_WIFI, LWNL_EVT_SCAN_DONE, g_buf, 24);

	return NULL;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int lwnl_sample_main(int argc, char *argv[])
#endif
{
	pthread_t *pid = NULL;
	int num_threads = 3;
	pthread_startroutine_t func = NULL;
	if (argc != 3 && argc != 2) {
		print_usage();
		return -1;
	}
	if (argc == 3) {
		/* the number of listener is dependent to lwnl queue size.
		 * please check LWNL_NPOLLWAITERS in lwnl_evt_queue.h.
		 */
		if (strncmp(argv[1], "wifi", strlen("wifi") + 1) == 0) {
			func = _wifi_event_listener;
		} else if (strncmp(argv[1], "ble", strlen("ble") + 1) == 0) {
			func = _ble_event_listener;
		} else {
			print_usage();
			return -1;
		}
		num_threads = atoi(argv[2]);
		pid = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
		if (!pid) {
			printf("allocate thread pid array fail\n");
			return -1;
		}
		printf("Run %d listener threads\n", num_threads);
		for (int i = 0; i < num_threads; i++) {
			pthread_create((pid + i), NULL, func, (void *)&i);
			pthread_detach(*(pid + i));
		}
	} else if (argc == 2 && strncmp(argv[1], "event", strlen("event") + 1) == 0) {
		pthread_t eid;
		pthread_create(&eid, NULL, _generate_event, NULL);
		pthread_detach(eid);
	} else if (argc == 2 && strncmp(argv[1], "full", strlen("full") + 1) == 0) {

	}

	return 0;
}
