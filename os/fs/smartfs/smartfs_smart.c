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
 * fs/smartfs/smartfs_smart.c
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
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
#include <sys/stat.h>
#include <sys/statfs.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/kmalloc.h>
#include <tinyara/fs/fs.h>
#include <tinyara/fs/dirent.h>
#include <tinyara/fs/ioctl.h>
#include <tinyara/fs/mtd.h>
#include <tinyara/fs/smart.h>

#include "smartfs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int smartfs_open(FAR struct file *filep, const char *relpath, int oflags, mode_t mode);
static int smartfs_close(FAR struct file *filep);
static ssize_t smartfs_read(FAR struct file *filep, char *buffer, size_t buflen);
static ssize_t smartfs_write(FAR struct file *filep, const char *buffer, size_t buflen);
static off_t smartfs_seek(FAR struct file *filep, off_t offset, int whence);
static int smartfs_ioctl(FAR struct file *filep, int cmd, unsigned long arg);

static int smartfs_sync(FAR struct file *filep);
static int smartfs_dup(FAR const struct file *oldp, FAR struct file *newp);
static int smartfs_fstat(FAR const struct file *filep, FAR struct stat *buf);

static int smartfs_opendir(struct inode *mountpt, const char *relpath, struct fs_dirent_s *dir);
static int smartfs_readdir(struct inode *mountpt, struct fs_dirent_s *dir);
static int smartfs_rewinddir(struct inode *mountpt, struct fs_dirent_s *dir);

static int smartfs_bind(FAR struct inode *blkdriver, const void *data, void **handle);
static int smartfs_unbind(void *handle, FAR struct inode **blkdriver);
static int smartfs_statfs(struct inode *mountpt, struct statfs *buf);

static int smartfs_unlink(struct inode *mountpt, const char *relpath);
static int smartfs_mkdir(struct inode *mountpt, const char *relpath, mode_t mode);
static int smartfs_rmdir(struct inode *mountpt, const char *relpath);
static int smartfs_rename(struct inode *mountpt, const char *oldrelpath, const char *newrelpath);
static void smartfs_stat_common(FAR struct smartfs_mountpt_s *fs, FAR struct smartfs_entry_s *entry, FAR struct stat *buf);
static int smartfs_stat(struct inode *mountpt, const char *relpath, struct stat *buf);

static off_t smartfs_seek_internal(struct smartfs_mountpt_s *fs, struct smartfs_ofile_s *sf, off_t offset, int whence);

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static uint8_t g_seminitialized = FALSE;
static sem_t g_sem;

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/* See fs_mount.c -- this structure is explicitly externed there.
 * We use the old-fashioned kind of initializers so that this will compile
 * with any compiler.
 */

const struct mountpt_operations smartfs_operations = {
	smartfs_open,				/* open */
	smartfs_close,				/* close */
	smartfs_read,				/* read */
	smartfs_write,				/* write */
	smartfs_seek,				/* seek */
	smartfs_ioctl,				/* ioctl */

	smartfs_sync,				/* sync */
	smartfs_dup,				/* dup */
	smartfs_fstat,				/* fstat */

	smartfs_opendir,			/* opendir */
	NULL,						/* closedir */
	smartfs_readdir,			/* readdir */
	smartfs_rewinddir,			/* rewinddir */

	smartfs_bind,				/* bind */
	smartfs_unbind,				/* unbind */
	smartfs_statfs,				/* statfs */

	smartfs_unlink,				/* unlinke */
	smartfs_mkdir,				/* mkdir */
	smartfs_rmdir,				/* rmdir */
	smartfs_rename,				/* rename */
	smartfs_stat				/* stat */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smartfs_open
 ****************************************************************************/

static int smartfs_open(FAR struct file *filep, const char *relpath, int oflags, mode_t mode)
{
	struct inode *inode;
	struct smartfs_mountpt_s *fs;
	int ret;
	uint16_t parentdirsector;
	const char *filename;
	struct smartfs_ofile_s *sf;
#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	struct smart_read_write_s readwrite;
#endif
	struct smartfs_entry_s newentry;
	uint16_t data_sector;
	bool new_chain = FALSE;

	/* Sanity checks */

	DEBUGASSERT((filep->f_priv == NULL) && (filep->f_inode != NULL));

	/* Get the mountpoint inode reference from the file structure and the
	 * mountpoint private data from the inode structure
	 */

	inode = filep->f_inode;
	fs = inode->i_private;

	DEBUGASSERT(fs != NULL);

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Locate the directory entry for this path */

	sf = (struct smartfs_ofile_s *)kmm_malloc(sizeof * sf);
	if (sf == NULL) {
		ret = -ENOMEM;
		goto errout_with_semaphore;
	}

	/* Allocate a sector buffer if CRC enabled in the MTD layer */

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	sf->buffer = (uint8_t *)kmm_malloc(fs->fs_llformat.availbytes);
	if (sf->buffer == NULL) {
		/* Error ... no memory */

		kmm_free(sf);
		ret = -ENOMEM;
		goto errout_with_semaphore;
	}

	sf->bflags = 0;
#endif							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */

	sf->entry.name = NULL;
	ret = smartfs_finddirentry(fs, &sf->entry, relpath, &parentdirsector, &filename);

	/* Three possibilities: (1) a node exists for the relpath and
	 * dirinfo describes the directory entry of the entity, (2) the
	 * node does not exist, or (3) some error occurred.
	 */

	if (ret == OK) {
		/* The name exists -- but is is a file or a directory ? */

		if (sf->entry.flags & SMARTFS_DIRENT_TYPE_DIR) {
			/* Can't open a dir as a file! */

			ret = -EISDIR;
			goto errout_with_buffer;
		}

		/* It would be an error if we are asked to create it exclusively */

		if ((oflags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
			/* Already exists -- can't create it exclusively */

			ret = -EEXIST;
			goto errout_with_buffer;
		}

		/* TODO: Test open mode based on the file mode */

		/* The file exists.  Check if we are opening it for O_CREAT or
		 * O_TRUNC mode and delete the sector chain if we are. */

		if ((oflags & (O_CREAT | O_TRUNC)) != 0) {
			/* Don't truncate if open for APPEND */

			if (!(oflags & O_APPEND)) {
				/* Truncate the file as part of the open */

				ret = smartfs_truncatefile(fs, &sf->entry, sf);
				if (ret < 0) {
					goto errout_with_buffer;
				}
			}
		}
	} else if (ret == -ENOENT) {
		/* The file does not exist.  Were we asked to create it? */
		if ((oflags & O_CREAT) == 0) {
			/* No.. then we fail with -ENOENT */

			ret = -ENOENT;
			goto errout_with_buffer;
		}

		/* Yes... test if the parent directory is valid */
		if (parentdirsector != 0xFFFF) {
			/* We can create in the given parent directory */
			/* First we allocate a new data sector for the file */
			ret = smartfs_alloc_firstsector(fs, &data_sector, SMARTFS_DIRENT_TYPE_FILE, sf);
			if (ret != OK) {
				fdbg("Failed to allocate data scetor for entry\n");
				goto errout_with_buffer;
			}

			/* First try to find an invalid or empty entry available in one of the chained parent sectors */
			ret = smartfs_find_availableentry(fs, parentdirsector, &newentry, &new_chain);
			if (ret != OK) {
				/* find_availableentry encountered a problem with lower layer operations, return error */
				fdbg("smartfs_find_availableentry() encountered a problem, unable to find entry for writing\n");
				goto errout_with_buffer;
			}

			/* At this point, either an available entry was found or a new one has been created */
			newentry.firstsector = data_sector;

			ret = smartfs_writeentry(fs, newentry, filename, SMARTFS_DIRENT_TYPE_FILE, mode, &sf->entry, new_chain);
			if (ret != OK) {
				fdbg("Write entry failed\n");
				goto errout_with_buffer;
			}
		} else {
			/* Trying to create in a directory that doesn't exist */

			ret = -ENOENT;
			goto errout_with_buffer;
		}
	} else {
		goto errout_with_buffer;
	}

	/* Now perform the "open" on the file in direntry */

	sf->oflags = oflags;
	sf->crefs = 1;
	sf->filepos = 0;
	sf->curroffset = sizeof(struct smartfs_chain_header_s);
	sf->currsector = sf->entry.firstsector;
	sf->byteswritten = 0;

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	if ((sf->bflags & SMARTFS_BFLAG_DIRTY) == 0) {
		/* When using sector buffering, current sector with its header should
		 * always be present in sf->buffer. Otherwise data corruption may arise
		 * when writing.
		 */

		if (sf->currsector != SMARTFS_ERASEDSTATE_16BIT) {
			smartfs_setbuffer(&readwrite, sf->currsector, 0, fs->fs_llformat.availbytes, (uint8_t *)sf->buffer);
			ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long) &readwrite);
			if (ret < 0) {
				fdbg("ERROR: Error %d reading sector %d header\n", ret, sf->currsector);
				goto errout_with_buffer;
			}
		}
	}
#endif

	/* Test if we opened for APPEND mode.  If we did, then seek to the
	 * end of the file.
	 */

	if (oflags & O_APPEND) {
		/* Perform the seek */

		smartfs_seek_internal(fs, sf, 0, SEEK_END);
	}

	/* Attach the private date to the struct file instance */

	filep->f_priv = sf;

	/* Then insert the new instance into the mountpoint structure.
	 * It needs to be there (1) to handle error conditions that effect
	 * all files, and (2) to inform the umount logic that we are busy
	 * (but a simple reference count could have done that).
	 */

	sf->fnext = fs->fs_head;
	fs->fs_head = sf;

	ret = OK;
	goto errout_with_semaphore;

errout_with_buffer:
#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	if (sf->buffer != NULL) {
		kmm_free(sf->buffer);
		sf->buffer = NULL;
	}
#endif
	if (sf->entry.name != NULL) {
		/* Free the space for the name too */

		kmm_free(sf->entry.name);
		sf->entry.name = NULL;
	}

	kmm_free(sf);

errout_with_semaphore:
	smartfs_semgive(fs);
	if (ret == -EINVAL) {
		ret = -EIO;
	}

	return ret;
}

/****************************************************************************
 * Name: smartfs_close
 ****************************************************************************/

static int smartfs_close(FAR struct file *filep)
{
	struct inode *inode;
	struct smartfs_mountpt_s *fs;
	struct smartfs_ofile_s *sf;
	struct smartfs_ofile_s *nextfile;
	struct smartfs_ofile_s *prevfile;

	/* Sanity checks */

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);

	/* Recover our private data from the struct file instance */

	inode = filep->f_inode;
	fs = inode->i_private;
	sf = filep->f_priv;

	/* Sync the file */

	smartfs_sync(filep);

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Check if we are the last one with a reference to the file and
	 * only close if we are. */

	if (sf->crefs > 1) {
		/* The file is opened more than once.  Just decrement the
		 * reference count and return. */

		sf->crefs--;
		goto okout;
	}

	/* Remove ourselves from the linked list */

	nextfile = fs->fs_head;
	prevfile = nextfile;
	while ((nextfile != sf) && (nextfile != NULL)) {
		/* Save the previous file pointer too */

		prevfile = nextfile;
		nextfile = nextfile->fnext;
	}

	if (nextfile != NULL) {
		/* Test if we were the first entry */

		if (nextfile == fs->fs_head) {
			/* Assign a new head */

			fs->fs_head = nextfile->fnext;
		} else {
			/* Take ourselves out of the list */

			prevfile->fnext = nextfile->fnext;
		}
	}

	/* Now free the pointer */

	filep->f_priv = NULL;;
	if (sf->entry.name != NULL) {
		/* Free the space for the name too */

		kmm_free(sf->entry.name);
		sf->entry.name = NULL;
	}
#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	if (sf->buffer) {
		kmm_free(sf->buffer);
	}
#endif

	kmm_free(sf);
	filep->f_priv = NULL;

okout:
	smartfs_semgive(fs);
	return OK;
}

/****************************************************************************
 * Name: smartfs_read
 ****************************************************************************/

static ssize_t smartfs_read(FAR struct file *filep, char *buffer, size_t buflen)
{
	struct inode *inode;
	struct smartfs_mountpt_s *fs;
	struct smartfs_ofile_s *sf;
	struct smart_read_write_s readwrite;
	struct smartfs_chain_header_s *header;
	int ret = OK;
	uint32_t bytesread;
	uint16_t bytestoread;
	uint16_t bytesinsector;

	/* Sanity checks */

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);

	/* Recover our private data from the struct file instance */

	sf = filep->f_priv;
	inode = filep->f_inode;
	fs = inode->i_private;

	DEBUGASSERT(fs != NULL);

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Loop until all byte read or error */

	bytesread = 0;
	while (bytesread != buflen) {
		/* Test if we are at the end of data */

		if (sf->currsector == SMARTFS_ERASEDSTATE_16BIT) {
			/* Break and return the number of bytes we read (may be zero) */

			break;
		}

		/* Read the curent sector into our buffer */

		smartfs_setbuffer(&readwrite, sf->currsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error %d reading sector %d data\n", ret, sf->currsector);
			goto errout_with_semaphore;
		}

		/* Point header to the read data to get used byte count */

		header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;

		/* Get number of used bytes in this sector */
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
		bytesinsector = get_leftover_used_byte_count((uint8_t *)readwrite.buffer, get_used_byte_count((uint8_t *)header->used));
#else
		bytesinsector = SMARTFS_USED(header);

		if (bytesinsector == SMARTFS_ERASEDSTATE_16BIT) {
			/* No bytes to read from this sector */

			bytesinsector = 0;
		}
#endif
		/* Calculate the number of bytes to read into the buffer */

		bytestoread = bytesinsector - (sf->curroffset - sizeof(struct smartfs_chain_header_s));
		if (bytestoread + bytesread > buflen) {
			/* Truncate bytesto read based on buffer len */

			bytestoread = buflen - bytesread;
		}

		/* Copy data to the read buffer */

		if (bytestoread > 0) {
			/* Do incremental copy from this sector */

			memcpy(&buffer[bytesread], &fs->fs_rwbuffer[sf->curroffset], bytestoread);
			bytesread += bytestoread;
			sf->filepos += bytestoread;
			sf->curroffset += bytestoread;
		}

		/* Test if we are at the end of the data in this sector */

		if ((bytestoread == 0) || (sf->curroffset == fs->fs_llformat.availbytes)) {
			/* Set the next sector as the current sector */

			sf->currsector = SMARTFS_NEXTSECTOR(header);
			sf->curroffset = sizeof(struct smartfs_chain_header_s);

			/* Test if at end of data */

			if (sf->currsector == SMARTFS_ERASEDSTATE_16BIT) {
				/* No more data!  Return what we have */

				break;
			}
		}
	}

	/* Return the number of bytes we read */

	ret = bytesread;

errout_with_semaphore:
	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_sync_internal
 *
 * Description: Synchronize the file state on disk to match internal, in-
 *   memory state.
 *
 ****************************************************************************/

static int smartfs_sync_internal(struct smartfs_mountpt_s *fs, struct smartfs_ofile_s *sf)
{
	struct smart_read_write_s readwrite;
	struct smartfs_chain_header_s *header;
	int ret = OK;
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
	int used_value;
#endif

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	if (sf->bflags & SMARTFS_BFLAG_DIRTY) {
		/* Update the header with the number of bytes written */

		header = (struct smartfs_chain_header_s *)sf->buffer;
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
		used_value = get_leftover_used_byte_count((uint8_t *)sf.buffer, get_used_byte_count((uint8_t *)header->used));
		if (used_value == 0) {
			set_used_byte_count((uint8_t *)header->used, sf->byteswritten);
#else
		if (header->used[0] == CONFIG_SMARTFS_ERASEDSTATE) {
			*((uint16_t *)header->used) = sf->byteswritten;
#endif
		} else {
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
			set_used_byte_count((uint8_t *)header->used, used_value + sf->byteswritten);
#else
			*((uint16_t *)header->used) += sf->byteswritten;
#endif
		}

		/* Write the entire sector to FLASH */

		smartfs_setbuffer(&readwrite, sf->currsector, 0, fs->fs_llformat.availbytes, (uint8_t *)sf->buffer);
		ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error %d writing used bytes for sector %d\n", ret, sf->currsector);
			goto errout;
		}

		sf->byteswritten = 0;
		sf->bflags = 0;
	}
#else							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */

	/* Test if we have written bytes to the current sector that
	 * need to be recorded in the chain header's used bytes field. */

	if (sf->byteswritten > 0) {
		fvdbg("Syncing sector %d\n", sf->currsector);

		/* Read the existing sector used bytes value */

		header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
		smartfs_setbuffer(&readwrite, sf->currsector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error %d reading sector %d data\n", ret, sf->currsector);
			goto errout;
		}
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
		used_value = sf->entry.datlen % (fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s));
		if (sf->entry.datlen > 0 && used_value == 0) {
			used_value = fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s);
		}

		set_used_byte_count((uint8_t *)header->used, used_value);
#else
		if (SMARTFS_USED(header) == SMARTFS_ERASEDSTATE_16BIT) {
			header->used[0] = (uint8_t)(sf->byteswritten & 0x00FF);
			header->used[1] = (uint8_t)(sf->byteswritten >> 8);
		} else {
			uint16_t tmp = SMARTFS_USED(header);
			tmp += sf->byteswritten;
			header->used[0] = (uint8_t)(tmp & 0x00FF);
			header->used[1] = (uint8_t)(tmp >> 8);
		}
#endif
		smartfs_setbuffer(&readwrite, sf->currsector, offsetof(struct smartfs_chain_header_s, used), sizeof(uint16_t), (uint8_t *)&fs->fs_rwbuffer[offsetof(struct smartfs_chain_header_s, used)]);
		ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error %d writing used bytes for sector %d\n", ret, sf->currsector);
			goto errout;
		}

		sf->byteswritten = 0;
	}
#endif							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */

errout:
	return ret;
}

/****************************************************************************
 * Name: smartfs_write
 ****************************************************************************/

static ssize_t smartfs_write(FAR struct file *filep, const char *buffer, size_t buflen)
{
	struct inode *inode;
	struct smartfs_mountpt_s *fs;
	struct smartfs_ofile_s *sf;
	struct smart_read_write_s readwrite;
	struct smartfs_chain_header_s *header;
	size_t byteswritten;
	int ret;

	/* Sanity checks.  I have seen the following assertion misfire if
	 * CONFIG_DEBUG_MM is enabled while re-directing output to a
	 * file.  In this case, the debug output can get generated while
	 * the file is being opened,  FAT data structures are being allocated,
	 * and things are generally in a perverse state.
	 */

#ifdef CONFIG_DEBUG_MM
	if (filep->f_priv == NULL || filep->f_inode == NULL) {
		return -ENXIO;
	}
#else
	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);
#endif

	/* Recover our private data from the struct file instance */

	sf = filep->f_priv;
	inode = filep->f_inode;
	fs = inode->i_private;

	DEBUGASSERT(fs != NULL);

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Test the permissions.  Only allow write if the file was opened with
	 * write flags.
	 */

	if ((sf->oflags & O_WROK) == 0) {
		ret = -EACCES;
		goto errout_with_semaphore;
	}

	/* First test if we are overwriting an existing location or writing to
	 * a new one. */

	header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
	byteswritten = 0;
	while ((sf->filepos < sf->entry.datlen) && (buflen > 0)) {
		/* Overwriting data caused by a seek, etc.  In this case, we need
		 * to check if the write causes the file length to be extended
		 * or not and update it accordingly.  We will write data up to
		 * the current end-of-file and then break, allowing the next while
		 * loop below to write the additional data to the end of the file.
		 */

		smartfs_setbuffer(&readwrite, sf->currsector, sf->curroffset, fs->fs_llformat.availbytes - sf->curroffset, (uint8_t *)&buffer[byteswritten]);
		/* Limit the write based on available data to write */

		if (readwrite.count > buflen) {
			readwrite.count = buflen;
		}

		/* Limit the write based on current file length */

		if (readwrite.count > sf->entry.datlen - sf->filepos) {
			/* Limit the write length so we write to the current EOF. */

			readwrite.count = sf->entry.datlen - sf->filepos;
		}

		/* Now perform the write. */

		if (readwrite.count > 0) {
			ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
			if (ret < 0) {
				fdbg("Error %d writing sector %d data\n", ret, sf->currsector);
				goto errout_with_semaphore;
			}

			/* Update our control variables */

			sf->filepos += readwrite.count;
			sf->curroffset += readwrite.count;
			buflen -= readwrite.count;
			byteswritten += readwrite.count;
		}

		/* Test if we wrote to the end of the current sector */

		if (sf->curroffset == fs->fs_llformat.availbytes) {
			/* Wrote to the end of the sector.  Update to point to the
			 * next sector for additional writes.  First read the sector
			 * header to get the sector chain info.
			 */

			smartfs_setbuffer(&readwrite, sf->currsector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
			ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
			if (ret < 0) {
				fdbg("Error %d reading sector %d header\n", ret, sf->currsector);
				goto errout_with_semaphore;
			}

			/* Now get the chained sector info and reset the offset */

			sf->curroffset = sizeof(struct smartfs_chain_header_s);
			sf->currsector = SMARTFS_NEXTSECTOR(header);
		}
	}

	/* Now append data to end of the file. */

	while (buflen > 0) {
		/* We will fill up the current sector. Write data to
		 * the current sector first.
		 */

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
		readwrite.count = fs->fs_llformat.availbytes - sf->curroffset;
		if (readwrite.count > buflen) {
			readwrite.count = buflen;
		}

		memcpy(&sf->buffer[sf->curroffset], &buffer[byteswritten], readwrite.count);
		sf->bflags |= SMARTFS_BFLAG_DIRTY;

#else							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */
		smartfs_setbuffer(&readwrite, sf->currsector, sf->curroffset, fs->fs_llformat.availbytes - sf->curroffset, (uint8_t *)&buffer[byteswritten]);
		if (readwrite.count > buflen) {
			/* Limit the write base on remaining bytes to write */

			readwrite.count = buflen;
		}

		/* Perform the write */

		if (readwrite.count > 0) {
			ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
			if (ret < 0) {
				fdbg("Error %d writing sector %d data\n", ret, sf->currsector);
				goto errout_with_semaphore;
			}
		}
#endif							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */

		/* Update our control variables */

		sf->entry.datlen += readwrite.count;
		sf->byteswritten += readwrite.count;
		sf->filepos += readwrite.count;
		sf->curroffset += readwrite.count;
		buflen -= readwrite.count;
		byteswritten += readwrite.count;

		/* Test if we wrote a full sector of data */

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
		if (sf->curroffset == fs->fs_llformat.availbytes && buflen) {
			/* First get a new chained sector */

			ret = FS_IOCTL(fs, BIOC_ALLOCSECT, 0xFFFF);
			if (ret < 0) {
				fdbg("Error %d allocating new sector\n", ret);
				goto errout_with_semaphore;
			}

			/* Copy the new sector to the old one and chain it */

			header = (struct smartfs_chain_header_s *)sf->buffer;
			*((uint16_t *)header->nextsector) = (uint16_t)ret;

			/* Now sync the file to write this sector out */

			ret = smartfs_sync_internal(fs, sf);
			if (ret != OK) {
				goto errout_with_semaphore;
			}

			/* Record the new sector in our tracking variables and
			 * reset the offset to "zero".
			 */

			if (sf->currsector == SMARTFS_NEXTSECTOR(header)) {
				/* Error allocating logical sector! */

				fdbg("Error - duplicate logical sector %d\n", sf->currsector);
			}

			sf->bflags = SMARTFS_BFLAG_DIRTY;
			sf->currsector = SMARTFS_NEXTSECTOR(header);
			sf->curroffset = sizeof(struct smartfs_chain_header_s);
			memset(sf->buffer, CONFIG_SMARTFS_ERASEDSTATE, fs->fs_llformat.availbytes);
			header->type = SMARTFS_DIRENT_TYPE_FILE;
		}
#else							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */

		if (sf->curroffset == fs->fs_llformat.availbytes) {
			/* Sync the file to write this sector out */

			ret = smartfs_sync_internal(fs, sf);
			if (ret != OK) {
				goto errout_with_semaphore;
			}

			/* Allocate a new sector if needed */

			if (buflen > 0) {
				/* Allocate a new sector */

				ret = FS_IOCTL(fs, BIOC_ALLOCSECT, 0xFFFF);
				if (ret < 0) {
					fdbg("Error %d allocating new sector\n", ret);
					goto errout_with_semaphore;
				}

				/* Copy the new sector to the old one and chain it */

				header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
				header->nextsector[0] = (uint8_t)(ret & 0x00FF);
				header->nextsector[1] = (uint8_t)((ret >> 8) & 0x00FF);

				smartfs_setbuffer(&readwrite, sf->currsector, offsetof(struct smartfs_chain_header_s, nextsector), sizeof(uint16_t), (uint8_t *)header->nextsector);
				ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
				if (ret < 0) {
					fdbg("Error %d writing next sector\n", ret);
					goto errout_with_semaphore;
				}

				/* Record the new sector in our tracking variables and
				 * reset the offset to "zero".
				 */

				if (sf->currsector == SMARTFS_NEXTSECTOR(header)) {
					/* Error allocating logical sector! */

					fdbg("Error - duplicate logical sector %d\n", sf->currsector);
				}

				sf->currsector = SMARTFS_NEXTSECTOR(header);
				sf->curroffset = sizeof(struct smartfs_chain_header_s);
			}
		}
#endif							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */
	}

	ret = byteswritten;

errout_with_semaphore:
	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_seek_internal
 *
 * Description: Performs the logic of the seek function.  This is an internal
 *              function because it does not provide semaphore protection and
 *              therefore must be called from one of the other public
 *              interface routines (open, seek, etc.).
 *
 ****************************************************************************/

static off_t smartfs_seek_internal(struct smartfs_mountpt_s *fs, struct smartfs_ofile_s *sf, off_t offset, int whence)
{
	struct smart_read_write_s readwrite;
	struct smartfs_chain_header_s *header;
	int ret;
	off_t newpos;
	off_t sectorstartpos;
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
	int sector_used = 0;
#endif
	/* Test if this is a seek to get the current file pos */

	if ((whence == SEEK_CUR) && (offset == 0)) {
		return sf->filepos;
	}

	/* Test if we need to sync the file */

	if (sf->byteswritten > 0) {
		/* Perform a sync */

		smartfs_sync_internal(fs, sf);
	}

	/* Calculate the file position to seek to based on current position */

	switch (whence) {
	case SEEK_SET:
		newpos = offset;
		break;

	case SEEK_CUR:
		newpos = sf->filepos + offset;
		break;

	case SEEK_END:
		newpos = sf->entry.datlen + offset;
		break;
	default:
		return -EINVAL;
	}

	/* Ensure newpos is in range */

	if (newpos < 0) {
		newpos = 0;
	}

	if (newpos > sf->entry.datlen) {
		newpos = sf->entry.datlen;
	}

	/* Now perform the seek.  Test if we are seeking within the current
	 * sector and can skip the search to save time.
	 */

	sectorstartpos = sf->filepos - (sf->curroffset - sizeof(struct smartfs_chain_header_s));
	if (newpos >= sectorstartpos && newpos < sectorstartpos + fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s)) {
		/* Seeking within the current sector.  Just update the offset */

		sf->curroffset = sizeof(struct smartfs_chain_header_s) + newpos - sectorstartpos;
		sf->filepos = newpos;

		return newpos;
	}

	/* Nope, we have to search for the sector and offset.  If the new pos is greater
	 * than the current pos, then we can start from the beginning of the current
	 * sector, otherwise we have to start from the beginning of the file.
	 */

	if (newpos > sf->filepos) {
		sf->filepos = sectorstartpos;
	} else {
		sf->currsector = sf->entry.firstsector;
		sf->filepos = 0;
	}

	header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
	while ((sf->currsector != SMARTFS_ERASEDSTATE_16BIT) && (sf->filepos + fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s) < newpos)) {
		/* Read the sector's header */

		smartfs_setbuffer(&readwrite, sf->currsector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error %d reading sector %d header\n", ret, sf->currsector);
			goto errout;
		}
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
		if (SMARTFS_NEXTSECTOR(header) == SMARTFS_ERASEDSTATE_16BIT) {
			sf->filepos += (sf->entry.datlen - (sector_used * (fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s))));
		} else {
			sf->filepos += (fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s));
		}
		sector_used++;
#else
		sf->filepos += SMARTFS_USED(header);
#endif
		sf->currsector = SMARTFS_NEXTSECTOR(header);
	}

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER

	/* When using sector buffering, we must read in the last buffer to our
	 * sf->buffer in case any changes are made.
	 */

	if (sf->currsector != SMARTFS_ERASEDSTATE_16BIT) {
		smartfs_setbuffer(&readwrite, sf->currsector, 0, fs->fs_llformat.availbytes, (uint8_t *)sf->buffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error %d reading sector %d header\n", ret, sf->currsector);
			goto errout;
		}
	}
#endif

	/* Now calculate the offset */

	sf->curroffset = sizeof(struct smartfs_chain_header_s) + newpos - sf->filepos;
	sf->filepos = newpos;
	return newpos;

errout:
	return ret;
}

/****************************************************************************
 * Name: smartfs_seek
 ****************************************************************************/

static off_t smartfs_seek(FAR struct file *filep, off_t offset, int whence)
{
	struct inode *inode;
	struct smartfs_mountpt_s *fs;
	struct smartfs_ofile_s *sf;
	int ret;

	/* Sanity checks */

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);

	/* Recover our private data from the struct file instance */

	sf = filep->f_priv;
	inode = filep->f_inode;
	fs = inode->i_private;

	DEBUGASSERT(fs != NULL);

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Call our internal routine to perform the seek */

	ret = smartfs_seek_internal(fs, sf, offset, whence);

	if (ret >= 0) {
		filep->f_pos = ret;
	}

	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_ioctl
 ****************************************************************************/

static int smartfs_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	/* We don't use any ioctls */

	return -ENOSYS;
}

/****************************************************************************
 * Name: smartfs_sync
 *
 * Description: Synchronize the file state on disk to match internal, in-
 *   memory state.
 *
 ****************************************************************************/

static int smartfs_sync(FAR struct file *filep)
{
	struct inode *inode;
	struct smartfs_mountpt_s *fs;
	struct smartfs_ofile_s *sf;
	int ret;

	/* Sanity checks */

	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);

	/* Recover our private data from the struct file instance */

	sf = filep->f_priv;
	inode = filep->f_inode;
	fs = inode->i_private;

	DEBUGASSERT(fs != NULL);

	/* Take the semaphore */

	smartfs_semtake(fs);

	ret = smartfs_sync_internal(fs, sf);

	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smart_dup
 *
 * Description: Duplicate open file data in the new file structure.
 *
 ****************************************************************************/

static int smartfs_dup(FAR const struct file *oldp, FAR struct file *newp)
{
	struct smartfs_ofile_s *sf;

	fvdbg("Dup %p->%p\n", oldp, newp);

	/* Sanity checks */

	DEBUGASSERT(oldp->f_priv != NULL && newp->f_priv == NULL && newp->f_inode != NULL);

	/* Recover our private data from the struct file instance */

	sf = oldp->f_priv;

	DEBUGASSERT(sf != NULL);

	/* Just increment the reference count on the ofile */

	sf->crefs++;
	newp->f_priv = (FAR void *)sf;

	return OK;
}

/****************************************************************************
 * Name: smartfs_fstat
 *
 * Description:
 *   Obtain information about an open file associated with the file
 *   descriptor 'fd', and will write it to the area pointed to by 'buf'.
 *
 ****************************************************************************/

static int smartfs_fstat(FAR const struct file *filep, FAR struct stat *buf)
{
	FAR struct inode *inode;
	FAR struct smartfs_mountpt_s *fs;
	FAR struct smartfs_ofile_s *sf;

	DEBUGASSERT(filep != NULL);
	DEBUGASSERT(filep->f_priv != NULL && filep->f_inode != NULL);
	DEBUGASSERT(buf != NULL);

	/* Recover our private data from the struct file instance */
	sf = filep->f_priv;
	inode = filep->f_inode;

	fs = inode->i_private;

	/* Take the semaphore */
	smartfs_semtake(fs);

	/* Return information about the directory entry in the stat structure */
	smartfs_stat_common(fs, &sf->entry, buf);
	smartfs_semgive(fs);
	return OK;
}

/****************************************************************************
 * Name: smartfs_opendir
 *
 * Description: Open a directory for read access
 *
 ****************************************************************************/

static int smartfs_opendir(struct inode *mountpt, const char *relpath, struct fs_dirent_s *dir)
{
	struct smartfs_mountpt_s *fs;
	int ret;
	struct smartfs_entry_s entry;
	uint16_t parentdirsector;
	const char *filename;

	/* Sanity checks */

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);

	/* Recover our private data from the inode instance */

	fs = mountpt->i_private;

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Search for the path on the volume */

	entry.name = NULL;
	ret = smartfs_finddirentry(fs, &entry, relpath, &parentdirsector, &filename);
	if (ret < 0) {
		goto errout_with_semaphore;
	}

	/* Populate our private data in the fs_dirent_s struct */

	dir->u.smartfs.fs_firstsector = entry.firstsector;
	dir->u.smartfs.fs_currsector = entry.firstsector;
	dir->u.smartfs.fs_curroffset = sizeof(struct smartfs_chain_header_s);

	ret = OK;

errout_with_semaphore:
	/* If space for the entry name was allocated, then free it */

	if (entry.name != NULL) {
		kmm_free(entry.name);
		entry.name = NULL;
	}

	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_readdir
 *
 * Description: Read the next directory entry
 *
 ****************************************************************************/

static int smartfs_readdir(struct inode *mountpt, struct fs_dirent_s *dir)
{
	struct smartfs_mountpt_s *fs;
	int ret;
	uint16_t entrysize;
	uint16_t namelen;
	struct smartfs_chain_header_s *header;
	struct smart_read_write_s readwrite;
	struct smartfs_entry_header_s *entry;

	/* Sanity checks */

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);

	/* Recover our private data from the inode instance */

	fs = mountpt->i_private;

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Read sectors and search entries until one found or no more */

	entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;
	while (dir->u.smartfs.fs_currsector != SMARTFS_ERASEDSTATE_16BIT) {
		/* Read the logical sector */

		smartfs_setbuffer(&readwrite, dir->u.smartfs.fs_currsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			goto errout_with_semaphore;
		}

		/* Now search for entries, starting at curroffset */

		while (dir->u.smartfs.fs_curroffset < ret) {
			/* Point to next entry */

			entry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[dir->u.smartfs.fs_curroffset];

			/* Test if this entry is valid and active */

			if (((entry->flags & SMARTFS_DIRENT_EMPTY) == (SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_EMPTY)) || ((entry->flags & SMARTFS_DIRENT_ACTIVE) != (SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_ACTIVE))) {
				/* This entry isn't valid, skip it */

				dir->u.smartfs.fs_curroffset += entrysize;
				continue;
			}

			/* Entry found!  Report it */

			if ((entry->flags & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_DIR) {
				dir->fd_dir.d_type = DTYPE_DIRECTORY;
			} else {
				dir->fd_dir.d_type = DTYPE_FILE;
			}

			/* Copy the entry name to dirent */

			namelen = fs->fs_llformat.namesize;
			if (namelen > NAME_MAX) {
				namelen = NAME_MAX;
			}

			memset(dir->fd_dir.d_name, 0, namelen);
			strncpy(dir->fd_dir.d_name, entry->name, namelen);

			/* Now advance to the next entry */

			dir->u.smartfs.fs_curroffset += entrysize;
			if (dir->u.smartfs.fs_curroffset + entrysize >= fs->fs_llformat.availbytes) {
				/* We advanced past the end of the sector.  Go to next sector */

				dir->u.smartfs.fs_curroffset = sizeof(struct smartfs_chain_header_s);
				header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
				dir->u.smartfs.fs_currsector = SMARTFS_NEXTSECTOR(header);
			}

			/* Now exit */

			ret = OK;
			goto errout_with_semaphore;
		}

		/* No more entries in this sector.  Move on to next sector and
		 * continue the search.  If no more sectors, then we are all
		 * done and will report ENOENT.
		 */

		header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
		dir->u.smartfs.fs_curroffset = sizeof(struct smartfs_chain_header_s);
		dir->u.smartfs.fs_currsector = SMARTFS_NEXTSECTOR(header);
	}

	/* If we arrive here, then there are no more entries */

	ret = -ENOENT;

errout_with_semaphore:
	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_rewindir
 *
 * Description: Reset directory read to the first entry
 *
 ****************************************************************************/

static int smartfs_rewinddir(struct inode *mountpt, struct fs_dirent_s *dir)
{
	int ret = OK;

	/* Sanity checks */

	DEBUGASSERT(mountpt != NULL && mountpt->i_private != NULL);

	/* Reset the directory to the first entry */

	dir->u.smartfs.fs_currsector = dir->u.smartfs.fs_firstsector;
	dir->u.smartfs.fs_curroffset = sizeof(struct smartfs_chain_header_s);

	return ret;
}

/****************************************************************************
 * Name: smartfs_bind
 *
 * Description: This implements a portion of the mount operation. This
 *  function allocates and initializes the mountpoint private data and
 *  binds the blockdriver inode to the filesystem private data.  The final
 *  binding of the private data (containing the blockdriver) to the
 *  mountpoint is performed by mount().
 *
 ****************************************************************************/

static int smartfs_bind(FAR struct inode *blkdriver, const void *data, void **handle)
{
	struct smartfs_mountpt_s *fs;
	int ret;

	/* Open the block driver */

	if (!blkdriver || !blkdriver->u.i_bops) {
		return -ENODEV;
	}

	if (blkdriver->u.i_bops->open && blkdriver->u.i_bops->open(blkdriver) != OK) {
		return -ENODEV;
	}

	/* Create an instance of the mountpt state structure */

	fs = (struct smartfs_mountpt_s *)kmm_zalloc(sizeof(struct smartfs_mountpt_s));
	if (!fs) {
		return -ENOMEM;
	}

	/* If the global semaphore hasn't been initialized, then
	 * initialized it now. */

	fs->fs_sem = &g_sem;
	if (!g_seminitialized) {
		sem_init(&g_sem, 0, 0);	/* Initialize the semaphore that controls access */
		g_seminitialized = TRUE;
	} else {
		/* Take the semaphore for the mount */

		smartfs_semtake(fs);
	}

	/* Initialize the allocated mountpt state structure.  The filesystem is
	 * responsible for one reference ont the blkdriver inode and does not
	 * have to addref() here (but does have to release in ubind().
	 */

	fs->fs_blkdriver = blkdriver;	/* Save the block driver reference */
	fs->fs_head = NULL;

	/* Now perform the mount.  */

	ret = smartfs_mount(fs, true);
	if (ret != 0) {
		goto error_with_semaphore;
	}

	*handle = (void *)fs;
	
	ret = smartfs_sector_recovery(fs);
	if (ret != 0) {
		goto error_with_semaphore;
	}

	smartfs_semgive(fs);
	return ret;

error_with_semaphore:
	smartfs_semgive(fs);
	kmm_free(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_unbind
 *
 * Description: This implements the filesystem portion of the umount
 *   operation.
 *
 ****************************************************************************/

static int smartfs_unbind(void *handle, FAR struct inode **blkdriver)
{
	struct smartfs_mountpt_s *fs = (struct smartfs_mountpt_s *)handle;
	int ret;

	if (!fs) {
		return -EINVAL;
	}

	/* Check if there are sill any files opened on the filesystem. */

	ret = OK;					/* Assume success */
	smartfs_semtake(fs);
	if (fs->fs_head != NULL) {
		/* We cannot unmount now.. there are open files */

		smartfs_semgive(fs);
		return -EBUSY;
	}
	/* Unmount ... close the block driver */
	ret = smartfs_unmount(fs);
	smartfs_semgive(fs);
	kmm_free(fs);

	return ret;
}

/****************************************************************************
 * Name: smartfs_statfs
 *
 * Description: Return filesystem statistics
 *
 ****************************************************************************/

static int smartfs_statfs(struct inode *mountpt, struct statfs *buf)
{
	struct smartfs_mountpt_s *fs;
	int ret;

	/* Sanity checks */

	DEBUGASSERT(mountpt && mountpt->i_private);

	/* Get the mountpoint private data from the inode structure */

	fs = mountpt->i_private;

	smartfs_semtake(fs);

	/* Implement the logic!! */

	memset(buf, 0, sizeof(struct statfs));
	buf->f_type = SMARTFS_MAGIC;

	/* Re-request the low-level format info to update free blocks */

	ret = FS_IOCTL(fs, BIOC_GETFORMAT, (unsigned long)&fs->fs_llformat);

	buf->f_namelen = fs->fs_llformat.namesize;
	buf->f_bsize = fs->fs_llformat.sectorsize;
	buf->f_blocks = fs->fs_llformat.nsectors;
	if (buf->f_blocks == 65535) {
		buf->f_blocks++;
	}

	buf->f_bfree = fs->fs_llformat.nfreesectors;
	buf->f_bavail = fs->fs_llformat.nfreesectors;
	buf->f_files = 0;
	buf->f_ffree = fs->fs_llformat.nfreesectors;

	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_unlink
 *
 * Description: Remove a file
 *
 ****************************************************************************/

static int smartfs_unlink(struct inode *mountpt, const char *relpath)
{
	struct smartfs_mountpt_s *fs;
	int ret;
	struct smartfs_entry_s entry;
	const char *filename;
	uint16_t parentdirsector;

	/* Sanity checks */

	DEBUGASSERT(mountpt && mountpt->i_private);

	/* Get the mountpoint private data from the inode structure */

	fs = mountpt->i_private;

	smartfs_semtake(fs);

	/* Locate the directory entry for this path */

	entry.name = NULL;
	ret = smartfs_finddirentry(fs, &entry, relpath, &parentdirsector, &filename);

	if (ret == OK) {
		/* The name exists -- validate it is a file, not a dir */

		if ((entry.flags & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_DIR) {
			ret = -EISDIR;
			goto errout_with_semaphore;
		}

		/* TODO:  Need to check permissions?  */

		/* Okay, we are clear to delete the file.  Use the deleteentry routine. */

		smartfs_deleteentry(fs, &entry);

	} else {
		/* Just report the error */

		goto errout_with_semaphore;
	}

	ret = OK;

errout_with_semaphore:
	if (entry.name != NULL) {
		kmm_free(entry.name);
	}
	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_mkdir
 *
 * Description: Create a directory
 *
 ****************************************************************************/

static int smartfs_mkdir(struct inode *mountpt, const char *relpath, mode_t mode)
{
	struct smartfs_mountpt_s *fs;
	int ret;
	struct smartfs_entry_s entry; //Details of newly written entry are returned with this variable
	uint16_t parentdirsector;
	const char *filename;
	struct smartfs_entry_s newentry;
	uint16_t data_sector;
	bool new_chain = FALSE;

	/* Sanity checks */

	DEBUGASSERT(mountpt && mountpt->i_private);

	/* Get the mountpoint private data from the inode structure */

	fs = mountpt->i_private;

	smartfs_semtake(fs);

	/* Locate the directory entry for this path */

	entry.name = NULL;
	ret = smartfs_finddirentry(fs, &entry, relpath, &parentdirsector, &filename);
	/* Three possibililities: (1) a node exists for the relpath and
	 * dirinfo describes the directory entry of the entity, (2) the
	 * node does not exist, or (3) some error occurred.
	 */

	if (ret == OK) {
		/* The name exists -- can't create */

		ret = -EEXIST;
		goto errout_with_semaphore;
	} else if (ret == -ENOENT) {

		/* It doesn't exist ... we can create it, but only if we have
		 * the right permissions and if the parentdirsector is valid. */

		if (parentdirsector == 0xFFFF) {
			fdbg("Invalid parent dir sector\n");
			/* Invalid entry in the path (non-existant dir segment) */
			goto errout_with_semaphore;
		}
		/* Create the directory */
		/* Allocate new data sector for entry */
		ret = smartfs_alloc_firstsector(fs, &data_sector, SMARTFS_DIRENT_TYPE_DIR, NULL);
		if (ret != OK) {
			fdbg("Failed to allocate data sector for entry\n");
			goto errout_with_semaphore;
		}
		/* Try to find empty/invalid entry available in one of he chained parent sectors */
		ret = smartfs_find_availableentry(fs, parentdirsector, &newentry, &new_chain);
		if (ret != OK) {
			fdbg("find_availableentry failed, cannot find entry for writing\n");
			goto errout_with_semaphore;
		}
		/* Now we have an entry allocated for writing */
		newentry.firstsector = data_sector;
		/* Write new entry to sector */
		ret = smartfs_writeentry(fs, newentry, filename, SMARTFS_DIRENT_TYPE_DIR, mode, &entry, new_chain);
		if (ret != OK) {
			fdbg("Failed to write new entry to sector\n");
			goto errout_with_semaphore;
		}
	}

errout_with_semaphore:
	if (entry.name != NULL) {
		/* Free the filename space allocation */

		kmm_free(entry.name);
		entry.name = NULL;
	}

	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_rmdir
 *
 * Description: Remove a directory
 *
 ****************************************************************************/

int smartfs_rmdir(struct inode *mountpt, const char *relpath)
{
	struct smartfs_mountpt_s *fs;
	int ret;
	struct smartfs_entry_s entry;
	const char *filename;
	uint16_t parentdirsector;

	/* Sanity checks */

	DEBUGASSERT(mountpt && mountpt->i_private);

	/* Get the mountpoint private data from the inode structure */

	fs = mountpt->i_private;

	/* Take the semaphore */

	smartfs_semtake(fs);

	/* Locate the directory entry for this path */

	entry.name = NULL;
	ret = smartfs_finddirentry(fs, &entry, relpath, &parentdirsector, &filename);

	if (ret == OK) {
		/* The name exists -- validate it is a dir, not a file */

		if ((entry.flags & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_FILE) {
			ret = -ENOTDIR;
			goto errout_with_semaphore;
		}

		/* TODO:  Need to check permissions?  */

		/* Check if the directory is emtpy */

		ret = smartfs_countdirentries(fs, &entry);
		if (ret < 0) {
			goto errout_with_semaphore;
		}

		/* Only continue if there are zero entries in the directory */

		if (ret != 0) {
			ret = -ENOTEMPTY;
			goto errout_with_semaphore;
		}

		/* Okay, we are clear to delete the directory.  Use the deleteentry routine. */

		ret = smartfs_deleteentry(fs, &entry);
		if (ret < 0) {
			goto errout_with_semaphore;
		}
	} else {
		/* Just report the error */

		goto errout_with_semaphore;
	}

	ret = OK;

errout_with_semaphore:
	if (entry.name != NULL) {
		kmm_free(entry.name);
	}
	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_rename
 *
 * Description: Rename a file or directory
 *
 ****************************************************************************/

int smartfs_rename(struct inode *mountpt, const char *oldrelpath, const char *newrelpath)
{
	int ret;
	struct smartfs_mountpt_s *fs;
	struct smartfs_entry_s oldentry;
	uint16_t oldparentdirsector;
	const char *oldfilename;
	struct smartfs_entry_s newentry;
	uint16_t newparentdirsector;
	const char *newfilename;
	mode_t mode;
	uint16_t type;
	uint16_t sector;
	uint16_t offset;
	uint16_t entrysize;
	struct smartfs_entry_header_s *direntry;
	struct smart_read_write_s readwrite;
	struct smartfs_entry_s f_newentry;
	bool new_chain = FALSE;

	/* Sanity checks */

	DEBUGASSERT(mountpt && mountpt->i_private);

	/* Get the mountpoint private data from the inode structure */

	fs = mountpt->i_private;

	smartfs_semtake(fs);

	/* Search for old entry to validate it exists */

	oldentry.name = NULL;
	newentry.name = NULL;
	ret = smartfs_finddirentry(fs, &oldentry, oldrelpath, &oldparentdirsector, &oldfilename);
	if (ret < 0) {
		fdbg("Old entry doesn't exist\n");
		goto errout_with_semaphore;
	}

	/* Search for the new entry and validate it DOESN'T exist, unless we
	 * are copying to a directory and keeping the same filename, such as:
	 *
	 *    mv /mntpoint/somefile.txt /mntpoint/somedir
	 *
	 * in which case, we need to validate the /mntpoint/somedir/somefile.txt
	 * doens't exsit.
	 */

	ret = smartfs_finddirentry(fs, &newentry, newrelpath, &newparentdirsector, &newfilename);
	if (ret == OK) {
		/* Test if it's a file.  If it is, then it's an error */

		if ((newentry.flags & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_FILE) {
			ret = -EEXIST;
			goto errout_with_semaphore;
		}

		/* Nope, it's a directory.  Now search the directory for oldfilename */

		newfilename = oldfilename;
		sector = newentry.firstsector;
		while (sector != SMARTFS_ERASEDSTATE_16BIT) {
			/* Read the next sector of diretory entries */

			smartfs_setbuffer(&readwrite, sector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
			ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
			if (ret < 0) {
				fdbg("Error %d reading sector %d data\n", ret, oldentry.dsector);
				goto errout_with_semaphore;
			}

			/* Search for newfilename in this sector */

			offset = sizeof(struct smartfs_chain_header_s);
			entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;
			while (offset + entrysize < fs->fs_llformat.availbytes) {
				/* Test the next entry */

				direntry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
				/* Check if entry flags are erased OR empty OR inactive and skip the entry */
				if ((direntry->flags == SMARTFS_ERASEDSTATE_16BIT) || ((direntry->flags & SMARTFS_DIRENT_EMPTY) == (SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_EMPTY)) || ((direntry->flags & SMARTFS_DIRENT_ACTIVE) != (SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_ACTIVE))) {
					/* This entry isn't valid, skip it */

					offset += entrysize;
					continue;
				}

				/* Test if this entry matches newfilename */

				if (strcmp(newfilename, direntry->name) == 0) {
					/* Uh-oh, looks like the entry already exists */

					ret = -EEXIST;
					goto errout_with_semaphore;
				}

				offset += entrysize;
			}

			/* Chain to next sector */

			sector = SMARTFS_NEXTSECTOR(((struct smartfs_chain_header_s *)fs->fs_rwbuffer));
		}

		/* Okay, we will create newfilename in the newentry directory */
		newparentdirsector = newentry.firstsector;
	}

	/* Test if the new parent directory is valid */
	if (newparentdirsector != 0xFFFF) {
		/* We can move to the given parent directory */

		mode = oldentry.flags & SMARTFS_DIRENT_MODE;
		type = oldentry.flags & SMARTFS_DIRENT_TYPE;

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
		if (oldparentdirsector == newparentdirsector) {
			/* We will not use any new entry found, we will overwrite the existing entry */
			ret = smartfs_writeentry(fs, oldentry, newfilename, type, mode, &newentry, new_chain);
			if (ret != OK) {
				fdbg("Error writing new entry\n");
			}
			/* Old entry doesn't have to be invalidated, directly go to end */
			goto errout_with_semaphore;
		}
#endif
		/* Find an available invalid/empty entry to write the new entry */
		ret = smartfs_find_availableentry(fs, newparentdirsector, &f_newentry, &new_chain);
		if (ret != OK) {
			fdbg("find_availableentry encountered a problem, cannot find entry for writing\n");
			goto errout_with_semaphore;
		}

		f_newentry.firstsector = oldentry.firstsector;
		ret = smartfs_writeentry(fs, f_newentry, newfilename, type, mode, &newentry, new_chain);
		if (ret != OK) {
			fdbg("Error writing new entry\n");
			goto errout_with_semaphore;
		}

		/* Now mark the old entry as inactive */
		ret = smartfs_invalidateentry(fs, oldentry.dsector, oldentry.doffset);
		if (ret != OK) {
			fdbg("Unable to invalidate entry\n");
			goto errout_with_semaphore;
		}
	} else {
		/* Trying to create in a directory that doesn't exist */

		ret = -ENOENT;
		goto errout_with_semaphore;
	}

	ret = OK;

errout_with_semaphore:
	if (oldentry.name != NULL) {
		kmm_free(oldentry.name);
		oldentry.name = NULL;
	}

	if (newentry.name != NULL) {
		kmm_free(newentry.name);
		newentry.name = NULL;
	}

	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Name: smartfs_stat_common
 *
 * Description:
 *   Return information about a directory entry
 *
 ****************************************************************************/

static void smartfs_stat_common(FAR struct smartfs_mountpt_s *fs,
								FAR struct smartfs_entry_s *entry,
								FAR struct stat *buf)
{
	/* Initialize the stat structure */
	memset(buf, 0, sizeof(struct stat));
	if (entry->firstsector == fs->fs_rootsector) {
		/* It's directory name of the mount point */
		buf->st_mode = S_IFDIR | S_IROTH | S_IRGRP | S_IRUSR | S_IWOTH |
					   S_IWGRP | S_IWUSR;
	} else {
		buf->st_mode = entry->flags & 0xFFF;
		if ((entry->flags & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_DIR) {
			buf->st_mode |= S_IFDIR;
		} else {
			buf->st_mode |= S_IFREG;
		}
	}
	buf->st_size = entry->datlen;
	buf->st_blksize = fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s);
	buf->st_blocks = (buf->st_size + buf->st_blksize - 1) / buf->st_blksize;
	buf->st_atime = 0;
	buf->st_ctime = 0;
}

/****************************************************************************
 * Name: smartfs_stat
 *
 * Description: Return information about a file or directory
 *
 ****************************************************************************/

static int smartfs_stat(struct inode *mountpt, const char *relpath, struct stat *buf)
{
	struct smartfs_mountpt_s *fs;
	struct smartfs_entry_s entry;
	int ret;
	uint16_t parentdirsector;
	const char *filename;

	/* Sanity checks */

	DEBUGASSERT(mountpt && mountpt->i_private);
	DEBUGASSERT(buf != NULL);

	/* Get the mountpoint private data from the inode structure */

	fs = mountpt->i_private;

	smartfs_semtake(fs);

	/* Find the directory entry corresponding to relpath */

	entry.name = NULL;
	ret = smartfs_finddirentry(fs, &entry, relpath, &parentdirsector, &filename);
	if (ret < 0) {
		goto errout_with_semaphore;
	}

	/* Initialize the stat structure */
	smartfs_stat_common(fs, &entry, buf);

	ret = OK;

errout_with_semaphore:
	if (entry.name != NULL) {
		kmm_free(entry.name);
		entry.name = NULL;
	}

	smartfs_semgive(fs);
	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
