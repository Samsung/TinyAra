/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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

#include <stdio.h>
#include <unistd.h>

#include "micomapp_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int main(int argc, char **argv)
{
#ifdef CONFIG_EXAMPLES_MESSAGING_TEST
	messaging_test();
#endif
	while (1) {
		sleep(10);
		printf("[%d] MICOM ALIVE\n", getpid());
	}

	return 0;
}
