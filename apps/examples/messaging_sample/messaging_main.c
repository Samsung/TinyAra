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
 * apps/examples/messaging_sample/messaging_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
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

#include <tinyara/config.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <messaging/messaging.h>

#define TEST1_PORT "test1_port"
#define TEST1_DATA "TEST1"

#define TEST2_PORT "test2_port"
#define TEST2_DATA "TEST2"
#define TEST2_REPLY "REPLY_MSG"

#define MSG_PRIO 10
#define TASK_PRIO 100
#define STACKSIZE 2048
#define BUFFER_SIZE 10

static char *sync_send_alloc;

static void recv_callback(int msg_type, pid_t sender_pid, void *data, void *recv_data)
{
	printf("Success to receive with non-block mode in recv callback from PID[%d]. : %s\n", sender_pid, (char *)recv_data);
}

int async_send(int argc, FAR char *argv[])
{
	int ret;
	msg_send_data_t param;
	param.priority = MSG_PRIO;
	param.type = MSG_SEND_TYPE_ASYNC;
	param.msg = TEST1_DATA;
	param.msglen = sizeof(TEST1_DATA);

	printf("- Send %s data with async mode.\n", TEST1_DATA);
	ret = messaging_send(TEST1_PORT, &param, NULL);
	if (ret != OK) {
		printf("Fail to async send message.\n");
		return ERROR;
	}

	return 0;
}

int sync_send(int argc, FAR char *argv[])
{
	int ret;
	msg_send_data_t send_data;
	msg_data_t reply_data;
	send_data.priority = MSG_PRIO;
	send_data.type = MSG_SEND_TYPE_SYNC;
	send_data.msg = TEST2_DATA;
	send_data.msglen = sizeof(TEST2_DATA);

	reply_data.msglen = BUFFER_SIZE;
	reply_data.msg = (char *)malloc(BUFFER_SIZE);
	if (reply_data.msg == NULL) {
		printf("Fail to sync send message : out of memory.\n");
		return ERROR;
	}
	sync_send_alloc = reply_data.msg;

	printf("- Send %s data with sync mode.\n", TEST2_DATA);
	ret = messaging_send(TEST2_PORT, &send_data, &reply_data);
	if (ret != OK) {
		printf("Fail to sync send message.\n");
		free(reply_data.msg);
		return ERROR;
	}

	printf("Success to receive reply from receiver. : %s\n", (char *)reply_data.msg);
	free(reply_data.msg);

	return 0;
}

int nonblock_recv(int argc, FAR char *argv[])
{
	int ret;
	msg_recv_param_t param;
	msg_data_t data;

	param.type = MSG_RECV_TYPE_NONBLOCK;
	param.cb_func = recv_callback;
	data.msg = (char *)malloc(BUFFER_SIZE);
	if (data.msg == NULL) {
		printf("Fail to recv nonblock message : out of memory.\n");
		return ERROR;
	}
	data.msglen = BUFFER_SIZE;

	ret = messaging_recv(TEST1_PORT, &data, &param);
	if (ret != OK) {
		printf("Fail to receive with non-block mode.\n");
		free(data.msg);
		return ERROR;
	}

	free(data.msg);
	return OK;
}

int block_recv(int argc, FAR char *argv[])
{
	int ret;
	msg_recv_param_t recv_param;
	msg_data_t recv_data;
	msg_data_t reply_data;

	recv_param.type = MSG_RECV_TYPE_BLOCK;
	recv_data.msg = (char *)malloc(BUFFER_SIZE);
	if (recv_data.msg == NULL) {
		printf("Fail to receive, because out of memory.\n");
		return ERROR;
	}
	recv_data.msglen = BUFFER_SIZE;

	ret = messaging_recv(TEST2_PORT, &recv_data, &recv_param);
	if (ret != OK) {
		printf("Fail to receive with block mode.\n");
		free(recv_data.msg);
		return ERROR;
	}

	printf("Success to receive with block mode in receiver. : %s\n", (char *)recv_data.msg);
	printf("- Reply to sender %s data.\n", TEST2_REPLY);

	reply_data.msg = TEST2_REPLY;
	reply_data.msglen = sizeof(TEST2_REPLY);
	reply_data.sender_pid = recv_data.sender_pid;
	if (recv_data.msg_type == MSG_SEND_TYPE_SYNC) {
		ret = messaging_send_reply(TEST2_PORT, &reply_data);
		if (ret != OK) {
			printf("Fail to reply.\n");
			free(recv_data.msg);
			return ERROR;
		}
	} else {
		printf("We will not reply, because received msg is not sync type.\n");
	}

	free(recv_data.msg);
	return OK;
}

void async_nonblock_messaging_test(void)
{
	int ret;
	int sender_pid;

	printf("\n=== Start the Async Non-block messaging test. ===\n");

	sender_pid = task_create("async_send", TASK_PRIO, STACKSIZE, async_send, NULL);
	if (sender_pid < 0) {
		printf("Fail to create async_send task.\n");
		return;
	}

	ret = task_create("nonblock_recv", TASK_PRIO, STACKSIZE, nonblock_recv, NULL);
	if (ret < 0) {
		task_delete(sender_pid);
		printf("Fail to create nonblock_recv task.\n");
		return;
	}

	/* Wait for finishing async_send and nonblock_recv tasks. */
	sleep(1);

	ret = messaging_unregister(TEST1_PORT);
	if (ret != OK) {
		printf("Fail to unregister TEST1_PORT.\n");
		return;
	}
}

void sync_block_messaging_test(void)
{
	int ret;
	int sender_pid;

	printf("\n=== Start the Sync Block messaging test. ===\n");

	sender_pid = task_create("sync_send", TASK_PRIO, STACKSIZE, sync_send, NULL);
	if (sender_pid < 0) {
		printf("Fail to create sync_send task.\n");
		return;
	}

	ret = task_create("block_recv", TASK_PRIO, STACKSIZE, block_recv, NULL);
	if (ret < 0) {
		free(sync_send_alloc);
		task_delete(sender_pid);
		printf("Fail to create block_recv task.\n");
		return;
	}
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int messaging_main(int argc, char *argv[])
#endif
{
	async_nonblock_messaging_test();

	/* Wait for finishing previous test. */
	sleep(1);

	sync_block_messaging_test();

	return 0;
}
