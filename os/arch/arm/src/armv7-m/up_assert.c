/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
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
 * arch/arm/src/armv7-m/up_assert.c
 *
 *   Copyright (C) 2009-2010, 2012-2014 Gregory Nutt. All rights reserved.
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

/* Output debug info if stack dump is selected -- even if debug is not
 * selected.
 */

#ifdef CONFIG_ARCH_STACKDUMP
#undef  CONFIG_DEBUG
#undef  CONFIG_DEBUG_ERROR
#undef  CONFIG_DEBUG_WARN
#undef  CONFIG_DEBUG_VERBOSE
#undef  CONFIG_LOGM
#define CONFIG_DEBUG 1
#define CONFIG_DEBUG_ERROR 1
#define CONFIG_DEBUG_WARN 1
#define CONFIG_DEBUG_VERBOSE 1
#endif

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <debug.h>

#include <tinyara/irq.h>
#include <tinyara/arch.h>
#include <tinyara/board.h>
#include <tinyara/syslog/syslog.h>

#include <arch/board/board.h>

#include "sched/sched.h"
#include "irq/irq.h"

#include "up_arch.h"
#include "up_internal.h"
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* USB trace dumping */

#ifndef CONFIG_USBDEV_TRACE
#undef CONFIG_ARCH_USBDUMP
#endif

#ifndef CONFIG_BOARD_RESET_ON_ASSERT
#define CONFIG_BOARD_RESET_ON_ASSERT 0
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_ARCH_STACKDUMP
static uint32_t s_last_regs[XCPTCONTEXT_REGS];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_getsp
 ****************************************************************************/

/* I don't know if the builtin to get SP is enabled */

static inline uint32_t up_getsp(void)
{
	uint32_t sp;
	__asm__
	(
		"\tmov %0, sp\n\t"
		: "=r"(sp)
		   );
	return sp;
}

/****************************************************************************
 * Name: up_stackdump
 ****************************************************************************/

#ifdef CONFIG_ARCH_STACKDUMP
static void up_stackdump(uint32_t sp, uint32_t stack_base)
{
	uint32_t stack;

	for (stack = sp & ~0x1f; stack < stack_base; stack += 32) {
#ifdef CONFIG_DEBUG_ALERT
		uint32_t *ptr = (uint32_t *)stack;
		_alert("%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
			   stack, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
#endif
	}
}
#else
#define up_stackdump(sp, stack_base)
#endif

/****************************************************************************
 * Name: up_taskdump
 ****************************************************************************/

#ifdef CONFIG_STACK_COLORATION
static void up_taskdump(FAR struct tcb_s *tcb, FAR void *arg)
{
	/* Dump interesting properties of this task */

#if CONFIG_TASK_NAME_SIZE > 0
	_alert("%s: PID=%d Stack Used=%lu of %lu\n",
		   tcb->name, tcb->pid, (unsigned long)up_check_tcbstack(tcb), (unsigned long)tcb->adj_stack_size);
#else
	_alert("PID: %d Stack Used=%lu of %lu\n",
		   tcb->pid, (unsigned long)up_check_tcbstack(tcb), (unsigned long)tcb->adj_stack_size);
#endif
}
#endif

/****************************************************************************
 * Name: up_showtasks
 ****************************************************************************/

#ifdef CONFIG_STACK_COLORATION
static inline void up_showtasks(void)
{
	/* Dump interesting properties of each task in the crash environment */

	sched_foreach(up_taskdump, NULL);
}
#else
#define up_showtasks()
#endif

/****************************************************************************
 * Name: up_registerdump
 ****************************************************************************/
/* Dump interesting properties of each task in the crash environment */
#ifdef CONFIG_ARCH_STACKDUMP
static inline void up_registerdump(void)
{
	volatile uint32_t *regs = current_regs;

	/* Are user registers available from interrupt processing? */

	if (regs == NULL) {
		/* No.. capture user registers by hand */

		up_saveusercontext(s_last_regs);
		regs = s_last_regs;
	}

	/* Dump the interrupt registers */

	_alert("R0: %08x %08x %08x %08x %08x %08x %08x %08x\n", regs[REG_R0], regs[REG_R1], regs[REG_R2], regs[REG_R3], regs[REG_R4], regs[REG_R5], regs[REG_R6], regs[REG_R7]);
	_alert("R8: %08x %08x %08x %08x %08x %08x %08x %08x\n", regs[REG_R8], regs[REG_R9], regs[REG_R10], regs[REG_R11], regs[REG_R12], regs[REG_R13], regs[REG_R14], regs[REG_R15]);

#ifdef CONFIG_ARMV7M_USEBASEPRI
	_alert("xPSR: %08x BASEPRI: %08x CONTROL: %08x\n", regs[REG_XPSR], regs[REG_BASEPRI], getcontrol());
#else
	_alert("xPSR: %08x PRIMASK: %08x CONTROL: %08x\n", regs[REG_XPSR], regs[REG_PRIMASK], getcontrol());
#endif

#ifdef REG_EXC_RETURN
	_alert("EXC_RETURN: %08x\n", regs[REG_EXC_RETURN]);
#endif
}
#else
#define up_registerdump()
#endif

/****************************************************************************
 * Name: assert_tracecallback
 ****************************************************************************/

#ifdef CONFIG_ARCH_USBDUMP
static int usbtrace_syslog(FAR const char *fmt, ...)
{
	va_list ap;
	int ret;

	/* Let vsyslog do the real work */

	va_start(ap, fmt);
	ret = lowvsyslog(LOG_INFO, fmt, ap);
	va_end(ap);
	return ret;
}

static int assert_tracecallback(FAR struct usbtrace_s *trace, FAR void *arg)
{
	usbtrace_trprintf(usbtrace_syslog, trace->event, trace->value);
	return 0;
}
#endif

/****************************************************************************
 * Name: up_dumpstate
 ****************************************************************************/

#ifdef CONFIG_ARCH_STACKDUMP
static void up_dumpstate(void)
{
	struct tcb_s *rtcb = running_task();
	uint32_t sp = up_getsp();
	uint32_t ustackbase;
	uint32_t ustacksize;
#if CONFIG_ARCH_INTERRUPTSTACK > 7
	uint32_t istackbase;
	uint32_t istacksize;
#endif

	/* Get the limits on the user stack memory */

	up_registerdump();

	/* Get the limits on the user stack memory */

	if (rtcb->pid == 0) {
		ustackbase = g_idle_topstack - 4;
		ustacksize = CONFIG_IDLETHREAD_STACKSIZE;
	} else {
		ustackbase = (uint32_t) rtcb->adj_stack_ptr;
		ustacksize = (uint32_t) rtcb->adj_stack_size;
	}

#if CONFIG_ARCH_INTERRUPTSTACK > 3
	/* Get the limits on the interrupt stack memory */

	istackbase = (uint32_t) & g_intstackbase;
	istacksize = (CONFIG_ARCH_INTERRUPTSTACK & ~3);

	/* Show interrupt stack info */

	_alert("sp:     %08x\n", sp);
	_alert("IRQ stack:\n");
	_alert("  base: %08x\n", istackbase);
	_alert("  size: %08x\n", istacksize);
#ifdef CONFIG_STACK_COLORATION
	_alert("  used: %08x\n", up_check_intstack());
#endif

	/* Does the current stack pointer lie within the interrupt
	 * stack?
	 */

	if (sp <= istackbase && sp > istackbase - istacksize) {
		/* Yes.. dump the interrupt stack */

		up_stackdump(sp, istackbase);
	} else if (current_regs) {
		_alert("ERROR: Stack pointer is not within the interrupt stack\n");
		up_stackdump(istackbase - istacksize, istackbase);
	}

	/* Extract the user stack pointer if we are in an interrupt handler.
	 * If we are not in an interrupt handler.  Then sp is the user stack
	 * pointer (and the above range check should have failed).
	 */

	if (current_regs) {
		sp = current_regs[REG_R13];
		_alert("sp:     %08x\n", sp);
	}

	_alert("User stack:\n");
	_alert("  base: %08x\n", ustackbase);
	_alert("  size: %08x\n", ustacksize);
#ifdef CONFIG_STACK_COLORATION
	_alert("  used: %08x\n", up_check_tcbstack(rtcb));
#endif

	/* Dump the user stack if the stack pointer lies within the allocated user
	 * stack memory.
	 */

	if (sp <= ustackbase && sp > ustackbase - ustacksize) {
		up_stackdump(sp, ustackbase);
	} else {
		_alert("ERROR: Stack pointer is not within the allocated stack\n");
		up_stackdump(ustackbase - ustacksize, ustackbase);
	}

#else

	/* Show user stack info */

	_alert("sp:         %08x\n", sp);
	_alert("stack base: %08x\n", ustackbase);
	_alert("stack size: %08x\n", ustacksize);
#ifdef CONFIG_STACK_COLORATION
	_alert("stack used: %08x\n", up_check_tcbstack(rtcb));
#endif

	/* Dump the user stack if the stack pointer lies within the allocated user
	 * stack memory.
	 */

	if (sp > ustackbase || sp <= ustackbase - ustacksize) {
		_alert("ERROR: Stack pointer is not within the allocated stack\n");
	} else {
		up_stackdump(sp, ustackbase);
	}

#endif

	/* Then dump the registers (if available) */

	/* Dump the state of all tasks (if available) */

	up_showtasks();

#ifdef CONFIG_ARCH_USBDUMP
	/* Dump USB trace data */

	(void)usbtrace_enumerate(assert_tracecallback, NULL);
#endif
}
#else
#define up_dumpstate()
#endif

/****************************************************************************
 * Name: _up_assert
 ****************************************************************************/

static void _up_assert(int errorcode) noreturn_function;
static void _up_assert(int errorcode)
{
	/* Flush any buffered SYSLOG data */

#if 0
	(void)syslog_flush();
#endif

	/* Are we in an interrupt handler or the idle task? */

	if (current_regs || (running_task())->pid == 0) {
		(void)irqsave();
		for (;;) {
#ifdef CONFIG_ARCH_LEDS
			board_led_on(LED_PANIC);
			up_mdelay(250);
			board_led_off(LED_PANIC);
			up_mdelay(250);
#endif
		}
	} else {
#if CONFIG_BOARD_RESET_ON_ASSERT >= 2
		board_reset(CONFIG_BOARD_ASSERT_RESET_VALUE);
#endif
		exit(errorcode);
	}
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_assert
 ****************************************************************************/

void up_assert(const uint8_t *filename, int lineno)
{
#if CONFIG_TASK_NAME_SIZE > 0 && defined(CONFIG_DEBUG_ALERT)
	struct tcb_s *rtcb = running_task();
#endif
	/* ToDo: implement as per the armv7-m architecture */
	board_led_on(LED_ASSERTION);

	/* Flush any buffered SYSLOG data (prior to the assertion) */

#if 0
	(void)syslog_flush();
#endif

#if CONFIG_TASK_NAME_SIZE > 0
	_alert("Assertion failed at file:%s line: %d task: %s\n", filename, lineno, this_task()->name);
#else
	_alert("Assertion failed at file:%s line: %d\n", filename, lineno);
#endif

	up_dumpstate();

#ifdef CONFIG_BOARD_CRASHDUMP
	board_crashdump(up_getsp(), running_task(), filename, lineno);
#endif

#ifdef CONFIG_BOARD_ASSERT_AUTORESET
	(void)boardctl(BOARDIOC_RESET, 0);
#endif

	_up_assert(EXIT_FAILURE);
}
