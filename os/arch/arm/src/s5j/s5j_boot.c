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
 * arch/arm/src/s5j/s5j_boot.c
 *
 *   Copyright (C) 2009-2010, 2014-2015 Gregory Nutt. All rights reserved.
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

#include <stdint.h>
#include <assert.h>
#include <debug.h>

#include <tinyara/init.h>

#include "up_arch.h"
#include "up_internal.h"

#include <chip.h>
#include "s5j_watchdog.h"
#include "arm.h"
#ifdef CONFIG_ARMV7M_MPU
#include "mpu.h"
#endif
#include "cache.h"
#include "fpu.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern uint32_t _vector_start;
extern uint32_t _vector_end;

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_UTASK_MEMORY_PROTECTION)
const struct mpu_region_info regions_info[] = {
	{
		&mpu_priv_intsram_wb, 0x0, 0x80000000, MPU_REG_ENTIRE_MAP,
	},
	{
		&mpu_user_intsram_wb, 0x02060000, 0x1000, MPU_REG_USER_DATA,
	},
	{
		&mpu_priv_noncache, 0x02100000, 0x80000, MPU_REG_KERN_REG0,
	},
	{
		&mpu_priv_flash, S5J_FLASH_MIRROR_PADDR, S5J_FLASH_MIRROR_SIZE, MPU_REG_KERN_REG1,
	},
	{
		&mpu_peripheral, S5J_PERIPHERAL_PADDR, S5J_PERIPHERAL_SIZE, MPU_REG_KERN_PERI,
	},
	{
		&mpu_user_flash, (uintptr_t)__uflash_segment_start__, (size_t)__uflash_segment_size__, MPU_REG_KERN_CODE,
	},
	{
		&mpu_priv_flash, S5J_IRAM_MIRROR_PADDR, S5J_IRAM_MIRROR_SIZE, MPU_REG_KERN_VEC,
	}
};
#elif defined(CONFIG_BUILD_PROTECTED)
const struct mpu_region_info regions_info[] = {
	{
		&mpu_priv_flash, 0x0, 0x80000000, MPU_REG0,
	},
	{
		&mpu_user_intsram_wb, S5J_IRAM_PADDR, S5J_IRAM_SIZE, MPU_REG1,
	},
	{
		&mpu_priv_intsram_wb, S5J_IRAM_PADDR, 0x40000, MPU_REG2,
	},
	{
		&mpu_priv_noncache, 0x02100000, 0x80000, MPU_REG3,
	},
	{
		&mpu_priv_intsram_wb, S5J_FLASH_PADDR, S5J_FLASH_SIZE, MPU_REG4,
	},
	{
		&mpu_user_intsram_ro, (uintptr_t)__uflash_segment_start__, (size_t)__uflash_segment_size__, MPU_REG5,
	},
	{
		&mpu_priv_flash, S5J_FLASH_MIRROR_PADDR, S5J_FLASH_MIRROR_SIZE, MPU_REG6,
	},
	{
		&mpu_priv_stronglyordered, S5J_PERIPHERAL_PADDR, S5J_PERIPHERAL_SIZE, MPU_REG7
	},
	{
		&mpu_priv_flash, S5J_IRAM_MIRROR_PADDR, S5J_IRAM_MIRROR_SIZE, MPU_REG8
	}
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#ifdef CONFIG_STACK_COLORATION
static inline void up_idlestack_color(void *pv, unsigned int nbytes)
{
	register void *r0 __asm__("r0");
	register unsigned int r1 __asm__("r1");

	r0 = pv;
	r1 = nbytes;

	/* Set the IDLE stack to the stack coloration value then jump to
	 * os_start().  We take extreme care here because were currently
	 * executing on this stack.
	 *
	 * We want to avoid sneak stack access generated by the compiler.
	 */

	__asm__ __volatile__("\tmovs  r1, r1, lsr #2\n"	/* R1 = nwords = nbytes >> 2 */
						 "\tbeq  2f\n"	/* (should not happen) */
						 "\tbic  r0, r0, #3\n"	/* R0 = Aligned stackptr */
						 "\tmovw r2, #0xbeef\n"	/* R2 = STACK_COLOR = 0xdeadbeef */
						 "\tmovt r2, #0xdead\n" "1:\n"	/* Top of the loop */
						 "\tsub  r1, r1, #1\n"	/* R1 nwords-- */
						 "\tcmp  r1, #0\n"	/* Check (nwords == 0) */
						 "\tstr  r2, [r0], #4\n"	/* Save stack color word, increment stackptr */
						 "\tbne  1b\n"	/* Bottom of the loop */
						 "2:\n" "\tmov  r14, #0\n"	/* LR = return address (none) */
						 "\tb    os_start\n"	/* Branch to os_start */
						 :		/* No Output */
						 : "r"(r0), "r"(r1)
						);

	__builtin_unreachable();
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void up_copyvectorblock(void)
{
	uint32_t *src = (uint32_t *)&_vector_start;
	uint32_t *end = (uint32_t *)&_vector_end;
	uint32_t *dest = (uint32_t *)VECTOR_BASE;

	while (src < end) {
		*dest++ = *src++;
	}
}

#ifdef CONFIG_ARMV7M_MPU
int s5j_mpu_initialize(void)
{
#ifdef CONFIG_BUILD_PROTECTED
	int i;

	for (i = 0; i < (sizeof(regions_info) / sizeof(struct mpu_region_info)); i++) {
		regions_info[i].call(regions_info[i].base, regions_info[i].size);
	}
#elif CONFIG_ARCH_CHIP_S5JT200
	/*
	 * Vector Table	0x02020000	0x02020FFF	4
	 * Reserved		0x02021000	0x020217FF	2
	 * BL1			0x02021800	0x020237FF	8
	 * TizenRT		0x02023800	0x0210FFFF	946(WBWA)
	 * WIFI			0x02110000	0x0215FFFF	320(NCNB)
	 */

	/* Region 0, Set read only for memory area */
	mpu_priv_flash(0x0, 0x80000000);

	/* Region 1, for ISRAM(0x02020000++1280KB, RW-WBWA */
	mpu_user_intsram_wb(S5J_IRAM_PADDR, S5J_IRAM_SIZE);

	/* Region 2, wifi driver needs non-$(0x02110000++320KB, RW-NCNB */
	mpu_priv_noncache(S5J_IRAM_PADDR + ((4 + 2 + 8 + 946) * 1024), (320 * 1024));

	/* Region 3, for FLASH area, default to set WBWA */
	mpu_user_intsram_wb(S5J_FLASH_PADDR, S5J_FLASH_SIZE);

	/* region 4, for Sflash Mirror area to be read only */
	mpu_priv_flash(S5J_FLASH_MIRROR_PADDR, S5J_FLASH_MIRROR_SIZE);

	/* Region 5, for SFR area read/write, strongly-ordered */
	mpu_priv_stronglyordered(S5J_PERIPHERAL_PADDR, S5J_PERIPHERAL_SIZE);

	/*
	 * Region 6, for vector table,
	 * set the entire high vector region as read-only.
	 */
	mpu_priv_flash(S5J_IRAM_MIRROR_PADDR, S5J_IRAM_MIRROR_SIZE);
#endif
	mpu_control(true);

	return 0;
}
#endif

void arm_boot(void)
{
	up_copyvectorblock();

	/* Disable the watchdog timer */
#ifdef CONFIG_S5J_WATCHDOG
	s5j_watchdog_disable();
#endif

#ifdef CONFIG_ARMV7R_MEMINIT
	/* Initialize the .bss and .data sections as well as RAM functions
	 * now after RAM has been initialized.
	 *
	 * NOTE that if SDRAM were supported, this call might have to be
	 * performed after returning from tms570_board_initialize()
	 */
	arm_data_initialize();
#endif

#ifdef CONFIG_BUILD_PROTECTED
	/* Initialize userspace .data and .bss section */
	s5j_userspace();
#endif

#ifdef CONFIG_ARMV7M_MPU
	s5j_mpu_initialize();
	arch_enable_icache();
	arch_enable_dcache();
#endif

	s5j_clkinit();

#ifdef USE_EARLYSERIALINIT
	up_earlyserialinit();
#endif

	/*
	 * Perform board-specific initialization. This must include:
	 *
	 * - Initialization of board-specific memory resources (e.g., SDRAM)
	 * - Configuration of board specific resources (GPIOs, LEDs, etc).
	 *
	 * NOTE: we must use caution prior to this point to make sure that
	 * the logic does not access any global variables that might lie
	 * in SDRAM.
	 */
	s5j_board_initialize();

	/*
	 * This function makes idle stack colored.
	 * Do not call any function between this and os_start.
	 */
#ifdef CONFIG_STACK_COLORATION
	up_idlestack_color((void *)&_ebss, CONFIG_IDLETHREAD_STACKSIZE);
#endif

	os_start();
}
