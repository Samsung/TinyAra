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
 * wqueue/wqueue.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#ifndef __OS_WQUEUE_WQUEUE_H
#define __OS_WQUEUE_WQUEUE_H

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdbool.h>
#include <queue.h>
#include <semaphore.h>

#include <tinyara/wqueue.h>

#ifdef CONFIG_SCHED_WORKQUEUE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HPWORKNAME "hpwork"
#define LPWORKNAME "lpwork"

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* This represents one worker */

struct worker_s {
	pid_t pid;					/* The task ID of the worker thread */
	volatile bool busy;			/* True: Worker is not available */
};

/* This structure defines the state of work queue */

struct wqueue_s {
	struct dq_queue_s q;		/* The queue of pending work */
	struct worker_s worker[1];	/* Describes a worker thread */
};

/* This structure defines the state of one high-priority work queue.  This
 * structure must be cast-compatible with kwork_wqueue_s.
 */

#ifdef CONFIG_SCHED_HPWORK
struct hp_wqueue_s {
	struct dq_queue_s q;		/* The queue of pending work */
	struct worker_s worker[1];	/* Describes the single high priority worker */
};
#endif

/* This structure defines the state of one high-priority work queue.  This
 * structure must be cast compatible with kwork_wqueue_s
 */

#ifdef CONFIG_SCHED_LPWORK
struct lp_wqueue_s {
	struct dq_queue_s q;		/* The queue of pending work */

	/* Describes each thread in the low priority queue's thread pool */
	struct worker_s worker[CONFIG_SCHED_LPNTHREADS];
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The state of the user mode work queue */

struct wqueue_s *get_usrwork(void);

#ifdef CONFIG_SCHED_HPWORK
struct hp_wqueue_s *get_hpwork(void);
#endif

#ifdef CONFIG_SCHED_LPWORK
struct lp_wqueue_s *get_lpwork(void); 
#endif

/* This semaphore/mutex supports exclusive access to the user-mode work queue */

struct wqueue_s *get_usrwork(void);

#ifdef CONFIG_BUILD_PROTECTED
sem_t *get_usrsem(void);
#else
pthread_mutex_t *get_usrmutex(void);
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: work_lock
 *
 * Description:
 *   Lock the user-mode work queue.
 *
 * Input parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success, a negated errno on failure.  This error may be
 *   reported:
 *
 *   -EINTR - Wait was interrupted by a signal
 *
 ****************************************************************************/
#if defined(CONFIG_SCHED_USRWORK) && !defined(__KERNEL__)
int work_lock(void);
#endif
/****************************************************************************
 * Name: work_unlock
 *
 * Description:
 *   Unlock the user-mode work queue.
 *
 * Input parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
#if defined(CONFIG_SCHED_USRWORK) && !defined(__KERNEL__)
void work_unlock(void);
#endif

/****************************************************************************
 * Name: work_qcancel
 *
 * Description:
 *   Cancel previously queued work.  This removes work from the work queue.
 *   After work has been cancelled, it may be re-queue by calling work_queue()
 *   again.
 *
 * Input parameters:
 *   qid    - The work queue ID
 *   work   - The previously queue work structure to cancel
 *
 * Returned Value:
 *   Zero (OK) on success, a negated errno on failure.  This error may be
 *   reported:
 *
 *   -ENOENT - There is no such work queued.
 *   -EINVAL - An invalid work queue was specified
 *
 ****************************************************************************/

int work_qcancel(FAR struct wqueue_s *wqueue, FAR struct work_s *work);

/****************************************************************************
 * Name: work_qqueue
 *
 * Description:
 *   Queue work to be performed at a later time.  All queued work will be
 *   performed on the worker thread of of execution (not the caller's).
 *
 *   The work structure is allocated by caller, but completely managed by
 *   the work queue logic.  The caller should never modify the contents of
 *   the work queue structure; the caller should not call work_queue()
 *   again until either (1) the previous work has been performed and removed
 *   from the queue, or (2) work_cancel() has been called to cancel the work
 *   and remove it from the work queue.
 *
 * Input parameters:
 *   qid    - The work queue ID (index)
 *   work   - The work structure to queue
 *   worker - The worker callback to be invoked.  The callback will invoked
 *            on the worker thread of execution.
 *   arg    - The argument that will be passed to the workder callback when
 *            int is invoked.
 *   delay  - Delay (in clock ticks) from the time queue until the worker
 *            is invoked. Zero means to perform the work immediately.
 *
 * Returned Value:
 *   Zero (OK) on success, a negated errno on failure.
 *
 ****************************************************************************/

int work_qqueue(FAR struct wqueue_s *wqueue, FAR struct work_s *work, worker_t worker, FAR void *arg, clock_t delay);

/****************************************************************************
 * Name: work_process
 *
 * Description:
 *   This is the logic that performs actions placed on any work list.  This
 *   logic is the common underlying logic to all work queues.  This logic is
 *   part of the internal implementation of each work queue; it should not
 *   be called from application level logic.
 *
 * Input parameters:
 *   wqueue - Describes the work queue to be processed
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void work_process(FAR struct wqueue_s *wqueue, int wdx);

/****************************************************************************
 * Name: work_qsignal
 *
 * Description:
 *   Signal the worker thread to process the work queue now.  This function
 *   is used internally by the work logic but could also be used by the
 *   user to force an immediate re-assessment of pending work.
 *
 * Input parameters:
 *   pid    - The work queue pid
 *
 * Returned Value:
 *   Zero on success, a negated errno on failure
 *
 ****************************************************************************/

int work_qsignal(pid_t pid);

#endif							/* CONFIG_SCHED_WORKQUEUE */
#endif							/* __OS_WQUEUE_WQUEUE_H */
