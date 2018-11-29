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
 * include/unistd.h
 *
 *   Copyright (C) 2007-2009, 2013-2014 Gregory Nutt. All rights reserved.
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
/**
 * @defgroup UNISTD_KERNEL UNISTD
 * @brief Provides APIs for Standard Symbolic Constants and Types
 * @ingroup KERNEL
 *
 * @{
 */

///@file unistd.h
///@brief POSIX operating system APIs

#ifndef __INCLUDE_UNISTD_H
#define __INCLUDE_UNISTD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <tinyara/compiler.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The number of functions that may be registered to be called
 * at program exit.
 */

#define ATEXIT_MAX 1

/* Values for seeking */

#define SEEK_SET    0			/* From the start of the file */
#define SEEK_CUR    1			/* From the current file offset */
#define SEEK_END    2			/* From the end of the file */

/* Bit values for the second argument to access */

#define F_OK        0			/* Test existence */
#define R_OK        1			/* Test read permission */
#define W_OK        2			/* Test write permission */
#define X_OK        4			/* Test execute permission */

/* POSIX feature set macros */

#define  POSIX_VERSION
#undef  _POSIX_SAVED_IDS
#undef  _POSIX_JOB_CONTROL
#define _POSIX_REALTIME_SIGNALS 1
#define _POSIX_MESSAGE_PASSING 1
#undef  _POSIX_MAPPED_FILES
#undef  _POSIX_SHARED_MEMORY_OBJECTS
#define _POSIX_PRIORITY_SCHEDULING 1
#define _POSIX_TIMERS 1
#undef  _POSIX_MEMLOCK
#undef  _POSIX_MEMLOCK_RANGE
#undef  _POSIX_FSYNC
#define _POSIX_SYNCHRONIZED_IO 1
#undef  _POSIX_ASYNCHRONOUS_IO
#undef  _POSIX_PRIORITIZED_IO

#ifdef CONFIG_SCHED_SPORADIC
#  define _POSIX_SPORADIC_SERVER 1
#  define _POSIX_THREAD_SPORADIC_SERVER 1
#else
#  undef  _POSIX_SPORADIC_SERVER
#  undef  _POSIX_THREAD_SPORADIC_SERVER
#endif

/* Execution time constants (not supported) */

#undef  _POSIX_CHOWN_RESTRICTED
#undef  _POSIX_NO_TRUNC
#undef  _POSIX_VDISABLE

#define _POSIX_SYNC_IO 1
#undef  _POSIX_ASYNC_IO
#undef  _POSIX_PRIO_IO

/* Constants used with POSIX sysconf().  sysconf() will return -2 and set
 * errno to ENOSYS for most of these.
 */

#define _SC_2_C_BIND                     0x0001
#define _SC_2_C_DEV                      0x0002
#define _SC_2_CHAR_TERM                  0x0003
#define _SC_2_FORT_DEV                   0x0004
#define _SC_2_FORT_RUN                   0x0005
#define _SC_2_LOCALEDEF                  0x0006
#define _SC_2_PBS                        0x0007
#define _SC_2_PBS_ACCOUNTING             0x0008
#define _SC_2_PBS_CHECKPOINT             0x0009
#define _SC_2_PBS_LOCATE                 0x000a
#define _SC_2_PBS_MESSAGE                0x000b
#define _SC_2_PBS_TRACK                  0x000c
#define _SC_2_SW_DEV                     0x000d
#define _SC_2_UPE                        0x000e
#define _SC_2_VERSION                    0x000f
#define _SC_ADVISORY_INFO                0x0010
#define _SC_AIO_LISTIO_MAX               0x0011
#define _SC_AIO_MAX                      0x0012
#define _SC_AIO_PRIO_DELTA_MAX           0x0013
#define _SC_ARG_MAX                      0x0014
#define _SC_ASYNCHRONOUS_IO              0x0015
#define _SC_ATEXIT_MAX                   0x0016
#define _SC_BARRIERS                     0x0017
#define _SC_BC_BASE_MAX                  0x0018
#define _SC_BC_DIM_MAX                   0x0019
#define _SC_BC_SCALE_MAX                 0x001a
#define _SC_BC_STRING_MAX                0x001b
#define _SC_CHILD_MAX                    0x001c
#define _SC_CLK_TCK                      0x001d
#define _SC_CLOCK_SELECTION              0x001e
#define _SC_COLL_WEIGHTS_MAX             0x001f
#define _SC_CPUTIME                      0x0020
#define _SC_DELAYTIMER_MAX               0x0021
#define _SC_EXPR_NEST_MAX                0x0022
#define _SC_FSYNC                        0x0023
#define _SC_GETGR_R_SIZE_MAX             0x0024
#define _SC_GETPW_R_SIZE_MAX             0x0025
#define _SC_HOST_NAME_MAX                0x0026
#define _SC_IOV_MAX                      0x0027
#define _SC_IPV6                         0x0028
#define _SC_JOB_CONTROL                  0x0029
#define _SC_LINE_MAX                     0x002a
#define _SC_LOGIN_NAME_MAX               0x002b
#define _SC_MAPPED_FILES                 0x002c
#define _SC_MEMLOCK                      0x002d
#define _SC_MEMLOCK_RANGE                0x002e
#define _SC_MEMORY_PROTECTION            0x002f
#define _SC_MESSAGE_PASSING              0x0030
#define _SC_MONOTONIC_CLOCK              0x0031
#define _SC_MQ_OPEN_MAX                  0x0032
#define _SC_MQ_PRIO_MAX                  0x0033
#define _SC_NGROUPS_MAX                  0x0034
#define _SC_OPEN_MAX                     0x0035
#define _SC_PAGE_SIZE                    0x0036
#define _SC_PAGESIZE                     _SC_PAGE_SIZE
#define _SC_PRIORITIZED_IO               0x0037
#define _SC_PRIORITY_SCHEDULING          0x0038
#define _SC_RAW_SOCKETS                  0x0039
#define _SC_RE_DUP_MAX                   0x003a
#define _SC_READER_WRITER_LOCKS          0x003b
#define _SC_REALTIME_SIGNALS             0x003c
#define _SC_REGEXP                       0x003d
#define _SC_RTSIG_MAX                    0x003e
#define _SC_SAVED_IDS                    0x003f
#define _SC_SEM_NSEMS_MAX                0x0040
#define _SC_SEM_VALUE_MAX                0x0041
#define _SC_SEMAPHORES                   0x0042
#define _SC_SHARED_MEMORY_OBJECTS        0x0043
#define _SC_SHELL                        0x0044
#define _SC_SIGQUEUE_MAX                 0x0045
#define _SC_SPAWN                        0x0046
#define _SC_SPIN_LOCKS                   0x0047
#define _SC_SPORADIC_SERVER              0x0048
#define _SC_SS_REPL_MAX                  0x0049
#define _SC_STREAM_MAX                   0x004a
#define _SC_SYMLOOP_MAX                  0x004b
#define _SC_SYNCHRONIZED_IO              0x004c
#define _SC_THREAD_ATTR_STACKADDR        0x004d
#define _SC_THREAD_ATTR_STACKSIZE        0x004e
#define _SC_THREAD_CPUTIME               0x004f
#define _SC_THREAD_DESTRUCTOR_ITERATIONS 0x0050
#define _SC_THREAD_KEYS_MAX              0x0051
#define _SC_THREAD_PRIO_INHERIT          0x0052
#define _SC_THREAD_PRIO_PROTECT          0x0053
#define _SC_THREAD_PRIORITY_SCHEDULING   0x0054
#define _SC_THREAD_PROCESS_SHARED        0x0055
#define _SC_THREAD_SAFE_FUNCTIONS        0x0056
#define _SC_THREAD_SPORADIC_SERVER       0x0057
#define _SC_THREAD_STACK_MIN             0x0058
#define _SC_THREAD_THREADS_MAX           0x0059
#define _SC_THREADS                      0x005a
#define _SC_TIMEOUTS                     0x005b
#define _SC_TIMER_MAX                    0x005c
#define _SC_TIMERS                       0x005d
#define _SC_TRACE                        0x005e
#define _SC_TRACE_EVENT_FILTER           0x005f
#define _SC_TRACE_EVENT_NAME_MAX         0x0060
#define _SC_TRACE_INHERIT                0x0061
#define _SC_TRACE_LOG                    0x0062
#define _SC_TRACE_NAME_MAX               0x0063
#define _SC_TRACE_SYS_MAX                0x0064
#define _SC_TRACE_USER_EVENT_MAX         0x0065
#define _SC_TTY_NAME_MAX                 0x0066
#define _SC_TYPED_MEMORY_OBJECTS         0x0067
#define _SC_TZNAME_MAX                   0x0068
#define _SC_V6_ILP32_OFF32               0x0069
#define _SC_V6_ILP32_OFFBIG              0x006a
#define _SC_V6_LP64_OFF64                0x006b
#define _SC_V6_LPBIG_OFFBIG              0x006c
#define _SC_VERSION                      0x006d
#define _SC_XBS5_ILP32_OFF32             0x006e  /* (LEGACY) */
#define _SC_XBS5_ILP32_OFFBIG            0x006f  /* (LEGACY) */
#define _SC_XBS5_LP64_OFF64              0x0070  /* (LEGACY) */
#define _SC_XBS5_LPBIG_OFFBIG            0x0071  /* (LEGACY) */
#define _SC_XOPEN_CRYPT                  0x0072
#define _SC_XOPEN_ENH_I18N               0x0073
#define _SC_XOPEN_LEGACY                 0x0074
#define _SC_XOPEN_REALTIME               0x0075
#define _SC_XOPEN_REALTIME_THREADS       0x0076
#define _SC_XOPEN_SHM                    0x0077
#define _SC_XOPEN_STREAMS                0x0078
#define _SC_XOPEN_UNIX                   0x0079
#define _SC_XOPEN_VERSION                0x007a

/* The following symbolic constants must be defined for file streams: */

#define STDERR_FILENO                    2       /* File number of stderr */
#define STDIN_FILENO                     0       /* File number of stdin */
#define STDOUT_FILENO                    1       /* File number of stdout */

#define HOST_NAME_MAX                    32

/* Helpers and legacy compatibility definitions */

#define fdatasync(f) fsync(f)

/****************************************************************************
 * Global Variables
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/* Used by getopt (obviously NOT thread safe!).  These variables cannot be
 * accessed directly by an external NXFLAT module.  In that case, accessor
 * functions must be used.
 */

#ifndef __NXFLAT__
EXTERN FAR char *optarg;		/* Optional argument following option */
EXTERN int optind;				/* Index into argv */
EXTERN int optopt;				/* unrecognized option character */
#else
#define optarg  (*(getoptargp()))
#define optind  (*(getoptindp()))
#define optopt  (*(getoptoptp()))
#endif

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

/* Task Control Interfaces */

/**
 * @cond
 * @internal
 * @ingroup UNISTD_KERNEL
 * @brief The vfork() function has the same effect as fork(), except that the behavior is
 *   undefined if the process created by vfork() either modifies any data other than
 *   a variable of type pid_t used to store the return value from vfork(), or returns
 *   from the function in which vfork() was called, or calls any other function before
 *   successfully calling _exit() or one of the exec family of functions.
 *
 * @details @b #include <unistd.h> \n
 * This thin layer implements vfork by simply calling up_vfork() with the vfork()
 *   context as an argument.  The overall sequence is:
 *
 *   1) User code calls vfork().  vfork() collects context information and
 *      transfers control up up_vfork().
 *   2) up_vfork()and calls task_vforksetup().
 *   3) task_vforksetup() allocates and configures the child task's TCB.  This
 *      consists of:
 *      - Allocation of the child task's TCB.
 *      - Initialization of file descriptors and streams
 *      - Configuration of environment variables
 *      - Setup the intput parameters for the task.
 *      - Initialization of the TCB (including call to up_initial_state()
 *   4) up_vfork() provides any additional operating context. up_vfork must:
 *      - Allocate and initialize the stack
 *      - Initialize special values in any CPU registers that were not
 *        already configured by up_initial_state()
 *   5) up_vfork() then calls task_vforkstart()
 *   6) task_vforkstart() then executes the child thread.
 *
 * @return Upon successful completion, vfork() returns 0 to the child process and returns
 *   the process ID of the child process to the parent process. Otherwise, -1 is
 *   returned to the parent, no child process is created, and errno is set to
 *   indicate the error.
 * @since TizenRT v1.0
 */
pid_t vfork(void);
/**
 * @endcond
 */
/**
 * @ingroup UNISTD_KERNEL
 * @brief get the process ID
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
pid_t getpid(void);

/* Check if a file descriptor corresponds to a terminal I/O file */
/**
 * @ingroup UNISTD_KERNEL
 * @brief test for a terminal device
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v2.1 PRE
 */
int isatty(int fd);
/**
 * @cond
 * @internal
 */
void _exit(int status) noreturn_function;
/**
 * @endcond
 */
/**
 * @ingroup UNISTD_KERNEL
 * @brief suspend execution for an interval of time
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
unsigned int sleep(unsigned int seconds);
/**
 * @ingroup UNISTD_KERNEL
 * @brief suspend execution for microsecond intervals
 * @details @b #include <unistd.h> \n
 * The usleep() function suspends execution of the calling thread for
 * (at least) usec microseconds.  The sleep may be lengthened slightly
 * by any system activity or by the time spent processing the call or by
 * the granularity of system timers.
 * @param[in] usec microsecond intervals
 * @since TizenRT v1.0
 */
int usleep(useconds_t usec);
/**
 * @ingroup UNISTD_KERNEL
 * @brief suspend the thread until a signal is received
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int pause(void);

/* File descriptor operations */

/**
 * @addtogroup UNISTD_KERNEL
 * @{
 */
/**
 * @brief close a file descriptor
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int close(int fd);
/**
 * @brief duplicate an open file descriptor
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int dup(int fd);
/**
 * @brief duplicate an open file descriptor
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int dup2(int fd1, int fd2);
/**
 * @brief synchronize changes to a file
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int fsync(int fd);
/**
 * @brief move the read/write file offset
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
off_t lseek(int fd, off_t offset, int whence);
/**
 * @brief read from a file
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
ssize_t read(int fd, FAR void *buf, size_t nbytes);
/**
 * @brief write to another user
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
ssize_t write(int fd, FAR const void *buf, size_t nbytes);
/**
 * @brief read from a file
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
ssize_t pread(int fd, FAR void *buf, size_t nbytes, off_t offset);
/**
 * @brief write on a file
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
ssize_t pwrite(int fd, FAR const void *buf, size_t nbytes, off_t offset);

/**
 * @}
 */
/* Memory management */

#if defined(CONFIG_ARCH_ADDRENV) && defined(CONFIG_MM_PGALLOC) && \
	defined(CONFIG_ARCH_USE_MMU)
/**
 * @cond
 * @internal
 */
FAR void *sbrk(intptr_t incr);
/**
 * @endcond
 */
#endif

/* Special devices */

/**
 * @ingroup UNISTD_KERNEL
 * @brief create an interprocess channel
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int pipe(int fd[2]);

/* Working directory operations */

/**
 * @ingroup UNISTD_KERNEL
 * @brief change working directory
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int chdir(FAR const char *path);
/**
 * @ingroup UNISTD_KERNEL
 * @brief get the pathname of the current working directory
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
FAR char *getcwd(FAR char *buf, size_t size);

/* File path operations */
/**
 * @ingroup STDLIB_LIBC
 * @brief determine accessibility of a file descriptor
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.1
 */
int access(FAR const char *path, int amode);
/**
 * @ingroup UNISTD_KERNEL
 * @brief remove a directory
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int rmdir(FAR const char *pathname);
/**
 * @ingroup UNISTD_KERNEL
 * @brief call the unlink function
 * @details @b #include <unistd.h> \n
 * SYSTEM CALL API \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int unlink(FAR const char *pathname);

/* Execution of programs from files */

#ifdef CONFIG_LIBC_EXECFUNCS
/**
 * @cond
 * @internal
 * @{
 */
int execl(FAR const char *path, ...);
int execv(FAR const char *path, FAR char *const argv[]);
/**
 * @endcond
 * @} */
#endif

/* Byte operations */
/**
 * @cond
 * @internal
 */
void swab(FAR const void *src, FAR void *dest, ssize_t nbytes);
/**
 * @endcond
 */

/* getopt and friends */
/**
 * @ingroup UNISTD_KERNEL
 * @brief command option parsing
 * @details @b #include <unistd.h> \n
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int getopt(int argc, FAR char *const argv[], FAR const char *optstring);

/* Accessor functions intended for use only by external NXFLAT
 * modules.  The global variables optarg, optind, and optopt cannot
 * be referenced directly from external modules.
 */
/**
 * @cond
 * @internal
 * @{
 */
FAR char **getoptargp(void);	/* Optional argument following option */
int *getoptindp(void);			/* Index into argv */
int *getoptoptp(void);			/* unrecognized option character */
/**
 * @endcond
 * @} */
#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif					/* __INCLUDE_UNISTD_H */
/**
 * @}
 */
