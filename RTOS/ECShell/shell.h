/**
 * @file	shell.h
 * @brief	Header file of the shell object
 * @author	Eggcar
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

#pragma once

#include "ec_api.h"
#include "ec_config.h"

#include <stdint.h>

#define SHELL_HISTORY_MAXNUM 16
#define SHELL_LINE_MAXLEN	 256
#define SHELL_PROMPT_MAXLEN	 64

typedef enum shell_status_s {
	e_SHELLSTAT_WaitUserLogin,
	e_SHELLSTAT_WaitUserAuthen,
	e_SHELLSTAT_NormalCMDLine,
	e_SHELLSTAT_RecvTelnetIAC,
	e_SHELLSTAT_UserProgramIO,
} shell_status_t;

typedef enum shell_type_s {
	e_SHELLTYPE_Default,
	e_SHELLTYPE_Telnet, /**< If shell is defined as telnet type, it supports telnet IAC sequences. */
} shell_type_t;

typedef struct ecshell_s {
	struct {
		int32_t stdin_fd;  /**< Input file descriptor number */
		int32_t stdout_fd; /**< Output file descriptor number. Under most situations equals to stdin_fd */
		size_t shell_cols;
	};
	union {
		int32_t shell_flags;
		struct {
			int echo_mode : 1;
			int multiline_mode : 1;
			int echo_mask_mode : 1;
		};
	};
	shell_status_t shell_status;
	size_t shell_used_rows;
	struct {
		char shell_prompt[SHELL_PROMPT_MAXLEN];
		size_t prompt_len;
	};
	struct {
		char cmd_line[SHELL_LINE_MAXLEN];
		size_t cmd_len;
		size_t cmd_cursor;
		size_t cmd_oldcursor;
	};
	struct {
		size_t history_head;
		size_t history_tail;
		size_t history_used;
		size_t history_offset; /**< History search offset cursor, start from tail, grows towards head. */
		char cmd_history[SHELL_HISTORY_MAXNUM][SHELL_LINE_MAXLEN];
	};
	/** 
	 * Timeout if no interaction, return to login. 0 for never timeout.
	 * @todo Not implemented yet.
	 */
	uint32_t timeout_ms;
} ecshell_t;

ecshell_t *ecshell_new(int32_t i_fd, int32_t o_fd, shell_type_t type, uint32_t timeout);

void ecshell_free(ecshell_t *sh);

int shell_run(ecshell_t *sh);

void ecshell_cmd_map_init(void);
