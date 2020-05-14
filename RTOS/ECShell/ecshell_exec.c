/**
 * @file	ecshell_exec.c
*/

/**
 * MIT License
 * 
 * Copyright (c) 2020 Eggcar(eggcar at qq.com or eggcar.luan at gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "ecshell_exec.h"

#include "avlhash.h"
#include "console_codes.h"
#include "ecshell_common.h"
#include "ecshell_exec_def.h"
#include "exceptions.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

static struct avl_hash_map cmd_map;

static size_t BKDRHash(const uint8_t *str)
{
	size_t hash = 0;
	size_t ch;
	if (str == NULL) {
		return 0;
	}
	while ((ch = (size_t)*str++) != 0) {
		hash = hash * 131 + ch;
	}
	return hash;
}

extern int ecshell_cmd_clear_screen(int argc, char *argv[], void *env);

void ecshell_cmd_map_init(void)
{
	avl_map_init(&cmd_map, BKDRHash, strcmp);
	/** Begin to regist your own cmds */
	REGIST_COMMAND(ecshell_cmd_clear_screen, "clear");
}

void ecshell_regist_cmd(ecshell_cmd_t *cmd, char *name)
{
	int success;
	avl_map_add(&cmd_map, name, cmd, &success);
}

ecshell_cmd_t *ecshell_get_cmd_by_name(char *name)
{
	return avl_map_get(&cmd_map, name);
}

int split_line_to_argv(char line[], char *argv[], int max_argc)
{
	enum {
		SKIP_SPACE,
		SKIP_NONSPACE,
		IN_DOUBLE_QUOTE,
		IN_SINGLE_QUOTE,
	} state;
	int argc = 0;
	int index = 0;
	for (state = SKIP_SPACE; (argc < max_argc) && (line[index] != '\0'); index++) {
		switch (state) {
		case SKIP_SPACE:
			if (isspace(line[index])) {
			}
			else {
				switch (line[index]) {
				case '\"':
					argv[argc++] = &line[index + 1];
					state = IN_DOUBLE_QUOTE;
					break;
				case '\'':
					argv[argc++] = &line[index + 1];
					state = IN_SINGLE_QUOTE;
					break;
				default:
					argv[argc++] = &line[index];
					state = SKIP_NONSPACE;
					break;
				}
			}
			break;
		case SKIP_NONSPACE:
			if ((line[index] != '\0') && (!isspace(line[index]))) {
			}
			else {
				line[index] = '\0';
				state = SKIP_SPACE;
			}
			break;
		case IN_DOUBLE_QUOTE:
			switch (line[index]) {
			case '\"':
				line[index] = '\0';
				state = SKIP_SPACE;
				break;
			case '\0':
				// Quote doesn't come in pair, return error.
				return -1;
			default:
				break;
			}
			break;
		case IN_SINGLE_QUOTE:
			switch (line[index]) {
			case '\'':
				line[index] = '\0';
				state = SKIP_SPACE;
				break;
			case '\0':
				// Quote doesn't come in pair, return error.
				return -1;
			default:
				break;
			}
			break;
		}
	}
	return argc;
}

#define MAX_ARGC 64
int ecshell_exec_by_line(const char line[], ecshell_env_t *env)
{
	int err;
	char *argv[MAX_ARGC];
	size_t len = strlen(line);
	char *new_line = sh_malloc(len + 1);

	const char perror_cmd_404[] =
		CSI_SGR(SGR_COL_FRONT(COL_RED)) "Command not found.\r\n" CSI_SGR(SGR_COL_FRONT(COL_DEFAULT));

	const char perror_cmd_400[] =
		CSI_SGR(SGR_COL_FRONT(COL_RED)) "Error while parsing command line.\r\n" CSI_SGR(SGR_COL_FRONT(COL_DEFAULT));

	if (new_line == NULL) {
		return -ENOMEM;
	}
	memcpy(new_line, line, len);
	new_line[len] = '\0';

	memset(argv, NULL, sizeof(argv));
	int argc = split_line_to_argv(new_line, argv, MAX_ARGC);
	ecshell_cmd_t *cmd;
	if (argc > 0) {
		cmd = ecshell_get_cmd_by_name(argv[0]);
		if (cmd != NULL) {
			err = cmd->cmd(argc, argv, env);
		}
		else {
			write(env->stdout_fd, perror_cmd_404, sizeof(perror_cmd_404));
			err = -ENOENT;
		}
	}
	else {
		write(env->stdout_fd, perror_cmd_400, sizeof(perror_cmd_400));
		err = -EINVAL;
	}
	sh_free(new_line);
	return err;
}
