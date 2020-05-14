/**
 * @file	ecshell_exec_def.h
 * @brief	Definitions used for user-implemented executable cmd.
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct ecshell_env_s {
	int32_t stdin_fd;
	int32_t stdout_fd;
	size_t shell_cols;
} ecshell_env_t;

typedef int(ecshell_exec_f)(int, char *[], void *);

typedef struct ecshell_cmd_s {
	ecshell_exec_f *cmd;
	/**
	 * For future extension...
	*/
} ecshell_cmd_t;

#define REGIST_COMMAND(func, name)                       \
	do {                                                 \
		static struct ecshell_cmd_s __cmd_def_##func = { \
			.cmd = func,                                 \
		};                                               \
		ecshell_regist_cmd(&__cmd_def_##func, name);     \
	} while (0)

void ecshell_regist_cmd(ecshell_cmd_t *cmd, char *name);
