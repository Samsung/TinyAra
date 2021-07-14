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
 *  apps/system/utils/fscmd.c
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
/// @file fscmd.c
/// @brief Cursor functions.
#include <tinyara/config.h>
#if CONFIG_NFILE_DESCRIPTORS > 0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <tinyara/fs/fs_utils.h>
#ifdef CONFIG_TASH
#include <apps/shell/tash.h>
#endif
#ifndef CONFIG_DISABLE_MOUNTPOINT
#include <sys/mount.h>
#endif

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define LSFLAGS_SIZE          1
#define LSFLAGS_LONG          2
#define LSFLAGS_RECURSIVE     4

#ifdef CONFIG_FSCMD_BUFFER_LEN
#define FSCMD_BUFFER_LEN      CONFIG_FSCMD_BUFFER_LEN
#else
#define FSCMD_BUFFER_LEN      256
#endif

#define FSCMD_ECHO_USAGE "Usage:  echo [OPTIONS] [input_text] [> or >>] > [target_file_path]\n\tOPTIONS: '-n' - do not output the trailing newline.\n\t\t '--help' - displays usage.\n"
#define FSCMD_CAT_USAGE "Usage:  cat [OPTIONS] [source_file_path] [> or >>] [target_file_path]\n\tOPTIONS: '--help' - displays usage.\n"
#define FSCMD_MV_USAGE "Usage:  mv [source_file_path] [target_file_path]\n"


/** Wrapper to prevent remove information by users **/
#define FSCMD_OUTPUT(...) printf(__VA_ARGS__)

/** Define Error String **/
#define TOO_MANY_ARGS   "%s : Too many Arguments\n"
#define INVALID_ARGS    "%s : Invalid Arguments\n"
#define MISSING_ARGS    "%s : Missing required argument(s)\n"
#define CMD_FAILED      "%s : %s failed\n"
#define OUT_OF_MEMORY   "%s : out of memory\n"
#define OUT_OF_RANGE    "%s : value out of range\n"
#define CMD_FAILED_ERRNO  "%s : %s failed: %d\n"

/** Define FS Type **/
#define NONEFS_TYPE     "None FS"
#define SMARTFS_TYPE    "smartfs"
#define PROCFS_TYPE     "procfs"
#define ROMFS_TYPE      "romfs"
#define TMPFS_TYPE      "tmpfs"

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef int (*direntry_handler_t)(const char *, struct dirent *, void *);

typedef enum {
	FSCMD_NONE     = 0,
	FSCMD_TRUNCATE = 1,
	FSCMD_APPEND   = 2
} redirection_mode_t;

struct fscmd_redirection {
	redirection_mode_t mode;
	int index;
};

typedef struct fscmd_redirection redirection_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void fscmd_free(FAR char *path)
{
	if (path) {
		free(path);
	}
}

#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_echo
 *
 * Description:
 *   Display input text or Redirects to a target file
 *
 * Usage:
 *   echo [OPTIONS] [input_text] [> or >>] [target_file_path]
 *   OPTIONS: '-n' - do not output the trailing newline.
 *            '--help' - displays usage.
 ****************************************************************************/
static int tash_echo(int argc, char **args)
{
	char *dest_fullpath = NULL;
	redirection_t direction = { FSCMD_NONE, argc };
	char fscmd_buffer[FSCMD_BUFFER_LEN];
	int i;
	int fd = 1;
	int flags;
	int len = 0;
	int arg_len = 0;
	int n_opt = 1;
	int ret = ERROR;

	if (argc == 1) {
		FSCMD_OUTPUT("\n");
		return OK;
	}
	if (!strncmp(args[1], "--help", strlen("--help") + 1)) {
		FSCMD_OUTPUT(FSCMD_ECHO_USAGE);
		return OK;
	}

	if (!strncmp(args[1], "-n", 2)) {
		n_opt = 2;
	}

	for (i = n_opt; i < argc; i++) {
		if (strcmp(args[i], ">") == 0) {
			direction.mode = FSCMD_TRUNCATE;
			direction.index = i;
			break;
		} else if (strcmp(args[i], ">>") == 0) {
			direction.mode = FSCMD_APPEND;
			direction.index = i;
			break;
		}
	}

	if (direction.mode != FSCMD_NONE && direction.index == argc - 2) {
		/* Redirection case */
		flags = O_WRONLY | O_CREAT;
		if (direction.mode == FSCMD_TRUNCATE) {
			flags |= O_TRUNC;
		} else {
			flags |= O_APPEND;
		}

		/* copy contents to target file
		 * echo <input_text> <redirection> <filepath> */
		dest_fullpath = get_fullpath(args[argc - 1]);
		if (!dest_fullpath) {
			FSCMD_OUTPUT(OUT_OF_MEMORY, args[argc - 1]);
			return ret;
		}

		fd = open(dest_fullpath, flags);
		if (fd < 0) {
			FSCMD_OUTPUT(CMD_FAILED_ERRNO, "echo", "open", errno);
			goto error;
		}
	 } else if (direction.mode != FSCMD_NONE && direction.index != argc - 2) {
		FSCMD_OUTPUT(INVALID_ARGS FSCMD_ECHO_USAGE, args[0]);
		return ret;
	 }

	for (i = n_opt; i < direction.index && len < FSCMD_BUFFER_LEN; i++) {
		if (i != n_opt) {
			memcpy(fscmd_buffer + len, " ", 1);
			len += 1;
		}

		arg_len = strlen(args[i]);
		if ((len + arg_len) > FSCMD_BUFFER_LEN) {
			FSCMD_OUTPUT("%s : Too long input text\n", args[0]);
			goto error_with_close;
		}
		memcpy(fscmd_buffer + len, args[i], arg_len);
		len += arg_len;
	}

	if (1 == n_opt) {
		if (len >= FSCMD_BUFFER_LEN) {
			FSCMD_OUTPUT("%s : Too long input text\n", args[0]);
			goto error_with_close;
		}
		memcpy(fscmd_buffer + len, "\n", 1);
		len += 1;
	}

	if (write(fd, fscmd_buffer, len) < 0) {
		FSCMD_OUTPUT(CMD_FAILED_ERRNO, args[0], "write", errno);
		goto error_with_close;
	}

	ret = OK;

error_with_close:
	if (fd > 2) {
		close(fd);
	}
error:
	fscmd_free(dest_fullpath);
	return ret;
}
#endif

#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_cat
 *
 * Description:
 *   copies and concatenates file or redirect file to another file
 *
 * Usage:
 *   cat [OPTIONS] [source_file_path] [> or >>] [target_file_path]
 *   OPTIONS: '--help' - display the usage.
 ****************************************************************************/
static int tash_cat(int argc, char **args)
{
	char *src_fullpath = NULL;
	char *dest_fullpath = NULL;
	char fscmd_buffer[FSCMD_BUFFER_LEN];
	redirection_t direction;
	int fd;
	int destfd;
	int i;
	int flags;
	ssize_t read_size;
	ssize_t written_size = 0;
	int ret;

	direction.mode = FSCMD_NONE;
	direction.index = -1;
	for (i = 1; i < argc; i++) {
		if (strcmp(args[i], ">") == 0) {
			direction.mode = FSCMD_TRUNCATE;
			direction.index = i;
			break;
		} else if (strcmp(args[i], ">>") == 0) {
			direction.mode = FSCMD_APPEND;
			direction.index = i;
			break;
		}
	}

	if (argc < 2) {
		FSCMD_OUTPUT(MISSING_ARGS FSCMD_CAT_USAGE, args[0]);
		return ERROR;
	} else if (argc == 2) {
		if (!strncmp(args[1], "--help", strlen("--help") + 1)) {
			FSCMD_OUTPUT(FSCMD_CAT_USAGE);
			return OK;
		}
		/* Basic case, cat <filepath> */
		if (direction.mode != FSCMD_NONE) {
			FSCMD_OUTPUT(INVALID_ARGS FSCMD_CAT_USAGE, args[0]);
			return ERROR;
		}

		src_fullpath = get_fullpath(args[1]);
		if (!src_fullpath) {
			FSCMD_OUTPUT(OUT_OF_MEMORY, args[1]);
			return ERROR;
		}

		fd = open(src_fullpath, O_RDONLY);
		if (fd < 0) {
			FSCMD_OUTPUT(CMD_FAILED, "open", src_fullpath);
			ret = ERROR;
			goto out_free;
		}
		do {
			memset(fscmd_buffer, 0, FSCMD_BUFFER_LEN);
			read_size = read(fd, fscmd_buffer, FSCMD_BUFFER_LEN - 1);
			if (read_size > 0) {
				fscmd_buffer[read_size] = '\0';
				FSCMD_OUTPUT("%s", fscmd_buffer);
			}
		} while (read_size > 0);

		FSCMD_OUTPUT("\n");
	} else if (argc == 4) {
		/* Below is redirection case */
		flags = O_WRONLY | O_CREAT;
		if (direction.mode == FSCMD_TRUNCATE) {
			flags |= O_TRUNC;
		} else {
			flags |= O_APPEND;
		}

		if (direction.index == 2) {
			/* copy contents from source file to target file
			 * cat <source filepath> <redirection> <target filepath> */
			if (strcmp(args[1], args[3]) == 0) {
				FSCMD_OUTPUT(INVALID_ARGS "Same File name", args[1]);
				return ERROR;
			}

			src_fullpath = get_fullpath(args[1]);
			if (!src_fullpath) {
				FSCMD_OUTPUT(OUT_OF_MEMORY, args[1]);
				return ERROR;
			}

			fd = open(src_fullpath, O_RDONLY);
			if (fd < 0) {
				FSCMD_OUTPUT(CMD_FAILED, "open", src_fullpath);
				ret = ERROR;
				goto out_free;
			}

			dest_fullpath = get_fullpath(args[3]);
			if (!dest_fullpath) {
				FSCMD_OUTPUT(OUT_OF_MEMORY, args[3]);
				ret = ERROR;
				goto out_close;
			}

			destfd = open(dest_fullpath, flags);
			if (destfd < 0) {
				FSCMD_OUTPUT(CMD_FAILED, "open", dest_fullpath);
				ret = ERROR;
				goto out_close;
			}

			do {
				memset(fscmd_buffer, 0, FSCMD_BUFFER_LEN);
				read_size = read(fd, fscmd_buffer, FSCMD_BUFFER_LEN);
				if (read_size > 0) {
					written_size = write(destfd, fscmd_buffer, read_size);
					if (written_size != read_size) {
						FSCMD_OUTPUT(CMD_FAILED, "write", dest_fullpath);
						ret = ERROR;
						close(destfd);
						goto out_close;
					}
				}
				
			} while (read_size > 0);

			close(destfd);
		} else {
			FSCMD_OUTPUT(INVALID_ARGS " : [> or >>] [file] [contents]\n", args[0]);
			return ERROR;
		}
	} else {
		/* Wrong case */
		FSCMD_OUTPUT(INVALID_ARGS FSCMD_CAT_USAGE, args[0]);
		return ERROR;
	}

	ret = OK;

out_close:
	close(fd);
out_free:
	fscmd_free(src_fullpath);
	fscmd_free(dest_fullpath);
	return ret;
}
#endif
#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_cd
 *
 * Description:
 *   change current working directory
 *
 * Usage:
 *   cd <directory or - or .. or ~>
 ****************************************************************************/
static int tash_cd(int argc, char **args)
{
	const char *path = args[1];
	char *temp = NULL;
	int ret;

	if (argc < 2 || strcmp(path, "~") == 0) {
		path = CONFIG_LIB_HOMEDIR;
	} else if (strcmp(path, "-") == 0) {
		temp = strdup(getwd(OLD_PWD));
		path = temp;
	} else if (strcmp(path, "..") == 0) {
		temp = strdup(getwd(PWD));
		path = dirname(temp);
	} else {
		temp = get_fullpath(path);
		path = temp;
	}
	ret = chdir(path);
	if (ret != 0) {
		FSCMD_OUTPUT(CMD_FAILED, args[0], path);
		ret = ERROR;
	}

	fscmd_free(temp);

	return ret;
}
#endif
#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: foreach_direntry
 *
 * Description:
 *   foreach_direntry : check whole contents in current directory
 *
 ****************************************************************************/
static int foreach_direntry(const char *cmd, const char *dirpath, direntry_handler_t handler, void *pvarg)
{
	DIR *dirp;
	int ret = OK;

	/* Trim trailing '/' from directory names */

	/* Open the directory */

	dirp = opendir(dirpath);

	if (!dirp) {
		/* Failed to open the directory */
		FSCMD_OUTPUT("\t TASH %s: no such directory: %s\n", cmd, dirpath);

		return ERROR;
	}

	/* Read each directory entry */

	for (;;) {
		struct dirent *entryp = readdir(dirp);
		if (!entryp) {
			/* Finished with this directory */
			break;
		}

		/* Call the handler with this directory entry */
		if (handler(dirpath, entryp, pvarg) < 0) {
			/* The handler reported a problem */
			ret = ERROR;
			break;
		}
	}
	closedir(dirp);

	return ret;
}

/****************************************************************************
 * Name: ls_specialdir / ls_handler / ls_recursive
 *
 * Description:
 *    ls_handler displays contents of specific directory.
 *    ls_recursive using ls_handler recursively to display whole contents
 *    under the specific directoy
 *    ls_specialdir checks directory's name started '.' or '..'
 *
 ****************************************************************************/
static int ls_specialdir(const char *dir)
{
	/* '.' and '..' directories are not listed like normal directories */
	return ((strcmp(dir, ".") == 0) || (strcmp(dir, "..") == 0));
}

static int ls_handler(FAR const char *dirpath, FAR struct dirent *entryp, FAR void *pvarg)
{
	unsigned int lsflags = (unsigned int)pvarg;
	int ret;

	/* Check if any options will require that we stat the file */

	if ((lsflags & (LSFLAGS_SIZE | LSFLAGS_LONG)) != 0) {
		struct stat buf;

		/* stat the file */

		if (entryp != NULL) {
			char *fullpath = get_dirpath(dirpath, entryp->d_name);
			ret = stat(fullpath, &buf);
			fscmd_free(fullpath);
		} else {
			/*
			 * A NULL entryp signifies that we are
			 * running ls on a single file
			 */
			ret = stat(dirpath, &buf);
		}

		if (ret != 0) {
			FSCMD_OUTPUT(CMD_FAILED, "ls", "stat");

			return ERROR;
		}

		if ((lsflags & LSFLAGS_LONG) != 0) {
			char details[] = "----------";
			if (S_ISDIR(buf.st_mode)) {
				details[0] = 'd';
			} else if (S_ISCHR(buf.st_mode)) {
				details[0] = 'c';
			} else if (S_ISBLK(buf.st_mode)) {
				details[0] = 'b';
			} else if (!S_ISREG(buf.st_mode)) {
				details[0] = '?';
			}

			if ((buf.st_mode & S_IRUSR) != 0) {
				details[1] = 'r';
			}

			if ((buf.st_mode & S_IWUSR) != 0) {
				details[2] = 'w';
			}

			if ((buf.st_mode & S_IXUSR) != 0) {
				details[3] = 'x';
			}

			if ((buf.st_mode & S_IRGRP) != 0) {
				details[4] = 'r';
			}

			if ((buf.st_mode & S_IWGRP) != 0) {
				details[5] = 'w';
			}

			if ((buf.st_mode & S_IXGRP) != 0) {
				details[6] = 'x';
			}

			if ((buf.st_mode & S_IROTH) != 0) {
				details[7] = 'r';
			}

			if ((buf.st_mode & S_IWOTH) != 0) {
				details[8] = 'w';
			}

			if ((buf.st_mode & S_IXOTH) != 0) {
				details[9] = 'x';
			}

			FSCMD_OUTPUT(" %s", details);
		}

		if ((lsflags & LSFLAGS_SIZE) != 0) {
			FSCMD_OUTPUT("%8d", buf.st_size);
		}
	}

	/*
	 * Then provide the filename that is common
	 * to normal and verbose output
	 */

	if (entryp != NULL) {
		FSCMD_OUTPUT(" %s", entryp->d_name);

		if (DIRENT_ISDIRECTORY(entryp->d_type) && !ls_specialdir(entryp->d_name)) {
			FSCMD_OUTPUT("/\n");
		} else {
			FSCMD_OUTPUT("\n");
		}
	} else {
		/* A single file */
		FSCMD_OUTPUT(" %s\n", dirpath);
	}

	return OK;
}

static int ls_recursive(const char *dirpath, struct dirent *entryp, void *pvarg)
{
	int ret = OK;

	/*
	 * Is this entry a directory (and not one of the special
	 * directories, . and ..)?
	 */

	if (DIRENT_ISDIRECTORY(entryp->d_type) && !ls_specialdir(entryp->d_name)) {
		/* Yes.. */

		char *newpath;
		newpath = get_dirpath(dirpath, entryp->d_name);

		/* List the directory contents */

		FSCMD_OUTPUT("%s:\n", newpath);

		/* Traverse the directory */

		ret = foreach_direntry("ls", newpath, ls_handler, pvarg);
		if (ret == 0) {
			/*
			 * Then recurse to list each directory
			 * within the directory
			 */

			ret = foreach_direntry("ls", newpath, ls_recursive, pvarg);
		}
		fscmd_free(newpath);
	}

	return ret;
}

/****************************************************************************
 * Name: tash_ls
 *
 * Description:
 *   Show contents of directory of specific directory.
 *   -R : shows contents recursively.
 *   -l  : shows size & attributes of contents
 *   -s : size of contents
 *
 * Usage:
 *   ls [-lRs] <directory>
 ****************************************************************************/
static int tash_ls(int argc, char **args)
{
	struct stat st;
	const char *relpath;
	unsigned int lsflags = 0;
	char *fullpath;
	int ret;
	int option;
	int badarg = false;

	optind = -1;
	while ((option = getopt(argc, args, "lRs")) != ERROR) {
		switch (option) {
		case 'l':
			lsflags |= (LSFLAGS_SIZE | LSFLAGS_LONG);
			break;

		case 'R':
			lsflags |= LSFLAGS_RECURSIVE;
			break;

		case 's':
			lsflags |= LSFLAGS_SIZE;
			break;

		case '?':
		default:
			badarg = true;
			break;
		}
	}
	if (badarg) {
		FSCMD_OUTPUT(INVALID_ARGS " : [-lRs] <dir-path>\n", args[0]);
		return 0;
	}

	if (optind + 1 < argc) {
		FSCMD_OUTPUT(TOO_MANY_ARGS, args[0]);
		return ERROR;
	} else if (optind >= argc) {
		relpath = getwd(PWD);
	} else {
		relpath = args[optind];
	}

	fullpath = get_fullpath(relpath);
	if (!fullpath) {
		return ERROR;
	}
	if (stat(fullpath, &st) < 0) {
		FSCMD_OUTPUT(CMD_FAILED, args[0], "stat");
		ret = ERROR;
	} else if (!S_ISDIR(st.st_mode)) {
		/*
		 * Pass a null dirent to ls_handler to signify
		 * that this is a single file
		 */
		ret = ls_handler(fullpath, NULL, (void *)lsflags);
	} else {
		/* List the directory contents */
		FSCMD_OUTPUT("%s:\n", fullpath);
		ret = foreach_direntry("ls", fullpath, ls_handler, (void *)lsflags);
		if (ret == OK && (lsflags & LSFLAGS_RECURSIVE) != 0) {
			/*
			 * Then recurse to list each directory
			 * within the directory
			 */
			ret = foreach_direntry("ls", fullpath, ls_recursive, (void *)lsflags);
		}
	}

	fscmd_free(fullpath);

	return ret;
}
#endif
#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_mkdir
 *
 * Description:
 *   create directory
 *
 * Usage:
 *   mkdir [directory name]
 ****************************************************************************/
static int tash_mkdir(int argc, char **args)
{
	char *fullpath;
	int ret = ERROR;

	fullpath = get_fullpath(args[1]);
	if (fullpath) {
		ret = mkdir(fullpath, 0777);
		if (ret < 0) {
			FSCMD_OUTPUT(CMD_FAILED, args[0], "mkdir");
		}
		fscmd_free(fullpath);
	}

	return ret;
}
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
#ifndef CONFIG_DISABLE_ENVIRON
static int mount_handler(FAR const char *mountpoint, FAR struct statfs *statbuf, FAR void *arg)
{
	char *fstype;

	/* Get the file system type */
	switch (statbuf->f_type) {
	case SMARTFS_MAGIC:
		fstype = SMARTFS_TYPE;
		break;
	case ROMFS_MAGIC:
		fstype = ROMFS_TYPE;
		break;
	case PROCFS_MAGIC:
		fstype = PROCFS_TYPE;
		break;
	case TMPFS_MAGIC:
		fstype = TMPFS_TYPE;
		break;
	default:
		fstype = NONEFS_TYPE;
		break;
	}
	FSCMD_OUTPUT("  %s type %s\n", mountpoint, fstype);

	return OK;
}

/****************************************************************************
 * Name: search_mountpoints
 *
 * Description:
 *    Search all mount points.
 *
 ****************************************************************************/
static int search_mountpoints(const char *dirpath, foreach_mountpoint_t handler)
{
	int ret = OK;
	DIR *dirp = opendir(dirpath);		/* Open the directory */
	if (!dirp) {
		/* Failed to open the directory */
		FSCMD_OUTPUT("\t Failed to open directory: %s\n", dirpath);
		return ERROR;
	}

	/* Read each directory entry */
	for (;;) {
		struct statfs buf;
		char *fullpath;
		struct dirent *entryp = readdir(dirp);
		if (!entryp) {
			/* Finished with this directory */
			break;
		}

		/* Call statfs with the given file path */
		fullpath = get_dirpath(dirpath, entryp->d_name);
		ret = statfs(fullpath, &buf);
		if (ret != OK) {
			FSCMD_OUTPUT("statfs is failed at %s\n", fullpath);
			fscmd_free(fullpath);
			closedir(dirp);
			return ERROR;
		}

		if ((buf.f_type == SMARTFS_MAGIC) || (buf.f_type == ROMFS_MAGIC) || (buf.f_type == PROCFS_MAGIC) || (buf.f_type == TMPFS_MAGIC)) {
			ret = handler(fullpath, &buf, NULL);
			if (ret != OK) {
				FSCMD_OUTPUT("handler is failed at %s\n", fullpath);
				fscmd_free(fullpath);
				closedir(dirp);
				return ERROR;
			}
		} else {
			if (DIRENT_ISDIRECTORY(entryp->d_type) && !ls_specialdir(entryp->d_name)) {
				search_mountpoints(fullpath, handler);
			}
		}

		fscmd_free(fullpath);
	}
	closedir(dirp);

	return ret;
}

int mount_show(void)
{
	return search_mountpoints(CONFIG_LIB_HOMEDIR, mount_handler);
}

/****************************************************************************
 * Name: tash_mount
 *
 * Description:
 *   Mount specific file system.
 *
 * Usage:
 *   mount -t <filesystem name> <source directory> <target directory>
 ****************************************************************************/
static int tash_mount(int argc, char **args)
{
	int option;
	int ret;
	const char *fs = NULL;
	const char *source;
	char *fullsource;
	const char *target;
	char *fulltarget;
	bool badarg = false;

	if (argc < 2) {
		return mount_show();
	}

	optind = -1;
	while ((option = getopt(argc, args, ":t:")) != ERROR) {
		switch (option) {
		case 't':
			fs = optarg;
			break;

		case ':':
			FSCMD_OUTPUT(MISSING_ARGS, args[0]);
			badarg = true;
			break;

		case '?':
		default:
			FSCMD_OUTPUT(INVALID_ARGS, args[0]);
			badarg = true;
			break;
		}
	}

	if (badarg) {
		FSCMD_OUTPUT(INVALID_ARGS " : [-t] <fs_type> <source> <target>\n", args[0]);

		return 0;
	}

	if (!fs || optind >= argc) {
		FSCMD_OUTPUT(MISSING_ARGS, args[0]);

		return ERROR;
	}

	source = NULL;
	target = args[optind];
	optind++;

	if (optind < argc) {
		source = target;
		target = args[optind];
		optind++;
		if (optind < argc) {
			FSCMD_OUTPUT(TOO_MANY_ARGS, args[0]);

			return ERROR;
		}
	}

	fullsource = NULL;
	fulltarget = NULL;
	if (source) {
		fullsource = get_fullpath(source);
		if (!fullsource) {
			return ERROR;
		}
	}
	fulltarget = get_fullpath(target);
	if (!fulltarget) {
		ret = ERROR;
		goto errout;
	}

	/* Perform the mount */

	ret = mount(fullsource, fulltarget, fs, 0, NULL);
	if (ret < 0) {
		FSCMD_OUTPUT(CMD_FAILED, args[0], fs);
	}

errout:
	fscmd_free(fulltarget);
	fscmd_free(fullsource);

	return ret;
}
#endif
#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_umount
 *
 * Description:
 *   Unmount specific file system.
 *
 * Usage:
 *   umount <mounted directory>
 ****************************************************************************/
static int tash_umount(int argc, char **args)
{
	char *path = get_fullpath(args[1]);
	int ret = ERROR;

	if (path) {
		ret = umount(path);
		if (ret < 0) {
			FSCMD_OUTPUT(CMD_FAILED, args[0], args[1]);
		}
		fscmd_free(path);
	}

	return ret;
}
#endif
#endif							/* END OF CONFIG_DISABLE_MOUNTPOINT */

#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_pwd
 *
 * Description:
 *   Show current working directory
 *
 * Usage:
 *   pwd
 ****************************************************************************/
static int tash_pwd(int argc, char **args)
{
	FSCMD_OUTPUT("\t %s\n", getwd(PWD));

	return 0;
}
#endif
#ifndef CONFIG_DISABLE_ENVIRON
static int delete_entry(const char *fullpath)
{
	int ret = OK;
	struct stat st;

	if (!fullpath) {
		FSCMD_OUTPUT("Path is null\n");
		return ERROR;
	}

	if (stat(fullpath, &st) < 0) {
		FSCMD_OUTPUT("stat failed with %s\n", fullpath);
		return ERROR;
	}

	if (!S_ISDIR(st.st_mode)) {
		ret = unlink(fullpath);
		if (ret != OK) {
			FSCMD_OUTPUT("unlink() failed with %s\n", fullpath);
			return ret;
		}
	} else {
		/* Iterate the directory contents */
		DIR *dirp = opendir(fullpath);		/* Open the directory */
		if (!dirp) {
			/* Failed to open the directory */
			FSCMD_OUTPUT("\t Failed to open directory: %s\n", fullpath);
			return ERROR;
		}

		/* Read each directory entry */
		for (;;) {
			char *entrypath = NULL;
			struct dirent *entryp = readdir(dirp);
			if (!entryp) {
				/* Finished with this directory */
				break;
			}

			/* Call delete_entey recursively */
			entrypath = get_dirpath(fullpath, entryp->d_name);
			ret = delete_entry(entrypath);
			if (ret != OK) {
				FSCMD_OUTPUT("delete_entry() failed with %s\n", entrypath);
				closedir(dirp);
				fscmd_free(entrypath);
				return ret;
			}
		}

		ret = rmdir(fullpath);
		if (ret != OK) {
			FSCMD_OUTPUT("rmdir failed with %s\n", fullpath);
			closedir(dirp);
			return ret;
		}

		closedir(dirp);
	}
	FSCMD_OUTPUT("%s deleted\n", fullpath);

	return ret;
}

/****************************************************************************
 * Name: tash_rm
 *
 * Description:
 *   unlink target file
 *
 * Usage:
 *   rm [file path]
 ****************************************************************************/
static int tash_rm(int argc, char **args)
{
	char *fullpath;
	int ret = OK;
	struct stat st;

	if (args[2]) {
		fullpath = get_fullpath(args[2]);
		if (strncmp(args[1], "-r", strlen(args[1])) == 0) {
			/* The request is to delete an entire directory recursively
			   Check if the entry requested is not a directory and report
			 */
			if (stat(fullpath, &st) < 0) {
				FSCMD_OUTPUT("stat failed with %s, error = %d\n", fullpath, errno);
				ret = -errno;
				goto errout;
			}

			if (!S_ISDIR(st.st_mode)) {
				FSCMD_OUTPUT("%s is not a directory\n", fullpath);
				ret = -ENOTDIR;
				goto errout;
			}

			ret = delete_entry(fullpath);
		} else {
			FSCMD_OUTPUT("Usage: rm [-r] [FILE/DIR]\n");
		}
	} else {
		fullpath = get_fullpath(args[1]);
		ret = unlink(fullpath);
		if (ret != OK) {
			FSCMD_OUTPUT(CMD_FAILED, args[0], "unlink");
			ret = -errno;
			goto errout;
		}
		FSCMD_OUTPUT("%s deleted\n", fullpath);
	}

errout:
	fscmd_free(fullpath);

	return ret;
}
#endif
#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_rmdir
 *
 * Description:
 *   unlink target directory
 *
 * Usage:
 *   rmdir [directory path]
 ****************************************************************************/
static int tash_rmdir(int argc, char **args)
{
	char *fullpath;
	int ret = ERROR;

	fullpath = get_fullpath(args[1]);
	if (fullpath) {
		ret = rmdir(fullpath);
		if (ret < 0) {
			FSCMD_OUTPUT(CMD_FAILED, args[0], "rmdir");
		}
		fscmd_free(fullpath);
	}

	return ret;
}
#endif

#ifndef CONFIG_DISABLE_ENVIRON
/****************************************************************************
 * Name: tash_mv
 *
 * Description:
 *   rename or move source file and data to subfolder to target folder
 *
 * Usage:
 *   mv [source_file_path] [target_file_path]
 *
 ****************************************************************************/
static int tash_mv(int argc, char **args)
{
	char *src_fullpath = NULL;
	char *dest_fullpath = NULL;
	int ret;

	if (argc != 3) {
		FSCMD_OUTPUT(FSCMD_MV_USAGE, args[0]);
		return ERROR;
	}
	
	src_fullpath = get_fullpath(args[1]);
	if (!src_fullpath) {
		FSCMD_OUTPUT(OUT_OF_MEMORY, args[1]);
		return ERROR;
	}

	dest_fullpath = get_fullpath(args[2]);
	if (!dest_fullpath) {
		FSCMD_OUTPUT(OUT_OF_MEMORY, args[2]);
		ret = ERROR;
		goto err_with_src;
	}

	ret = rename(src_fullpath, dest_fullpath);
	if (ret == ERROR) {
		FSCMD_OUTPUT(CMD_FAILED_ERRNO, "mv", "rename", errno);
	}

	free(dest_fullpath);
err_with_src:
	free(src_fullpath);
	return ret;
}
#endif

static int df_handler(FAR const char *mountpoint, FAR struct statfs *statbuf, FAR void *arg)
{
	printf("%6u %8d %8d  %8d %s\n", statbuf->f_bsize, statbuf->f_blocks, statbuf->f_blocks - statbuf->f_bavail, statbuf->f_bavail, mountpoint);

	return OK;
}
static const char *get_fstype(FAR struct statfs *statbuf)
{
	FAR const char *fstype;

	/* Get the file system type */

	switch (statbuf->f_type) {
#ifdef CONFIG_FS_FAT
	case MSDOS_SUPER_MAGIC:
		fstype = "vfat";
		break;
#endif

#ifdef CONFIG_FS_ROMFS
	case ROMFS_MAGIC:
		fstype = "romfs";
		break;
#endif

#ifdef CONFIG_FS_TMPFS
	case TMPFS_MAGIC:
		fstype = "tmpfs";
		break;
#endif

#ifdef CONFIG_FS_BINFS
	case BINFS_MAGIC:
		fstype = "binfs";
		break;
#endif

#ifdef CONFIG_FS_NXFFS
	case NXFFS_MAGIC:
		fstype = "nxffs";
		break;
#endif

#ifdef CONFIG_NFS
	case NFS_SUPER_MAGIC:
		fstype = "nfs";
		break;
#endif

#ifdef CONFIG_FS_SMARTFS
	case SMARTFS_MAGIC:
		fstype = "smartfs";
		break;
#endif

#ifdef CONFIG_FS_PROCFS
	case PROCFS_MAGIC:
		fstype = "procfs";
		break;
#endif

#ifdef CONFIG_FS_UNIONFS
	case UNIONFS_MAGIC:
		fstype = "unionfs";
		break;
#endif

#ifdef CONFIG_FS_HOSTFS
	case HOSTFS_MAGIC:
		fstype = "hostfs";
		break;
#endif

	default:
		fstype = "Unrecognized";
		break;
	}

	return fstype;
}

static int df_man_readable_handler(FAR const char *mountpoint, FAR struct statfs *statbuf, FAR void *arg)
{
	uint32_t size;
	uint32_t used;
	uint32_t free;
	int which;
	char sizelabel;
	char freelabel;
	char usedlabel;
	const char labels[5] = { 'B', 'K', 'M', 'G', 'T' };

	size = statbuf->f_bsize * statbuf->f_blocks;
	free = statbuf->f_bsize * statbuf->f_bavail;
	used = size - free;

	/* Find the label for size */

	which = 0;
	while (size >= 9999 || ((size & 0x3ff) == 0 && size != 0)) {
		which++;
		size >>= 10;
	}

	sizelabel = labels[which];

	/* Find the label for free */

	which = 0;
	while (free >= 9999 || ((free & 0x3ff) == 0 && free != 0)) {
		which++;
		free >>= 10;
	}

	freelabel = labels[which];

	/* Find the label for used */

	which = 0;
	while (used >= 9999 || ((used & 0x3ff) == 0 && used != 0)) {
		which++;
		used >>= 10;
	}

	usedlabel = labels[which];

	printf("%-10s %6u%c %8u%c  %8u%c %s\n", get_fstype(statbuf), size, sizelabel, used, usedlabel, free, freelabel, mountpoint);

	return OK;
}

/****************************************************************************
 * Name: tash_df
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
static int tash_df(int argc, char **args)
{
	if (argc > 1 && strcmp(args[1], "-h") == 0) {
		printf("Filesystem    Size      Used  Available Mounted on\n");
		return search_mountpoints(CONFIG_LIB_HOMEDIR, df_man_readable_handler);
	} else {
		printf("  Block  Number\n");
		printf("  Size   Blocks     Used Available Mounted on\n");
		return search_mountpoints(CONFIG_LIB_HOMEDIR, df_handler);
	}

	return 0;
}
#endif

const static tash_cmdlist_t fs_utilcmds[] = {
#ifndef CONFIG_DISABLE_ENVIRON
	{"cat",       tash_cat,       TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"cd",        tash_cd,        TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
	{"df",        tash_df,        TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"echo",      tash_echo,      TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"ls",        tash_ls,        TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"mkdir",     tash_mkdir,     TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"mv",        tash_mv,        TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
#ifndef CONFIG_DISABLE_ENVIRON
	{"mount",     tash_mount,     TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"umount",    tash_umount,    TASH_EXECMD_SYNC},
#endif
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"pwd",       tash_pwd,       TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"rm",        tash_rm,        TASH_EXECMD_SYNC},
#endif

#ifndef CONFIG_DISABLE_ENVIRON
	{"rmdir",     tash_rmdir,     TASH_EXECMD_SYNC},
#endif

	{NULL,        NULL,           0}
};

void fs_register_utilcmds(void)
{
	tash_cmdlist_install(fs_utilcmds);
}
#endif							/* END OF CONFIG_NFILE_DESCRIPTORS */
