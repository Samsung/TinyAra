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
 * fs/smartfs/smartfs_utils.c
 *
 *   Copyright (C) 2013-2014 Ken Pettit. All rights reserved.
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <queue.h>

#include <tinyara/kmalloc.h>
#include <tinyara/fs/fs.h>
#include <tinyara/fs/ioctl.h>

#include "smartfs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
#define SET1BITFROMMSB 128		//Binary: 0b10000000
#define SET2BITSFROMMSB 192		//Binary: 0b11000000
#define SET3BITSFROMMSB 224		//Binary: 0b11100000
#define SET4BITSFROMMSB 240		//Binary: 0b11110000
#define SET5BITSFROMMSB 248		//Binary: 0b11111000
#define SET6BITSFROMMSB 252		//Binary: 0b11111100
#define SET7BITSFROMMSB 254		//Binary: 0b11111110
#define SET8BITSFROMMSB 255		//Binary: 0b11111111
#define NEG8BIT(A) ((uint8_t)(~A & 0x000000FF))
#endif

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

#if defined(CONFIG_SMARTFS_MULTI_ROOT_DIRS) || \
	(defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_SMARTFS))
static struct smartfs_mountpt_s *g_mounthead = NULL;
#endif

#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
extern uint16_t chunk_shift;
extern uint16_t used_block_divident;
extern size_t smart_sect_header_size;
#define CHUNK_SIZE                              (CONFIG_MTD_SMART_SECTOR_SIZE / (USED_ARRAY_SIZE * (1 << 3)))
#endif

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
#define ENTRY_VALID(e) ((smartfs_rdle16(&(e)->flags) & SMARTFS_DIRENT_EMPTY) != \
						(SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_EMPTY)) && \
						((smartfs_rdle16(&(e)->flags) & SMARTFS_DIRENT_ACTIVE) == \
						(SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_ACTIVE))
#else
#define ENTRY_VALID(e) (((e)->flags & SMARTFS_DIRENT_EMPTY) != \
						(SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_EMPTY)) && \
						(((e)->flags & SMARTFS_DIRENT_ACTIVE) == \
						(SMARTFS_ERASEDSTATE_16BIT & SMARTFS_DIRENT_ACTIVE))
#endif

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
#define RESET_ENTRY_FLAGS(e) (smartfs_wrle16(&(e)->flags, SMARTFS_ERASEDSTATE_16BIT))
#else
#define RESET_ENTRY_FLAGS(e) ((e)->flags = SMARTFS_ERASEDSTATE_16BIT)
#endif

#if CONFIG_SMARTFS_ERASEDSTATE == 0xFF

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
#define SET_ENTRY_FLAGS(f, m, t) (smartfs_wrle16(&f, (uint16_t)(SMARTFS_DIRENT_ACTIVE | SMARTFS_DIRENT_DELETING | SMARTFS_DIRENT_RESERVED | t | (m & SMARTFS_DIRENT_MODE))));
#else
#define SET_ENTRY_FLAGS(f, m, t) (f = (uint16_t)(SMARTFS_DIRENT_ACTIVE | SMARTFS_DIRENT_DELETING | SMARTFS_DIRENT_RESERVED | t | (m & SMARTFS_DIRENT_MODE)));
#endif

#else                                                   /* CONFIG_SMARTFS_ERASEDSTATE == 0xFF */

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
#define SET_ENTRY_FLAGS(f, m, t) (smartfs_wrle16(&f, (uint16_t)(SMARTFS_DIRENT_EMPTY | t | (m & SMARTFS_DIRENT_MODE))));
#else
#define SET_ENTRY_FLAGS(f, m, t) (f = (uint16_t)(SMARTFS_DIRENT_EMPTY | t | (m & SMARTFS_DIRENT_MODE)));
#endif

#endif

/* This macro determines whether an entry is invalid/empty or not,
   i.e. whether it is available for writing a new entry.
 */
#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
#define IS_AVAILABLE_ENTRY(e) ((smartfs_rdle16(&(e)->flags) == SMARTFS_ERASEDSTATE_16BIT) ||\
				((smartfs_rdle16(&(e)->flags) & (SMARTFS_DIRENT_EMPTY | SMARTFS_DIRENT_ACTIVE)) ==\
					(~SMARTFS_ERASEDSTATE_16BIT & (SMARTFS_DIRENT_EMPTY | SMARTFS_DIRENT_ACTIVE))))
#else
#define IS_AVAILABLE_ENTRY(e) (((e)->flags == SMARTFS_ERASEDSTATE_16BIT) ||\
				(((e)->flags & (SMARTFS_DIRENT_EMPTY | SMARTFS_DIRENT_ACTIVE)) ==\
					(~SMARTFS_ERASEDSTATE_16BIT & (SMARTFS_DIRENT_EMPTY | SMARTFS_DIRENT_ACTIVE))))
#endif

#define SET_TO_REMAIN(v, n)		v[n / 8] |= (1 << (7 - (n % 8)))
#define SET_TO_USED(v, n)		v[n / 8] &= ~(1 << (7 - (n % 8)))
#define IS_SECTOR_REMAIN(v, n)	v[n / 8] & (1 << (7 - (n % 8)))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sector_entry_queue_s {
	struct sector_entry_queue_s *flink;
	uint8_t type;				/* File or Directory */
	uint16_t logsector;			/* Logical sector number */
	uint16_t parentsector;		/* Logical sector which is chain with this */
	uint16_t parentoffset;		/* Offset of parent entry in parent sector */
	uint16_t time;				/* Timestamp of entry */
};

struct sector_recover_info_s {
	uint16_t totalsector;
	uint16_t isolatedsector;
	uint16_t cleanentries;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

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
 * Name: smartfs_semtake
 ****************************************************************************/

void smartfs_semtake(struct smartfs_mountpt_s *fs)
{
	/* Take the semaphore (perhaps waiting) */

	while (sem_wait(fs->fs_sem) != 0) {
		/* The only case that an error should occur here is if
		 * the wait was awakened by a signal.
		 */

		ASSERT(*get_errno_ptr() == EINTR);
	}
}

/****************************************************************************
 * Name: smartfs_semgive
 ****************************************************************************/

void smartfs_semgive(struct smartfs_mountpt_s *fs)
{
	sem_post(fs->fs_sem);
}

/****************************************************************************
 * Name: smartfs_rdle16
 *
 * Description:
 *   Get a (possibly unaligned) 16-bit little endian value.
 *
 * Input Parameters:
 *   val - A pointer to the first byte of the little endian value.
 *
 * Returned Value:
 *   A uint16_t representing the whole 16-bit integer value
 *
 ****************************************************************************/

uint16_t smartfs_rdle16(FAR const void *val)
{
	return (uint16_t)((FAR const uint8_t *)val)[1] << 8 | (uint16_t)((FAR const uint8_t *)val)[0];
}

/****************************************************************************
 * Name: smartfs_wrle16
 *
 * Description:
 *   Put a (possibly unaligned) 16-bit little endian value.
 *
 * Input Parameters:
 *   dest - A pointer to the first byte to save the little endian value.
 *   val - The 16-bit value to be saved.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void smartfs_wrle16(FAR void *dest, uint16_t val)
{
	((FAR uint8_t *)dest)[0] = val & 0xff;	/* Little endian means LS byte first in byte stream */
	((FAR uint8_t *)dest)[1] = val >> 8;
}

/****************************************************************************
 * Name: smartfs_rdle32
 *
 * Description:
 *   Get a (possibly unaligned) 32-bit little endian value.
 *
 * Input Parameters:
 *   val - A pointer to the first byte of the little endian value.
 *
 * Returned Value:
 *   A uint32_t representing the whole 32-bit integer value
 *
 ****************************************************************************/

uint32_t smartfs_rdle32(FAR const void *val)
{
	/* Little endian means LS halfword first in byte stream */

	return (uint32_t)smartfs_rdle16(&((FAR const uint8_t *)val)[2]) << 16 | (uint32_t)smartfs_rdle16(val);
}

/****************************************************************************
 * Name: smartfs_wrle32
 *
 * Description:
 *   Put a (possibly unaligned) 32-bit little endian value.
 *
 * Input Parameters:
 *   dest - A pointer to the first byte to save the little endian value.
 *   val - The 32-bit value to be saved.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void smartfs_wrle32(uint8_t *dest, uint32_t val)
{
	/* Little endian means LS halfword first in byte stream */

	smartfs_wrle16(dest, (uint16_t)(val & 0xffff));
	smartfs_wrle16(dest + 2, (uint16_t)(val >> 16));
}

/****************************************************************************
 * Name: smartfs_setbuffer
 *
 * Description:
 *   Set the attributes of the smart_read_write_s structure objects for
 *   BIOC_READSECT and BIOC_WRITESECT requests.
 *
 * Input Parameters:
 *   rw - pointer to read_write object to be set.
 *   logsector - logicalsector to read from/write to.
 *   offset - offset to read from/write to.
 *   count - number of btes to be read/written.
 *   buffer - data to buffer to read to/write from.
 *
 * Returned Value:
 *   None
 *
 ***************************************************************************/

void smartfs_setbuffer(struct smart_read_write_s *rw, uint16_t logsector, uint16_t offset, uint16_t count, uint8_t *buffer)
{
	rw->logsector = logsector;
	rw->offset = offset;
	rw->count = count;
	rw->buffer = buffer;
	return;
}

#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER

uint16_t get_used_byte_count(uint8_t *used)
{
	uint16_t retval = 0, i = 0;
	while ((i < USED_ARRAY_SIZE) && (used[i] == NEG8BIT(CONFIG_SMARTFS_ERASEDSTATE))) {
		i++;
	}
	retval = (i << (used_block_divident));
	if (i >= USED_ARRAY_SIZE) {
		goto out;
	}

#if (CONFIG_SMARTFS_ERASEDSTATE == 0x00)
	switch (used[i]) {
	case SET1BITFROMMSB:
		retval += (1 << chunk_shift);
		break;
	case SET2BITSFROMMSB:
		retval += (2 << chunk_shift);
		break;
	case SET3BITSFROMMSB:
		retval += (3 << chunk_shift);
		break;
	case SET4BITSFROMMSB:
		retval += (4 << chunk_shift);
		break;
	case SET5BITSFROMMSB:
		retval += (5 << chunk_shift);
		break;
	case SET6BITSFROMMSB:
		retval += (6 << chunk_shift);
		break;
	case SET7BITSFROMMSB:
		retval += (7 << chunk_shift);
		break;
	}
#elif (CONFIG_SMARTFS_ERASEDSTATE == 0xFF)
	switch (used[i]) {
	case NEG8BIT(SET1BITFROMMSB):
		retval += (1 << chunk_shift);
		break;
	case NEG8BIT(SET2BITSFROMMSB):
		retval += (2 << chunk_shift);
		break;
	case NEG8BIT(SET3BITSFROMMSB):
		retval += (3 << chunk_shift);
		break;
	case NEG8BIT(SET4BITSFROMMSB):
		retval += (4 << chunk_shift);
		break;
	case NEG8BIT(SET5BITSFROMMSB):
		retval += (5 << chunk_shift);
		break;
	case NEG8BIT(SET6BITSFROMMSB):
		retval += (6 << chunk_shift);
		break;
	case NEG8BIT(SET7BITSFROMMSB):
		retval += (7 << chunk_shift);
		break;
	}
#endif
out:
	return retval;
}

uint16_t get_leftover_used_byte_count(uint8_t *buffer, uint16_t base_index)
{
	uint16_t i, ending;
	int sector_data_length = (CONFIG_MTD_SMART_SECTOR_SIZE - sizeof(struct smartfs_chain_header_s) - smart_sect_header_size);
	if (base_index >= sector_data_length) {
		return sector_data_length;
	}

	base_index += sizeof(struct smartfs_chain_header_s);
	//We have to read from the last byte of the active chunk
	//But if the last byte is already written then
	//we will update corresponding bit of USED_BYTE in Chain Header
	//So, we can start from the byte before the last byte of the active chunk
	//And the index of that byte, suppose, nth byte, should be n-1
	if ((base_index + (CHUNK_SIZE - 1) - 1) < (CONFIG_MTD_SMART_SECTOR_SIZE - smart_sect_header_size - 1)) {
		ending = base_index + (CHUNK_SIZE - 1) - 1;
	} else {
		ending = CONFIG_MTD_SMART_SECTOR_SIZE - smart_sect_header_size - 1;
	}

	for (i = ending; i >= base_index && buffer[i] == CONFIG_SMARTFS_ERASEDSTATE; i--) ;
	i++;
	i -= sizeof(struct smartfs_chain_header_s);

	return i;
}

uint16_t get_used_byte_count_from_end(uint8_t *buffer)
{
	uint16_t i, ending = CONFIG_MTD_SMART_SECTOR_SIZE - smart_sect_header_size - 1;

	for (i = ending; i >= sizeof(struct smartfs_chain_header_s) && buffer[i] == (uint8_t)CONFIG_SMARTFS_ERASEDSTATE; i--) ;
	i++;
	i -= sizeof(struct smartfs_chain_header_s);

	return i;
}

int set_used_byte_count(uint8_t *used, uint16_t count)
{
	int i = 0, iter = (count >> (used_block_divident)), leftover_bits;
	leftover_bits = (count - (iter << (used_block_divident))) >> chunk_shift;

	if (count > (CONFIG_MTD_SMART_SECTOR_SIZE - sizeof(struct smartfs_chain_header_s)) || iter > USED_ARRAY_SIZE) {
		return ERROR;
	}
	while (i < iter) {
		used[i] = NEG8BIT(CONFIG_SMARTFS_ERASEDSTATE);
		i++;
	}

	if (i >= USED_ARRAY_SIZE) {
		goto out;
	}

#if (CONFIG_SMARTFS_ERASEDSTATE == 0x00)
	if (leftover_bits > 0) {
		switch (leftover_bits) {
		case 1:
			used[i] = SET1BITFROMMSB;
			break;
		case 2:
			used[i] = SET2BITSFROMMSB;
			break;
		case 3:
			used[i] = SET3BITSFROMMSB;
			break;
		case 4:
			used[i] = SET4BITSFROMMSB;
			break;
		case 5:
			used[i] = SET5BITSFROMMSB;
			break;
		case 6:
			used[i] = SET6BITSFROMMSB;
			break;
		case 7:
			used[i] = SET7BITSFROMMSB;
			break;
		}
	}
#elif (CONFIG_SMARTFS_ERASEDSTATE == 0xFF)
	if (leftover_bits > 0) {
		switch (leftover_bits) {
		case 1:
			used[i] = NEG8BIT(SET1BITFROMMSB);
			break;
		case 2:
			used[i] = NEG8BIT(SET2BITSFROMMSB);
			break;
		case 3:
			used[i] = NEG8BIT(SET3BITSFROMMSB);
			break;
		case 4:
			used[i] = NEG8BIT(SET4BITSFROMMSB);
			break;
		case 5:
			used[i] = NEG8BIT(SET5BITSFROMMSB);
			break;
		case 6:
			used[i] = NEG8BIT(SET6BITSFROMMSB);
			break;
		case 7:
			used[i] = NEG8BIT(SET7BITSFROMMSB);
			break;
		}
	}
#endif
out:
	return OK;
}
#endif
/****************************************************************************
 * Name: smartfs_mount
 *
 * Desciption: This function is called only when the mountpoint is first
 *   established.  It initializes the mountpoint structure and verifies
 *   that a valid SMART filesystem is provided by the block driver.
 *
 *   The caller should hold the mountpoint semaphore
 *
 ****************************************************************************/

int smartfs_mount(struct smartfs_mountpt_s *fs, bool writeable)
{
	FAR struct inode *inode;
	struct geometry geo;
	int ret = OK;
#if defined(CONFIG_SMARTFS_MULTI_ROOT_DIRS)
	struct smartfs_mountpt_s *nextfs;
#endif

	/* Assume that the mount is not successful */

	fs->fs_mounted = false;

	/* Check if there is media available */

	inode = fs->fs_blkdriver;
	if (!inode || !inode->u.i_bops || !inode->u.i_bops->geometry || inode->u.i_bops->geometry(inode, &geo) != OK || !geo.geo_available) {
		ret = -ENODEV;
		goto errout;
	}

	/* Make sure that that the media is write-able (if write access is needed) */

	if (writeable && !geo.geo_writeenabled) {
		ret = -EACCES;
		goto errout;
	}

	/* Get the SMART low-level format information to validate the device has been
	 * formatted and scan properly for logical to physical sector mapping.
	 */

	ret = FS_IOCTL(fs, BIOC_GETFORMAT, (unsigned long)&fs->fs_llformat);
	if (ret != OK) {
		fdbg("Error getting device low level format: %d\n", ret);
		goto errout;
	}

	/* Validate the low-level format is valid */

	if (!(fs->fs_llformat.flags & SMART_FMT_ISFORMATTED)) {
		fdbg("No low-level format found\n");
		ret = -ENODEV;
		goto errout;
	}

	/* Allocate a read/write buffer */

#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
	/* Scan linked list of mounted file systems to find another FS with
	 * the same blockdriver.  We will reuse the buffers.
	 */

	nextfs = g_mounthead;
	while (nextfs != NULL) {
		/* Test if this FS uses the same block driver */

		if (nextfs->fs_blkdriver == fs->fs_blkdriver) {
			/* Yep, it's the same block driver.  Reuse the buffers.
			 * we can do this because we are protected by the same
			 * semaphore.
			 */

			fs->fs_rwbuffer = nextfs->fs_rwbuffer;
			fs->fs_workbuffer = nextfs->fs_workbuffer;
			break;
		}

		/* Advance to next FS */

		nextfs = nextfs->fs_next;
	}

	/* If we didn't find a FS above, then allocate some buffers */

	if (nextfs == NULL) {
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
		fs->fs_chunk_buffer = (char *)kmm_malloc(fs->fs_llformat.availbytes);
#endif
		fs->fs_rwbuffer = (char *)kmm_malloc(fs->fs_llformat.availbytes);
		fs->fs_workbuffer = (char *)kmm_malloc(256);
	}

	/* Now add ourselves to the linked list of SMART mounts */

	fs->fs_next = g_mounthead;
	g_mounthead = fs;

	/* Set our root directory sector based on the directory entry
	 * reported by the block driver (based on which device is
	 * associated with this mount.
	 */

	fs->fs_rootsector = SMARTFS_ROOT_DIR_SECTOR + fs->fs_llformat.rootdirnum;
#endif							/* CONFIG_SMARTFS_MULTI_ROOT_DIRS */

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_SMARTFS) && !defined(CONFIG_SMARTFS_MULTI_ROOT_DIRS)
	/* Now add ourselves to the linked list of SMART mounts */

	fs->fs_next = g_mounthead;
	g_mounthead = fs;
#endif

#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
	fs->fs_chunk_buffer = (uint8_t *)kmm_malloc(fs->fs_llformat.availbytes);
#endif
	fs->fs_rwbuffer = (char *)kmm_malloc(fs->fs_llformat.availbytes);
	fs->fs_workbuffer = (char *)kmm_malloc(256);
	fs->fs_rootsector = SMARTFS_ROOT_DIR_SECTOR;

	/* We did it! */

	fs->fs_mounted = TRUE;
#ifdef CONFIG_SMARTFS_ENTRY_TIMESTAMP
	fs->entry_seq = 0;
#endif

	fdbg("SMARTFS:\n");
	fdbg("\t    Sector size:     %d\n", fs->fs_llformat.sectorsize);
	fdbg("\t    Bytes/sector     %d\n", fs->fs_llformat.availbytes);
	fdbg("\t    Num sectors:     %d\n", fs->fs_llformat.nsectors);
	fdbg("\t    Free sectors:    %d\n", fs->fs_llformat.nfreesectors);
	fdbg("\t    Max filename:    %d\n", fs->fs_llformat.namesize);
#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
	fdbg("\t    RootDirEntries:  %d\n", fs->fs_llformat.nrootdirentries);
#endif
	fdbg("\t    RootDirSector:   %d\n", fs->fs_rootsector);

errout:
	return ret;
}

/****************************************************************************
 * Name: smartfs_unmount
 *
 * Desciption: This function is called only when the mountpoint is being
 *   unbound.  If we are serving multiple directories, then we have to
 *   remove ourselves from the mount linked list, and potentially free
 *   the shared buffers.
 *
 *   The caller should hold the mountpoint semaphore
 *
 ****************************************************************************/

int smartfs_unmount(struct smartfs_mountpt_s *fs)
{
	int ret = OK;
	struct inode *inode;
#if defined(CONFIG_SMARTFS_MULTI_ROOT_DIRS) || \
	(defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_SMARTFS))
	struct smartfs_mountpt_s *nextfs;
	struct smartfs_mountpt_s *prevfs;
	int count = 0;
	int found = FALSE;
#endif

#if defined(CONFIG_SMARTFS_MULTI_ROOT_DIRS) || \
	(defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_SMARTFS))
	/* Start at the head of the mounts and search for our entry.  Also
	 * count the number of entries that match our blkdriver.
	 */

	nextfs = g_mounthead;
	prevfs = NULL;
	while (nextfs != NULL) {
		/* Test if this FS's blkdriver matches ours (it could be us) */

		if (nextfs->fs_blkdriver == fs->fs_blkdriver) {
			count++;
		}

		/* Test if this entry is our's */

		if (nextfs == fs) {
			found = TRUE;
		}

		/* Keep track of the previous entry until our's is found */

		if (!found) {
			/* Save this entry as the previous entry */

			prevfs = nextfs;
		}

		/* Advance to the next entry */

		nextfs = nextfs->fs_next;
	}

	/* Ensure we found our FS */

	if (!found) {
		/* Our entry not found!  Invalid unmount or bug somewhere */

		return -EINVAL;
	}

	/* If the count is only one, then we need to delete the shared
	 * buffers because we are the last ones.
	 */

	if (count == 1) {
		/* Close the block driver */

		if (fs->fs_blkdriver) {
			inode = fs->fs_blkdriver;
			if (inode) {
				if (inode->u.i_bops && inode->u.i_bops->close) {
					(void)inode->u.i_bops->close(inode);
				}
			}
		}

		/* Free the buffers */
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
		kmm_free(fs->fs_chunk_buffer);
#endif
		kmm_free(fs->fs_rwbuffer);
		kmm_free(fs->fs_workbuffer);

		/* Set the buffer's to invalid value to catch program bugs */

		fs->fs_rwbuffer = (char *)0xDEADBEEF;
		fs->fs_workbuffer = (char *)0xDEADBEEF;
	}

	/* Now removed ourselves from the linked list */

	if (fs == g_mounthead) {
		/* We were the first ones.  Set a new head */

		g_mounthead = fs->fs_next;
	} else {
		/* Remove from the middle of the list somewhere */

		prevfs->fs_next = fs->fs_next;
	}
#else
	if (fs->fs_blkdriver) {
		inode = fs->fs_blkdriver;
		if (inode) {
			if (inode->u.i_bops && inode->u.i_bops->close) {
				(void)inode->u.i_bops->close(inode);
			}
		}
	}

	/* Release the mountpoint private data */
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
	kmm_free(fs->fs_chunk_buffer);
#endif
	kmm_free(fs->fs_rwbuffer);
	kmm_free(fs->fs_workbuffer);
#endif

	return ret;
}

/****************************************************************************
 * Name: smartfs_finddirentry
 *
 * Description: Finds an entry in the filesystem as specified by relpath.
 *              If found, the direntry will be populated with information
 *              for accessing the entry.
 *
 *              If the final directory segment of relpath just before the
 *              last segment (the target file/dir) is valid, then the
 *              parentdirsector will indicate the logical sector number of
 *              the parent directory where a new entry should be created,
 *              and the filename pointer will point to the final segment
 *              (i.e. the "filename").
 *
 ****************************************************************************/

int smartfs_finddirentry(struct smartfs_mountpt_s *fs, struct smartfs_entry_s *direntry, const char *relpath, uint16_t *parentdirsector, const char **filename)
{
	int ret = -ENOENT;
	const char *segment;
	const char *ptr;
	uint16_t seglen;
	uint16_t depth = 0;
	uint16_t dirstack[CONFIG_SMARTFS_DIRDEPTH];
	uint16_t dirsector;
	uint16_t entrysize;
	uint16_t offset;
	struct smartfs_chain_header_s *header;
	struct smart_read_write_s readwrite;
	struct smartfs_entry_header_s *entry;
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
	int used_value;
#endif

	/* Initialize directory level zero as the root sector */

	dirstack[0] = fs->fs_rootsector;
	entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;

	/* Test if this is a request for the root directory */

	if (*relpath == '\0') {
		direntry->firstsector = fs->fs_rootsector;
		direntry->flags = SMARTFS_DIRENT_TYPE_DIR | 0777;
		direntry->utc = 0;
		direntry->dsector = 0;
		direntry->doffset = 0;
		direntry->dfirst = fs->fs_rootsector;
		direntry->name = NULL;
		direntry->datlen = 0;

		*parentdirsector = 0;	/* Our parent is the format sector I guess */
		return OK;
	}

	/* Parse through each segment of relpath */

	segment = relpath;
	while (segment != NULL && *segment != '\0') {
		/* Find the end of this segment.  It will be '/' or NULL. */

		ptr = segment;
		seglen = 0;
		while (*ptr != '/' && *ptr != '\0') {
			seglen++;
			ptr++;
		}

		strncpy(fs->fs_workbuffer, segment, seglen);
		fs->fs_workbuffer[seglen] = '\0';

		/* Search for "." and ".." as segment names */

		if (strcmp(fs->fs_workbuffer, ".") == 0) {
			/* Just ignore this segment.  Advance ptr if not on NULL */

			if (*ptr == '/') {
				ptr++;
			}

			segment = ptr;
			continue;
		} else if (strcmp(fs->fs_workbuffer, "..") == 0) {
			/* Up one level */

			if (depth == 0) {
				/* We went up one level past our mount point! */

				goto errout;
			}

			/* "Pop" to the previous directory level */

			depth--;
			if (*ptr == '/') {
				ptr++;
			}

			segment = ptr;
			continue;
		} else {
			/* Search for the entry in the current directory */

			dirsector = dirstack[depth];

			/* Read the directory */

			offset = 0xFFFF;

#if CONFIG_SMARTFS_ERASEDSTATE == 0xFF
			while (dirsector != 0xFFFF)
#else
			while (dirsector != 0)
#endif
			{
				/* Read the next directory in the chain */

				smartfs_setbuffer(&readwrite, dirsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
				ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
				if (ret < 0) {
					goto errout;
				}

				/* Point to next sector in chain */

				header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
				dirsector = SMARTFS_NEXTSECTOR(header);

				/* Search for the entry */

				offset = sizeof(struct smartfs_chain_header_s);
				entry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
				while (offset < readwrite.count) {
					/* Test if this entry is valid and active */

					if (!(ENTRY_VALID(entry))) {
						/* This entry isn't valid, skip it */

						offset += entrysize;
						entry = (struct smartfs_entry_header_s *)
								&fs->fs_rwbuffer[offset];

						continue;
					}

					/* Test if the name matches */

					if (strncmp(entry->name, fs->fs_workbuffer, fs->fs_llformat.namesize) == 0) {
						/* We found it!  If this is the last segment entry,
						 * then report the entry.  If it isn't the last
						 * entry, then validate it is a directory entry and
						 * open it and continue searching.
						 */

						if (*ptr == '\0') {
							/* We are at the last segment.  Report the entry */

							/* Fill in the entry */

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
							direntry->firstsector = smartfs_rdle16(&entry->firstsector);
							direntry->flags = smartfs_rdle16(&entry->flags);
							direntry->utc = smartfs_rdle32(&entry->utc);
#else
							direntry->firstsector = entry->firstsector;
							direntry->flags = entry->flags;
							direntry->utc = entry->utc;
#endif
							direntry->dsector = readwrite.logsector;
							direntry->doffset = offset;
							direntry->dfirst = dirstack[depth];
							if (direntry->name == NULL) {
								direntry->name = (char *)kmm_malloc(fs->fs_llformat.namesize + 1);
								if (direntry->name == NULL) {
									ret = ERROR;
									goto errout;
								}
							}

							memset(direntry->name, 0, fs->fs_llformat.namesize + 1);
							strncpy(direntry->name, entry->name, fs->fs_llformat.namesize);
							direntry->datlen = 0;

							/* Scan the file's sectors to calculate the length and perform
							 * a rudimentary check.
							 */

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
							if ((smartfs_rdle16(&entry->flags) & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_FILE) {
								dirsector = smartfs_rdle16(&entry->firstsector);
#else
							if ((entry->flags & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_FILE) {
								dirsector = entry->firstsector;
#endif
								while (dirsector != SMARTFS_ERASEDSTATE_16BIT) {
									/* Read the next sector of the file */

									smartfs_setbuffer(&readwrite, dirsector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
									ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
									if (ret < 0) {
										fdbg("Error in sector chain at %d!\n", dirsector);
										break;
									}
#ifdef CONFIG_SMARTFS_DYNAMIC_HEADER
									if (SMARTFS_NEXTSECTOR(header) == SMARTFS_ERASEDSTATE_16BIT) {

										readwrite.count = fs->fs_llformat.availbytes;
										readwrite.buffer = (uint8_t *)fs->fs_chunk_buffer;

										ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
										if (ret < 0) {
											fdbg("Error %d reading sector %d header\n", ret, sf->currsector);
											break;
										}
										used_value = get_leftover_used_byte_count((uint8_t *)readwrite.buffer, get_used_byte_count((uint8_t *)header->used));
										direntry->datlen += used_value;
									} else {
										direntry->datlen += (fs->fs_llformat.availbytes - sizeof(struct smartfs_chain_header_s));
									}
									readwrite.buffer = (uint8_t *)fs->fs_rwbuffer;
#else
									/* Add used bytes to the total and point to next sector */
									if (SMARTFS_USED(header) != SMARTFS_ERASEDSTATE_16BIT) {
										direntry->datlen += SMARTFS_USED(header);
									}
#endif
									dirsector = SMARTFS_NEXTSECTOR(header);
								}
							}

							*parentdirsector = dirstack[depth];
							*filename = segment;
							ret = OK;
							goto errout;
						} else {
							/* Validate it's a directory */

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
							if ((smartfs_rdle16(&entry->flags) & SMARTFS_DIRENT_TYPE) != SMARTFS_DIRENT_TYPE_DIR)
#else
							if ((entry->flags & SMARTFS_DIRENT_TYPE) != SMARTFS_DIRENT_TYPE_DIR)
#endif
							{
								/* Not a directory!  Report the error */

								ret = -ENOTDIR;
								goto errout;
							}

							/* "Push" the directory and continue searching */

							if (depth >= CONFIG_SMARTFS_DIRDEPTH - 1) {
								/* Directory depth too big */

								ret = -ENAMETOOLONG;
								goto errout;
							}
#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
							dirstack[++depth] = smartfs_rdle16(&entry->firstsector);
#else
							dirstack[++depth] = entry->firstsector;
#endif
							segment = ptr + 1;
							break;
						}
					}

					/* Not this entry.  Skip to the next one */

					offset += entrysize;
					entry = (struct smartfs_entry_header_s *)
							&fs->fs_rwbuffer[offset];
				}

				/* Test if a directory entry was found and break if it was */

				if (offset < readwrite.count) {
					break;
				}
			}

			/* If we found a dir entry, then continue searching */

			if (offset < readwrite.count) {
				/* Update the segment pointer */

				if (*ptr != '\0') {
					ptr++;
				}

				segment = ptr;
				continue;
			}

			/* Entry not found!  Report the error.  Also, if this is the last
			 * segment, then report the parent directory sector.
			 */

			if (*ptr == '\0') {
				*parentdirsector = dirstack[depth];
				*filename = segment;
			} else {
				*parentdirsector = 0xFFFF;
				*filename = NULL;
			}

			ret = -ENOENT;
			goto errout;
		}
	}

errout:
	if (direntry->name != NULL) {
		kmm_free(direntry->name);
		direntry->name = NULL;
	}
	if ((filename != NULL) && (strlen(*filename) > fs->fs_llformat.namesize)) {
		fdbg("File name too long\n");
		return -ENAMETOOLONG;
	}
	return ret;
}

/****************************************************************************
 * Name: smartfs_createentry
 *
 * Description: Chains a new sector to the parent directory sector if
 *   no empty or invalidated entries were found and there is no space to
 *   allocate a new entry. Then returns the 1st entry of the new sector.
 *
 * Input Parameters:
 *   fs - pointer to smartfs mountpoint structure.
 *   parentdirsector - logical sector to which a new sector needs to be
 *     chained to allocate a new entry.
 *   direntry - pointer to smartfs entry to return details of allocated
 *     entry.
 *
 * Returned Values:
 *   OK - on successfully chaining a new sector and allocating an entry.
 *   ERROR - appropriate error code otherwise.
 *
 ****************************************************************************/

int smartfs_createentry(struct smartfs_mountpt_s *fs, uint16_t parentdirsector, struct smartfs_entry_s *direntry)
{
	struct smart_read_write_s readwrite;
	int ret;
	uint16_t nextsector;
	struct smartfs_chain_header_s *chainheader;

	/* Start at the 1st sector in the parent directory */
	/* If we are in the context of smartfs_createentry, then smartfs_find_availableentry did not find an available entry.
	   Createentry has to chain a new nextsector to parentdirsector, and allocate the first entry.
	 */

	smartfs_setbuffer(&readwrite, parentdirsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
	ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
	if (ret < 0) {
		fdbg("Unable to read parent sector\n");
		return ret;
	}

	chainheader = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
	nextsector = SMARTFS_NEXTSECTOR(chainheader);

	if (nextsector != SMARTFS_ERASEDSTATE_16BIT) {
		fdbg("Cannot chain new sector\n");
		return -EIO;
	}
	/* Allocate a new sector and chain it to the last one */

	ret = FS_IOCTL(fs, BIOC_ALLOCSECT, 0xFFFF);
	if (ret < 0) {
		return ret;
	}

	nextsector = (uint16_t)ret;

	/* Chain the next sector into this sector sector */
	chainheader->nextsector[0] = (uint8_t)(nextsector & 0x00FF);
	chainheader->nextsector[1] = (uint8_t)(nextsector >> 8);

	/* Here we are writing to the current sector's chainheader which needed a new sector in the chain.
	   Not writing to the newly chained sector, so it is completely empty as of now.
	 */

	smartfs_setbuffer(&readwrite, parentdirsector, offsetof(struct smartfs_chain_header_s, nextsector), sizeof(uint16_t), chainheader->nextsector);
	ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
	if (ret < 0) {
		fdbg("Error chaining sector %d\n", nextsector);
		return ret;
	}

	/* Automatically selecting first entry of new chained sector */
	direntry->doffset = sizeof(struct smartfs_chain_header_s);
	direntry->dsector = nextsector;
	direntry->flags = SMARTFS_ERASEDSTATE_16BIT;

	return OK;
}

/****************************************************************************
 * Name: smartfs_find_availableentry
 *
 * Description: Finds an invalid or empty entry in the parent directory that
 *   can be used to create a new file entry without allocating a new sector.
 *   The first one to be found is used.
 *
 * Input Parameters:
 *   fs - pointer to smartfs mountpoint structure.
 *   parentdirsetor - sector in which an available entry is to be found
 *   direntry - pointer to smartfs entry structure for returning details
 *     of the available entry found.
 *     If no available entry is found, it returns with the logical sector
 *     number of the last sector chained to parentdirsector.
 *   new_chain - pointer to boolean object to return whether a new sector
 *     was chained to the parent for the new entry or not.
 *
 * Returned Value:
 *   OK - on finding an available entry.
 *   -ENOENT - if no available entry is found
 *   ERROR code - otherwise
 *
 ****************************************************************************/

int smartfs_find_availableentry(struct smartfs_mountpt_s *fs, uint16_t parentdirsector, struct smartfs_entry_s *direntry, bool *new_chain)
{
	int ret = -ENOENT;
	struct smart_read_write_s readwrite;
	uint16_t psector;
	uint16_t nextsector;
	uint16_t offset;
	uint16_t entrysize;
	struct smartfs_entry_header_s *entry;
	struct smartfs_chain_header_s *chainheader;

	psector = parentdirsector;
	entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;

	/* Scan through the entire parent sector chain to find an available entry */
	while (psector != SMARTFS_ERASEDSTATE_16BIT) {
		/* Read the next sector */

		smartfs_setbuffer(&readwrite, psector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			return ret;
		}

		/* Get the next chained sector */
		chainheader = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
		nextsector = SMARTFS_NEXTSECTOR(chainheader);

		/* Search for an available entry in this sector */
		offset = sizeof(struct smartfs_chain_header_s);
		entry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];

		while ((offset + entrysize) < readwrite.count) {
			/* Check if this entry is available */
			if (IS_AVAILABLE_ENTRY(entry)) {
				direntry->dsector = psector;
				direntry->doffset = offset;
				direntry->flags = entry->flags;
				return OK;
			}

			/* Not an available entry, check the next one */
			offset += entrysize;
			entry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
		}
		direntry->dsector = psector;
		psector = nextsector;
	}

	if (ret != OK) {
		/* No available entry was found, so we create one */
		*new_chain = TRUE;
		ret = smartfs_createentry(fs, direntry->dsector, direntry);
		if (ret != OK) {
			fdbg("Createentry unable to create space for writing\n");
		}
	}

	return ret;
}

/****************************************************************************
 * Name: smartfs_writeentry
 *
 * Description: Writes the new entry to the entry allocated by
 *   find_availableentry or createentry.
 *
 * Input Parameters:
 *   fs - pointer to smartfs mountpoint structure.
 *   new_entry - the details of the entry in MTD where new entry is to be
 *     written.
 *   filename - name of new entry.
 *   type - type of new entry (file/dir).
 *   mode - mode of creation of new entry.
 *   direntry - pointer to smartfs entry object to return details of newly
 *     written entry.
 *
 * Returned Values:
 *   OK - on successfully writing new entry to MTD.
 *   ERROR - appropriate error code returned otherwise.
 *
 ****************************************************************************/

int smartfs_writeentry(struct smartfs_mountpt_s *fs, struct smartfs_entry_s new_entry, const char *filename, uint16_t type, mode_t mode, struct smartfs_entry_s *direntry, bool newchain)
{
	/* TODO: Need to check sf buffer and hold data until write buffer is full.
		Then perform filesystem sync.
	 */
	int ret;
	struct smart_read_write_s readwrite;
	uint16_t psector;
	uint16_t offset;
	uint16_t entrysize;
	struct smartfs_entry_header_s *entry;
	struct smartfs_chain_header_s *chainheader;
	char *tmp_buf = NULL;

	entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;
	psector = new_entry.dsector;
	offset = new_entry.doffset;

	/* If we are working with an invalidated/empty entry, the flags need to be set for the new entry. */
	if (IS_AVAILABLE_ENTRY(&new_entry)) {
		/* If it is an invalidated entry, flags need to be reset first. */
#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
		if (smartfs_rdle16(&new_entry.flags) != SMARTFS_ERASEDSTATE_16BIT) {
#else
		if (new_entry.flags != SMARTFS_ERASEDSTATE_16BIT) {
#endif
			RESET_ENTRY_FLAGS(&new_entry);
		}
		SET_ENTRY_FLAGS(new_entry.flags, mode, type);
	}

	if (newchain) {
		/* We cannot read the new sector into fs->fs_rwbuffer as it is totally empty.
		   MTD header will be written when MTD_WRITE is called for the sector 1st time.
		   Chainheader is written along with the 1st entry.
		   So currently, smart_readsector will fail to invalidate CRC on empty sector
		 */
		tmp_buf = (char *)kmm_malloc(entrysize + sizeof(struct smartfs_entry_header_s));
		if (tmp_buf == NULL) {
			fdbg("Unable to allocate memory\n");
			return -ENOMEM;
		}
		memset(tmp_buf, CONFIG_SMARTFS_ERASEDSTATE, entrysize + sizeof(struct smartfs_entry_header_s));
		entry = (struct smartfs_entry_header_s *)&tmp_buf[sizeof(struct smartfs_chain_header_s)];
		chainheader = (struct smartfs_chain_header_s *)&tmp_buf[0];
		chainheader->type = SMARTFS_SECTOR_TYPE_DIR;
	} else {
		smartfs_setbuffer(&readwrite, psector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Unable to read sector %d for writing the new entry\n", psector);
			return ret;
		}
		entry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
	}

	/* Populate the directory entry to be written in the parent's sector */

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
	smartfs_wrle16(&entry->firstsector, new_entry.firstsector);
#ifdef CONFIG_SMARTFS_ENTRY_TIMESTAMP
	smartfs_wrle32((uint8_t*)&entry->utc, fs->entry_seq++);
#else
	smartfs_wrle32((uint8_t*)&entry->utc, time(NULL));
#endif
#else
	entry->firstsector = new_entry.firstsector;
#ifdef CONFIG_SMARTFS_ENTRY_TIMESTAMP
	entry->utc = fs->entry_seq++;
#else
	entry->utc = time(NULL);
#endif
#endif

	memset(entry->name, 0, fs->fs_llformat.namesize);
	strncpy(entry->name, filename, fs->fs_llformat.namesize);
	entry->flags = new_entry.flags;

	/* Now write the new entry to the parent directory sector */
	if (newchain) {
		/* If this is a newly chained sector, write new entry and chain header both */
		smartfs_setbuffer(&readwrite, psector, 0, entrysize + sizeof(struct smartfs_chain_header_s), (uint8_t *)&tmp_buf[0]);
	} else {
		/* Otherwise, we only have to write the new entry */
		smartfs_setbuffer(&readwrite, psector, offset, entrysize, (uint8_t *)&fs->fs_rwbuffer[offset]);
	}
	ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long) &readwrite);
	if (ret < 0) {
		fdbg("failed to write new entry to parent directory psector : %d\n", new_entry.dsector);
		goto errout;
	}

	/* Now fill in the entry */
	direntry->firstsector = new_entry.firstsector;
	direntry->dsector = psector;
	direntry->doffset = offset;
#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
	direntry->flags = smartfs_rdle16(&entry->flags);
	direntry->utc = smartfs_rdle32(&entry->utc);
#else
	direntry->flags = entry->flags;
	direntry->utc = entry->utc;
#endif
	direntry->datlen = 0;
	if (direntry->name == NULL) {
		direntry->name = (FAR char *)kmm_malloc(fs->fs_llformat.namesize + 1);
		if (direntry->name == NULL) {
			ret = ERROR;
			goto errout;
		}
	}

	memset(direntry->name, 0, fs->fs_llformat.namesize + 1);
	strncpy(direntry->name, filename, fs->fs_llformat.namesize);

	ret = OK;

errout:
	if (tmp_buf) {
		kmm_free(tmp_buf);
	}
	return ret;
}

/****************************************************************************
 * Name: smartfs_alloc_firstsecttor
 *
 * Description: Allocate new data sector for a new file/directory entry.
 *
 * Input Parameters:
 *   fs - ponter to smartfs mountpoint structure.
 *   sector - pointer to return the sector number allocated.
 *   type - to set the type(file/dir) of the new sector allocated.
 *   sf - pointer to smartfs ofile structure in case a new file is being
 *     opened.
 *
 * Returned Values:
 *   OK - on successful allocation of a new sector.
 *   ERROR - apprpriate error code otherwise.
 *
 ****************************************************************************/

int smartfs_alloc_firstsector(struct smartfs_mountpt_s *fs, uint16_t *sector, uint16_t type, struct smartfs_ofile_s *sf)
{
	int ret;
	struct smart_read_write_s readwrite;
	uint16_t alloc_sector;
	uint8_t chainheader_type;
#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	struct smartfs_chain_header_s *chainheader;
#endif

	ret = FS_IOCTL(fs, BIOC_ALLOCSECT, 0xFFFF);
	if (ret < 0) {
		return ret;
	}

	alloc_sector = (uint16_t)ret;

	/* Set the newly allocated sector's type (file or dir) */

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	if (sf) {
		/* Using sector buffer and we have an open file context. Just update
		 * the sector buffer in the open file context.
		 */
		memset(sf->buffer, CONFIG_SMARTFS_ERASEDSTATE, fs->fs_llformat.availbytes);
		chainheader = (struct smartfs_chain_header_s *)sf->buffer;
		chainheader->type = SMARTFS_SECTOR_TYPE_FILE;
		sf->bflags = SMARTFS_BFLAG_DIRTY | SMARTFS_BFLAG_NEWALLOC;
	} else
#endif
	{
		if ((type & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_DIR) {
			chainheader_type = SMARTFS_SECTOR_TYPE_DIR;
		} else {
			chainheader_type = SMARTFS_SECTOR_TYPE_FILE;
		}

		smartfs_setbuffer(&readwrite, alloc_sector, offsetof(struct smartfs_chain_header_s, type), 1, (uint8_t *)&chainheader_type);
		ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error %d setting new sector type for sector %d\n", ret, alloc_sector);
			return ret;
		}
	}

	*sector = alloc_sector;
	return OK;
}

/****************************************************************************
 * Name: smartfs_invalidateentry
 *
 * Description: Invalidates the smartfs entry passed as a parameter.
 *
 * Input Parameters:
 *  fs - pointer to smartfs mountpoint structure.
 *  parentdirsector - parent sector of entry to be marked invalid.
 *  offset - offset of entry in parent directory sector
 *
 * Returned Values:
 *  OK - on sucessfully invalidating the entry.
 *  ERROR - appropriate error code otherwise
 *
 ****************************************************************************/

/* TODO IF entry is empty after invalid, we'd better release it. To do this entry needed as a parameter...*/
int smartfs_invalidateentry(struct smartfs_mountpt_s *fs, uint16_t parentdirsector, uint16_t offset)
{
	int ret;
	struct smart_read_write_s readwrite;
	uint8_t *entry_flags;

	smartfs_setbuffer(&readwrite, parentdirsector, offset, sizeof(uint16_t), (uint8_t *)fs->fs_rwbuffer);
	ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
	if (ret < 0) {
		fdbg("Error %d reading sector %d\n", ret, parentdirsector);
		return ret;
	}

	entry_flags = (uint8_t *)&fs->fs_rwbuffer[0];
#if CONFIG_SMARTFS_ERASEDSTATE == 0xFF
#if CONFIG_SMARTFS_ALIGNED_ACCESS
	smartfs_wrle16(entry_flags, smartfs_rdle16(entry_flags) & ~SMARTFS_DIRENT_ACTIVE);
#else
	*entry_flags &= ~SMARTFS_DIRENT_ACTIVE;
#endif
#else
#if CONFIG_SMARTFS_ALIGNED_ACCESS
	smartfs_wrle16(entry_flags, smartfs_rdle16(entry_flags) | SMARTFS_DIRENT_ACTIVE);
#else
	*entry_flags |= SMARTFS_DIRENT_ACTIVE;
#endif
#endif
	/* Now write the updated flags back to the device */

	smartfs_setbuffer(&readwrite, parentdirsector, offset, sizeof(uint16_t), (uint8_t *)&fs->fs_rwbuffer[0]);
	ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
	if (ret < 0) {
		fdbg("Error %d writing flag bytes for sector %d\n", ret, readwrite.logsector);
		return ret;
	}

	return ret;
}

/****************************************************************************
 * Name: smartfs_deleteentry
 *
 * Description: Deletes an entry from the filesystem (file or dir) by
 *              freeing all the entry's sectors and then marking it inactive
 *              in it's parent's directory sector.  For a directory, it
 *              does not validate the directory is empty, nor does it do
 *              a recursive delete.
 *
 ****************************************************************************/

int smartfs_deleteentry(struct smartfs_mountpt_s *fs, struct smartfs_entry_s *entry)
{
	int ret;
	uint16_t nextsector;
	uint16_t sector;
	uint16_t count;
	uint16_t entrysize;
	uint16_t offset;
	struct smartfs_entry_header_s *direntry;
	struct smartfs_chain_header_s *header;
	struct smart_read_write_s readwrite;
	bool inactive_entry = TRUE;
	/* Our policy is that integrity entry & chaining.
	 * If Isolated sector created but entry or chain is safe, we can handle it through the fs_recover.
	 * So We will always process regarding entry & chain first when delete entry.
	 */

	/* First Find current directory has only one item which is target entry */
	ret = OK;
	header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
	smartfs_setbuffer(&readwrite, entry->dsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
	ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
	if (ret < 0) {
		fdbg("Error reading directory info at sector %d\n", entry->dsector);
		goto errout;
	}

	if ((entry->dsector != fs->fs_rootsector) && (entry->dsector != entry->dfirst)) {
		count = 0;
		/* Scan the sector and count used entries */
		offset = sizeof(struct smartfs_chain_header_s);
		entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;
		while (offset + entrysize < fs->fs_llformat.availbytes) {
			/* Test the next entry */
			direntry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
			if (ENTRY_VALID(direntry)) {
				/* Count this entry */
				count++;
				if (count == 2) {
					/* If valid entry count exceeds 1, we know we cannot release the sector */
					break;
				}
			}

			/* Advance to next entry */
			offset += entrysize;
		}

		/* Test if the count is one.  If it is, then we will release the sector */
		if (count == 1) {
			/* We don't have to inactive entry, we will release entire of sector */
			inactive_entry = FALSE;

			/* Okay, to release the sector, we must find the sector that we
			 * are chained to and remove ourselves from the chain.
			 * First save our nextsector value so we can "unchain" ourselves.
			 */

			nextsector = SMARTFS_NEXTSECTOR(header);

			/* Now loop through the dir sectors to find ourselves in the chain */

			sector = entry->dfirst;
			while (sector != SMARTFS_ERASEDSTATE_16BIT) {
				/* Read the header for the next sector */

				smartfs_setbuffer(&readwrite, sector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
				ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
				if (ret < 0) {
					fdbg("Error reading sector %d\n", nextsector);
					goto errout;
				}

				/* Test if this sector "points" to us */

				if (SMARTFS_NEXTSECTOR(header) == entry->dsector) {
					/* We found ourselves in the chain.  Update the chain. */

					header->nextsector[0] = (uint8_t)(nextsector & 0x00FF);
					header->nextsector[1] = (uint8_t)(nextsector >> 8);

					smartfs_setbuffer(&readwrite, sector, offsetof(struct smartfs_chain_header_s, nextsector), sizeof(uint16_t), header->nextsector);
					ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
					if (ret < 0) {
						fdbg("Error unchaining sector (%d)\n", nextsector);
						goto errout;
					}

					/* Now release our sector */
					ret = FS_IOCTL(fs, BIOC_FREESECT, (unsigned long)entry->dsector);
					if (ret < 0) {
						fdbg("Error freeing sector %d\n", entry->dsector);
						goto errout;
					}

					/* Break out of the loop, we are done! */

					break;
				}

				/* Chain to the next sector */

				sector = SMARTFS_NEXTSECTOR(header);
			}
		} 
	}

	if (inactive_entry == TRUE) {
		/* OK, another entry exist, inactive current one */
		smartfs_setbuffer(&readwrite, entry->dsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
		/* TODO We read again here to mark that entry as inactive, let's find a way to avoid read 2 times */
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error reading directory info at sector %d\n", entry->dsector);
			goto errout;
		}

		/* Mark this entry as inactive */

		direntry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[entry->doffset];
#if CONFIG_SMARTFS_ERASEDSTATE == 0xFF
#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
		smartfs_wrle16(&direntry->flags, smartfs_rdle16(&direntry->flags) & ~SMARTFS_DIRENT_ACTIVE);
#else
		direntry->flags &= ~SMARTFS_DIRENT_ACTIVE;
#endif
#else							/* CONFIG_SMARTFS_ERASEDSTATE == 0xFF */
#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
		smartfs_wrle16(&direntry->flags, smartfs_rdle16(&direntry->flags) | SMARTFS_DIRENT_ACTIVE);
#else
		direntry->flags |= SMARTFS_DIRENT_ACTIVE;
#endif
#endif							/* CONFIG_SMARTFS_ERASEDSTATE == 0xFF */

		/* Write the updated flags back to the sector */

		smartfs_setbuffer(&readwrite, entry->dsector, entry->doffset, sizeof(uint16_t), (uint8_t *)&direntry->flags);
		ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error marking entry inactive at sector %d\n", entry->dsector);
			goto errout;
		}
	}
	/* Now Free Chained sector from target entry */

	nextsector = entry->firstsector;
	header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
	while (nextsector != SMARTFS_ERASEDSTATE_16BIT) {
		/* Read the next sector into our buffer */

		sector = nextsector;
		smartfs_setbuffer(&readwrite, sector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error reading sector %d\n", nextsector);
			goto errout;
		}

		/* Release this sector */

		nextsector = SMARTFS_NEXTSECTOR(header);
		ret = FS_IOCTL(fs, BIOC_FREESECT, sector);
		if (ret < 0) {
			fdbg("Error freeing sector %d\n", nextsector);
			goto errout;
		}
	}

errout:
	return ret;
}

/****************************************************************************
 * Name: smartfs_countdirentries
 *
 * Description: Counts the number of items in the specified directory entry.
 *              This routine assumes you have validated the entry you are
 *              passing is in fact a directory sector, though it checks
 *              just in case you were stupid :-)
 *
 ****************************************************************************/

int smartfs_countdirentries(struct smartfs_mountpt_s *fs, struct smartfs_entry_s *entry)
{
	int ret;
	uint16_t nextsector;
	uint16_t offset;
	uint16_t entrysize;
	int count;
	struct smartfs_entry_header_s *direntry;
	struct smartfs_chain_header_s *header;
	struct smart_read_write_s readwrite;

	/* Walk through the directory's sectors and count entries */

	count = 0;
	nextsector = entry->firstsector;
	while (nextsector != SMARTFS_ERASEDSTATE_16BIT) {
		/* Read the next sector into our buffer */

		smartfs_setbuffer(&readwrite, nextsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error reading sector %d\n", nextsector);
			break;
		}

		/* Validate this is a directory type sector */

		header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
		if (header->type != SMARTFS_SECTOR_TYPE_DIR) {
			fdbg("Sector %d is not a DIR sector!\n", nextsector);
			goto errout;
		}

		/* Loop for all entries in this sector and count them */

		offset = sizeof(struct smartfs_chain_header_s);
		entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;
		direntry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
		while (offset + entrysize < readwrite.count) {
			if (ENTRY_VALID(direntry)) {
				/* Count this entry */
				count++;
			}

			offset += entrysize;
			direntry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
		}

		/* Get the next sector from the header */

		nextsector = SMARTFS_NEXTSECTOR(header);
	}

	ret = count;

errout:
	return ret;
}

/****************************************************************************
 * Name: smartfs_truncatefile
 *
 * Description: Truncates an existing file on the device so that it occupies
 *              zero bytes and can be completely re-written.
 *
 ****************************************************************************/

int smartfs_truncatefile(struct smartfs_mountpt_s *fs, struct smartfs_entry_s *entry, FAR struct smartfs_ofile_s *sf)
{
	int ret;
	uint16_t nextsector;
	uint16_t sector;
	struct smartfs_chain_header_s *header;
	struct smart_read_write_s readwrite;

	/* Walk through the directory's sectors and count entries */

	nextsector = entry->firstsector;
	header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;

	while (nextsector != SMARTFS_ERASEDSTATE_16BIT) {
		/* Read the next sector's header into our buffer */

		smartfs_setbuffer(&readwrite, nextsector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
		if (ret < 0) {
			fdbg("Error reading sector %d header\n", nextsector);
			return ret;
		}

		/* Get the next chained sector */

		sector = SMARTFS_NEXTSECTOR(header);

		/* If this is the 1st sector of the file, then just overwrite
		 * the sector data with the erased state value.  The underlying
		 * SMART block driver will detect this and release the old
		 * sector and create a new one with the new (blank) data.
		 */

		if (nextsector == entry->firstsector) {
#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER

			/* When we have a sector buffer in use, simply skip the first sector */

			nextsector = sector;
			continue;

#else

			/* Fill our buffer with erased data */

			memset(fs->fs_rwbuffer, CONFIG_SMARTFS_ERASEDSTATE, fs->fs_llformat.availbytes);
			header->type = SMARTFS_SECTOR_TYPE_FILE;

			/* Now write the new sector data */

			readwrite.count = fs->fs_llformat.availbytes;
			ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
			if (ret < 0) {
				fdbg("Error blanking 1st sector (%d) of file\n", nextsector);
				return ret;
			}

			/* Set the entry's data length to zero ... we just truncated */

			entry->datlen = 0;
#endif							/* CONFIG_SMARTFS_USE_SECTOR_BUFFER */
		} else {
			/* Not the 1st sector -- release it */

			ret = FS_IOCTL(fs, BIOC_FREESECT, (unsigned long)nextsector);
			if (ret < 0) {
				fdbg("Error freeing sector %d\n", nextsector);
				return ret;
			}
		}

		/* Now move on to the next sector */

		nextsector = sector;
	}

	/* Now deal with the first sector in the event we are using a sector buffer
	   like we would be if CRC is enabled.
	 */

#ifdef CONFIG_SMARTFS_USE_SECTOR_BUFFER
	if (sf) {
		/* Using sector buffer and we have an open file context.  Just update
		 * the sector buffer in the open file context.
		 */

		smartfs_setbuffer(&readwrite, entry->firstsector, 0, sizeof(struct smartfs_chain_header_s), (uint8_t *)fs->fs_rwbuffer);
		ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);

		memset(sf->buffer, CONFIG_SMARTFS_ERASEDSTATE, fs->fs_llformat.availbytes);
		header = (struct smartfs_chain_header_s *)sf->buffer;
		header->type = SMARTFS_SECTOR_TYPE_FILE;
		sf->bflags = SMARTFS_BFLAG_DIRTY;
		entry->datlen = 0;
	}
#endif

	return OK;
}

/****************************************************************************
 * Name: smartfs_get_first_mount
 *
 * Description: Returns a pointer to the first mounted smartfs volume.
 *
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_SMARTFS)
struct smartfs_mountpt_s *smartfs_get_first_mount(void)
{
	return g_mounthead;
}

/****************************************************************************
 * Name: smartfs_invalidate_old_entry
 *
 * Description: If we found 2 entry that is point to same sector,
 *              we compare timestamp, and release old one.
 *
 ****************************************************************************/

int smartfs_invalidate_old_entry(struct smartfs_mountpt_s *fs, uint16_t logsector, sq_queue_t *entry_queue, struct sector_recover_info_s *info)
{
	struct sector_entry_queue_s *tmp;
	struct sector_entry_queue_s *firstparent = NULL;
	struct sector_entry_queue_s *secondparent = NULL;
	int ret = OK;
	int count = 0;

	/* Find same parent sector that is point to us */
	tmp = (struct sector_entry_queue_s *)sq_peek(entry_queue);
	while (tmp != NULL) {
		if (tmp->logsector == logsector) {
			fvdbg("found logsector : %d parent : %d\n", logsector, tmp->parentsector);
			if (count == 0) {
				firstparent = tmp;
				count++;
			} else if (count == 1) {
				secondparent = tmp;
				count++;
				break;
			}
		}
		tmp = (struct sector_entry_queue_s *)sq_next(tmp);
	}

	/* OK, we found 2 parents, now release old one, that is probably source path of rename */
	if (count == 2) {
		fvdbg("firstparent logsector : %d parentsector : %d parentoffset : %d time : %d\n", firstparent->logsector,\
			firstparent->parentsector, firstparent->parentoffset, firstparent->time);
		fvdbg("secondparent logsector : %d parentsector : %d parentoffset : %d time : %d\n", secondparent->logsector,\
			secondparent->parentsector, secondparent->parentoffset, secondparent->time);
		if (firstparent->time > secondparent->time) {
			ret = smartfs_invalidateentry(fs, secondparent->parentsector, secondparent->parentoffset);
			if (ret == OK) {
				sq_rem((FAR sq_entry_t *)secondparent, entry_queue);
				free(secondparent);
				info->cleanentries++;
			}
		} else {
			ret = smartfs_invalidateentry(fs, firstparent->parentsector, firstparent->parentoffset); 
			if (ret == OK) {
				sq_rem((FAR sq_entry_t *)firstparent, entry_queue);
				free(firstparent);
				info->cleanentries++;
			}
		}
	}
	return ret;
}

/****************************************************************************
 * Name: smartfs_scan_entry
 *
 * Description: Scan all active logical sectors.
 *              Compare with physical sector Map from blockdriver, mark sector
 *              which is aligned with logical sector.
 *              If current sector has invalid child or next sector, clean it.
 *
 ****************************************************************************/

int smartfs_scan_entry(struct smartfs_mountpt_s *fs, char *map, struct sector_recover_info_s *info)
{
	int ret = OK;
	struct sector_entry_queue_s *nodeitem;
	struct sector_entry_queue_s *node;
	sq_queue_t entry_queue;
	struct smart_read_write_s readwrite;
	struct smartfs_entry_header_s *entry;
	struct smartfs_chain_header_s *header;
	uint32_t timestamp;
	uint16_t logsector;
	uint16_t nextsector;
	uint16_t firstsector;
	uint16_t offset;
	uint8_t type;
	uint8_t entrytype;
	uint16_t entrysize = sizeof(struct smartfs_entry_header_s) + fs->fs_llformat.namesize;
	
	sq_init(&entry_queue);

	/* Fill Node for Rootsector and add to queue */
	nodeitem = (struct sector_entry_queue_s *)kmm_malloc(sizeof(struct sector_entry_queue_s));
	if (!nodeitem) {
		return -ENOMEM;
	}
	nodeitem->logsector = fs->fs_rootsector;
	nodeitem->parentsector = 0;
	nodeitem->parentoffset = 0;
	nodeitem->type = SMARTFS_SECTOR_TYPE_DIR;
	nodeitem->time = 0;
	sq_addlast((FAR sq_entry_t *)nodeitem, &entry_queue);
	
	node = (struct sector_entry_queue_s *)sq_peek(&entry_queue);
	while (node != NULL) {
		logsector = node->logsector;
		type = node->type;

		/* Actually This Must not happened, But skip recovery process to clean later */
		if (type != SMARTFS_SECTOR_TYPE_DIR && type != SMARTFS_SECTOR_TYPE_FILE) {
			fdbg("Critical Bug..What should we do? type : %d\n", type);
			continue;
		}

		while ((logsector != SMARTFS_ERASEDSTATE_16BIT) && (logsector < fs->fs_llformat.nsectors)) {
			/* Read all of entries in current sector */
			smartfs_setbuffer(&readwrite, logsector, 0, fs->fs_llformat.availbytes, (uint8_t *)fs->fs_rwbuffer);
			ret = FS_IOCTL(fs, BIOC_READSECT, (unsigned long)&readwrite);
			if (ret < 0) {
				fdbg("Error %d reading sector %d data\n", ret, logsector);
				goto errout;
			}
			
			/* Check it is already marked */
			ret = IS_SECTOR_REMAIN(map, logsector);
			if (ret == 0) {
				/* Already marked by other sector, we will find them & release it */
				fvdbg("Already Marked. ret : %d logsector : %d\n", ret, logsector);
				ret = smartfs_invalidate_old_entry(fs, logsector, &entry_queue, info);
			} else if (ret != SMARTFS_ERASEDSTATE_16BIT) {
				/* Otherwise, mark it */
				fvdbg("Set to Used!! logsector : %d ret : %d\n", logsector, ret);
				SET_TO_USED(map, logsector);
			}
			
			/* Point header to the read data to get used byte count */
			header = (struct smartfs_chain_header_s *)fs->fs_rwbuffer;
			nextsector = SMARTFS_NEXTSECTOR(header);

			/* If it is directory, we will check saved entry in current sector and put each entry to queue */
			if (type == SMARTFS_SECTOR_TYPE_DIR) {
				offset = sizeof(struct smartfs_chain_header_s);
				/* Test if this entry is valid and active */
				while (offset < readwrite.count) {
					entry = (struct smartfs_entry_header_s *)&fs->fs_rwbuffer[offset];
					if (!(ENTRY_VALID(entry))) {
						/* This entry isn't valid, skip it */
						offset += entrysize;
						continue;
					}
#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
					firstsector = smartfs_rdle16(&entry->firstsector);
					if ((smartfs_rdle16(&entry->flags) & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_FILE)
#else
					firstsector = entry->firstsector;
					if ((entry->flags & SMARTFS_DIRENT_TYPE) == SMARTFS_DIRENT_TYPE_FILE)
#endif
					{
						entrytype = SMARTFS_SECTOR_TYPE_FILE;
					} else {
						entrytype = SMARTFS_SECTOR_TYPE_DIR;
					}

#ifdef CONFIG_SMARTFS_ALIGNED_ACCESS
					timestamp = smartfs_rdle32(&entry->utc);
#else
					timestamp = entry->utc;
#endif
					
					/* Update timestamp as Lastest*/
#ifdef CONFIG_SMARTFS_ENTRY_TIMESTAMP
					if (fs->entry_seq <= timestamp) {
						fs->entry_seq = timestamp + 1;
					}
#endif	
					/** Check validation of Logical sector of first sector to block driver.
					  * Use IOCTL instead of IS_SECTOR_REMAIN to find entries point to same sector.
					  */
					ret = FS_IOCTL(fs, BIOC_FIBMAP, (unsigned long)firstsector);
					if (ret < 0) {
						fdbg("Get Bitmap Error ret : %d sector : %d\n", ret, firstsector);
						goto errout;
					}

					/* It wasn't written, so clean current entry */
					if ((ret == SMARTFS_ERASEDSTATE_16BIT) || (ret >= fs->fs_llformat.nsectors)) {
						fdbg("Invalid entry, firstsector is not exist sector : %d firstsector : %d\n", logsector, firstsector);
						ret = smartfs_invalidateentry(fs, logsector, offset);
						if (ret < 0) {
							fdbg("Error marking entry inactive at sector %d\n", firstsector);
							goto errout;
						}

					} else {
						/* Make New node of Entry's first sector and put it to queue */
						nodeitem = (struct sector_entry_queue_s *)kmm_malloc(sizeof(struct sector_entry_queue_s));
						if (!node) {
							fdbg("failed to alloc node.\n");
							ret = -ENOMEM;
							goto errout;
						}
						nodeitem->logsector = firstsector;
						nodeitem->type = entrytype;
						nodeitem->parentsector = logsector;
						nodeitem->parentoffset = offset;
						nodeitem->time = timestamp;
						sq_addlast((FAR sq_entry_t *)nodeitem, &entry_queue);
						fvdbg("new node added for currentsector : %d firsector : %d offset : %d\n", logsector, firstsector, offset);
					}
					offset += entrysize;

				}
			} 

			/* Check next sector is valid or Not */
			ret = FS_IOCTL(fs, BIOC_FIBMAP, (unsigned long)nextsector);
			if (ret < 0) {
				fdbg("Get Bitmap Error ret : %d sector : %d\n", ret, nextsector);
				goto errout;
			}
			fvdbg("Nextsector : %d ret : 0x%x\n", nextsector, ret);

			/* Previous Header was updated but Next sector wasn't written, clean chain */
			if ((nextsector != SMARTFS_ERASEDSTATE_16BIT) && ((ret == SMARTFS_ERASEDSTATE_16BIT) || (ret >= fs->fs_llformat.nsectors))) {
				fvdbg("next sector [%d] is not exist, reset sector [%d] header is_remain : %d\n", nextsector, logsector, IS_SECTOR_REMAIN(map, nextsector));
				header->nextsector[0] = CONFIG_SMARTFS_ERASEDSTATE;
				header->nextsector[1] = CONFIG_SMARTFS_ERASEDSTATE;
				smartfs_setbuffer(&readwrite, logsector, offsetof(struct smartfs_chain_header_s, nextsector), sizeof(uint16_t), header->nextsector);
				nextsector = SMARTFS_ERASEDSTATE_16BIT;
				ret = FS_IOCTL(fs, BIOC_WRITESECT, (unsigned long)&readwrite);
				if (ret < 0) {
					fdbg("Error unchaining sector (%d)\n", nextsector);
					goto errout;
				}

				break;
			}

			logsector = nextsector;
		}
		
		node = (struct sector_entry_queue_s *)sq_next(node);

	}
	ret = OK;
errout:
	while (!sq_empty(&entry_queue)) {
		node = (struct sector_entry_queue_s *)sq_remfirst(&entry_queue);
		if (node) {
			fdbg("free node's sector : %d\n", node->logsector);
			kmm_free(node);
		}
	}
	return ret;
}

/****************************************************************************
 * Name: smartfs_sector_recovery
 *
 * Description: Recover Isolated sector that is not able to access in smartfs.
 *              Because of power off, Isolated sector can be exist.
 *              This works stand alone, but it works properly with journaling
 *
 ****************************************************************************/

int smartfs_sector_recovery(struct smartfs_mountpt_s *fs)
{
	int ret;
	long sector;
	struct sector_recover_info_s info = {0,};
	size_t size = (fs->fs_llformat.nsectors / 8) + 1;
	
	/* Alloc Logical Map */
	char *map = (char *)kmm_malloc(sizeof(uint8_t *) * size);
	if (map == NULL) {
		return -ENOMEM;
	}
	fvdbg("sector recover start\n");
	memset(map, 0, size);

	/* If any of logical sector is mapped to physical block it means active block, Mark it */
	for (sector = SMARTFS_ROOT_DIR_SECTOR; sector < fs->fs_llformat.nsectors; sector++) {
		ret = FS_IOCTL(fs, BIOC_FIBMAP, (unsigned long)sector);
		if (ret < 0) {
			fdbg("Get Bitmap Error %d\n", ret);
			goto error_with_map;
		}

		if ((ret != SMARTFS_ERASEDSTATE_16BIT) && (ret < fs->fs_llformat.nsectors)) {
			SET_TO_REMAIN(map, sector);
			fvdbg("sector : %d ret : 0x%x\n", sector, ret);
			info.totalsector++;
		} else {
			/* TODO Root sector(/mnt) is gone.. what should we do alloc again?? */
			if (sector == SMARTFS_ROOT_DIR_SECTOR) {
				fdbg("Critical Bug!! Root sector has gone!!\n");
				ret = -EIO;
				goto error_with_map;
			}
		}
	}

	/* TODO Find all active Logical sectors from root sector and unmark for exist sector */
	ret = smartfs_scan_entry(fs, map, &info);
	for (sector = SMARTFS_ROOT_DIR_SECTOR; sector < fs->fs_llformat.nsectors; sector++) {
		if (IS_SECTOR_REMAIN(map, sector)) {
			fdbg("Recover sector : %d\n", sector);
			info.isolatedsector++;
			ret = FS_IOCTL(fs, BIOC_FREESECT, sector);
			if (ret < 0) {
				fdbg("Error freeing sector %d\n", sector);
				goto error_with_map;
			}
		}

	}
	fdbg("###############################\n");
	fdbg("#      FS Recovery Report     #\n");
	fdbg("###############################\n");
	fdbg("Total of active sectors : %d\n", info.totalsector);
	fdbg("Recovered Isolated Sectors : %d\n", info.isolatedsector);
	fdbg("Cleaned Entries : %d\n\n", info.cleanentries);

error_with_map:
	if (map) {
		kmm_free(map);
	}
	return ret;
}

#endif
