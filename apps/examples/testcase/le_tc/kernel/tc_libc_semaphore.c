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

/// @file tc_libc_semaphore.c
/// @brief Test Case Example for Libc Semaphore API

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/types.h>
#include "tc_internal.h"

#define SEM_VALUE SEM_VALUE_MAX
#define PSHARED			0
#define SEM_PRIO_DEFAULT	3
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @fn                   :tc_libc_semaphore_sem_init
 * @brief                :this tc test sem_init function
 * @Scenario             :If sem is NULL or sem value is bigger then SEM_VALUE_MAX, it return ERROR and errno EINVAL is set.
 *                        Else sem is intialized to the default value.
 * API's covered         :sem_init
 * Preconditions         :NA
 * Postconditions        :NA
 * @return               :total_pass on success.
 */
static void tc_libc_semaphore_sem_init(void)
{
	sem_t sem;
	unsigned int value = SEM_VALUE;
	int ret_chk;

	ret_chk = sem_init(NULL, PSHARED, 0);
	TC_ASSERT_EQ("sem_init", ret_chk, ERROR);
	TC_ASSERT_EQ("sem_init", get_errno(), EINVAL);

	ret_chk = sem_init(&sem, PSHARED, SEM_VALUE_MAX + 1);
	TC_ASSERT_EQ("sem_init", ret_chk, ERROR);
	TC_ASSERT_EQ("sem_init", get_errno(), EINVAL);

	ret_chk = sem_init(&sem, PSHARED, value);
	TC_ASSERT_EQ("sem_init", ret_chk, OK);
	TC_ASSERT_EQ("sem_init", sem.semcount, value);
#ifdef CONFIG_PRIORITY_INHERITANCE
	TC_ASSERT_EQ("sem_init", sem.flags, 0);
#if CONFIG_SEM_PREALLOCHOLDERS > 0
	TC_ASSERT_EQ("sem_init", sem.hhead, NULL);
#else
	TC_ASSERT_EQ("sem_init", sem.holder.htcb, NULL);
	TC_ASSERT_EQ("sem_init", sem.holder.counts, 0);
#endif
#endif

	TC_SUCCESS_RESULT();
	return;
}


/**
 * @fn                   :tc_libc_semaphore_sem_getvalue
 * @brief                :this tc test sem_init function
 * @Scenario             :If sem or sval is NULL, it return ERROR and errno EINVAL is set.
 *                        Else sval get sem->semcount.
 * API's covered         :sem_getvalue
 * Preconditions         :NA
 * Postconditions        :NA
 * @return               :total_pass on success.
 */
static void tc_libc_semaphore_sem_getvalue(void)
{
	sem_t sem;
	unsigned int value = SEM_VALUE;
	int sval;
	int ret_chk;

	ret_chk = sem_init(&sem, PSHARED, value);
	TC_ASSERT_EQ("sem_init", ret_chk, OK);

	ret_chk = sem_getvalue(NULL, &sval);
	TC_ASSERT_EQ("sem_getvalue", ret_chk, ERROR);
	TC_ASSERT_EQ("sem_getvalue", get_errno(), EINVAL);

	ret_chk = sem_getvalue(&sem, NULL);
	TC_ASSERT_EQ("sem_getvalue", ret_chk, ERROR);
	TC_ASSERT_EQ("sem_getvalue", get_errno(), EINVAL);

	ret_chk = sem_getvalue(&sem, &sval);
	TC_ASSERT_EQ("sem_getvalue", ret_chk, OK);
	TC_ASSERT_EQ("sem_getvalue", sval, value);

	TC_SUCCESS_RESULT();
	return;
}

/**
 * @fn                   :tc_libc_semaphore_sem_getprotocol
 * @brief                :this tc test sem_getprotocol function
 * @Scenario             :Return the value of the semaphore protocol attribute.
 * API's covered         :sem_init, sem_getprotocol
 * Preconditions         :NA
 * Postconditions        :NA
 * @return               :total_pass on success.
 */
static void tc_libc_semaphore_sem_getprotocol(void)
{
	sem_t sem;
	unsigned int value = SEM_VALUE;
	int protocol;
	int ret_chk;

	ret_chk = sem_init(&sem, PSHARED, value);
	TC_ASSERT_EQ("sem_init", ret_chk, OK);

	ret_chk = sem_getprotocol(&sem, &protocol);
	TC_ASSERT_EQ("sem_getprotocol", ret_chk, OK);

	TC_SUCCESS_RESULT();
	return;
}

#ifndef CONFIG_PRIORITY_INHERITANCE
/**
 * @fn                   :tc_libc_semaphore_sem_setprotocol
 * @brief                :this tc test sem_setprotocol function
 * @Scenario             :Set the value of the semaphore protocol attribute.
 * API's covered         :sem_init, sem_setprotocol
 * Preconditions         :NA
 * Postconditions        :NA
 * @return               :total_pass on success.
 */
static void tc_libc_semaphore_sem_setprotocol(void)
{
	sem_t sem;
	unsigned int value = SEM_VALUE;
	int protocol;
	int ret_chk;

	ret_chk = sem_init(&sem, PSHARED, value);
	TC_ASSERT_EQ("sem_init", ret_chk, OK);

	protocol = SEM_PRIO_NONE;
	ret_chk = sem_setprotocol(&sem, protocol);
	TC_ASSERT_EQ("sem_setprotocol", ret_chk, OK);

	protocol = SEM_PRIO_INHERIT;
	ret_chk = sem_setprotocol(&sem, protocol);
	TC_ASSERT_EQ("sem_setprotocol", ret_chk, ERROR);
	TC_ASSERT_EQ("sem_setprotocol", errno, ENOSYS);

	protocol = SEM_PRIO_DEFAULT;
	ret_chk = sem_setprotocol(&sem, protocol);
	TC_ASSERT_EQ("sem_setprotocol", ret_chk, ERROR);
	TC_ASSERT_EQ("sem_setprotocol", errno, EINVAL);

	ret_chk = sem_destroy(&sem);
	TC_ASSERT_EQ("sem_destroy", ret_chk, OK);

	TC_SUCCESS_RESULT();

	return;
}
#endif

/****************************************************************************
 * Name: libc_semaphore
 ****************************************************************************/

int libc_semaphore_main(void)
{
	tc_libc_semaphore_sem_getvalue();
	tc_libc_semaphore_sem_getprotocol();
	tc_libc_semaphore_sem_init();
#ifndef CONFIG_PRIORITY_INHERITANCE
	tc_libc_semaphore_sem_setprotocol();
#endif

	return 0;
}
