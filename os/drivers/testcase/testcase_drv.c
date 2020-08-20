/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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
#include <errno.h>
#include <debug.h>
#include <tinyara/fs/fs.h>
#include <tinyara/testcase_drv.h>
#include <tinyara/sched.h>
#include "signal/signal.h"
#include "testcase_proto.h"
#ifdef CONFIG_EXAMPLES_MPU_TEST
#include "binary_manager/binary_manager.h"
#include <tinyara/binfmt/binfmt.h>
#include <tinyara/mpu_test.h>
#endif

/****************************************************************************
 * Public variables
 ****************************************************************************/
#ifdef CONFIG_EXAMPLES_MPU_TEST
extern uint32_t _stext;
extern uint32_t _sdata;
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int testcase_drv_ioctl(FAR struct file *filep, int cmd, unsigned long arg);
static ssize_t testcase_drv_read(FAR struct file *filep, FAR char *buffer, size_t len);
static ssize_t testcase_drv_write(FAR struct file *filep, FAR const char *buffer, size_t len);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations testcase_drv_fops = {
	0,						/* open */
	0,						/* close */
	testcase_drv_read,				/* read */
	testcase_drv_write,				/* write */
	0,						/* seek */
	testcase_drv_ioctl				/* ioctl */
#ifndef CONFIG_DISABLE_POLL
	, 0						/* poll */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/************************************************************************************
 * Name: testcase_drv_ioctl
 *
 * Description:  The standard ioctl method.
 *
 ************************************************************************************/

static int testcase_drv_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	/* Handle built-in ioctl commands */

	switch (cmd) {
	/* TESTFWIOC_DRIVER_ANALOG - Run the test case for /os/driver/analog module
	 *
	 *   ioctl argument:  An integer value indicating the particular test to be run
	 */

	case TESTIOC_ANALOG:
		break;
#ifndef CONFIG_DISABLE_SIGNALS
	case TESTIOC_GET_SIG_FINDACTION_ADD:
	case TESTIOC_SIGNAL_PAUSE:
	case TESTIOC_GET_TCB_SIGPROCMASK:
		ret = test_signal(cmd, arg);
		break;
#endif
#ifdef CONFIG_TC_KERNEL_ROUNDROBIN
	case TESTIOC_GET_TCB_TIMESLICE:
#endif
	case TESTIOC_GET_SELF_PID:
	case TESTIOC_IS_ALIVE_THREAD:
	case TESTIOC_GET_TCB_ADJ_STACK_SIZE:
	case TESTIOC_SCHED_FOREACH:
		ret = test_sched(cmd, arg);
		break;
	case TESTIOC_CLOCK_ABSTIME2TICKS_TEST:
		ret = test_clock(cmd, arg);
		break;
	case TESTIOC_TIMER_INITIALIZE_TEST:
		ret = test_timer(cmd, arg);
		break;
	case TESTIOC_SEM_TICK_WAIT_TEST:
		ret = test_sem(cmd, arg);
		break;
#if defined(CONFIG_SCHED_HAVE_PARENT) && defined(CONFIG_SCHED_CHILD_STATUS)
	case TESTIOC_GROUP_ADD_FINED_REMOVE_TEST:
	case TESTIOC_GROUP_ALLOC_FREE_TEST:
	case TESTIOC_GROUP_EXIT_CHILD_TEST:
	case TESTIOC_GROUP_REMOVECHILDREN_TEST:
		ret = test_group(cmd, arg);
		break;
#endif
	case TESTIOC_TASK_REPARENT:
	case TESTIOC_TASK_INIT_TEST:
		ret = test_task(cmd, arg);
		break;
#ifdef CONFIG_TC_COMPRESS_READ
	case TESTIOC_COMPRESSION_TEST:
		ret = test_compress_decompress(cmd, arg);
		break;
#endif

#ifdef CONFIG_EXAMPLES_MPU_TEST
	case TESTIOC_MPUTEST:
		ret = OK;
		struct mputest_arg_s *obj = (struct mputest_arg_s*)arg;
		
		if (!obj) {
			return -EINVAL;
		}

		switch (obj->type) {
		case MPUTEST_KERNEL_CODE:
			obj->addr = &_stext;
			break;
		case MPUTEST_KERNEL_DATA:
			obj->addr = &_sdata;
			break;
		case MPUTEST_APP_ADDR:
		{
			/* Find the current executing app and return an address
			* which belongs to any other app in the system. Here,
			* we choose to return the address of the app heap
			*/
			uint32_t binid = sched_self()->group->tg_binidx;
			binid = (binid + 1) % (binary_manager_get_ucount() + 1);
			if (binid == 0) {
				binid++;
			}
			obj->addr = (volatile uint32_t *)BIN_LOADINFO(binid)->uheap;
			break;
		}
		default:
			ret = -EINVAL;
			break;
		}

		break;
#endif

	default:
		vdbg("Unrecognized cmd: %d arg: %ld\n", cmd, arg);
		break;
	}

	return ret;
}


static ssize_t testcase_drv_read(FAR struct file *filep, FAR char *buffer, size_t len)
{
	return 0;                                       /* Return EOF */
}

static ssize_t testcase_drv_write(FAR struct file *filep, FAR const char *buffer, size_t len)
{
	return len;                                     /* Say that everything was written */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: testcase_drv_register
 *
 * Description:
 *   Register /dev/testcase
 *
 ****************************************************************************/

void testcase_drv_register(void)
{
	(void)register_driver(TESTCASE_DRVPATH, &testcase_drv_fops, 0666, NULL);
}
