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
 * fs/vfs/fs_stat.c
 *
 *   Copyright (C) 2007-2009, 2012 Gregory Nutt. All rights reserved.
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

#include <sys/stat.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <tinyara/kmalloc.h>

#include "inode/inode.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: statroot
 ****************************************************************************/

static inline int statroot(FAR struct stat *buf)
{
	/* There is no inode associated with the fake root directory */

	memset(buf, 0, sizeof(struct stat));
	buf->st_mode = S_IFDIR | S_IROTH | S_IRGRP | S_IRUSR;
	return OK;
}

/****************************************************************************
 * Global Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stat
 *
 * Returned Value:
 *   Zero on success; -1 on failure with errno set:
 *
 *   EACCES  Search permission is denied for one of the directories in the
 *           path prefix of path.
 *   EFAULT  Bad address.
 *   ENOENT  A component of the path path does not exist, or the path is an
 *           empty string.
 *   ENOMEM  Out of memory
 *   ENOTDIR A component of the path is not a directory.
 *
 ****************************************************************************/

int stat(FAR const char *path, FAR struct stat *buf)
{
	FAR struct inode *inode;
	const char *relpath = NULL;
	const char *norm_path = NULL;
	char *tmp = NULL;
	int ret = OK;

	/* Sanity checks */

	if (!path || !buf) {
		ret = EFAULT;
		goto errout;
	}

	if (!path[0]) {
		ret = ENOENT;
		goto errout;
	}

	/* Check for the fake root directory (which has no inode) */

	if (strcmp(path, "/") == 0) {
		return statroot(buf);
	}

	/* Get an inode for this file */

	tmp = (char *)kmm_malloc(strlen(path));
	normalize_path(path, &tmp);
	norm_path = tmp;

	inode = inode_find(norm_path, &relpath);
	if (!inode) {
		/* This name does not refer to a psudeo-inode and there is no
		 * mountpoint that includes in this path.
		 */

		ret = ENOENT;
		goto errout;
	}

	/* The way we handle the stat depends on the type of inode that we
	 * are dealing with.
	 */

#ifndef CONFIG_DISABLE_MOUNTPOINT
	if (INODE_IS_MOUNTPT(inode)) {
		/* The node is a file system mointpoint. Verify that the mountpoint
		 * supports the stat() method
		 */

		if (inode->u.i_mops && inode->u.i_mops->stat) {
			/* Perform the stat() operation */

			ret = inode->u.i_mops->stat(inode, relpath, buf);
		}
	} else
#endif
	{
		/* The node is part of the root pseudo file system */

		ret = inode_stat(inode, buf);
	}

	/* Check if the stat operation was successful */

	if (ret < 0) {
		ret = -ret;
		goto errout_with_inode;
	}

	/* Successfully stat'ed the file */

	inode_release(inode);
	kmm_free(tmp);
	return OK;

	/* Failure conditions always set the errno appropriately */

errout_with_inode:
	inode_release(inode);
errout:
	set_errno(ret);
	kmm_free(tmp);
	return ERROR;
}

/****************************************************************************
 * Name: inode_stat
 *
 * Description:
 *   The inode_stat() function will obtain information about an 'inode' in
 *   the pseudo file system and will write it to the area pointed to by 'buf'.
 *
 *   The 'buf' argument is a pointer to a stat structure, as defined in
 *   <sys/stat.h>, into which information is placed concerning the file.
 *
 * Input Parameters:
 *   inode - The inode of interest
 *   buf   - The caller provide location in which to return information about
 *           the inode.
 *
 * Returned Value:
 *   Zero (OK) returned on success.  Otherwise, a negated errno value is
 *   returned to indicate the nature of the failure.
 *
 ****************************************************************************/

int inode_stat(FAR struct inode *inode, FAR struct stat *buf)
{
	DEBUGASSERT(inode != NULL && buf != NULL);

	memset(buf, 0, sizeof(*buf));

	if (INODE_IS_SPECIAL(inode)) {
#if defined(CONFIG_FS_NAMED_SEMAPHORES)
		if (INODE_IS_NAMEDSEM(inode)) {
			buf->st_mode = S_IFSEM;
		} else
#endif
#if !defined(CONFIG_DISABLE_MQUEUE)
		if (INODE_IS_MQUEUE(inode)) {
			buf->st_mode = S_IFMQ;
		} else
#endif
		{
		}
	} else if (inode->u.i_ops != NULL) {
		/* Determine read/write privileges based on the existence of read
		 * and write methods.
		 */
		if (inode->u.i_ops->read) {
			buf->st_mode = S_IROTH | S_IRGRP | S_IRUSR;
		}
		if (inode->u.i_ops->write) {
			buf->st_mode |= S_IWOTH | S_IWGRP | S_IWUSR;
		}
		/* Determine the type of the inode */
		if (INODE_IS_MOUNTPT(inode)) {
			buf->st_mode |= S_IFDIR;
		} else if (INODE_IS_BLOCK(inode)) {
			/* What is if also has child inodes? */
			buf->st_mode |= S_IFBLK;
		} else /* if (INODE_IS_DRIVER(inode)) */ {
			/* What is it if it also has child inodes? */
			buf->st_mode |= S_IFCHR;
		}
	} else {
		/* If it has no operations, then it must just be a intermediate
		 * node in the inode tree. It is something like a directory.
		 * We'll say that all pseudo-directories are read-able but not
		 * write-able.
		 */
		buf->st_mode |= S_IFDIR | S_IROTH | S_IRGRP | S_IRUSR;
	}

	return OK;
}
