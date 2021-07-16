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
#ifndef __INCLUDE_REBOOT_REASON_H
#define __INCLUDE_REBOOT_REASON_H

/********************************************************************************************
 * Included Files
 ********************************************************************************************/
#include <sys/prctl.h>

typedef enum {
	/* [Reboot Reason Code]
	 * System : 0 ~ 29,
	 * Network : 30 ~ 59,
	 * Common Service : 60 ~ 89,
	 * App : 90 ~ 254
	 */
	REBOOT_REASON_INITIALIZED          = 0,
	REBOOT_SYSTEM_DATAABORT            = 1,	 /* Data abort */
	REBOOT_SYSTEM_PREFETCHABORT        = 2,	 /* Prefetch abort */
	REBOOT_SYSTEM_MEMORYALLOCFAIL      = 3,	 /* Memory allocation failure */
	REBOOT_SYSTEM_WATCHDOG             = 4,	 /* Watchdog timeout */
	REBOOT_SYSTEM_HW_RESET             = 5,	 /* HW power reset */
	REBOOT_SYSTEM_USER_INTENDED        = 6,	 /* Reboot from user intention */
	REBOOT_SYSTEM_BINARY_UPDATE        = 7,	 /* Reboot for Binary Update */
	REBOOT_SYSTEM_BINARY_RECOVERYFAIL  = 8,	 /* Binary Recovery Fail */

	REBOOT_SYSTEM_DSLP_RESET           = 27, /* System wake up from deep sleep */
	REBOOT_SYSTEM_SYS_RESET            = 28, /* System reset by calling sys_reset */
	REBOOT_SYSTEM_BOD_RESET            = 29, /* Brownout reset */

	REBOOT_NETWORK_WIFICORE_WATCHDOG   = 30, /* Wi-Fi Core Watchdog Reset */
	REBOOT_NETWORK_WIFICORE_PANIC      = 31, /* Wi-Fi Core Panic */

	REBOOT_UNKNOWN                     = 255,
} reboot_reason_code_t;

#define WRITE_REBOOT_REASON(x) do {                                       \
					prctl(PR_REBOOT_REASON_WRITE, x); \
				} while (0)
#define READ_REBOOT_REASON() prctl(PR_REBOOT_REASON_READ)
#define CLEAR_REBOOT_REASON() do {                                     \
					prctl(PR_REBOOT_REASON_CLEAR); \
				} while (0)

#endif					/* __INCLUDEREBOOT_REASON_H */
