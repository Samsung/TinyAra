/****************************************************************************
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

#include <tinyara/fs/fs.h>

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
 * Name: normalize_path
 *
 * Description: Normalize the path if it has extra '/' characters in the
 *  middle or at the end.
 *
 ****************************************************************************/

void normalize_path(const char *path, char **ret_path)
{
	int i;
	int j = 1;
	char *tmp = *ret_path;

	/* Copy the first character anyways */
	tmp[0] = path[0];

	for (i = 1; path[i] != '\0'; i++) {
		/* If the current and previous characters are both '/' then skip the current one */
		if ((path[i] == path[i - 1]) && (path[i] == '/')) {
			continue;
		}
		tmp[j] = path[i];
		j++;
	}
	tmp[j] = '\0';

	return;
}

