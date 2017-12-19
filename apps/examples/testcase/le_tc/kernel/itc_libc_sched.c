/****************************************************************************
 *
 * Copyright 2017-2018 Samsung Electronics All Rights Reserved.
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

/// @file itc_libc_sched.c

/// @brief ITC Test Case Example for Libc Sched API

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include "tc_internal.h"

/**
* @fn                   :itc_libc_sched_sched_get_priority_max_n
* @brief                :returns the maximum priority value
* @Scenario             :Function returns the error code by policy.
* API's covered         :sched_get_priority_max
* Preconditions         :NA
* Postconditions        :NA
* @return               :void
*/
static void itc_libc_sched_sched_get_priority_max_n(void)
{
	int ret_chk;

	ret_chk = sched_get_priority_max(SCHED_OTHER);
	TC_ASSERT_EQ("sched_get_priority_max", ret_chk, ERROR);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :itc_libc_sched_sched_get_priority_min_n
* @brief                :returns the minimum priority value
* @Scenario             :Function returns the error code by policy.
* API's covered         :sched_get_priority_max
* Preconditions         :NA
* Postconditions        :NA
* @return               :void
*/
static void itc_libc_sched_sched_get_priority_min_n(void)
{
	int ret_chk;

	ret_chk = sched_get_priority_min(SCHED_OTHER);
	TC_ASSERT_EQ("sched_get_priority_min", ret_chk, ERROR);

	TC_SUCCESS_RESULT();
}

/**
 * @fn                   :itc_libc_sched_task_setcanceltype_p
 * @brief                :sets calling task's cancelability type to indicated type, returns previous cancelability type
 * @Scenario             :The task_setcanceltype() function atomically both sets the calling
 *                        task's cancelability type to the indicated type and returns the
 *                        previous cancelability type at the location referenced by oldtype
 *                        If successful pthread_setcanceltype() function shall return zero;
 *                        otherwise, an error number shall be returned to indicate the error.
 * @API'scovered         :task_setcanceltype
 * @Preconditions        :none
 * @Postconditions       :none
 */
#ifndef CONFIG_CANCELLATION_POINTS
static void itc_libc_sched_task_setcanceltype_p(void)
{
	int type_arr[] = { TASK_CANCEL_ASYNCHRONOUS, TASK_CANCEL_DEFERRED };
	int type;
	int oldtype;
	int ret_chk;
	int index;

	for (index = 0; index < 2; index++) {
		type = type_arr[index];
		ret_chk = task_setcanceltype(type, &oldtype);
		TC_ASSERT_EQ("task_setcanceltype", ret_chk, OK);
	}

	TC_SUCCESS_RESULT();
}
#endif

/**
 * @fn                   :itc_libc_sched_task_setcanceltype_n
 * @brief                :This tc tests tc_libc_sched_task_setcanceltype()
 * @Scenario             :The task_setcanceltype() sending invalid arg shoud return ENOSYS
 * @API'scovered         :task_setcanceltype
 * @Preconditions        :none
 * @Postconditions       :none
 * @return               :void
 */
#ifndef CONFIG_CANCELLATION_POINTS
static void itc_libc_sched_task_setcanceltype_n(void)
{
	int ret_chk;
	ret_chk = task_setcanceltype(3, NULL);
	TC_ASSERT_EQ("task_setcanceltype", ret_chk, ENOSYS);

	TC_SUCCESS_RESULT();
}
#endif


/****************************************************************************
 * Name: libc_sched
 ****************************************************************************/

int itc_libc_sched_main(void)
{
	itc_libc_sched_sched_get_priority_max_n();
	itc_libc_sched_sched_get_priority_min_n();
#ifndef CONFIG_CANCELLATION_POINTS
	itc_libc_sched_task_setcanceltype_p();
	itc_libc_sched_task_setcanceltype_n();
#endif
	return 0;
}
