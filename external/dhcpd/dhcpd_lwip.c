/****************************************************************************
 * netutils/dhcpd/dhcpd.c
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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
 * netutils/dhcpd/dhcpd.c
 *
 *   Copyright (C) 2007-2009, 2011-2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/


#include <tinyara/config.h>		/* TinyAra configuration */
#include <stdio.h>
#include <debug.h>				/* For ndbg, vdbg */
#include <tinyara/compiler.h>	/* For CONFIG_CPP_HAVE_WARNING */
#include <protocols/dhcpd.h>	/* Advertised DHCPD APIs */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <net/if.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <pthread.h>
#include <netutils/netlib.h>
#include <mqueue.h>
#include <sys/ioctl.h>
#include <netdb.h>

#undef nvdbg
#undef ndbg
#define ndbg(...) printf(__VA_ARGS__)
#define nvdbg(...) printf(__VA_ARGS__)

/****************************************************************************
 * Global Data
 ****************************************************************************/
/****************************************************************************
 * Private Data
 ****************************************************************************/
#define DHCP_SERVER_PORT         67
#define DHCP_CLIENT_PORT         68

#define DHCPD_WAIT_TASK_TERM									\
	do {														\
		pthread_mutex_lock(&dhcpdterm_lock);					\
		pthread_cond_wait(&dhcpdterm_signal, &dhcpdterm_lock);	\
		pthread_mutex_unlock(&dhcpdterm_lock);					\
	} while (0)

#define DHCPD_SIGNAL_TASK_TERM					\
	do {										\
		pthread_mutex_lock(&dhcpdterm_lock);	\
		pthread_cond_signal(&dhcpdterm_signal);	\
		pthread_mutex_unlock(&dhcpdterm_lock);	\
	} while (0)

static pthread_t g_dhcpd_tid = -1;
/****************************************************************************
 * Private Types
 ****************************************************************************/
/****************************************************************************
 * Private Data
 ****************************************************************************/
//static pthread_mutex_t dhcpdterm_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t dhcpdterm_signal = PTHREAD_COND_INITIALIZER;
static int g_dhcpd_term = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#define DHCPD_MQ_NAME "dhcpd_queue"

void *_dhcpd_join_handler(void *arg)
{
	struct mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = 10;
	attr.mq_flags = 0;
	attr.mq_curmsgs = 0;

	mqd_t md = mq_open(DHCPD_MQ_NAME, O_RDWR | O_CREAT, 0666, &attr);
	if (md == NULL) {
		ndbg("Failed to open mq\n");
		return NULL;
	}

	while (!g_dhcpd_term) {
		char msg[2];
		int msglen = 2;
		int prio = 0;
		int nbytes = mq_receive(md, msg, msglen, &prio);
		if (nbytes <= 0) {
			printf("mq receive fail\n");
			break;
		}
		sleep(1);
	}
	return NULL;
}

/****************************************************************************
 * Global Functions
 ****************************************************************************/
/****************************************************************************
 * Name: dhcps_server_status
 ****************************************************************************/
int dhcp_server_status(char *intf)
{
	int ret = -1;
	struct req_lwip_data req;

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() failed with errno: %d\n", errno);
		return ret;
	}

	memset(&req, 0, sizeof(req));
	req.type = DHCPDSTATUS;
	req.host_name = intf;

	ret = ioctl(sockfd, SIOCLWIP, (unsigned long)&req);
	if (ret == ERROR) {
		printf("ioctl() failed with errno: %d\n", errno);
		close(sockfd);
		return ret;
	}

	ret = req.req_res;
	close(sockfd);
	return ret;
}

/****************************************************************************
 * Name: dhcps_server_start
 ****************************************************************************/
int dhcp_server_start(char *intf, dhcp_sta_joined dhcp_join_cb)
{
	int ret = -1;
	struct req_lwip_data req;

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() failed with errno: %d\n", errno);
		return ret;
	}

	// pthread create
	g_dhcpd_term = 0;
	ret = pthread_create(&g_dhcpd_tid, NULL, _dhcpd_join_handler, NULL);
	if (ret < 0) {
		printf("[dhcpd] create iotapi handler fail(%d)\n", ret);
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.type = DHCPDSTART;
	req.host_name = intf;

	ret = ioctl(sockfd, SIOCLWIP, (unsigned long)&req);
	if (ret == ERROR) {
		printf("ioctl() failed with errno: %d\n", errno);
		close(sockfd);
		g_dhcpd_term = 1;
		pthread_join(g_dhcpd_tid, NULL);
		return ret;
	}

	ret = req.req_res;
	close(sockfd);
	return ret;
}

/****************************************************************************
 * Name: dhcps_server_stop
 ****************************************************************************/
int dhcp_server_stop(char *intf)
{
	int ret = -1;
	struct req_lwip_data req;

	// remove thread
	g_dhcpd_term = 1;
	pthread_join(g_dhcpd_tid, NULL);
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() failed with errno: %d\n", errno);
		return ret;
	}

	memset(&req, 0, sizeof(req));
	req.type = DHCPDSTOP;
	req.host_name = intf;

	ret = ioctl(sockfd, SIOCLWIP, (unsigned long)&req);
	if (ret == ERROR) {
		printf("ioctl() failed with errno: %d\n", errno);
		close(sockfd);
		return ret;
	}

	ret = req.req_res;
	close(sockfd);

	return ret;
}
