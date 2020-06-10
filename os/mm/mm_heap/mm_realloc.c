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
 * mm/mm_heap/mm_realloc.c
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef CONFIG_DEBUG_MM_HEAPINFO
#include <tinyara/sched.h>
#endif
#include <tinyara/mm/mm.h>
#include "mm_node.h"

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
 * Name: mm_realloc
 *
 * Description:
 *   If the reallocation is for less space, then:
 *
 *     (1) the current allocation is reduced in size
 *     (2) the remainder at the end of the allocation is returned to the
 *         free list.
 *
 *  If the request is for more space and the current allocation can be
 *  extended, it will be extended by:
 *
 *     (1) Taking the additional space from the following free chunk, or
 *     (2) Taking the additional space from the preceding free chunk.
 *     (3) Or both
 *
 *  If the request is for more space but the current chunk cannot be
 *  extended, then malloc a new buffer, copy the data into the new buffer,
 *  and free the old buffer.
 *
 ****************************************************************************/
#ifdef CONFIG_DEBUG_MM_HEAPINFO
FAR void *mm_realloc(FAR struct mm_heap_s *heap, FAR void *oldmem, size_t size, mmaddress_t caller_retaddr)
#else
FAR void *mm_realloc(FAR struct mm_heap_s *heap, FAR void *oldmem, size_t size)
#endif
{
	FAR struct mm_allocnode_s *oldnode;
#ifndef CONFIG_REALLOC_DISABLE_NEIGHBOR_EXTENSION
	FAR struct mm_freenode_s  *prev;
	FAR struct mm_freenode_s  *next;
#endif
	size_t newsize;
	size_t oldsize;
#ifndef CONFIG_REALLOC_DISABLE_NEIGHBOR_EXTENSION
	size_t prevsize = 0;
	size_t nextsize = 0;
#endif
	FAR void *newmem;

	/* If oldmem is NULL, then realloc is equivalent to malloc */

	if (!oldmem) {
#ifdef CONFIG_DEBUG_MM_HEAPINFO
		return mm_malloc(heap, size, caller_retaddr);
#else
		return mm_malloc(heap, size);
#endif
	}

	/* If size is zero, then realloc is equivalent to free */

	if (size < 1) {
		mm_free(heap, oldmem);
		return NULL;
	}

	/* Adjust the size to account for (1) the size of the allocated node and
	 * (2) to make sure that it is an even multiple of our granule size.
	 */

	newsize = MM_ALIGN_UP(size + SIZEOF_MM_ALLOCNODE);

	/* Map the memory chunk into an allocated node structure */

	oldnode = (FAR struct mm_allocnode_s *)((FAR char *)oldmem - SIZEOF_MM_ALLOCNODE);

	/* We need to hold the MM semaphore while we muck with the nodelist. */

	mm_takesemaphore(heap);
	sched_lock();

	/* Check if this is a request to reduce the size of the allocation. */

	oldsize = oldnode->size;

	if (newsize <= oldsize) {
		/* Handle the special case where we are not going to change the size
		 * of the allocation.
		 */

		if (newsize < oldsize) {
#ifdef CONFIG_DEBUG_MM_HEAPINFO
			/* modify the current allocated size of old node */
			heapinfo_subtract_size(heap, oldnode->pid, oldsize);
			heapinfo_update_total_size(heap, (-1) * oldsize, oldnode->pid);
#endif

			mm_shrinkchunk(heap, oldnode, newsize);
#ifdef CONFIG_DEBUG_MM_HEAPINFO
			/* update the chunk to realloc task information */
			heapinfo_update_node(oldnode, caller_retaddr);

			heapinfo_add_size(heap, oldnode->pid, oldnode->size);
			heapinfo_update_total_size(heap, oldnode->size, oldnode->pid);
#endif
		}

		/* Then return the original address */

		sched_unlock();
		mm_givesemaphore(heap);
		return oldmem;
	}

#ifndef CONFIG_REALLOC_DISABLE_NEIGHBOR_EXTENSION
	/* This is a request to increase the size of the allocation,  Get the
	 * available sizes before and after the oldnode so that we can make the
	 * best decision
	 */

	next = (FAR struct mm_freenode_s *)((FAR char *)oldnode + oldnode->size);
	if ((next->preceding & MM_ALLOC_BIT) == 0) {
		nextsize = next->size;
	}

	prev = (FAR struct mm_freenode_s *)((FAR char *)oldnode - (oldnode->preceding & ~MM_ALLOC_BIT));
	if ((prev->preceding & MM_ALLOC_BIT) == 0) {
		prevsize = prev->size;
	}

	/* Now, check if we can extend the current allocation or not */

	if (nextsize + prevsize + oldsize >= newsize) {
		size_t needed   = newsize - oldsize;
		size_t takeprev = 0;
		size_t takenext = 0;

#ifdef CONFIG_DEBUG_MM_HEAPINFO
		/* modify the current allocated size of old node */
		heapinfo_subtract_size(heap, oldnode->pid, oldsize);
		heapinfo_update_total_size(heap, (-1) * oldsize, oldnode->pid);
#endif

		/* Check if we can extend into the previous chunk and if the
		 * previous chunk is smaller than the next chunk.
		 */

		if (prevsize > 0 && (nextsize >= prevsize || nextsize < 1)) {
			/* Can we get everything we need from the previous chunk? */

			if (needed > prevsize) {
				/* No, take the whole previous chunk and get the
				 * rest that we need from the next chunk.
				 */

				takeprev = prevsize;
				takenext = needed - prevsize;
			} else {
				/* Yes, take what we need from the previous chunk */

				takeprev = needed;
				takenext = 0;
			}

			needed = 0;
		}

		/* Check if we can extend into the next chunk and if we still need
		 * more memory.
		 */

		if (nextsize > 0 && needed) {
			/* Can we get everything we need from the next chunk? */

			if (needed > nextsize) {
				/* No, take the whole next chunk and get the rest that we
				 * need from the previous chunk.
				 */

				takeprev = needed - nextsize;
				takenext = nextsize;
			} else {
				/* Yes, take what we need from the previous chunk */

				takeprev = 0;
				takenext = needed;
			}
		}

		/* Extend into the previous free chunk */

		newmem = oldmem;
		if (takeprev) {
			FAR struct mm_allocnode_s *newnode;

			/* Remove the previous node.  There must be a predecessor, but
			 * there may not be a successor node.
			 */

			REMOVE_NODE_FROM_LIST(prev);

			/* Extend the node into the previous free chunk */
			/* Did we consume the entire preceding chunk? */

			if ((prevsize - takeprev) >= SIZEOF_MM_FREENODE) {
				/* No.. just take what we need from the previous chunk and put
				 * it back into the free list
				 */
				newnode            = (FAR struct mm_allocnode_s *)((FAR char *)oldnode - takeprev);
				prev->size        -= takeprev;
				newnode->size      = oldsize + takeprev;
				newnode->preceding = prev->size | MM_ALLOC_BIT;
				next->preceding    = newnode->size | (next->preceding & MM_ALLOC_BIT);

				/* Return the previous free node to the nodelist (with the new size) */

				mm_addfreechunk(heap, prev);
			} else {
				/* Yes.. update its size (newnode->preceding is already set) */
				takeprev            = prev->size;
				newnode             = (FAR struct mm_allocnode_s *)((FAR char *)oldnode - takeprev);
				newnode->size      += oldsize;
				newnode->preceding |= MM_ALLOC_BIT;
				next->preceding     = newnode->size | (next->preceding & MM_ALLOC_BIT);
			}

			oldnode = newnode;
			oldsize = newnode->size;

			/* Now we have to move the user contents 'down' in memory.  memcpy should
			 * should be save for this.
			 */

			newmem = (FAR void *)((FAR char *)newnode + SIZEOF_MM_ALLOCNODE);
			memcpy(newmem, oldmem, oldsize - SIZEOF_MM_ALLOCNODE);
		}

		/* Extend into the next free chunk */

		if (takenext) {
			FAR struct mm_freenode_s *newnode;
			FAR struct mm_allocnode_s *andbeyond;

			/* Get the chunk following the next node (which could be the tail
			 * chunk)
			 */

			andbeyond = (FAR struct mm_allocnode_s *)((char *)next + nextsize);

			/* Remove the next node.  There must be a predecessor, but there
			 * may not be a successor node.
			 */

			REMOVE_NODE_FROM_LIST(next);

			/* Extend the node into the next chunk */
			/* Did we consume the entire preceding chunk? */

			if ((nextsize - takenext) >= SIZEOF_MM_FREENODE) {
				/* No, take what we need from the next chunk and return it to
				 * the free nodelist.
				 */
				oldnode->size        = oldsize + takenext;
				newnode              = (FAR struct mm_freenode_s *)((char *)oldnode + oldnode->size);
				newnode->size        = nextsize - takenext;
				newnode->preceding   = oldnode->size;
				andbeyond->preceding = newnode->size | (andbeyond->preceding & MM_ALLOC_BIT);

				/* Add the new free node to the nodelist (with the new size) */

				mm_addfreechunk(heap, newnode);
			} else {
				/* Yes, just update some pointers. */
				takenext             = nextsize;
				oldnode->size        = oldsize + takenext;
				andbeyond->preceding = oldnode->size | (andbeyond->preceding & MM_ALLOC_BIT);
			}
		}
#ifdef CONFIG_DEBUG_MM_HEAPINFO
		/* update the chunk to realloc task information */
		heapinfo_update_node(oldnode, caller_retaddr);

		heapinfo_add_size(heap, oldnode->pid, oldnode->size);
		heapinfo_update_total_size(heap, oldnode->size, oldnode->pid);
#endif

		sched_unlock();
		mm_givesemaphore(heap);
		return newmem;
	}

	/* The current chunk cannot be extended.  Just allocate a new chunk and copy */

	else
#endif
	{
		/* Allocate a new block.  On failure, realloc must return NULL but
		 * leave the original memory in place.
		 */
		sched_unlock();
		mm_givesemaphore(heap);
#ifdef CONFIG_DEBUG_MM_HEAPINFO
		newmem = (FAR void *)mm_malloc(heap, size, caller_retaddr);
#else
		newmem = (FAR void *)mm_malloc(heap, size);
#endif
		if (newmem) {
			memcpy(newmem, oldmem, oldsize - SIZEOF_MM_ALLOCNODE);
			mm_free(heap, oldmem);
		}

		return newmem;
	}
}
