/****************************************************************************
 *
 * libc/termios/lib_tccheckfifo.c
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
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
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/ioctl.h>

#include <termios.h>
#include <errno.h>

#include <tinyara/serial/tioctl.h>

/****************************************************************************
 * Pre-processor Definitions
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
 * Name: tccheckfifo
 *
 * Description:
 *   calls the ioctl.
 *
 * Input Parameters:
 *   fd  - The 'fd' argument is an open file descriptor associated with a terminal.
 *   cmd - ioctl argument.
 *
 * Returned Value:
 *   Upon successful completion, 1 is returned. Otherwise, 0 is returned.
 *
 ****************************************************************************/

int tccheckfifo(int fd, int cmd)
{
	return ioctl(fd, cmd, 0);
}
