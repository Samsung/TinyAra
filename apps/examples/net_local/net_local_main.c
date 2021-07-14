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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sched.h>
#include <ctype.h>
#include <pthread.h>

#include "net_local.h"

int net_local_server_start(void)
{
	int pid = task_create("net_local_server", SCHED_PRIORITY_DEFAULT, 4096, net_local_server_main, (FAR char *const *)NULL);
	if (pid < 0) {
		printf("net_local_server start failed, err = %d\n", pid);
	}
	return pid;
}

int net_local_client_start(void)
{
	int pid = task_create("net_local_client", SCHED_PRIORITY_DEFAULT, 4096, net_local_client_main, (FAR char *const *)NULL);
	if (pid < 0) {
		printf("net_local_client start failed, err = %d\n", pid);
	}
	return pid;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int net_local_main(int argc, char *argv[])
#endif
{
	// Test socket between tasks
	net_local_server_start();
	sleep(1);
	net_local_client_start();

	return 0;
}
