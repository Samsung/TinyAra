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
 * apps/tzshlib/tzsh_system.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tzsh.h"
#include "tzsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tzsh_system
 *
 * Description:
 *   This is the TZSH-specific implementation of the standard system()
 *   command.
 *
 *   NOTE: This assumes that other TZSH instances have previously ran and so
 *   common TZSH logic is already initialized.
 *
 * Input Parameters:
 *   Standard task start-up arguments.  Expects argc == 2 with argv[1] being
 *   the command to execute
 *
 * Returned Values:
 *   EXIT_SUCCESS or EXIT_FAILURE
 *
 ****************************************************************************/

int tzsh_system(int argc, char *argv[])
{
	/* Expect argc == 2 with argv[1] being the command to execute */

	if (argc >= 2) {
		FAR struct console_stdio_s *pstate = tzsh_newconsole();
		FAR struct tzsh_vtbl_s *vtbl;

		DEBUGASSERT(pstate != NULL);
		vtbl = &pstate->cn_vtbl;

		/* Parse process the command */

		(void)tzsh_parse(vtbl, argv[1]);
#if CONFIG_NFILE_STREAMS > 0
		fflush(pstate->cn_outstream);
#endif

		/* Exit upon return */

		tzsh_exit(&pstate->cn_vtbl, OK);
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
