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

/*  @file tc_net_poll.c
 * @brief Test Case Example for lwip_poll() API
 */

#include <tinyara/config.h>
#include <stdio.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netutils/netlib.h>
#include "tc_internal.h"

/**
* @testcase             :tc_net_poll_p
* @brief                :check for poll in multi-thread env
* @scenario             :thread1 call poll while thread_p close the socket which is used for poll().
* @apicovered           :lwip_poll()
* @precondition         :
* @postcondition        :
*/

static void tc_net_multi_thread_poll_p(int ret, int sd)
{
	TC_ASSERT_LEQ_CLEANUP("lwip_poll", ret, 0, close(sd));
	TC_SUCCESS_RESULT();
}

static void *wait_event(void *arg)
{
	struct pollfd fds[1];
	int sd = *(int *)arg;
	int ret = -1;

	fds[0].fd = sd;
	fds[0].events = POLLIN;

	ret = poll(fds, 1, 1000);
	tc_net_multi_thread_poll_p(ret, sd);

	return NULL;
}

static void *multi_thread_poll(void *arg)
{
	int sd = -1;
	pthread_t thread1;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		printf("socket fail %s:%d", __FUNCTION__, __LINE__);
		return NULL;
	}

	pthread_create(&thread1, NULL, wait_event, (void *)&sd);
	pthread_detach(thread1);

	close(sd);

	return NULL;
}

/**
* @testcase             :tc_net_poll_p
* @brief                :check for poll
* @scenario             :
* @apicovered           :lwip_poll()
* @precondition         :
* @postcondition        :
*/

static void tc_net_poll_p(void)
{
	int ret = -1;
	int sd = -1;
	struct pollfd fds[1];

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	TC_ASSERT_GEQ("socket", sd, 0);

	fds[0].fd = sd;
	fds[0].events = POLLIN;

	ret = poll(fds, 1, 1000);
	TC_ASSERT_GEQ_CLEANUP("lwip_poll", ret, 0, close(sd));

	close(sd);

	TC_SUCCESS_RESULT();
}


/**
* @testcase             :tc_net_poll_events_n
* @brief                :test with wrong value of events
* @scenario             :
* @apicovered           :lwip_poll()
* @precondition         :
* @postcondition        :
*/

static void tc_net_poll_events_n(void)
{
	int ret = -1;
	int sd = -1;
	struct pollfd fds[1];

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	TC_ASSERT_GEQ("socket", sd, 0);

	fds[0].fd = sd;
	fds[0].events = 0;

	ret = poll(fds, 1, 1000);
	TC_ASSERT_GEQ_CLEANUP("lwip_poll", ret, 0, close(sd));

	close(sd);

	TC_SUCCESS_RESULT();
}

/**
* @testcase             :tc_net_poll_sd_n
* @brief                :test with wrong value of socket descriptor
* @scenario             :
* @apicovered           :lwip_poll()
* @precondition         :
* @postcondition        :
*/

static void tc_net_poll_sd_n(void)
{
	int ret = -1;
	int sd = 1000;
	struct pollfd fds[1];

	fds[0].fd = sd;
	fds[0].events = POLLIN;

	ret = poll(fds, 1, 1000);
	TC_ASSERT_LEQ("lwip_poll", ret, -1);
	TC_SUCCESS_RESULT();
}

int net_poll_main(void)
{

	pthread_t thread_p;
	pthread_create(&thread_p, NULL, multi_thread_poll, NULL);
	pthread_join(thread_p, NULL);

	tc_net_poll_p();
	tc_net_poll_events_n();
	tc_net_poll_sd_n();

	return 0;
}
