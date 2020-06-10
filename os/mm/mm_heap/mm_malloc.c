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
 * mm/mm_heap/mm_malloc.c
 *
 *   Copyright (C) 2007, 2009, 2013-2014  Gregory Nutt. All rights reserved.
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

#include <debug.h>
#include <sched.h>

#include <tinyara/mm/mm.h>

#ifdef CONFIG_DEBUG_MM_HEAPINFO
#include  <tinyara/sched.h>
#endif
#include "mm_node.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef NULL
#define NULL ((void*)0)
#endif

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_malloc
 *
 * Description:
 *  Find the smallest chunk that satisfies the request. Take the memory from
 *  that chunk, save the remaining, smaller chunk (if any).
 *
 *  8-byte alignment of the allocated data is assured.
 *
 ****************************************************************************/
#ifdef CONFIG_DEBUG_MM_HEAPINFO
FAR void *mm_malloc(FAR struct mm_heap_s *heap, size_t size, mmaddress_t caller_retaddr)
#else
FAR void *mm_malloc(FAR struct mm_heap_s *heap, size_t size)
#endif
{
	FAR struct mm_freenode_s *node;
	void *ret = NULL;
	int ndx;

	/* Handle bad sizes */

	if (size < 1) {
		return NULL;
	}

	if (size > MM_ALIGN_DOWN(MMSIZE_MAX) - SIZEOF_MM_ALLOCNODE) {
		mdbg("Because of mm_allocnode, %u cannot be allocated. The maximum \
			 allocable size is (MM_ALIGN_DOWN(MMSIZE_MAX) - SIZEOF_MM_ALLOCNODE) \
			 : %u\n.", size, (MM_ALIGN_DOWN(MMSIZE_MAX) - SIZEOF_MM_ALLOCNODE));
		return NULL;
	}

	/* Adjust the size to account for (1) the size of the allocated node and
	 * (2) to make sure that it is an even multiple of our granule size.
	 */

	size = MM_ALIGN_UP(size + SIZEOF_MM_ALLOCNODE);

	/* We need to hold the MM semaphore while we muck with the nodelist. */

	mm_takesemaphore(heap);
	sched_lock();

	/* Get the location in the node list to start the search
	 * by converting the request size into a nodelist index.
	 */

	ndx = mm_size2ndx(size);

	/* Search for a large enough chunk in the list of nodes.
	 * This list is ordered by size in a descending order.
	 * If this list does not have free nodes whose size is large enough
	 * to accomodate the requested size, malloc() will fail due to no more space.
	 */

	node = heap->mm_nodelist[ndx].flink;
	if (!(node && node->size >= size)) {
		while (++ndx < MM_NNODES && !(node = heap->mm_nodelist[ndx].flink)) ;
	}

	/* If node is not NULL, there exists a free node big enough to allocate. */
	FAR struct mm_freenode_s *prev = &heap->mm_nodelist[ndx];
	for ( ; node && node->size > size; prev = node, node = node->flink) ;
	if (!(node && node->size == size)) {
		node = prev;
	}

	/* If we found a node with non-zero size, then this is one to use. Since
	 * the list is ordered, we know that is must be best fitting chunk
	 * available.
	 */

	if (node->size) {
		FAR struct mm_freenode_s *remainder;
		FAR struct mm_freenode_s *next;
		size_t remaining;

		/* Remove the node.  There must be a predecessor, but there may not be
		 * a successor node.
		 */

		REMOVE_NODE_FROM_LIST(node);

		/* Check if we have to split the free node into one of the allocated
		 * size and another smaller freenode.  In some cases, the remaining
		 * bytes can be smaller (they may be SIZEOF_MM_ALLOCNODE).  In that
		 * case, we will just carry the few wasted bytes at the end of the
		 * allocation.
		 */

		remaining = node->size - size;
		if (remaining >= SIZEOF_MM_FREENODE) {
			/* Get a pointer to the next node in physical memory */

			next = (FAR struct mm_freenode_s *)(((char *)node) + node->size);

			/* Create the remainder node */

			remainder = (FAR struct mm_freenode_s *)(((char *)node) + size);
			remainder->size = remaining;
			remainder->preceding = size;

			/* Adjust the size of the node under consideration */

			node->size = size;

			/* Adjust the 'preceding' size of the (old) next node, preserving
			 * the allocated flag.
			 */

			next->preceding = remaining | (next->preceding & MM_ALLOC_BIT);

			/* Add the remainder back into the nodelist */

			mm_addfreechunk(heap, remainder);
		}

		/* Handle the case of an exact size match */

		node->preceding |= MM_ALLOC_BIT;

#ifdef CONFIG_DEBUG_MM_HEAPINFO
		heapinfo_update_node((struct mm_allocnode_s *)node, caller_retaddr);
		heapinfo_add_size(heap, ((struct mm_allocnode_s *)node)->pid, node->size);
		heapinfo_update_total_size(heap, node->size, ((struct mm_allocnode_s *)node)->pid);
#endif
		ret = (void *)((char *)node + SIZEOF_MM_ALLOCNODE);
	}

	sched_unlock();
	mm_givesemaphore(heap);

	/* If CONFIG_DEBUG_MM is defined, then output the result of the allocation
	 * to the SYSLOG.
	 */

#ifdef CONFIG_DEBUG_MM
	if (!ret) {
		mdbg("Allocation failed, size %u\n", size);
#ifdef CONFIG_DEBUG_MM_HEAPINFO
		heapinfo_parse(heap, HEAPINFO_DETAIL_ALL, HEAPINFO_PID_ALL);
#endif
	} else {
		mvdbg("Allocated %p, size %u\n", ret, size);
	}
#endif

	return ret;
}
