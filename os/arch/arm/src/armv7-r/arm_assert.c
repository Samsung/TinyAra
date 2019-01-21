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
 * arch/arm/src/armv7-r/arm_assert.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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
#include <string.h>
#include <assert.h>
#include <debug.h>

#include <tinyara/irq.h>
#include <tinyara/arch.h>
#include <tinyara/board.h>
#include <tinyara/usb/usbdev_trace.h>
#include <arch/board/board.h>

#include "up_arch.h"
#include "sched/sched.h"
#include "up_internal.h"
#ifdef CONFIG_ARMV7M_MPU
#include "mpu.h"
#endif
#include <stdbool.h>
#ifdef CONFIG_BOARD_ASSERT_AUTORESET
#include <sys/boardctl.h>
#endif
#ifdef CONFIG_DEBUG_DISPLAY_SYMBOL
#include <stdio.h>
#include <arch/board/symtab.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* USB trace dumping */

#ifndef CONFIG_USBDEV_TRACE
#undef CONFIG_ARCH_USBDUMP
#endif

#define FRAME_POINTER_ADDR __builtin_frame_address(0)

/* Flag used to detect the whether upassert called or not */
bool g_upassert = false;

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_DEBUG_DISPLAY_SYMBOL
static int symbolnum;
static bool symtab_init = false;
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
		uint32_t *ptr = (uint32_t *)stack;
		lldbg("%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n", stack, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
	}
}
#else
#define up_stackdump(sp, stack_base)
#endif

typedef struct regs {
	uint32_t reg[ARM_CONTEXT_REGS];	/* For 32 bit processor, R0 to R15 and CPSR total 16 */
} arm_regs_t;

arm_regs_t task_ctxt_regs;		/* Global variable to store the register context */
struct arm_mode_regs_s cpu_ctxt_regs;	/* Global variable to store the register context */

#ifdef CONFIG_FRAME_POINTER
struct stackframe {
	uint32_t programCounter;	// R15
	uint32_t linkRegister;		// R14
	uint32_t stackPointer;		// R13
	uint32_t framePointer;		// R11
};

/****************************************************************************
 * Name: is_text_address
 ****************************************************************************/

static int is_text_address(unsigned long programCounter)
{

#ifdef CONFIG_BUILD_PROTECTED
	/* In the kernel build, SVCalls are expected in either the base, kernel
	 * FLASH region or in the user FLASH region.
	 */

	if (((uintptr_t)programCounter >= (uintptr_t)&_stext &&
		 (uintptr_t)programCounter < (uintptr_t)&_etext) ||
		((uintptr_t)programCounter >= (uintptr_t)USERSPACE->us_textstart &&
		 (uintptr_t)programCounter < (uintptr_t)USERSPACE->us_textend)) {
		return 1;
	}
#else
	/* SVCalls are expected only from the base, kernel FLASH region */

	if ((uintptr_t)programCounter >= (uintptr_t)&_stext &&
		(uintptr_t)programCounter < (uintptr_t)&_etext) {
		return 1;
	}
#endif
	return 0;
}

#define MEM_READ(x) (*(uint32_t volatile*)(x))

#ifdef CONFIG_DEBUG_DISPLAY_SYMBOL
void up_symtab_init(void)
{
	if (up_symtab_flashdrv_init() == OK) {
		symtab_init = true;
	}
}

static unsigned long getaddr(int index)
{
	unsigned long addr;
	/* (index-1) corresponds to start of index's byte address */

	up_symtab_flashdrv_read(&addr, (index - 1) * 4 + 4, 4);
	return addr;
}

static void getname(int ind, char *buffer, size_t buflen)
{
	unsigned long sym_offlen;
	unsigned long nextsym_offlen;
	unsigned long symlen;

	memset(buffer, '\0', buflen);
	/* Reading offset symbol length of the symbol */

	up_symtab_flashdrv_read(&sym_offlen, (symbolnum * 4) + ((ind - 1) * 4) + 4, 4);
	/* Reading offset symbol length of next symbol */

	up_symtab_flashdrv_read(&nextsym_offlen, (symbolnum * 4) + (ind * 4) + 4, 4);
	/* Calculating the symbol length based on above two offset lengths */

	symlen = nextsym_offlen - sym_offlen;
	/* Reading the symbol name */

	up_symtab_flashdrv_read(buffer, symbolnum * 8 + sym_offlen + 4, symlen);
}

static void getsymbolnum(void)
{
	up_symtab_flashdrv_read(&symbolnum, 0, 4);
}

/****************************************************************************
 * Name: get_symbol
 * Below API only works if symtab flash driver is initialized
 ****************************************************************************/
static int get_symbol(unsigned long search_addr, char *buffer, size_t buflen)
{
	int first;
	int last;
	int mid;
	unsigned long addr;
	unsigned long addr_next;

	if (buffer == NULL) {
		lldbg("Invalid buffer input \n");
		return -1;
	}

	getsymbolnum();
	first = 1;
	last = symbolnum;

	while (first <= last) {

		mid = (first + last) / 2;
		addr = getaddr(mid);
		addr_next = getaddr(mid + 1);
		/* Checking whether given address lies in the range */

		if (addr > search_addr) {
			last = mid - 1;
		} else if (addr <= search_addr && search_addr < addr_next) {
			/* If present, the symbol name is obtained */

			getname(mid, buffer, buflen);
			snprintf(&buffer[strlen(buffer)], buflen - strlen(buffer), "+0x%lx/0x%lx", search_addr - addr, addr_next-addr);

			return OK;
		} else {
			first = mid + 1;
		}
	}
	lldbg("Symbol not found in System.map\n");
	return -1;
}
#endif

/****************************************************************************
 * Name: unwind_frame_with_fp
 ****************************************************************************/

static int unwind_frame_with_fp(struct stackframe *stack_frame, uint32_t stacksize)
{
	uint32_t high, low, stack_align_mask;
	uint32_t fp = stack_frame->framePointer;	/* Read the frame pointer */

	low = stack_frame->stackPointer;	/* Read the Stack pointer from frame */
	stack_align_mask = stacksize - 1;
	high = (low + stack_align_mask) & ~(stack_align_mask);

	/* lldbg("\n %s : low : 0x%p mask: 0x%p and high: 0x%p\n", __func__, low, mask, high); */

	/*
	 * check whether fp is  within the lower and higher stack boundary
	 */
	if (fp < low + 12 || fp > high - 4) {
		lldbg("End of Callstack\n");
		return -1;
	}

	/* Restore registers from stack frame */
	stack_frame->programCounter = MEM_READ(fp - 4);
	stack_frame->stackPointer = MEM_READ(fp - 8);
	stack_frame->framePointer = MEM_READ(fp - 12);

	return 0;
}

/****************************************************************************
 * Name: unwind_backtrace_with_fp
 ****************************************************************************/

static void unwind_backtrace_with_fp(arm_regs_t *regs, struct tcb_s *task)
{
	uint32_t ustacksize;
	struct stackframe stack_frame;
	struct tcb_s *current = this_task();

#if CONFIG_TASK_NAME_SIZE > 0
	lldbg("Task: [%s], Pid: [%d], TaskAddr: [0x%p] and [Current : %s]\n", task ? task->name : "No Task", task->pid, task, current ? current->name : "No Task");
#else
	lldbg("pid: %d, TaskAddr: [0x%p] \n", task->pid, task);
#endif

	if (0 == task->pid) {
		ustacksize = CONFIG_IDLETHREAD_STACKSIZE;
	} else {
		ustacksize = (uint32_t)task->adj_stack_size;
	}

	lldbg("stack size 0x%p\n", ustacksize);
	if (!current_regs && task == current) {
		stack_frame.programCounter = (uint32_t)unwind_backtrace_with_fp;
		stack_frame.stackPointer = up_getsp();
		stack_frame.framePointer = (uint32_t)FRAME_POINTER_ADDR;
	} else if (regs && regs->reg[REG_R15] && regs->reg[REG_R14] && task == current) {
		lldbg("Registers are Valid, We may be either Interrupt context/exception context\n");
		stack_frame.programCounter = regs->reg[REG_R15];	/* pc */
		stack_frame.stackPointer = regs->reg[REG_R13];	/* sp */
		stack_frame.framePointer = regs->reg[REG_R11];	/* fp */

		/* Check for text section, If PC is corrupted, use LR in that case. */
		if (!is_text_address(stack_frame.programCounter)) {
			stack_frame.programCounter = regs->reg[REG_R14];
		}
	} else {
		lldbg("Task would be blocked in context switch : up_saveusercontext\n");
		/* task blocked in up_saveusercontext */
		stack_frame.programCounter = task->xcp.regs[REG_R15];
		stack_frame.stackPointer = task->xcp.regs[REG_R13];	/* sp */
		stack_frame.framePointer = task->xcp.regs[REG_R11];	/* fp */
	}

	lldbg("fp:0x%p , sp:0x%p , pc:0x%p\n", stack_frame.framePointer, stack_frame.stackPointer, stack_frame.programCounter);

	lldbg("Call Stack:\n");
	for (;;) {
		uint32_t current_addr = stack_frame.programCounter;
		if (unwind_frame_with_fp(&stack_frame, ustacksize) >= 0) {
			/* Print the call stack address */
#ifdef CONFIG_DEBUG_DISPLAY_SYMBOL
			/* Only if symtab flash driver is initialized, get symbol is called */

			if (symtab_init) {
				char buffer[128];
				if (get_symbol(current_addr, buffer, sizeof(buffer)) == 0) {
					lldbg("[<0x%p>] %s\n", (void *)current_addr, buffer);
				}
			} else
#endif
			{
				lldbg("[<0x%p>]\n", (void *)current_addr);
			}
		} else {
			/* End of stack */
			break;
		}
	}
}

/****************************************************************************
 * Name: print_callstack
 ****************************************************************************/

static void print_callstack(FAR struct tcb_s *tcb, FAR void *arg)
{
	uint32_t regs;
	memset(&task_ctxt_regs, 0, sizeof(arm_regs_t));

	if (current_regs) {
		for (regs = REG_R0; regs < ARM_CONTEXT_REGS; regs++) {
			task_ctxt_regs.reg[regs] = current_regs[regs];
		}
	}
	/* Unwind the task stack by using fp (R11) */
	unwind_backtrace_with_fp(&task_ctxt_regs, tcb);
	lldbg("*******************************************************************************\n");

}

/****************************************************************************
 * Name: dump_all_stack
 ****************************************************************************/

void dump_all_stack(void)
{
	lldbg("*******************************************************************************\n");
	lldbg("Printing the call stack of all the tasks in the system\n");
	lldbg("*******************************************************************************\n");

	/* Display Call stack for all available tasks in the system */
	sched_foreach(print_callstack, NULL);
}

/****************************************************************************
 * Name: dump_stack
 ****************************************************************************/

void dump_stack(void)
{
	/* Disable the irqs */
	irqstate_t flags = irqsave();

	struct tcb_s *current = this_task();

	/* Display call stack for current task */
	print_callstack(current, NULL);

	/* Restore the irqs */
	irqrestore(flags);
}
#endif						/* End of CONFIG_FRAME_POINTER */

/****************************************************************************
 * Name: up_taskdump
 ****************************************************************************/

#ifdef CONFIG_STACK_COLORATION
static void up_taskdump(FAR struct tcb_s *tcb, FAR void *arg)
{
	/* Dump interesting properties of this task */

#if CONFIG_TASK_NAME_SIZE > 0
	lldbg("%10s | %5d | %4d | %7lu / %7lu\n",
			tcb->name, tcb->pid, tcb->sched_priority,
			(unsigned long)up_check_tcbstack(tcb), (unsigned long)tcb->adj_stack_size);
#else
	lldbg("%5d | %4d | %7lu / %7lu\n",
			tcb->pid, tcb->sched_priority, (unsigned long)up_check_tcbstack(tcb),
			(unsigned long)tcb->adj_stack_size);
#endif

	if (tcb->pid != 0 && up_check_tcbstack(tcb) == tcb->adj_stack_size) {
		lldbg("  !!! PID (%d) STACK OVERFLOW !!! \n", tcb->pid);
	}
}
#endif

/****************************************************************************
 * Name: up_showtasks
 ****************************************************************************/

#ifdef CONFIG_STACK_COLORATION
static inline void up_showtasks(void)
{
	lldbg("*******************************************\n");
	lldbg("List of all tasks in the system:\n");
	lldbg("*******************************************\n");

#if CONFIG_TASK_NAME_SIZE > 0
	lldbg("   NAME   |  PID  |  PRI |    USED /  TOTAL STACK\n");
	lldbg("--------------------------------------------------\n");
#else
	lldbg("  PID | PRI |   USED / TOTAL STACK\n");
	lldbg("----------------------------------\n");
#endif

	/* Dump interesting properties of each task in the crash environment */

	sched_foreach(up_taskdump, NULL);
}
#else
#define up_showtasks()
#endif

struct arm_mode_regs_s {
	/* ARM provides 37 General purpos registers as below */
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	/* When we enter exception mode, registers are saved in SVC mode
	 * so map svc registers bank registers to save first
	 * SVC mode has R13_SVC(sp), R14_SVC(lr) and SPSR_SVC as banked registers*/
	uint32_t sp_svc;
	uint32_t lr_svc;
	/* R15 and cpsr will have pc and current execution mode status */
	uint32_t pc;
	uint32_t spsr_svc;
	uint32_t cpsr;
	/* ARM provides other modes like USER/FIQ/IRQ/ABT/UND/SYS modes so save their
	 * banked registers for easy debugging.
	 * USR/SYS mode both uses same register set (USR mode:0x10 and SYS:0x1F)*/
	uint32_t sp_usr;
	uint32_t lr_usr;
	/* FIQ mode has R8_FIQ-R12_FIQ, R14_FIQ (lr), R13_FIQ(sp) and SPSR_FIQ */
	uint32_t r8_fiq;
	uint32_t r9_fiq;
	uint32_t r10_fiq;
	uint32_t r11_fiq;
	uint32_t r12_fiq;
	uint32_t sp_fiq;
	uint32_t lr_fiq;
	uint32_t spsr_fiq;
	/* IRQ mode has R13_IRQ(sp), R14_IRQ(lr) and SPSR_IRQ as banked registers */
	uint32_t sp_irq;
	uint32_t lr_irq;
	uint32_t spsr_irq;
	/* ABT mode has R13_ABT(sp), R14_ABT(lr) and SPSR_ABT as banked registers */
	uint32_t sp_abt;
	uint32_t lr_abt;
	uint32_t spsr_abt;
	/* UND mode has R13_UND(sp), R14_UND(lr) and SPSR_UND as banked registers */
	uint32_t sp_und;
	uint32_t lr_und;
	uint32_t spsr_und;

};

void save_cpuctxt_regs(struct arm_mode_regs_s *cpu_context_regs)
{
	/* cpu_regs is in r0 and thus we can use r0 as base address
	   and save the rest of the registers at multiple of 4 byte
	   address offset */
	asm("str r0, [%0, #0]\n"
		"mov r0, %0\n"
		"str r1, [r0, #0x4]\n"	/* r0 is a reference for cpu_context_regs struct */
		"str r2, [r0, #0x8]\n"
		"str r3, [r0, #0xC]\n"
		"str r4, [r0, #0x10]\n"
		"str r5, [r0, #0x14]\n"
		"str r6, [r0, #0x18]\n"
		"str r7, [r0, #0x1C]\n"
		"str r8, [r0, #0x20]\n"
		"str r9, [r0, #0x24]\n"
		"str r10, [r0, #0x28]\n"
		"str r11, [r0, #0x2C]\n"
		"str r12, [r0, #0x30]\n"
		/* presently in SVC mode */
		"str r13, [r0, #0x34]\n"	/* SP_SVC */
		"str r14, [r0, #0x38]\n"	/* LR_SVC */
		/* PC */
		"sub r1, r15, #0x4\n"	/* PC     */
		"str r1, [r0, #0x3C]\n"
		"mrs r1, spsr\n"	/* SPSR_SVC */
		"str r1, [r0, #0x40]\n"
		/* CPSR */
		"mrs r1, cpsr\n"		/* CPSR */
		"str r1, [r0, #0x44]\n"
		/* SYS/USR (0x1F) */
		"mrs r1, cpsr\n"		/* switch to SYS mode to save USER mode regs */
		"and r1, r1, #0xFFFFFFE0\n"	/* Mode bits is of 5 bits, so mask it */
		"orr r1, r1, #0x1F\n"
		"msr cpsr, r1\n"
		"str r13, [r0, #0x48]\n"	/* SP_USR */
		"str r14, [r0, #0x4C]\n"	/* LR_USR */
		/* FIQ (0x11) */
		"mrs r1, cpsr\n"		/* switch to FIQ mode */
		"and r1, r1, #0xFFFFFFE0\n"
		"orr r1, r1, #0x11\n"
		"msr cpsr, r1\n"
		"str r8, [r0, #0x50]\n"	/* R8_FIQ   */
		"str r9, [r0, #0x54]\n"	/* R9_FIQ   */
		"str r10, [r0, #0x58]\n"	/* R10_FIQ  */
		"str r11, [r0, #0x5C]\n"	/* R11_FIQ  */
		"str r12, [r0, #0x60]\n"	/* R12_FIQ  */
		"str r13, [r0, #0x64]\n"	/* SP_FIQ   */
		"str r14, [r0, #0x68]\n"	/* LR_FIQ   */
		"mrs r1, spsr\n"		/* SPSR_FIQ */
		"str r1, [r0, #0x6C]\n"
		/* IRQ (0x12) */
		"mrs r1, cpsr\n"		/* switch to IRQ mode */
		"and r1, r1, #0xFFFFFFE0\n"
		"orr r1, r1, #0x12\n"
		"msr cpsr, r1\n"
		"str r13, [r0, #0x70]\n"	/* SP_IRQ   */
		"str r14, [r0, #0x74]\n"	/* LR_IRQ   */
		"mrs r1, spsr\n"		/* SPSR_IRQ */
		"str r1, [r0, #0x78]\n"
		/* ABT mode (0x17) */
		"mrs r1, cpsr\n"		/* switch to Abort mode */
		"and r1, r1, #0xFFFFFFE0\n"
		"orr r1, r1, #0x17\n"
		"msr cpsr, r1\n"
		"str r13, [r0, #0x7C]\n"	/* SP_ABT   */
		"str r14, [r0, #0x80]\n"	/* LR_ABT   */
		"mrs r1, spsr\n"		/* SPSR_ABT */
		"str r1, [r0, #0x84]\n"
		/* UND mode (0x1B) */
		"mrs r1, cpsr\n"		/* switch to undef mode */
		"and r1, r1, #0xFFFFFFE0\n"
		"orr r1, r1, #0x1B\n"
		"msr cpsr, r1\n"
		"str r13, [r0, #0x88]\n"	/* SP_UND   */
		"str r14, [r0, #0x8C]\n"	/* LR_UND   */
		"mrs r1, spsr\n"		/* SPSR_UND */
		"str r1, [r0, #0x90]\n"
		/* Change to previous SVC mode (0x13) */
		"mrs r1, cpsr\n"		/* switch to SVC mode */
		"and r1, r1, #0xFFFFFFE0\n"
		"orr r1, r1, #0x13\n"
		"msr cpsr, r1\n"
		:	/* output */
		: "r"(cpu_context_regs)	/* input */
		: "%r0", "%r1"			/* clobbered registers */
	   );

}

/****************************************************************************
 * Name: up_registerdump
 ****************************************************************************/

#ifdef CONFIG_ARCH_STACKDUMP

static inline void up_registerdump(void)
{
	int regs;
	uint32_t *ptr;
	/* Are user registers available from interrupt processing? */
	if (!current_regs) {
		lldbg("current_regs is Null, up_assert(panic) was called directly.\n");

		/* Save the complete CPU context registers */
		save_cpuctxt_regs(&cpu_ctxt_regs);

		/* Save to local pointer */
		ptr = (uint32_t *)&cpu_ctxt_regs.r0;

		/* Set exception mode caused due to up_assert */
		g_upassert = true;

		/* Printing the other register information */
		lldbg("SPSR_SVC: %08x\t CPSR : %08x\t\n", cpu_ctxt_regs.spsr_svc, cpu_ctxt_regs.cpsr);
		lldbg("SP_IRQ: %08x\t LR_IRQ: %08x\t SPSR_IRQ : %08x\t\n", cpu_ctxt_regs.sp_irq, cpu_ctxt_regs.lr_irq, cpu_ctxt_regs.spsr_irq);
		lldbg("SP_ABT: %08x\t LR_ABT: %08x\t SPSR_ABT : %08x\t\n", cpu_ctxt_regs.sp_abt, cpu_ctxt_regs.lr_abt, cpu_ctxt_regs.spsr_abt);
		lldbg("SP_UND: %08x\t LR_UND: %08x\t SPSR_UND : %08x\t\n", cpu_ctxt_regs.sp_und, cpu_ctxt_regs.lr_und, cpu_ctxt_regs.spsr_und);
	} else {
		/* current_regs was saved by processor for exception */
		ptr = (uint32_t *)&current_regs[0];
	}

	/* Yes.. dump the interrupt registers */
	for (regs = REG_R0; regs <= REG_R15; regs += 8) {
		lldbg("R%d: %08x %08x %08x %08x %08x %08x %08x %08x\n", regs, ptr[regs], ptr[regs + 1], ptr[regs + 2], ptr[regs + 3], ptr[regs + 4], ptr[regs + 5], ptr[regs + 6], ptr[regs + 7]);
	}

	lldbg("CPSR: %08x\n", ptr[REG_CPSR]);
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
	struct tcb_s *rtcb = this_task();
	uint32_t sp = up_getsp();
	uint32_t ustackbase;
	uint32_t ustacksize;
#ifdef CONFIG_MPU_STACKGUARD
	uint32_t uguardsize = 0;
#endif
#if CONFIG_ARCH_INTERRUPTSTACK > 3
	uint32_t istackbase;
	uint32_t istacksize;
#endif
#if defined(CONFIG_ARCH_DABORTSTACK) && CONFIG_ARCH_DABORTSTACK >= 512
	uint32_t dabtstackbase;
	uint32_t dabtstacksize;
#endif
#ifdef CONFIG_ARCH_KERNEL_STACK
	uint32_t kstackbase = 0;
#endif

	/* Get the limits on the user stack memory */

	if (rtcb->pid == 0) {
		ustackbase = g_idle_topstack - 4;
		ustacksize = CONFIG_IDLETHREAD_STACKSIZE;
	} else {
		ustackbase = (uint32_t)rtcb->adj_stack_ptr;
		ustacksize = (uint32_t)rtcb->adj_stack_size;
#ifdef CONFIG_MPU_STACKGUARD
		uguardsize = (uint32_t)rtcb->guard_size;
#endif
	}

	lldbg("Current sp: %08x\n", sp);

#if CONFIG_ARCH_INTERRUPTSTACK > 3
	/* Get the limits on the interrupt stack memory */

	istackbase = (uint32_t)&g_intstackbase;
	istacksize = (CONFIG_ARCH_INTERRUPTSTACK & ~3);

	/* Show interrupt stack info */

	lldbg("Interrupt stack:\n");
	lldbg("  base: %08x\n", istackbase);
	lldbg("  size: %08x\n", istacksize);
#ifdef CONFIG_STACK_COLORATION
	lldbg("  used: %08x\n", up_check_intstack());
#endif
#endif

#if defined(CONFIG_ARCH_DABORTSTACK) && CONFIG_ARCH_DABORTSTACK >= 512
	/* Get the limits on the dabort stack memory */

	dabtstackbase = (uint32_t)&g_dabtstackbase;
	dabtstacksize = (CONFIG_ARCH_DABORTSTACK & ~3);

	/* Show data abort stack info */

	lldbg("Data abort stack:\n");
	lldbg("  base: %08x\n", dabtstackbase);
	lldbg("  size: %08x\n", dabtstacksize);
#endif

	/* Show user stack info */

	lldbg("User stack:\n");
	lldbg("  base: %08x\n", ustackbase);
	lldbg("  size: %08x\n", ustacksize);
#ifdef CONFIG_STACK_COLORATION
	lldbg("  used: %08x\n", up_check_tcbstack(rtcb));
#endif

#ifdef CONFIG_ARCH_KERNEL_STACK
	/* This this thread have a kernel stack allocated? */

	if (rtcb->xcp.kstack) {
		kstackbase = (uint32_t)rtcb->xcp.kstack + CONFIG_ARCH_KERNEL_STACKSIZE - 4;

		lldbg("Kernel stack:\n");
		lldbg("  base: %08x\n", kstackbase);
		lldbg("  size: %08x\n", CONFIG_ARCH_KERNEL_STACKSIZE);
	}
#endif

#if CONFIG_ARCH_INTERRUPTSTACK > 3
	/* Does the current stack pointer lie within the interrupt stack? */

	if (sp > istackbase - istacksize && sp < istackbase) {
		/* Yes.. dump the interrupt stack */

		lldbg("Interrupt Stack\n", sp);
		up_stackdump(sp, istackbase);

		/* Extract the user stack pointer which should lie
		 * at the base of the interrupt stack.
		 */

		sp = g_intstackbase;
		lldbg("User sp: %08x\n", sp);
	}
#endif

#if defined(CONFIG_ARCH_DABORTSTACK) && CONFIG_ARCH_DABORTSTACK >= 512
	/* Does the current stack pointer lie within the databort stack? */

	if (sp > dabtstackbase - dabtstacksize && sp < dabtstackbase) {
		/* Yes.. dump the data abort stack */

		lldbg("Dataabort Stack\n", sp);
		up_stackdump(sp, dabtstackbase);

		/* Extract the user stack pointer which should lie
		 * at the base of the data abort stack.
		 */

		sp = g_dabtstackbase;
		lldbg("User sp: %08x\n", sp);
	}
#endif

	/* Dump the user stack if the stack pointer lies within the allocated user
	 * stack memory including guard size if there any.
	 */
#ifdef CONFIG_MPU_STACKGUARD
	if (sp > (ustackbase - ustacksize - uguardsize) && sp < ustackbase) {
#else
	if (sp > ustackbase - ustacksize && sp < ustackbase) {
#endif
		lldbg("User Stack\n", sp);
		up_stackdump(sp, ustackbase);
	}

#ifdef CONFIG_ARCH_KERNEL_STACK
	/* Dump the user stack if the stack pointer lies within the allocated
	 * kernel stack memory.
	 */

	if (sp >= (uint32_t)rtcb->xcp.kstack && sp < kstackbase) {
		lldbg("Kernel Stack\n", sp);
		up_stackdump(sp, kstackbase);
	}
#endif

	/* Then dump the registers (if available) */

	up_registerdump();

#ifdef CONFIG_FRAME_POINTER
	/* Dump the stack */
	lldbg("*******************************************\n");
	lldbg("Call stack of aborted task:\n");
	lldbg("*******************************************\n");
	dump_stack();
#endif

	/* Dump the state of all tasks (if available) */

	up_showtasks();

#ifdef CONFIG_FRAME_POINTER
	/* Display the call stack of all tasks */
	dump_all_stack();
#endif

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
	/* Are we in an interrupt handler or the idle task? */

	if (g_upassert || current_regs || (this_task())->pid == 0) {
		(void)irqsave();
		for (;;) {
#ifdef CONFIG_ARCH_LEDS
			board_autoled_on(LED_PANIC);
			up_mdelay(250);
			board_autoled_off(LED_PANIC);
			up_mdelay(250);
#endif
		}
	} else {
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
	board_autoled_on(LED_ASSERTION);

#if CONFIG_TASK_NAME_SIZE > 0
	lldbg("Assertion failed at file:%s line: %d task: %s\n", filename, lineno, this_task()->name);
#else
	lldbg("Assertion failed at file:%s line: %d\n", filename, lineno);
#endif
	up_dumpstate();

#ifdef CONFIG_BOARD_CRASHDUMP
	board_crashdump(up_getsp(), this_task(), (uint8_t *)filename, lineno);
#endif

#ifdef CONFIG_BOARD_ASSERT_AUTORESET
	(void)boardctl(BOARDIOC_RESET, 0);
#endif

	_up_assert(EXIT_FAILURE);
}
