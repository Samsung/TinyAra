/****************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 *
 *   Copyright (C) 2020 Gregory Nutt. All rights reserved.
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
#include <errno.h>
#include <debug.h>
#include <tinyara/clock.h>

#if defined(CONFIG_AUDIO_I2SCHAR) && defined(CONFIG_AMEBAD_I2S)

#include <tinyara/audio/i2s.h>
#include "amebad_i2s.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_AMEBAD_I2SCHAR_MINOR
#define CONFIG_AMEBAD_I2SCHAR_MINOR 0
#endif
/*****************************************************************************
 * Private Functions
 ****************************************************************************/

static void err_cb(FAR struct i2s_dev_s *dev, FAR void *arg, int flags)
{
/* Do something here */
} 

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2schar_devinit
 *
 * Description:
 *   All architectures must provide the following interface in order to
 *   work with apps/examples/i2schar.
 *
 ****************************************************************************/

int i2schar_devinit(void)
{
	static bool initialized;
	struct i2s_dev_s *i2s;
	int ret;

	/* Have we already initialized? */

	if (!initialized) {
		/* Call amebad_i2s_initialize() to get an instance of the I2S interface */

		i2s = amebad_i2s_initialize(0);
		if (!i2s) {
			auddbg("ERROR: Failed to get the AMEBAD I2S driver\n");
			return -ENODEV;
		}

		/* Register the I2S character driver at "/dev/i2schar0" */

		ret = i2schar_register(i2s, CONFIG_AMEBAD_I2SCHAR_MINOR);
		if (ret < 0) {
			auddbg("ERROR: i2schar_register failed: %d\n", ret);
			return ret;
		}

		I2S_ERR_CB_REG(i2s, err_cb, "Error_Test_String");  


		/* Now we are initialized */

		initialized = true;
	}

	return OK;
}

#endif							/* CONFIG_AUDIO_I2SCHAR && CONFIG_AMEBAD_I2S */
