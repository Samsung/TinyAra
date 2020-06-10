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
 * mm/mm_heap/mm_memalign.c
 *
 *   Copyright (C) 2007, 2009, 2011, 2013-2014 Gregory Nutt. All rights reserved.
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

#include <assert.h>
#include <sched.h>

#include <tinyara/mm/mm.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_memalign
 *
 * Description:
 *   memalign requests more than enough space from malloc, finds a region
 *   within that chunk that meets the alignment request and then frees any
 *   leading or trailing space.
 *
 *   The alignment argument must be a power of two (not checked).  8-byte
 *   alignment is guaranteed by normal malloc calls.
 *
 ****************************************************************************/
#ifdef CONFIG_DEBUG_MM_HEAPINFO
FAR void *mm_memalign(FAR struct mm_heap_s *heap, size_t alignment, size_t size, mmaddress_t caller_retaddr)
#else
FAR void *mm_memalign(FAR struct mm_heap_s *heap, size_t alignment, size_t size)
#endif
{
	FAR struct mm_allocnode_s *node;
	size_t rawchunk;
	size_t alignedchunk;
	size_t mask = (size_t)(alignment - 1);
	size_t allocsize;

	/* If this requested alinement's less than or equal to the natural alignment
	 * of malloc, then just let malloc do the work.
	 */

	if (alignment <= MM_MIN_CHUNK) {
#ifdef CONFIG_DEBUG_MM_HEAPINFO
		return mm_malloc(heap, size, caller_retaddr);
#else
		return mm_malloc(heap, size);
#endif
	}

	/* Adjust the size to account for (1) the size of the allocated node, (2)
	 * to make sure that it is an even multiple of our granule size, and to
	 * include the alignment amount.
	 *
	 * Notice that we increase the allocation size by twice the requested
	 * alignment.  We do this so that there will be at least two valid
	 * alignment points within the allocated memory.
	 *
	 * NOTE:  These are sizes given to malloc and not chunk sizes. They do
	 * not include SIZEOF_MM_ALLOCNODE.
	 */

	size = MM_ALIGN_UP(size);	/* Make multiples of our granule size */
	allocsize = size + 2 * alignment;	/* Add double full alignment size */

	/* Then malloc that size */
#ifdef CONFIG_DEBUG_MM_HEAPINFO
	/*Passing Zero as caller addr to avoid adding memalloc info in malloc function,
	   alloc info will be added in this functiona after memory aligment . */
	rawchunk = (size_t)mm_malloc(heap, allocsize, caller_retaddr);
#else
	rawchunk = (size_t)mm_malloc(heap, allocsize);
#endif
	if (rawchunk == 0) {
		return NULL;
	}

	/* We need to hold the MM semaphore while we muck with the chunks and
	 * nodelist.
	 */

	mm_takesemaphore(heap);
	sched_lock();

	/* Get the node associated with the allocation and the next node after
	 * the allocation.
	 */

	node = (FAR struct mm_allocnode_s *)(rawchunk - SIZEOF_MM_ALLOCNODE);

#ifdef CONFIG_DEBUG_MM_HEAPINFO
		heapinfo_subtract_size(heap, node->pid, node->size);
		heapinfo_update_total_size(heap, ((-1) * (node->size)), node->pid);
#endif
	/* Find the aligned subregion */

	alignedchunk = (rawchunk + mask) & ~mask;

	/* Check if there is free space at the beginning of the aligned chunk */

	if (alignedchunk != rawchunk) {
		FAR struct mm_allocnode_s *newnode;
		FAR struct mm_allocnode_s *next;
		size_t precedingsize;

		/* Get the node the next node after the allocation. */

		next = (FAR struct mm_allocnode_s *)((char *)node + node->size);

		/* Make sure that there is space to convert the preceding mm_allocnode_s
		 * into an mm_freenode_s.  I think that this should always be true
		 */

		DEBUGASSERT(alignedchunk >= rawchunk + 8);

		newnode = (FAR struct mm_allocnode_s *)(alignedchunk - SIZEOF_MM_ALLOCNODE);

		/* Preceding size is full size of the new 'node,' including
		 * SIZEOF_MM_ALLOCNODE
		 */

		precedingsize = (size_t)newnode - (size_t)node;

		/* If we were unlucky, then the alignedchunk can lie in such a position
		 * that precedingsize < SIZEOF_NODE_FREENODE.  We can't let that happen
		 * because we are going to cast 'node' to struct mm_freenode_s below.
		 * This is why we allocated memory large enough to support two
		 * alignment points.  In this case, we will simply use the second
		 * alignment point.
		 */

		if (precedingsize < SIZEOF_MM_FREENODE) {
			alignedchunk += alignment;
			newnode = (FAR struct mm_allocnode_s *)(alignedchunk - SIZEOF_MM_ALLOCNODE);
			precedingsize = (size_t)newnode - (size_t)node;
		}

		/* Set up the size of the new node */

		newnode->size = (size_t)next - (size_t)newnode;
		newnode->preceding = precedingsize | MM_ALLOC_BIT;

		/* Reduce the size of the original chunk and mark it not allocated, */

		node->size = precedingsize;
		node->preceding &= ~MM_ALLOC_BIT;

		/* Fix the preceding size of the next node */

		next->preceding = newnode->size | (next->preceding & MM_ALLOC_BIT);

		/* Convert the newnode chunk size back into malloc-compatible size by
		 * subtracting the header size SIZEOF_MM_ALLOCNODE.
		 */

		allocsize = newnode->size - SIZEOF_MM_ALLOCNODE;

		/* Add the original, newly freed node to the free nodelist */

		mm_addfreechunk(heap, (FAR struct mm_freenode_s *)node);

		/* Replace the original node with the newlay realloaced,
		 * aligned node
		 */
		node = newnode;
	}

	/* Check if there is free space at the end of the aligned chunk */

	if (allocsize > size) {
		/* Shrink the chunk by that much -- remember, mm_shrinkchunk wants
		 * internal chunk sizes that include SIZEOF_MM_ALLOCNODE, and not the
		 * malloc-compatible sizes that we have.
		 */
		mm_shrinkchunk(heap, node, size + SIZEOF_MM_ALLOCNODE);
	}
#ifdef CONFIG_DEBUG_MM_HEAPINFO
	heapinfo_update_node(node, caller_retaddr);

	heapinfo_add_size(heap, node->pid, node->size);
	heapinfo_update_total_size(heap, node->size, node->pid);
#endif
	sched_unlock();
	mm_givesemaphore(heap);
	return (FAR void *)alignedchunk;
}
