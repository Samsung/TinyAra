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
 * fs/vfs/fs_open.c
 *
 *   Copyright (C) 2007-2009, 2011-2012 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#ifdef CONFIG_FILE_MODE
#include <stdarg.h>
#endif

#include <tinyara/cancelpt.h>
#include <tinyara/fs/fs.h>
#include <tinyara/kmalloc.h>

#include "inode/inode.h"
#include "driver/block/driver.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: inode_checkflags
 *
 * Description:
 *   Check if the access described by 'oflags' is supported on 'inode'
 *
 ****************************************************************************/

int inode_checkflags(FAR struct inode *inode, int oflags)
{
	if (((oflags & O_RDOK) != 0 && !inode->u.i_ops->read) || ((oflags & O_WROK) != 0 && !inode->u.i_ops->write)) {
		return -EACCES;
	} else {
		return OK;
	}
}

/****************************************************************************
 * Name: vopen
 *
 * Description:
 *   vopen() is identical to 'open' except that it accepts a va_list
 *   as an argument versus taking a variable length list of arguments.
 *
 *   vopen() is an internal TizenRT interface and should not be called from
 *   applications.
 *
 * Returned Value:
 *   The new file descriptor is returned on success; a negated errno value is
 *   returned on any failure.
 *
 ****************************************************************************/

int vopen(FAR const char *path, int oflags, va_list ap)
{
	FAR struct file *filep;
	FAR struct inode *inode;
	FAR const char *relpath = NULL;
	FAR const char *norm_path = NULL;
	FAR char *tmp = NULL;
#if defined(CONFIG_FILE_MODE) || !defined(CONFIG_DISABLE_MOUNTPOINT)
	mode_t mode = 0666;
#endif
	int ret;
	int fd;

	if (path == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_FILE_MODE
#ifdef CONFIG_CPP_HAVE_WARNING
#warning "File creation not implemented"
#endif

	/* If the file is opened for creation, then get the mode bits */

	if ((oflags & (O_WRONLY | O_CREAT)) != 0) {
		mode = va_arg(ap, mode_t);
	}
#endif

	/* Get an inode for this file */

	if (strncmp(path, "/", 1) != 0) {
		tmp = (char *)kmm_malloc(strlen(path));
		normalize_path(path, &tmp);
		norm_path = tmp;

		inode = inode_find(norm_path, &relpath);
	} else {
		inode = inode_find(path, &relpath);
	}
	if (!inode) {
		/* "O_CREAT is not set and the named file does not exist.  Or, a
		 * directory component in pathname does not exist or is a dangling
		 * symbolic link."
		 */

		ret = -ENOENT;
		goto errout;
	}

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && \
	!defined(CONFIG_DISABLE_PSEUDOFS_OPERATIONS) && defined(CONFIG_BCH)
	/*
	 * If the inode is block driver, then we may return a character driver
	 * proxy for the block driver. block_proxy() will instantiate a BCH
	 * character driver wrapper around the block driver, open(), then
	 * unlink() the character driver. On success, block_proxy() will
	 * return the file descriptor of the opened character driver.
	 *
	 * NOTE: This will recurse to open the character driver proxy.
	 */
	if (INODE_IS_BLOCK(inode)) {
		/* Release the inode reference */
		inode_release(inode);

		/* Get the file descriptor of the opened character driver proxy */
		fd = block_proxy(path, oflags);
		if (fd < 0) {
			ret = fd;
			goto errout;
		}
		kmm_free(tmp);

		return fd;
	} else
#endif

	/* Verify that the inode is valid and either a "normal" character driver or a
	 * mountpoint.  We specifically exclude block drivers and and "special"
	 * inodes (semaphores, message queues, shared memory).
	 */

#ifndef CONFIG_DISABLE_MOUNTPOINT
	if ((!INODE_IS_DRIVER(inode) && !INODE_IS_MOUNTPT(inode)) || !inode->u.i_ops)
#else
	if (!INODE_IS_DRIVER(inode) || !inode->u.i_ops)
#endif
	{
		ret = -ENXIO;
		goto errout_with_inode;
	}

	/* Make sure that the inode supports the requested access */

	ret = inode_checkflags(inode, oflags);
	if (ret < 0) {
		goto errout_with_inode;
	}

	/* Associate the inode with a file structure */

	fd = files_allocate(inode, oflags, 0, 0);
	if (fd < 0) {
		ret = -EMFILE;
		goto errout_with_inode;
	}

	/* Get the file structure corresponding to the file descriptor. */

	ret = fs_getfilep(fd, &filep);
	if (ret < 0) {
		goto errout_with_inode;
	}

	/* Perform the driver open operation.  NOTE that the open method may be
	 * called many times.  The driver/mountpoint logic should handled this
	 * because it may also be closed that many times.
	 */

	ret = OK;
	if (inode->u.i_ops->open) {
#ifndef CONFIG_DISABLE_MOUNTPOINT
		if (INODE_IS_MOUNTPT(inode)) {
			ret = inode->u.i_mops->open(filep, relpath, oflags, mode);
		} else
#endif
		{
			ret = inode->u.i_ops->open(filep);
		}
	}

	if (ret < 0) {
		goto errout_with_fd;
	}

	return fd;

errout_with_fd:
	files_release(fd);
errout_with_inode:
	inode_release(inode);
errout:
	kmm_free(tmp);
	return ret;
}

/****************************************************************************
 * Name: open
 *
 * Description:
 *   Standard 'open' interface
 *
 * Returned Value:
 *   The new file descriptor is returned on success; -1 (ERROR) is returned
 *   on any failure the errno value set appropriately.
 *
 ****************************************************************************/

int open(FAR const char *path, int oflags, ...)
{
	va_list ap;
	int fd;

	/* open() is a cancellation point */

	(void)enter_cancellation_point();

	/* Let vopen() do most of the work */

	va_start(ap, oflags);
	fd = vopen(path, oflags, ap);
	va_end(ap);

	/* Set the errno value if any errors were reported by open() */

	if (fd < 0) {
		set_errno(-fd);
		fd = ERROR;
	}

	leave_cancellation_point();
	return fd;
}
