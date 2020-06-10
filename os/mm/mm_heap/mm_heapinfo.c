/****************************************************************************
 *
 * Copyright 2016-2017 Samsung Electronics All Rights Reserved.
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
 * mm/mm_heap/mm_heapinfo.c
 *
 *   Copyright (C) 2007, 2009, 2013-2014 Gregory Nutt. All rights reserved.
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
#include <tinyara/sched.h>
#include <tinyara/mm/mm.h>
#include <tinyara/arch.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>
#ifdef CONFIG_HEAPINFO_USER_GROUP
#include <string.h>
#include <tinyara/mm/heapinfo_internal.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define HEAPINFO_INT INT16_MAX
#define HEAPINFO_NONSCHED (INT16_MAX - 1)

#ifdef CONFIG_HEAPINFO_USER_GROUP
int max_group;
struct heapinfo_group_s heapinfo_group[HEAPINFO_USER_GROUP_NUM];
struct heapinfo_group_info_s group_info[HEAPINFO_THREAD_NUM];
#endif

#if CONFIG_MM_NHEAPS > 1
heapinfo_total_info_t total_info;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_MM_HEAPINFO
/****************************************************************************
 * Name: heapinfo_parse
 *
 * Description:
 *   This function walk through heap and displays alloc info.
 ****************************************************************************/
void heapinfo_parse(FAR struct mm_heap_s *heap, int mode, pid_t pid)
{
	struct mm_allocnode_s *node;
	size_t mxordblk = 0;
	int    ordblks  = 0;		/* Number of non-inuse chunks */
	size_t fordblks = 0;		/* Total non-inuse space */
	size_t heap_resource;
	size_t stack_resource;
	size_t nonsched_resource;
	int nonsched_idx;
	struct sched_param sched_data;
	size_t heap_size;

	/* This nonsched can be 3 types : group resources, freed when child task finished, leak */
	pid_t nonsched_list[CONFIG_MAX_TASKS];
	size_t nonsched_size[CONFIG_MAX_TASKS];

#if CONFIG_MM_REGIONS > 1
	int region;
#else
#define region 0
#endif

#ifdef CONFIG_DEBUG_CHECK_FRAGMENTATION
	int ndx;
	int nodelist_cnt[MM_NNODES] = {0, };
	size_t nodelist_size[MM_NNODES] = {0, };
	FAR struct mm_freenode_s *fnode;
#endif

	/* initialize the heap, stack and nonsched resource */
	nonsched_resource = 0;
	heap_resource = 0;
	stack_resource = 0;
	for (nonsched_idx = 0; nonsched_idx < CONFIG_MAX_TASKS; nonsched_idx++) {
		nonsched_list[nonsched_idx] = HEAPINFO_NONSCHED;
		nonsched_size[nonsched_idx] = 0;
	}

	/* Visit each region */

#if CONFIG_MM_REGIONS > 1
	for (region = 0; region < heap->mm_nregions; region++)
#endif
	{
		/* Visit each node in the region
		 * Retake the semaphore for each region to reduce latencies
		 */
		mm_takesemaphore(heap);
		sched_lock();

		if (mode != HEAPINFO_SIMPLE) {
			printf("****************************************************************\n");
			printf("REGION #%d Start=0x%p, End=0x%p, Size=%d\n",
				region,
				heap->mm_heapstart[region],
				heap->mm_heapend[region],
				(int)heap->mm_heapend[region] - (int)heap->mm_heapstart[region] + SIZEOF_MM_ALLOCNODE);
			printf("****************************************************************\n");
			printf("  MemAddr |   Size   | Status |    Owner   |  Pid  |\n");
			printf("----------|----------|--------|------------|-------|\n");
		}

		for (node = heap->mm_heapstart[region]; node < heap->mm_heapend[region]; node = (struct mm_allocnode_s *)((char *)node + node->size)) {

			/* Check if the node corresponds to an allocated memory chunk */
			if ((pid == HEAPINFO_PID_ALL || node->pid == pid) && (node->preceding & MM_ALLOC_BIT) != 0) {
				if (mode == HEAPINFO_DETAIL_ALL || mode == HEAPINFO_DETAIL_PID || mode == HEAPINFO_DETAIL_SPECIFIC_HEAP) {
					if (node->pid >= 0) {
						printf("0x%x | %8u |   %c    | 0x%8x | %3d   |\n", node, node->size, 'A', node->alloc_call_addr, node->pid);
					} else {
						printf("0x%x | %8u |   %c    | 0x%8x | %3d(S)|\n", node, node->size, 'A', node->alloc_call_addr, -(node->pid));
					}
				}

#if CONFIG_TASK_NAME_SIZE > 0
				if (node->pid == HEAPINFO_INT && mode != HEAPINFO_SIMPLE) {
					printf("INT Context\n");
				} else if (node->pid < 0 && sched_getparam((-1) * (node->pid), &sched_data) != ERROR) {
					stack_resource += node->size;
				} else if (sched_getparam(node->pid, &sched_data) == ERROR) {
					nonsched_list[PIDHASH(node->pid)] = node->pid;
					nonsched_size[PIDHASH(node->pid)] += node->size;
					nonsched_resource += node->size;
				} else {
					heap_resource += node->size;
				}
#else
				if (mode != HEAPINFO_SIMPLE) {
					printf("\n");
				}
#endif
			} else {
				ordblks++;
				fordblks += node->size;
				if (node->size > mxordblk) {
					mxordblk = node->size;
				}
				if (mode == HEAPINFO_DETAIL_ALL || mode == HEAPINFO_DETAIL_FREE || mode == HEAPINFO_DETAIL_SPECIFIC_HEAP) {
					printf("0x%x | %8d |   %c    |            |       |\n", node, node->size, 'F');
				}
			}
		}

		if (mode != HEAPINFO_SIMPLE) {
			printf("** PID(S) in Pid colum means that mem is used for stack of PID\n\n");
		}
		sched_unlock();
		mm_givesemaphore(heap);
	}
#undef region

#if CONFIG_MM_NHEAPS > 1
	total_info.total_heap_size += heap->mm_heapsize;
	total_info.cur_free += fordblks;
	if (total_info.largest_free_size < mxordblk) {
		total_info.largest_free_size = mxordblk;
	}
	total_info.cur_dead_thread += nonsched_resource;
	total_info.sum_of_stacks += stack_resource;
	total_info.sum_of_heaps += heap_resource - (heap->mm_nregions * SIZEOF_MM_ALLOCNODE);

	if (mode == HEAPINFO_SIMPLE) {
		return;
	}
#endif
	printf("\n****************************************************************\n");
	printf("     Summary of Heap Usages (Size in Bytes)\n");
	printf("****************************************************************\n");
	heap_size = heap->mm_heapsize;

	printf("Total                           : %u (100%%)\n", heap_size);
	printf("  - Allocated (Current / Peak)  : %u (%d%%) / %u (%d%%)\n",\
		heap->total_alloc_size, (size_t)((uint64_t)(heap->total_alloc_size) * 100 / heap_size),\
		heap->peak_alloc_size,  (size_t)((uint64_t)(heap->peak_alloc_size) * 100 / heap_size));
	printf("  - Free (Current)              : %u (%d%%)\n", fordblks, (size_t)((uint64_t)fordblks * 100 / heap_size));
	printf("  - Reserved                    : %u\n", SIZEOF_MM_ALLOCNODE * 2);

	printf("\n****************************************************************\n");
	printf("     Details of Heap Usages (Size in Bytes)\n");
	printf("****************************************************************\n");
	printf("< Free >\n");
	printf("  - Number of Free Node               : %d\n", ordblks);
	printf("  - Largest Free Node Size            : %u\n", mxordblk);
	printf("\n< Allocation >\n");
	printf("  - Current Size (Alive Allocation) = (1) + (2) + (3)\n");
	printf("     . by Dead Threads (*) (1)        : %u\n", nonsched_resource);
	printf("     . by Alive Threads\n");
	printf("        - Sum of \"STACK\"(**) (2)      : %u\n", stack_resource);
	printf("        - Sum of \"CURR_HEAP\" (3)      : %u\n", heap_resource - SIZEOF_MM_ALLOCNODE);	// Because of above for loop (node < heap->mm_heapend[region];),
													// one of SIZEOF_MM_ALLOCNODE is subtracted.
	printf("** NOTE **\n");
	printf("(*)  Alive allocation by dead threads might be used by others or might be a leakage.\n");
	printf("(**) Only Idle task has a separate stack region,\n");
	printf("  rest are all allocated on the heap region.\n");

#ifdef CONFIG_DEBUG_CHECK_FRAGMENTATION
	printf("\nAvailable fragmented memory segments in heap memory\n");

	mm_takesemaphore(heap);
	sched_lock();

	for (ndx = 0; ndx < MM_NNODES; ++ndx) {
		for (fnode = heap->mm_nodelist[ndx].flink; fnode && fnode->size; fnode = fnode->flink) {
			++nodelist_cnt[ndx];
			nodelist_size[ndx] += fnode->size;
		}
	}

	sched_unlock();
	mm_givesemaphore(heap);

	for (ndx = 0; ndx < MM_NNODES; ++ndx) {
		printf("Nodelist[%d] ranging [%u, %u] : num %d, size %u [Bytes]\n", ndx, ((ndx > 0 ? (1 << (ndx + MM_MIN_SHIFT)) : 0) + 1), 1 << (ndx + MM_MIN_SHIFT + 1), nodelist_cnt[ndx], nodelist_size[ndx]);
	}
#endif

	if (mode != HEAPINFO_SIMPLE) {
		printf("\n< by Dead Threads >\n");
		printf(" Pid | Size \n");
		printf("-----|------\n");
		for (nonsched_idx = 0; nonsched_idx < CONFIG_MAX_TASKS; nonsched_idx++) {
			if (nonsched_list[nonsched_idx] != HEAPINFO_NONSCHED) {
				printf("%4d | %5u\n", nonsched_list[nonsched_idx], nonsched_size[nonsched_idx]);
			}
		}
	}

	return;
}
/****************************************************************************
 * Name: heapinfo_update_group
 *
 * Description:
 * Update Peak heap size for Group
 ****************************************************************************/
#ifdef CONFIG_HEAPINFO_USER_GROUP
static void heapinfo_update_group(mmsize_t size, pid_t pid)
{
	int check_idx;
	int group_num;
	int stack_pid;
	for (check_idx = 0; check_idx < HEAPINFO_THREAD_NUM; check_idx++) {
		if (pid == group_info[check_idx].pid) {
			group_num = group_info[check_idx].group;
			heapinfo_group[group_num].curr_size += size;
			/* Update peak size */
			if (heapinfo_group[group_num].curr_size > heapinfo_group[group_num].peak_size) {
				heapinfo_group[group_num].peak_size = heapinfo_group[group_num].curr_size;
				/* calculate the summation of stacks */
				heapinfo_group[group_num].stack_size = 0;
				for (stack_pid = 0; stack_pid < HEAPINFO_THREAD_NUM; stack_pid++) {
					if (group_info[stack_pid].pid != -1 && group_info[stack_pid].group == group_num) {
						heapinfo_group[group_num].stack_size += group_info[stack_pid].stack_size;
					}
				}
				heapinfo_group[group_num].heap_size = heapinfo_group[group_num].peak_size - heapinfo_group[group_num].stack_size;
			}
			break;
		}
	}
}
#endif
/****************************************************************************
 * Name: heapinfo_add_size
 *
 * Description:
 * Add the allocated size in tcb
 ****************************************************************************/
void heapinfo_add_size(struct mm_heap_s *heap, pid_t pid, mmsize_t size)
{
	pid_t hash_pid;

	hash_pid = PIDHASH(pid);
	if (heap->alloc_list[hash_pid].pid == HEAPINFO_INIT_INFO || heap->alloc_list[hash_pid].pid == pid) {
			heap->alloc_list[hash_pid].pid = pid;
			heap->alloc_list[hash_pid].curr_alloc_size += size;
			if (heap->alloc_list[hash_pid].curr_alloc_size > heap->alloc_list[hash_pid].peak_alloc_size) {
				heap->alloc_list[hash_pid].peak_alloc_size = heap->alloc_list[hash_pid].curr_alloc_size;
			}
			heap->alloc_list[hash_pid].num_alloc_free++;
	}
}

/****************************************************************************
 * Name: heapinfo_subtract_size
 *
 * Description:
 * Subtract the allocated size in tcb
 ****************************************************************************/
void heapinfo_subtract_size(struct mm_heap_s *heap, pid_t pid, mmsize_t size)
{
	pid_t hash_pid;

	hash_pid = PIDHASH(pid);
	if (heap->alloc_list[hash_pid].pid == pid) {
			heap->alloc_list[hash_pid].curr_alloc_size -= size;
			heap->alloc_list[hash_pid].num_alloc_free--;
	}
}

/****************************************************************************
 * Name: heapinfo_update_total_size
 *
 * Description:
 * Calculate the total allocated size and update the peak allocated size for heap
 ****************************************************************************/
void heapinfo_update_total_size(struct mm_heap_s *heap, mmsize_t size, pid_t pid)
{
	heap->total_alloc_size += size;
	if (heap->total_alloc_size > heap->peak_alloc_size) {
		heap->peak_alloc_size = heap->total_alloc_size;
	}
#if CONFIG_MM_NHEAPS > 1
	total_info.cur_alloc_size += size;
	if (total_info.cur_alloc_size > total_info.peak_alloc_size) {
		total_info.peak_alloc_size = total_info.cur_alloc_size;
	}
#endif
#ifdef CONFIG_HEAPINFO_USER_GROUP
	heapinfo_update_group(size, pid);
#endif
}
/****************************************************************************
 * Name: heapinfo_update_node
 *
 * Description:
 * Adds pid and malloc caller return address to mem chunk
 ****************************************************************************/
void heapinfo_update_node(FAR struct mm_allocnode_s *node, mmaddress_t caller_retaddr)
{
	node->alloc_call_addr = caller_retaddr;
	node->reserved = 0;
	node->pid = getpid();
	return;
}

/****************************************************************************
 * Name: heapinfo_exclude_stacksize
 *
 * Description:
 * when create a stack, subtract the stacksize from parent
 ****************************************************************************/
void heapinfo_exclude_stacksize(void *stack_ptr)
{
	struct mm_allocnode_s *node;
	pid_t hash_pid;
	struct mm_heap_s *heap = mm_get_heap(stack_ptr);
	if (heap == NULL) {
		return;
	}
	node = (struct mm_allocnode_s *)(stack_ptr - SIZEOF_MM_ALLOCNODE);
	hash_pid = PIDHASH(node->pid);
	heap->alloc_list[hash_pid].curr_alloc_size -= node->size;
#ifdef CONFIG_HEAPINFO_USER_GROUP
	int check_idx;
	int group_num;

	for (check_idx = 0; check_idx <= max_group; check_idx++) {
		if (node->pid == group_info[check_idx].pid) {
			group_num = group_info[check_idx].group;
			heapinfo_group[group_num].curr_size -= node->size;
			break;
		}
	}
#endif
}

#ifdef CONFIG_HEAPINFO_USER_GROUP
/****************************************************************************
 * Name: heapinfo_update_group_info
 *
 * Description:
 * when create or release task/thread, check that the task/thread is 
 * in group list
 ****************************************************************************/
void heapinfo_update_group_info(pid_t pid, int group, int type)
{
	int info_idx;
	struct tcb_s *tcb;
	switch (type) {
	case HEAPINFO_INIT_INFO:
		for (info_idx = 0; info_idx < HEAPINFO_THREAD_NUM; info_idx++) {
			group_info[info_idx].pid = pid;
			group_info[info_idx].group = group;
			group_info[info_idx].stack_size = 0;
		}
		break;
	case HEAPINFO_ADD_INFO:
		if (group > max_group) {
			max_group = group;
		}
		for (info_idx = 0; info_idx < HEAPINFO_THREAD_NUM; info_idx++) {
			if (group_info[info_idx].pid <= 0) {
				group_info[info_idx].pid = pid;
				group_info[info_idx].group = group;
				tcb = sched_gettcb(pid);
				if (tcb) {
					group_info[info_idx].stack_size = tcb->adj_stack_size;
					heapinfo_update_group(tcb->adj_stack_size, pid);
				}
				break;
			}
		}
		break;
	case HEAPINFO_DEL_INFO:
		for (info_idx = 0; info_idx < HEAPINFO_THREAD_NUM; info_idx++) {
			if (pid == group_info[info_idx].pid) {
				heapinfo_update_group((-1) * group_info[info_idx].stack_size, pid);

				group_info[info_idx].pid = -1;
				group_info[info_idx].group = -1;
				group_info[info_idx].stack_size = 0;
				break;
			}
		}
		break;
	default:
		break;
	}
}

/****************************************************************************
 * Name: heapinfo_check_group_list
 *
 * Description:
 * check that task/thread is in group list
 ****************************************************************************/
void heapinfo_check_group_list(pid_t pid, char *name)
{
	char *thread_list = CONFIG_HEAPINFO_USER_GROUP_LIST;

	int group_num = 0;

	char *name_start;
	char *name_end;
	int name_length;
	name_start = name_end = thread_list;

	while (*name_start != '\0') {
		if (*name_end == '/' || *name_end == ',' || *name_end == '\0') {
			name_length = name_end - name_start;
			if (name_length == strlen(name)) {
				if (strncmp(name_start, name, name_end - name_start) == 0) {
					heapinfo_update_group_info(pid, group_num, HEAPINFO_ADD_INFO);
					break;
				}
			}
			if (*name_end == '/') {
				group_num++;
			} else if (*name_end == '\0') {
				name_start = name_end;
				continue;
			}
			name_end++;
			name_start = name_end;
		} else {
			name_end++;
		}
	}
}
#endif /* CONFIG_HEAPINFO_USER_GROUP */

/****************************************************************************
 * Name: heapinfo_peak_init
 *
 * Description:
 * initialize the peak allocation size in heap
 ****************************************************************************/
void heapinfo_peak_init(struct mm_heap_s *heap)
{
	int tcb_idx;
	int heap_idx;

	for (heap_idx = 0; heap_idx < CONFIG_MM_NHEAPS; heap_idx++) {
		for (tcb_idx = 0; tcb_idx < CONFIG_MAX_TASKS; tcb_idx++) {
			heap[heap_idx].alloc_list[tcb_idx].peak_alloc_size = 0;
		}
	}
}
#endif
