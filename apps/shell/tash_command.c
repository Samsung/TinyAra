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
/// @file   tash_command.c
/// @brief  functions to treate commands

#include <pthread.h>
#include <tinyara/config.h>
#include <tinyara/ascii.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/boardctl.h>
#include <apps/shell/tash.h>
#ifdef CONFIG_BUILTIN_APPS
#include <apps/builtin.h>
#endif
#include "tash_internal.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Following defines are fixed to avoid many configuration variables for TASH */
#define TASH_CMD_MAXSTRLENGTH		(16) /* example: length of "ifconfig" command is 8 */
#ifdef CONFIG_TASH_MAX_COMMANDS
#define TASH_MAX_COMMANDS			CONFIG_TASH_MAX_COMMANDS
#else
#define TASH_MAX_COMMANDS			(32)
#endif
#ifdef CONFIG_TASH_CMDTASK_STACKSIZE
#define TASH_CMDTASK_STACKSIZE		CONFIG_TASH_CMDTASK_STACKSIZE
#else
#define TASH_CMDTASK_STACKSIZE		(4096)
#endif
#ifdef CONFIG_TASH_CMDTASK_PRIORITY
#define TASH_CMDTASK_PRIORITY		CONFIG_TASH_CMDTASK_PRIORITY
#else
#define TASH_CMDTASK_PRIORITY		(SCHED_PRIORITY_DEFAULT)
#endif
#define TASH_CMDS_PER_LINE			(4)

#if TASH_MAX_STORE > 0
#define CMD_INDEX_UP(x)                                   \
	do {                                                  \
		((x) == TASH_MAX_STORE - 1) ? (x) = 0 : (x)++;    \
	} while (0)

#define CMD_INDEX_DOWN(x)                                 \
	do {                                                  \
		((x) == 0) ? (x) = TASH_MAX_STORE - 1 : (x)--;    \
	} while (0)
#endif
/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/**
 * @brief TASH internal data structure to store command strings & callback function
 * pointers
 */
struct tash_cmd_s {
	char str[TASH_CMD_MAXSTRLENGTH];	/* Command strings- eg, ifconfig */
	TASH_CMD_CALLBACK cb;				/* Function pointer for Callback */
	int exec_type;						/* Execution type of this command */
};

struct tash_cmd_info_s {
	pthread_mutex_t tmutex;						/* Mutex for protection */
	struct tash_cmd_s cmd[TASH_MAX_COMMANDS];
	int count;									/* Number of TASH commands */
};

/********************************************************************************
 * Private Function Prototypes
 ********************************************************************************/

static int tash_help(int argc, char **args);
static int tash_clear(int argc, char **args);
static int tash_exit(int argc, char **args);
#if defined(CONFIG_BOARDCTL_RESET)
static int tash_reboot(int argc, char **argv);
#endif
#if TASH_MAX_STORE   > 0
static int tash_history(int argc, char **argv);
#endif

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static int is_sorted_list = FALSE;
static struct tash_cmd_info_s tash_cmds_info = {PTHREAD_MUTEX_INITIALIZER};

const static tash_cmdlist_t tash_basic_cmds[] = {
	{"exit",  tash_exit,   TASH_EXECMD_SYNC},
	{"help",  tash_help,   TASH_EXECMD_SYNC},
	{"clear",  tash_clear,   TASH_EXECMD_SYNC},
#ifdef CONFIG_TASH_SCRIPT
	{"sh",    tash_script, TASH_EXECMD_SYNC},
#endif
#ifndef CONFIG_DISABLE_SIGNALS
	{"sleep", tash_sleep,  TASH_EXECMD_SYNC},
#ifdef CONFIG_TASH_USLEEP
	{"usleep", tash_usleep, TASH_EXECMD_SYNC},
#endif
#endif
#if defined(CONFIG_BOARDCTL_RESET)
	{"reboot", tash_reboot, TASH_EXECMD_SYNC},
#endif
#if TASH_MAX_STORE > 0
	{"history", tash_history, TASH_EXECMD_SYNC},
#endif
	{NULL,    NULL,        0}
};

#if TASH_MAX_STORE   > 0
static char cmd_store[TASH_MAX_STORE][TASH_LINEBUFLEN];
static char cmd_line[TASH_LINEBUFLEN];
static int cmd_pos;
static int cmd_head;
static int cmd_tail;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void tash_sort_cmds(void)
{
	int i;
	int j;
	int length = tash_cmds_info.count;
	int gap = tash_cmds_info.count;
	struct tash_cmd_s temp;
	int tash_cmd_size = sizeof(struct tash_cmd_s);

	while (gap > 1) {
		/* gap = max(floor(gap * 5 / 11), 1) */

		gap = (gap * 5 / 11) > 1 ? (gap * 5 / 11) : 1;
		for (i = gap; i < length; i++) {
			memcpy(&temp, &tash_cmds_info.cmd[i], tash_cmd_size);
			for (j = i; j >= gap && (strcmp(temp.str, tash_cmds_info.cmd[j - gap].str) < 0); j -= gap) {
				memcpy(&tash_cmds_info.cmd[j], &tash_cmds_info.cmd[j - gap], tash_cmd_size);
			}
			memcpy(&tash_cmds_info.cmd[j], &temp, tash_cmd_size);
		}
	}
}

/** @brief Help function in TASH to list all available commands
 *  @ingroup tash
 */
static int tash_help(int argc, char **args)
{
	int cmd_idx;

	printf("\t TASH command list \n");
	printf("\t --------------------\n");

	if (is_sorted_list == FALSE) {
		tash_sort_cmds();
		is_sorted_list = TRUE;
	}

	for (cmd_idx = 0; cmd_idx < tash_cmds_info.count; cmd_idx++) {
		printf("%-16s ", tash_cmds_info.cmd[cmd_idx].str);
		if (cmd_idx % TASH_CMDS_PER_LINE == (TASH_CMDS_PER_LINE - 1)) {
			printf("\n");
		}
	}
	printf("\n");

	return 0;
}

/** @brief Clear the terminal screen and move cursor to top
 *  @ingroup tash
 */
static int tash_clear(int argc, char **args)
{
	printf("\x1b[1;1H\x1b[2J\n");
	return 0;
}

/** @brief TASH exit function. It cannot be re-launched.
 *  @ingroup tash
 */
static int tash_exit(int argc, char **args)
{
	printf("TASH: Good bye!!\n");
	tash_stop();
	exit(0);
}

/** @brief Help function in TASH to list all available commands
 *  @ingroup tash
 */

#if TASH_MAX_STORE > 0
static int tash_history(int argc, char **args)
{
	int cmd_idx = 1;

	printf("\t TASH command history\n");
	printf("\t --------------------\n");

	int head_idx = cmd_head;
	while (head_idx != cmd_tail) {
		printf(" %d \t %s\n", cmd_idx++, cmd_store[head_idx]);
		CMD_INDEX_UP(head_idx);
	}

	return 0;
}

/**
 * @brief      Get the command in the history list
 * @details    Return the address of command buffer which is selected by the index in history list
 * @param[in]  hist_idx The index in the history list
 * @return     The address of command buffer selected on success, null on failure
 */

static char *tash_get_cmd_from_history(int hist_idx)
{
	int cmd_idx;

	if (hist_idx < 0) {
		printf("Not supported\n");
		return NULL;
	} else if (hist_idx == 0 || hist_idx >= TASH_MAX_STORE) {
		printf("!%d: event not found\n", hist_idx);
		return NULL;
	}

	/* Get the command index by adding given number from first command index in ring buffer */

	cmd_idx = cmd_head + (hist_idx - 1);

	/* Check overflow of buffer size */

	if (cmd_idx >= TASH_MAX_STORE) {
		cmd_idx -= TASH_MAX_STORE;
	}

	/* Check the given number is in a valid range */

	if ((cmd_idx >= cmd_tail) && (cmd_head < cmd_tail)) {
		printf("!%d: event not found\n", hist_idx);
		return NULL;
	}

	/* Return the address of command buffer indexed */

	return cmd_store[cmd_idx];
}

void tash_store_cmd(char *cmd)
{
	/* If there is no command, it is not saved. */
	if (cmd == NULL || cmd[0] == '\0') {
		return;
	}

	if (cmd_head != cmd_tail) {
		/* If it is the same as the previous command, it is not saved. */
		int prev = cmd_tail;
		CMD_INDEX_DOWN(prev);

		if (strncmp(cmd_store[prev], cmd, TASH_LINEBUFLEN) == 0) {
			return;
		}
	}

	/* Save current command. */
	strncpy(cmd_store[cmd_tail], cmd, TASH_LINEBUFLEN);
	CMD_INDEX_UP(cmd_tail);

	/* Move the head when the storage space is full. */
	if (cmd_tail == cmd_head) {
		CMD_INDEX_UP(cmd_head);
	}
	cmd_pos = cmd_tail;
}

bool tash_search_cmd(char *cmd, int *pos, char status)
{
	int idx = 0;
	if (cmd_pos == cmd_tail) {
		if (status == ASCII_A) {
			strncpy(cmd_line, cmd, TASH_LINEBUFLEN);
		} else {
			return false;
		}
	} else if (cmd_pos == cmd_head && status == ASCII_A) {
		return false;
	} else {
		/* Save current command. */
		while (idx < TASH_LINEBUFLEN && cmd[idx] != 0) {
			cmd_store[cmd_pos][idx] = cmd[idx];
			idx++;
		}
		cmd_store[cmd_pos][idx] = 0;
	}

	if (status == ASCII_A) { // up
		CMD_INDEX_DOWN(cmd_pos);
		*pos = 0;

		while (*pos < TASH_LINEBUFLEN && cmd_store[cmd_pos][*pos] != 0) {
			cmd[*pos] = cmd_store[cmd_pos][*pos];
			(*pos)++;
		}
	} else if (status == ASCII_B) { // down
		CMD_INDEX_UP(cmd_pos);
		*pos = 0;
		if (cmd_pos == cmd_tail) {
			while (*pos < TASH_LINEBUFLEN && cmd_line[*pos] != 0) {
				cmd[*pos] = cmd_line[*pos];
				(*pos)++;
			}
		} else {
			while (*pos < TASH_LINEBUFLEN && cmd_store[cmd_pos][*pos] != 0) {
				cmd[*pos] = cmd_store[cmd_pos][*pos];
				(*pos)++;
			}
		}
	}
	return true;
}

/**
 * @brief     treat exclamation command
 * @details   Find the exclamation command and replace it to real command into buffer
 * @param[in] buff The pointer of user input string
 * @return    OK on success, ERROR on failure
 */

int check_exclam_cmd(char *buff)
{
	int hist_idx;
	int histcmd_size;
	char *histcmd_ptr;
	char *exclam_ptr;
	int ret = OK;

	/* Find the '!' in the input character */

	while ((exclam_ptr = strchr(buff, ASCII_EXCLAM)) != NULL) {
		/* Get the command from history according to the given index */

		hist_idx = strtol(exclam_ptr + 1, &buff, 10);
		histcmd_ptr = tash_get_cmd_from_history(hist_idx);
		if (!histcmd_ptr) {
			/* No command, Let's finish */

			ret = ERROR;
			break;
		} else {
			histcmd_size = strlen(histcmd_ptr);
			if (histcmd_size != (int)(buff - exclam_ptr)) {
				/* "!number" does not have the same size as the command from the history list.
				 * Let's adjust the location of next string to replace "!number" to real command string.
				 */

				memmove(exclam_ptr + histcmd_size, buff, strlen(buff));
			}
			/* Replace "!number" to real command string in buff */

			strncpy(exclam_ptr, histcmd_ptr, histcmd_size);
		}
	}
	return ret;
}
#endif

#if defined(CONFIG_BOARDCTL_RESET)
static int tash_reboot(int argc, char **argv)
{
	/*
	 * Invoke the BOARDIOC_RESET board control to reset the board. If
	 * the board_reset() function returns, then it was not possible to
	 * reset the board due to some constraints.
	 */
	boardctl(BOARDIOC_RESET, EXIT_SUCCESS);

	/*
	 * boarctl() will not return in this case.  It if does, it means that
	 * there was a problem with the reset operaion.
	 */
	return ERROR;
}
#endif /* CONFIG_BOARDCTL_RESET */

/** @brief Launch a task to run tash cmd asynchronously
 *  @ingroup tash
 */
static int tash_launch_cmdtask(TASH_CMD_CALLBACK cb, int argc, char **args)
{
	int ret = 0;
	int pri = TASH_CMDTASK_PRIORITY;
	long stack_size = TASH_CMDTASK_STACKSIZE;
#ifdef	CONFIG_EXAMPLES_TESTCASE_TCP_TLS_STRESS
	stack_size = 8192;
#endif
#if defined(CONFIG_BUILTIN_APPS)
	int cmd_idx;

	for (cmd_idx = 0; (cmd_idx < tash_cmds_info.count) && (builtin_list[cmd_idx].name != NULL); cmd_idx++) {
		if (!(strncmp(args[0], builtin_list[cmd_idx].name, TASH_CMD_MAXSTRLENGTH - 1))) {
			pri = builtin_list[cmd_idx].priority;
			stack_size = builtin_list[cmd_idx].stacksize;
			break;
		}
	}
#endif

	shvdbg("Command will be launched with pri (%d), stack size(%d)\n", pri, stack_size);

	ret = task_create(args[0], pri, stack_size, cb, &args[1]);

	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/** @brief TASH command execution in tash task context
 *  @ingroup tash
 */
int tash_execute_cmd(char **args, int argc)
{
	int cmd_idx;
	int cmd_found = 0;

	/* lock mutex */
	pthread_mutex_lock(&tash_cmds_info.tmutex);

	for (cmd_idx = 0; cmd_idx < tash_cmds_info.count; cmd_idx++) {
		cmd_found = (strncmp(args[0], tash_cmds_info.cmd[cmd_idx].str, TASH_CMD_MAXSTRLENGTH - 1) == 0) ? 1 : 0;
		if (cmd_found) {
				/* unlock mutex before executing */
				pthread_mutex_unlock(&tash_cmds_info.tmutex);

			if (tash_cmds_info.cmd[cmd_idx].exec_type == TASH_EXECMD_SYNC) {
				/* function call to execute SYNC command */
				(*tash_cmds_info.cmd[cmd_idx].cb) (argc, args);
			} else if (tash_cmds_info.cmd[cmd_idx].exec_type == TASH_EXECMD_ASYNC) {
				/* launch a task to execute ASYNC command */
				if (tash_launch_cmdtask(tash_cmds_info.cmd[cmd_idx].cb, argc, args)) {
					shdbg("TASH: error in command task launch \n");
				}
			} else {
				shdbg("TASH: cmd (%s) has wrong value on exec type\n", args[0]);
			}
			break;
		}						/* cmd_found */
	}							/* for */

	if (!cmd_found) {
		/* unlock mutex */
		pthread_mutex_unlock(&tash_cmds_info.tmutex);

		printf("TASH: cmd (%s) not registered\n", args[0]);
	}

	return 0;					/* Need to pass the appropriate error later */
}

/** @name tash_do_autocomplete
 * @brief API to TASH tab commands
 * @ingroup tash
 * @param[in] cmd - command's first string
 * @param[in] pos - length of command's first string
 * @param[in] double_tab : true - tab is pressed more than once
 *                         false - tab is pressed once
 */
bool tash_do_autocomplete(char *cmd, int *pos, bool double_tab)
{
	int cmd_idx;
	bool ret = false;
	int found_cnt = 0;
	int idx;
	uint16_t matched[TASH_MAX_COMMANDS] = {0,};

	/* lock mutex */
	pthread_mutex_lock(&tash_cmds_info.tmutex);

	for (cmd_idx = 0; cmd_idx < tash_cmds_info.count; cmd_idx++) {
		/* search for commands starting with cmd */
		if (strncmp(cmd, tash_cmds_info.cmd[cmd_idx].str, *pos) == 0) {
			matched[found_cnt++] = cmd_idx;
		}
	}

	if (found_cnt == 1 && double_tab == false) {
		idx = matched[0];
		/* If it finds only one, it autocompletes the command */
		while (tash_cmds_info.cmd[idx].str[*pos] != 0) {
			cmd[*pos] = tash_cmds_info.cmd[idx].str[*pos];
			(*pos)++;
		}
		ret = true;
	} else if (found_cnt > 1 && double_tab == true) {
		printf("\n");
		for (int cnt = 0; cnt < found_cnt; cnt++) {
			idx = matched[cnt];
			/* Show commands starting with cmd */
			printf("%-16s ", tash_cmds_info.cmd[idx].str);
			if (cnt % TASH_CMDS_PER_LINE == (TASH_CMDS_PER_LINE - 1) && cnt != found_cnt - 1) {
				printf("\n");
			}
		}
		printf("\n");
		ret = true;
	}
	/* unlock mutex */
	pthread_mutex_unlock(&tash_cmds_info.tmutex);
	return ret;
}

/** @name tash_cmd_install
 * @brief API to install TASH commands
 * @ingroup tash
 * @param[in] str - command's first string(Example: ps, ls, ifconfig etc)
 * @param[in] cb - callback function for the command
 * @param[in] thread_exec: TASH_EXECMD_ASYNC -execute callback as a thread
						   TASH_EXECMD_SYNC- invoke callback directly
 * @retval 0 Success
 * @retval -ve Failure
 *
 * Example Usage:
 * @code
 *
 * @endcode
 */
int tash_cmd_install(const char *str, TASH_CMD_CALLBACK cb, int thread_exec)
{
	int cmd_idx;
	static int fail_cmd_count = 0;

	if (TASH_MAX_COMMANDS == tash_cmds_info.count) {
		printf("Allowed Max tash cmds: %d and Current tash cmd count: %d\n",
				TASH_MAX_COMMANDS, tash_cmds_info.count + ++fail_cmd_count);
		printf("Couldn't install cmd: (%s), Refer CONFIG_TASH_MAX_COMMANDS\n", str);
		return -1;				/* MAX cmd count reached */
	}

	/* Lock mutex */
	pthread_mutex_lock(&tash_cmds_info.tmutex);

	/* CHeck if cmd is already installed */
	for (cmd_idx = 0; cmd_idx < tash_cmds_info.count; cmd_idx++) {
		if (strncmp(str, tash_cmds_info.cmd[cmd_idx].str, TASH_CMD_MAXSTRLENGTH - 1) == 0) {
			pthread_mutex_unlock(&tash_cmds_info.tmutex);
			return -2;			/* CMD already installed */
		}
	}

	/* store command string - no need of explicit NULL termination */
	strncpy(tash_cmds_info.cmd[tash_cmds_info.count].str, str, TASH_CMD_MAXSTRLENGTH - 1);
	/* store callback */
	tash_cmds_info.cmd[tash_cmds_info.count].cb = cb;
	/* store thread_exec flags */
	tash_cmds_info.cmd[tash_cmds_info.count].exec_type = thread_exec;
	/* Increment command count value */
	tash_cmds_info.count++;
	pthread_mutex_unlock(&tash_cmds_info.tmutex);

	is_sorted_list = FALSE;

	return 0;
}

/** @name tash_cmdlist_install
 * @brief API to register TASH command list
 * @ingroup tash
 * @param[in] list[] - tash command list structure which has command name, callback fnc, and attribute
 *
 * Example Usage:
 * @code
 *
 * @endcode
 **/
void tash_cmdlist_install(const tash_cmdlist_t list[])
{
	const tash_cmdlist_t *map;

	for (map = list; map->entry; map++) {
		tash_cmd_install(map->name, map->entry, map->exectype);
	}
}

void tash_register_basic_cmds(void)
{
	tash_cmdlist_install(tash_basic_cmds);
}

#if defined(CONFIG_TASH_COMMAND_INTERFACE)
/** @name tash_get_cmdscount
 * @brief API to get the number of registered tash commands.
 * In protected build, it returns the count of user context commands only.
 * @ingroup tash
 * @ret +ve - Tash commands count
 *     -ve - Error
 *
 * Example Usage:
 * @code
 * @endcode
 **/
int tash_get_cmdscount(void)
{
	int count = 0;

	pthread_mutex_lock(&tash_cmds_info.tmutex);
	count = tash_cmds_info.count;
	pthread_mutex_unlock(&tash_cmds_info.tmutex);

	return count;
}

/** @name tash_get_cmdpair
 * @brief API to get registered tash commands one at a time
 * @ingroup tash
 * @param[in] str- Pointer to get the command string with space for 32 bytes
 * @param[in] cb- Callback function pointer
 * @param[in] index- Tash command index

 * @ret 0 - Successful
 *     -1 - Error
 *
 * Example Usage:
 * @code
	count = tash_get_cmdscount();
	for (i=0; i<count; i++)
		tash_get_cmdpair(str, &cb, i );
 * @endcode
 **/
int tash_get_cmdpair(char *str, TASH_CMD_CALLBACK *cb, int index)
{
	int count = 0;
	int ret = -1;

	pthread_mutex_lock(&tash_cmds_info.tmutex);
	count = tash_cmds_info.count;
	/* NOTE: This has to be modified for protected build */
	if (index <= count) {
		/* Copy string */
		strncpy(str, tash_cmds_info.cmd[index].str, TASH_CMD_MAXSTRLENGTH - 1);
		/* Copy function ptr */
		*cb = tash_cmds_info.cmd[index].cb;
		ret = OK;
	}
	pthread_mutex_unlock(&tash_cmds_info.tmutex);

	return ret;
}
#endif
