/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
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

#include "utc_libcxx.h"

int utc_libcxx_main(int argc, char *argv[])
{

#if defined(CONFIG_HAVE_CXXINITIALIZE)
	up_cxxinitialize();
#endif

	if (tc_handler(TC_START, "Libc++ TC") == ERROR) {
		return ERROR;
	}

#if defined(CONFIG_LIBCXX_UTC_ALGORITHMS)
	tc_algorithms_main();
#endif

#if defined(CONFIG_LIBCXX_UTC_UTILITIES)
	tc_utilities_main();
#endif

	tc_handler(TC_END, "Libc++ TC");

	return OK;
}
