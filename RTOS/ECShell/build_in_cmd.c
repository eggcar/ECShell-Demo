/**
 * @file	build_in_cmd.c
 * @brief	ECShell build-in commands. Also as an example of howto write your own command.
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

#include "console_codes.h"
#include "ec_api.h"
#include "ecshell_exec_def.h"
#include "optparse.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

int ecshell_cmd_clear_screen(int argc, char *argv[], void *env)
{
	int32_t ifd, ofd;
	ifd = ((ecshell_env_t *)env)->stdin_fd;
	ofd = ((ecshell_env_t *)env)->stdout_fd;

	const char help_info[] =
		CSI_SGR(SGR_COL_FRONT(COL_CYAN)) "clear" CSI_SGR(SGR_COL_FRONT(COL_DEFAULT)) "\r\n"
																					 "Clear screen.\r\n";
	const char err_info[] =
		CSI_SGR(SGR_COL_FRONT(COL_RED)) "Invalid argument.\r\n" CSI_SGR(SGR_COL_FRONT(COL_DEFAULT)) "\r\n";
	struct optparse_long longopts[] = {
		{"help", 'h', OPTPARSE_NONE},
		{0},
	};
	struct optparse options;
	optparse_init(&options, argv);
	int option;

	while ((option = optparse_long(&options, longopts, NULL)) != -1) {
		switch (option) {
		case 'h':
			write(ofd, help_info, strlen(help_info));
			return 0;
		default:
			write(ofd, err_info, strlen(err_info));
			return 0;
		}
	}

	write(ofd, "\x1b[H\x1b[2J", 7);
	return 0;
}
