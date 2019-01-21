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
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ADDR_LENGTH 4
#define HEXADECIMAL_BASE 16
#define MAP_FILE_PATH "../build/output/bin/System.map"
#define BIN_FILE_PATH "../build/output/bin/Symtab.bin"
#define MAX_NUM_SYM 8000
#define MAX_SYM_LEN 128

/****************************************************************************
 * Global Functions
 ****************************************************************************/

static uint32_t symbolnum;
char symbolname[MAX_NUM_SYM][MAX_SYM_LEN];
uint32_t symboladdress[MAX_NUM_SYM] = {0};
uint32_t symboloffset[MAX_NUM_SYM] = {0};
char symbol[2 * MAX_SYM_LEN] = {0};

int main()
{
	FILE *fp;
	size_t ret;
	char *ptr;
	int cnt;

	fp = fopen(MAP_FILE_PATH, "r");

	/* If the file could not be opened */
	if (fp == NULL) {
		printf("Symtab.bin creation failed \n");
		return -1;
	}

	/* Reading the system.map file line by line */
	while (fgets(symbol, sizeof(symbol), fp) != NULL) {

		ptr = symbol;
		/* Extracting the symbol address from the symbol variable */

		symboladdress[symbolnum] = (unsigned int)strtoul(ptr, &ptr, HEXADECIMAL_BASE);
		/* Copying the symbol name */

		strncpy(symbolname[symbolnum], (char *)(ptr + 3), strlen((char *)(ptr + 3)) - 1);

		/* Calculating the symbol offset length */

		if (symbolnum != 0) {
			symboloffset[symbolnum] = (uint32_t)(symboloffset[symbolnum - 1] + (uint32_t)strlen(symbolname[symbolnum - 1]));
		}

		symbolnum++;
	}

	fclose(fp);

	/* Opening & Writing the symbol table binary file */
	fp = fopen(BIN_FILE_PATH, "wb");

	if (fp == NULL) {
		printf("Symtab.bin creation failed \n");
		return 0;
	}

	/* Writing the number of symbols in the first 4 bytes of the file */
	ret = fwrite(&symbolnum, sizeof(uint32_t), 1, fp);

	/* Writing the Symbol addresses into the binary file */
	ret = fwrite(&symboladdress, sizeof(uint32_t), symbolnum, fp);

	/*Writing the symbol offset addresses into the binary file */
	ret = fwrite(symboloffset, sizeof(uint32_t), symbolnum, fp);

	cnt = 0;

	while (cnt < symbolnum) {
		/* Copying the symbol names into the binary file */

		ret = fwrite(symbolname[cnt], strlen(symbolname[cnt]), 1, fp);
		cnt++;
	}

	fclose(fp);
}
