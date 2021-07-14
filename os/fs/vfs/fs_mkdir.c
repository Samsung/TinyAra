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
 * fs/vfs/fs_mkdir.c
 *
 *   Copyright (C) 2007, 2008, 2014 Gregory Nutt. All rights reserved.
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <tinyara/fs/fs.h>
#include <tinyara/kmalloc.h>

#include "inode/inode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef FS_HAVE_WRITABLE_MOUNTPOINT
#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FS_WRITABLE) && \
	CONFIG_NFILE_STREAMS > 0
#define FS_HAVE_WRITABLE_MOUNTPOINT 1
#endif

#undef FS_HAVE_PSEUDOFS_OPERATIONS
#if !defined(CONFIG_DISABLE_PSEUDOFS_OPERATIONS) && CONFIG_NFILE_STREAMS > 0
#define FS_HAVE_PSEUDOFS_OPERATIONS 1
#endif

#undef FS_HAVE_MKDIR
#if defined(FS_HAVE_WRITABLE_MOUNTPOINT) || defined(FS_HAVE_PSEUDOFS_OPERATIONS)
#define FS_HAVE_MKDIR 1
#endif

#ifdef FS_HAVE_MKDIR

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mkdir
 *
 * Description:  Create a directory
 *
 ****************************************************************************/

int mkdir(const char *pathname, mode_t mode)
{
	FAR struct inode *inode;
	const char *relpath = NULL;
	const char *norm_path = NULL;
	char *tmp = NULL;
	int errcode = 0;
	int ret;
#ifndef CONFIG_DISABLE_MOUNTPOINT
	FAR char *parent;
	const char *ptr;
	int index;
	int len;
	struct stat st;
#endif

	/* Find the inode that includes this path */

	tmp = (char *)kmm_malloc(strlen(pathname));
	normalize_path(pathname, &tmp);
	norm_path = tmp;

	inode = inode_find(norm_path, &relpath);

	if (inode) {
		/* An inode was found that includes this path and possibly refers to a
		 * mountpoint.
		 */

#ifndef CONFIG_DISABLE_MOUNTPOINT
		/* Check if the inode is a valid mountpoint. */

		if (!INODE_IS_MOUNTPT(inode) || !inode->u.i_mops) {
			/* The inode is not a mountpoint */

			errcode = ENXIO;
			goto errout_with_inode;
		}

		ptr = relpath;
		index = 0;
		len = 0;

		while (ptr != NULL && *ptr != '\0') {
			/* Search a path of parent directory */
			if (*ptr == '/' && *(ptr + 1) != '\0') {
				index = len;
			}
			len++;
			ptr++;
		}

		if (index > 0) {
			/* In another directory */
			parent = (FAR char *)kmm_malloc(index + 1);
			if (parent == NULL) {
				ret = ENOMEM;
				goto errout_with_inode;
			}
			memset(parent, 0, index + 1);
			strncpy(parent, relpath, index);

			if (inode->u.i_mops->stat) {
				/* Perform the stat() operation */

				ret = inode->u.i_mops->stat(inode, parent, &st);
				kmm_free(parent);
				if (ret < 0) {
					ret = -ret;
					goto errout_with_inode;
				} else if ((st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
					ret = EACCES;
					goto errout_with_inode;
				}
			} else {
				ret = ENOSYS;
				kmm_free(parent);
				goto errout_with_inode;
			}
		}

		/* Perform the mkdir operation using the relative path
		 * at the mountpoint.
		 */

		if (inode->u.i_mops->mkdir) {
			ret = inode->u.i_mops->mkdir(inode, relpath, mode);
			if (ret < 0) {
				errcode = -ret;
				goto errout_with_inode;
			}
		} else {
			errcode = ENOSYS;
			goto errout_with_inode;
		}

		/* Release our reference on the inode */

		inode_release(inode);
#else
		/* But mountpoints are not supported in this configuration */

		errcode = EEXIST;
		goto errout_with_inode;
#endif
	}
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	/* No inode exists that contains this path.  Create a new inode in the
	 * pseudo-filesystem at this location.
	 */
	else {
		/* Create an inode in the pseudo-filesystem at this path */

		inode_semtake();
		ret = inode_reserve(pathname, &inode);
		inode_semgive();

		if (ret < 0) {
			errcode = -ret;
			goto errout;
		}
	}
#else
	else {
		errcode = ENXIO;
		goto errout;
	}
#endif

	/* Directory successfully created */

	return OK;

errout_with_inode:
	inode_release(inode);
errout:
	set_errno(errcode);
	kmm_free(tmp);
	return ERROR;
}

#endif							/* FS_HAVE_MKDIR */
