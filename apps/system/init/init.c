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
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <tinyara/config.h>
#ifdef CONFIG_SCHED_USRWORK
#include <tinyara/wqueue.h>
#endif
#ifdef CONFIG_TASH
#include <apps/shell/tash.h>
#include <apps/system/utils.h>
#endif
#ifdef CONFIG_DEBUG_SYSTEM_APP
#include <apps/system/sysdbgapp_init.h>
#endif
#ifdef CONFIG_BUILTIN_APPS
#include <apps/builtin.h>
#endif
#ifdef CONFIG_SYSTEM_INFORMATION
#include <apps/system/sysinfo.h>
#endif
#ifdef CONFIG_TASK_MANAGER
#include <task_manager/task_manager.h>
#endif
#ifdef CONFIG_EVENTLOOP
#include <tinyara/eventloop.h>
#endif
#ifdef CONFIG_MEDIA
#include <media/media_init.h>
#endif
#ifdef CONFIG_WIFI_MANAGER
#include <tinyara/wifi/wifi_manager.h>
#endif
#ifdef CONFIG_BLE_MANAGER
#include <tinyara/ble/ble_handler.h>
#endif
#ifdef CONFIG_PREFERENCE
#include <tinyara/preference.h>
#endif
#ifdef CONFIG_HAVE_CXXINITIALIZE
#include <semaphore.h>
#include <errno.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* In the protected build (only) we also need to start the user work queue */

#if !defined(CONFIG_BUILD_PROTECTED)
#undef CONFIG_SCHED_USRWORK
#endif

#ifdef CONFIG_ENABLE_IOTJS
extern void iotjs_register_cmds(void);
#endif

/****************************************************************************
 * Pravite Functions
 ****************************************************************************/
#ifdef CONFIG_TASH
static void tash_register_cmds(void)
{
#if defined(CONFIG_SYSTEM_CMDS)
	system_register_utilcmds();
#endif

#ifdef CONFIG_FS_CMDS
	fs_register_utilcmds();
#endif

#ifdef CONFIG_NET_CMDS
	net_register_utilcmds();
	net_register_appcmds();
#endif

#ifdef CONFIG_BUILTIN_APPS
	register_examples_cmds();
#endif

#ifdef CONFIG_ENABLE_IOTJS
	iotjs_register_cmds();
#endif

#ifdef CONFIG_DEBUG_SYSTEM_APP
	sysdbgapp_init();
#endif
}
#endif							/* CONFIG_TASH */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int preapp_start(int argc, char *argv[])
#endif
{
#if defined(CONFIG_SCHED_USRWORK) || defined(CONFIG_TASH) || defined(CONFIG_EVENTLOOP) ||\
	defined(CONFIG_TASK_MANAGER) || defined(CONFIG_MEDIA) || defined(CONFIG_PREFERENCE)
	int ret;
#endif

#ifdef CONFIG_HAVE_CXXINITIALIZE
	sem_t	sem;
	sem_init(&sem, 0, 0);

	up_cxxinitialize();
#endif

#ifdef CONFIG_SYSTEM_INFORMATION
	sysinfo();
#endif

#ifdef CONFIG_PREFERENCE
	ret = preference_init();
	if (ret < 0) {
		printf("Preference initialization is failed, error %d\n", ret);
	}
#endif

#ifdef CONFIG_SCHED_USRWORK
	/* Start the user-space work queue */
	ret = work_usrstart();
	if (ret <= 0) {
		printf("user work queue is failed to start, error code is %d\n", ret);
		return ret;
	}
#endif

#ifdef CONFIG_TASH
	ret = tash_start();
	if (ret <= 0) {
		printf("TASH is failed to start, error code is %d\n", ret);
		return ret;
	}

	tash_register_cmds();
#endif

#ifdef CONFIG_TASK_MANAGER
#define TASKMGR_STACK_SIZE 2048
#define TASKMGR_PRIORITY 200
	ret = task_create("task_manager", TASKMGR_PRIORITY, TASKMGR_STACK_SIZE, task_manager, (FAR char *const *)NULL);
	if (ret < 0) {
		printf("Failed to create Task Manager\n");
		return ret;
	}
#endif

#ifdef CONFIG_EVENTLOOP
	ret = eventloop_task_start();
	if (ret <= 0) {
		printf("eventloop is failed to start, error code is %d\n", ret);
		return ret;
	}
#endif

#ifdef CONFIG_MEDIA
	ret = media_init();
	if (ret < 0) {
		printf("media is failed to start, error code is %d\n", ret);
		return ret;
	}
#endif

#if defined(CONFIG_WIFI_MANAGER)
	//(void)wifimgr_run_msghandler();
#endif

#if defined(CONFIG_BLE_MANAGER)
	(void)blemgr_run_msghandler();
#endif

/***********************************************************************************
 *	current preapp_start does the up_cxxinitialize which initializes the
 *	static constructors. All the tasks and threads created by preapp refer to
 *	these constructors. To make sure that c++ test cases from tash runs properly,
 *	we need to keep preapp/constructor initializations alive with below changes.
 **********************************************************************************/
#ifdef CONFIG_HAVE_CXXINITIALIZE
	while (sem_wait(&sem) != 0) {
		/* The only case that an error should occur here is if the wait was
		 * awakened by a signal.
		 */
		if (errno == EINTR) {
			printf("awakened by signal..\n");
		}
	}
#endif

	return 0;
}
