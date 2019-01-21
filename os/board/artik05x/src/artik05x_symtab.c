/*****************************************************************************
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
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arch/board/symtab.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MAX_SYMBOL_NAME_LEN 256
/****************************************************************************
 * Private Variables
 ****************************************************************************/
static uint32_t symtab_base_addr = 0;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int up_symtab_flashdrv_init(void)
{
	char *a;
	char *b;
	int name_ind = 0;
	int len_ind = 0;
	char s[MAX_SYMBOL_NAME_LEN];

	a = CONFIG_ARTIK05X_FLASH_PART_NAME;
	b = CONFIG_ARTIK05X_FLASH_PART_LIST;

	if (a == NULL || b == NULL) {
		printf("Invalid Configs\n");
		return -1;
	}

	/* Computing the name_ind for symtab partition */
	while (a[0] != '\0') {
		if (a[0] == 's') {
			if (strncmp("symtab", a, 6) == 0) {
				break;
			}
		}
		while (a[0] != ',') {
			a++;
		}
		a++;
		name_ind++;
	}

	if (a == NULL) {
		printf("Unable to initialize the flash driver for symbol table \n");
		return -1;
	}

	memset(s, '\0', sizeof(s));
	/* Computing the offset length for symtab partition */

	while (b[0] != '\0') {
		if (name_ind == 0) {
			break;
		} else {
			s[len_ind] = b[0];
			len_ind++;
			b++;
			if (b[0] == ',') {
				int part_size;
				sscanf(s, "%d", &part_size);
				symtab_base_addr += part_size * 1024;

				b++;
				name_ind--;
				len_ind = 0;
				memset(s, '\0', sizeof(s));
			}
		}
	}
	if (symtab_base_addr == 0) {
		return -1;
	}

	/* Symbol table offset length is added to the flash base address */
	symtab_base_addr += (uint32_t)FLASH_BASE;
	return OK;
}

void up_symtab_flashdrv_read(void *buffer, uint32_t offset_addr, int bytes)
{
	if (buffer == NULL || bytes < 0) {
		return;
	}
	memcpy(buffer, (void *)(symtab_base_addr + offset_addr), bytes);
}
