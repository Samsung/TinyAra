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
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <signal.h>
#include <errno.h>

#include <tinyara/wqueue.h>

#include "wqueue.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: work_signal
 *
 * Description:
 *   Signal the worker thread to process the work queue now.  This function
 *   is used internally by the work logic but could also be used by the
 *   user to force an immediate re-assessment of pending work.
 *
 * Input parameters:
 *   qid    - The work queue ID
 *
 * Returned Value:
 *   Zero on success, a negated errno on failure
 *
 ****************************************************************************/

int work_signal(int qid)
{
	pid_t pid;
	/* Get the process ID of the worker thread */
#ifdef CONFIG_SCHED_HPWORK
	if (qid == HPWORK) {
		struct hp_wqueue_s *hwq = get_hpwork();
		pid = hwq->worker[0].pid;
	} else
#endif
#ifdef CONFIG_SCHED_LPWORK
	if (qid == LPWORK) {
		int wndx;
		int i;
		struct lp_wqueue_s *lwq = get_lpwork();

		/* Find an IDLE worker thread */

		for (wndx = 0, i = 0; i < CONFIG_SCHED_LPNTHREADS; i++) {
			/* Is this worker thread busy? */

			if (!lwq->worker[i].busy) {
				/* No.. select this thread */

				wndx = i;
				break;
			}
		}

		/* Use the process ID of the IDLE worker thread (or the ID of worker
			* thread 0 if all of the worker threads are busy).
			*/

		pid = lwq->worker[wndx].pid;
	} else
#endif
	{
		return -EINVAL;
	}

	return work_qsignal(pid);
}
