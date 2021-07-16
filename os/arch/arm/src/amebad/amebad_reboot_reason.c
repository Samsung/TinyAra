/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 *
 *   Copyright (C) 2020 Gregory Nutt. All rights reserved.
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

#include <stdbool.h>
#include <debug.h>
#include <tinyara/reboot_reason.h>
#include "ameba_soc.h"
/****************************************************************************
 * Public functions
 ****************************************************************************/
#ifdef CONFIG_SYSTEM_REBOOT_REASON

static reboot_reason_code_t reboot_reason;

static reboot_reason_code_t up_reboot_reason_get_hw_value(void)
{
	u32 boot_reason = 0;

	/* Read the same backup register for the boot reason */
	boot_reason = BKUP_Read(BKUP_REG1);

	if (boot_reason != REBOOT_REASON_INITIALIZED) {
		return boot_reason;
	} else {
		/* Read AmebaD Boot Reason, WDT and HW reset supported */
		boot_reason = BOOT_Reason();

		/* HW reset */
		if (boot_reason == 0) {
			return REBOOT_SYSTEM_HW_RESET;
		}

		/* KM4 or KM0 WDT reset */
		else if ((boot_reason & BIT_BOOT_KM4WDG_RESET_HAPPEN) || (boot_reason & BIT_BOOT_WDG_RESET_HAPPEN)) {
			return REBOOT_SYSTEM_WATCHDOG;
		}

		/* KM4 deep sleep handled by KM0 (KM4 sleep + KM0 tickless, KM4 deep sleep + KM0 deep sleep AON) */
		else if (boot_reason & BIT_BOOT_DSLP_RESET_HAPPEN) {
			return REBOOT_SYSTEM_DSLP_RESET;
		}

		/* KM4 or KM0 System reset */
		else if ((boot_reason & BIT_BOOT_KM4SYS_RESET_HAPPEN) || (boot_reason & BIT_BOOT_SYS_RESET_HAPPEN)) {
			return REBOOT_SYSTEM_SYS_RESET;
		}

		/* Brownout reset */
		else if (boot_reason & BIT_BOOT_BOD_RESET_HAPPEN) {
			return REBOOT_SYSTEM_BOD_RESET;
		}
	}

	return REBOOT_UNKNOWN;
}

void up_reboot_reason_init(void)
{
	reboot_reason = up_reboot_reason_get_hw_value();
	BKUP_Write(BKUP_REG1, REBOOT_REASON_INITIALIZED);
}

reboot_reason_code_t up_reboot_reason_read(void)
{
	int reason = reboot_reason;
	rrvdbg("Read Reboot Reason : %d\n", reason);
	return reason;
}

void up_reboot_reason_write(reboot_reason_code_t reason)
{
	rrvdbg("Write Reboot Reason : %d\n", reason);
	/* Set the specific bit in BKUP_REG1 */
	BKUP_Write(BKUP_REG1, (u32)reason);
}

void up_reboot_reason_clear(void)
{
	rrvdbg("Clear Reboot Reason\n");
	/* Reboot Reason Clear API writes the REBOOT_REASON_INITIALIZED by default.
	 * If chip vendor needs another thing to do, please change the below.
	 */
	up_reboot_reason_write(REBOOT_REASON_INITIALIZED);
	reboot_reason = REBOOT_REASON_INITIALIZED;
}

bool up_reboot_reason_is_written(void)
{
	if (BKUP_Read(BKUP_REG1) != REBOOT_REASON_INITIALIZED) {
		return true;
	}

	return false;
}
#endif
