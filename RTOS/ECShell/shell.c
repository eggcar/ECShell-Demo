/**
 * @file	shell.c
 * @brief	Implementation of the shell object
 * 			Import part of linenoise(https://github.com/antirez/linenoise)
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

#include "shell.h"

#include "console_codes.h"
#include "ec_api.h"
#include "ecshell_common.h"
#include "ecshell_exec.h"
#include "ecshell_exec_def.h"
#include "exceptions.h"
#include "readline.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int user_authentication(char *uname, size_t ulen, char *passwd, size_t plen)
{
	// Test only...
	if (strcmp(uname, passwd) == 0) {
		return 0;
	}
	else {
		return -1;
	}
}

void display_welcome(ecshell_t *sh)
{
	return;
}

ecshell_t *ecshell_new(int32_t i_fd, int32_t o_fd, shell_type_t type, uint32_t timeout)
{
	ecshell_t *sh = NULL;
	if ((i_fd < 0) || (o_fd < 0)) {
		goto exit;
	}
	sh = sh_malloc(sizeof(ecshell_t));
	if (sh == NULL) {
		goto exit;
	}
	memset(sh, 0, sizeof(ecshell_t));
	sh->stdin_fd = i_fd;
	sh->stdout_fd = o_fd;
	sh->echo_mode = 1;
	sh->shell_status = e_SHELLSTAT_WaitUserLogin;
	sh->shell_prompt[0] = '\0';
	sh->cmd_len = 0;
	sh->cmd_cursor = 0;
	sh->history_head = 0;
	sh->history_tail = 0;
	sh->history_used = 0;
	sh->history_offset = 0;
	sh->timeout_ms = timeout;
exit:
	return sh;
}

void ecshell_free(ecshell_t *sh)
{
	sh_free(sh);
}

int shell_run(ecshell_t *sh)
{
	int err;
	char outbuff[256];
	char *_user_name;
	char *_password;
	size_t _user_name_len, _password_len;
	ecshell_env_t exec_env;
	if (sh == NULL) {
		err = -EINVAL;
		goto exit;
	}
	if ((sh->stdin_fd < 0) || (sh->stdout_fd < 0)) {
		err = -EBADF;
		goto exit;
	}

	for (;;) {
		switch (sh->shell_status) {
		case e_SHELLSTAT_WaitUserLogin:
			sh->prompt_len = sprintf(sh->shell_prompt, "User Login:");
			err = linenoiseEdit(sh);
			write(sh->stdout_fd, "\r\n", 2);
			if (err > 0) {
				_user_name_len = sh->cmd_len;
				_user_name = sh_malloc(_user_name_len + 1);
				memset(_user_name, 0, _user_name_len + 1);
				if (_user_name == NULL) {
					err = -ENOMEM;
					goto exit;
				}
				strncpy(_user_name, sh->cmd_line, _user_name_len);
				_user_name[_user_name_len] = '\0';
				sh->shell_status = e_SHELLSTAT_WaitUserAuthen;
				sh->echo_mask_mode = 1;
			}
			break;
		case e_SHELLSTAT_WaitUserAuthen:
			sh->prompt_len = sprintf(sh->shell_prompt, "Password:");
			err = linenoiseEdit(sh);
			sh->echo_mask_mode = 0;
			write(sh->stdout_fd, "\r\n", 2);
			if (err > 0) {
				_password_len = sh->cmd_len;
				_password = sh_malloc(sh->cmd_len + 1);
				memset(_password, 0, _password_len + 1);
				if (_password == NULL) {
					// Clear memory, prevent information leakage.
					memset(_user_name, 0, _user_name_len);
					sh_free(_user_name);
					err = -ENOMEM;
					goto exit;
				}
				strncpy(_password, sh->cmd_line, _password_len);
				// Clear buffered password as soon as possible.
				memset(sh->cmd_line, 0, SHELL_LINE_MAXLEN);
				// Now we have username and password, check it.
				if (user_authentication(_user_name, _user_name_len, _password, _password_len) == 0) {
					sh->shell_status = e_SHELLSTAT_NormalCMDLine;
					display_welcome(sh);
				}
				else {
					sh->shell_status = e_SHELLSTAT_WaitUserLogin;
				}
				// Keep user name and free password.
				memset(_password, 0, _password_len);
				sh_free(_password);
			}
			else {
				memset(_user_name, 0, _user_name_len);
				sh_free(_user_name);
				sh->shell_status = e_SHELLSTAT_WaitUserLogin;
			}
			break;
		case e_SHELLSTAT_NormalCMDLine:
			sh->prompt_len = sprintf(sh->shell_prompt, "%s@ecshell>", _user_name);
			err = linenoiseEdit(sh);
			write(sh->stdout_fd, "\r\n", 2);
			if (err > 0) {
				linenoiseHistoryAdd(sh, sh->cmd_line);
				memcpy(&exec_env, sh, sizeof(exec_env));
				sh->shell_status = e_SHELLSTAT_UserProgramIO;
				ecshell_exec_by_line(sh->cmd_line, &exec_env);
				sh->shell_status = e_SHELLSTAT_NormalCMDLine;
			}
			break;
		case e_SHELLSTAT_RecvTelnetIAC:
			// Not implemented yet. May be moved to my modified linenoise processing method later.
			break;
		case e_SHELLSTAT_UserProgramIO:
		default:
			break;
		}
	}

exit:
	return err;
}
