/****************************************************************************
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
 * wqueue/uwqueue/uwork_usrthread.c
 *
 *   Copyright (C) 2009-2014 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <assert.h>
#include <queue.h>

#include <tinyara/wqueue.h>
#include <tinyara/clock.h>

#include "wqueue.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The state of the user mode work queue. */

static struct wqueue_s g_usrwork;

/* This semaphore supports exclusive access to the user-mode work queue */

#ifdef CONFIG_BUILD_PROTECTED
static sem_t g_usrsem;
#else
static pthread_mutex_t g_usrmutex;
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_usrthread
 *
 * Description:
 *   This is the worker thread that performs the actions placed on the user
 *   work queue.
 *
 *   This is a user mode work queue.  It must be used by applications for
 *   miscellaneous operations.  The user work thread must be started by
 *   application start-up logic by calling work_usrstart().
 *
 * Input parameters:
 *   argc, argv (not used)
 *
 * Returned Value:
 *   Does not return
 *
 ****************************************************************************/

#ifdef CONFIG_BUILD_PROTECTED
static int work_usrthread(int argc, char *argv[])
#else
static pthread_addr_t work_usrthread(pthread_addr_t arg)
#endif
{
	struct wqueue_s *usrwq = get_usrwork();
	/* Loop forever */

	for (;;) {
		/* Then process queued work.  We need to keep the work queue locked
		 * while we process items in the work list.
		 */

		work_process(usrwq, 0);
	}

#ifdef CONFIG_BUILD_PROTECTED
	return OK;					/* To keep some compilers happy */
#else
	return NULL;				/* To keep some compilers happy */
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
struct wqueue_s *get_usrwork(void)
{
	return &g_usrwork;
}

#ifdef CONFIG_BUILD_PROTECTED
sem_t *get_usrsem(void)
{
	return &g_usrsem;
}
#else
pthread_mutex_t *get_usrmutex(void)
{
	return &g_usrmutex;
}
#endif

/****************************************************************************
 * Name: work_usrstart
 *
 * Description:
 *   Start the user mode work queue.
 *
 * Input parameters:
 *   None
 *
 * Returned Value:
 *   The task ID of the worker thread is returned on success.  A negated
 *   errno value is returned on failure.
 *
 ****************************************************************************/

int work_usrstart(void)
{
	/* Initialize work queue data structures */

	struct wqueue_s *usrwq = get_usrwork();
	dq_init(&usrwq->q);

#ifdef CONFIG_BUILD_PROTECTED
	{
		/* Set up the work queue lock */

		(void)sem_init(&g_usrsem, 0, 1);

		/* Start a user-mode worker thread for use by applications. */

		usrwq->worker[0].pid = task_create("uwork", CONFIG_SCHED_USRWORKPRIORITY, CONFIG_SCHED_USRWORKSTACKSIZE, (main_t)work_usrthread, (FAR char *const *)NULL);

		DEBUGASSERT(usrwq->worker[0].pid > 0);
		if (usrwq->worker[0].pid < 0) {
			int errcode = errno;
			DEBUGASSERT(errcode > 0);
			return -errcode;
		}
		usrwq->worker[0].busy = true;
		return usrwq->worker[0].pid;
	}
#else
	{
		pthread_t usrwork;
		pthread_attr_t attr;
		struct sched_param param;
		int status;

		/* Set up the work queue lock */

		(void)pthread_mutex_init(&g_usrmutex, NULL);

		/* Start a user-mode worker thread for use by applications. */

		(void)pthread_attr_init(&attr);
		(void)pthread_attr_setstacksize(&attr, CONFIG_SCHED_USRWORKSTACKSIZE);

		param.sched_priority = CONFIG_SCHED_USRWORKPRIORITY;
		(void)pthread_attr_setschedparam(&attr, &param);

		status = pthread_create(&usrwork, &attr, work_usrthread, NULL);
		if (status != 0) {
			return -status;
		}

		/* Detach because the return value and completion status will not be
		 * requested.
		 */

		(void)pthread_detach(usrwork);

		usrwq->worker[0].pid = (pid_t)usrwork;
		usrwq->worker[0].busy = true;
		return usrwq->worker[0].pid;
	}
#endif
}
