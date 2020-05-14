/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2016, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward n chars
 *
 * CUB (CUrsor Backward)
 *    Sequence: ESC [ n D
 *    Effect: moves cursor backward n chars
 *
 * The following is used to get the terminal width if getting
 * the width with the TIOCGWINSZ ioctl fails
 *
 * DSR (Device Status Report)
 *    Sequence: ESC [ 6 n
 *    Effect: reports the current cusor position as ESC [ n ; m R
 *            where n is the row and m is the column
 *
 * When multi line mode is enabled, we also use an additional escape
 * sequence. However multi line editing is disabled by default.
 *
 * CUU (Cursor Up)
 *    Sequence: ESC [ n A
 *    Effect: moves cursor up of n chars.
 *
 * CUD (Cursor Down)
 *    Sequence: ESC [ n B
 *    Effect: moves cursor down of n chars.
 *
 * When linenoiseClearScreen() is called, two additional escape sequences
 * are used in order to clear the screen and position the cursor at home
 * position.
 *
 * CUP (Cursor position)
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED (Erase display)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 *
 */

/**
 * Modified by Eggcar to adapt ECLayer and ECShell.
 * 
 * Rewrite 'history' methods to use ring-buffer instead of original
 * dynamic arrays.
 * 
 * Hints and tab-completion are not fully imported by Eggcar, would
 * be done later.
 * 
 * getColumns() method works with my ubuntu terminal, but doesn't work
 * with my serial terminal softwares. So I turned it off at the moment.
 * 
 * Other functions not used in ECShell are not imported yet. May be done
 * in later version.
*/

#include "readline.h"

#include "ec_api.h"
#include "ecshell_common.h"
#include "shell.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static linenoiseCompletionCallback *completionCallback = NULL;
static linenoiseHintsCallback *hintsCallback = NULL;
static linenoiseFreeHintsCallback *freeHintsCallback = NULL;

/* The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct linenoiseState {
	int ifd;			/* Terminal stdin file descriptor. */
	int ofd;			/* Terminal stdout file descriptor. */
	char *buf;			/* Edited line buffer. */
	size_t buflen;		/* Edited line buffer size. */
	const char *prompt; /* Prompt to display. */
	size_t plen;		/* Prompt length. */
	size_t pos;			/* Current cursor position. */
	size_t oldpos;		/* Previous refresh cursor position. */
	size_t len;			/* Current edited line length. */
	size_t cols;		/* Number of columns in terminal. */
	size_t maxrows;		/* Maximum num of rows used so far (multiline mode) */
	int history_index;	/* The history index we are currently editing. */
};

enum KEY_ACTION {
	KEY_NULL = 0,	/* NULL */
	CTRL_A = 1,		/* Ctrl+a */
	CTRL_B = 2,		/* Ctrl-b */
	CTRL_C = 3,		/* Ctrl-c */
	CTRL_D = 4,		/* Ctrl-d */
	CTRL_E = 5,		/* Ctrl-e */
	CTRL_F = 6,		/* Ctrl-f */
	CTRL_H = 8,		/* Ctrl-h */
	TAB = 9,		/* Tab */
	CTRL_K = 11,	/* Ctrl+k */
	CTRL_L = 12,	/* Ctrl+l */
	ENTER = 13,		/* Enter */
	CTRL_N = 14,	/* Ctrl-n */
	CTRL_P = 16,	/* Ctrl-p */
	CTRL_T = 20,	/* Ctrl-t */
	CTRL_U = 21,	/* Ctrl+u */
	CTRL_W = 23,	/* Ctrl+w */
	ESC = 27,		/* Escape */
	BACKSPACE = 127 /* Backspace */
};

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
static int getCursorPosition(int ifd, int ofd)
{
	char buf[32];
	int cols, rows;
	unsigned int i = 0;

	/* Report cursor location */
	if (write(ofd, "\x1b[6n", 4) != 4)
		return -1;

	/* Read the response: ESC [ rows ; cols R */
	while (i < sizeof(buf) - 1) {
		if (read(ifd, buf + i, 1) != 1)
			break;
		if (buf[i] == 'R')
			break;
		i++;
	}
	buf[i] = '\0';

	/* Parse it. */
	if (buf[0] != ESC || buf[1] != '[')
		return -1;
	if (sscanf(buf + 2, "%d;%d", &rows, &cols) != 2)
		return -1;
	return cols;
}

/* Try to get the number of columns in the current terminal, or assume 80
 * if it fails. */
static int getColumns(int32_t ifd, int32_t ofd)
{
#if 0
	/* ioctl() failed. Try to query the terminal itself. */
	int start, cols;

	/* Get the initial position so we can restore it later. */
	start = getCursorPosition(ifd, ofd);
	if (start == -1)
		goto failed;

	/* Go to right margin and get position. */
	if (write(ofd, "\x1b[999C", 6) != 6)
		goto failed;
	cols = getCursorPosition(ifd, ofd);
	if (cols == -1)
		goto failed;

	/* Restore position. */
	if (cols > start) {
		char seq[32];
		snprintf(seq, 32, "\x1b[%dD", cols - start);
		if (write(ofd, seq, strlen(seq)) == -1) {
			/* Can't recover... */
		}
	}
	return cols;
#endif
failed:
	return 132;
}

/* This is an helper function for linenoiseEdit() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 *
 * The state of the editing is encapsulated into the pointed linenoiseState
 * structure as described in the structure definition. */
static int completeLine(struct linenoiseState *ls)
{
	linenoiseCompletions lc = {0, NULL};
	int nread, nwritten;
	char c = 0;
#if 0
    completionCallback(ls->buf,&lc);
    if (lc.len == 0) {
        linenoiseBeep();
    } else {
        size_t stop = 0, i = 0;

        while(!stop) {
            /* Show completion or original buffer */
            if (i < lc.len) {
                struct linenoiseState saved = *ls;

                ls->len = ls->pos = strlen(lc.cvec[i]);
                ls->buf = lc.cvec[i];
                refreshLine(ls);
                ls->len = saved.len;
                ls->pos = saved.pos;
                ls->buf = saved.buf;
            } else {
                refreshLine(ls);
            }

            nread = read(ls->ifd,&c,1);
            if (nread <= 0) {
                freeCompletions(&lc);
                return -1;
            }

            switch(c) {
                case 9: /* tab */
                    i = (i+1) % (lc.len+1);
                    if (i == lc.len) linenoiseBeep();
                    break;
                case 27: /* escape */
                    /* Re-show original buffer */
                    if (i < lc.len) refreshLine(ls);
                    stop = 1;
                    break;
                default:
                    /* Update buffer and return */
                    if (i < lc.len) {
                        nwritten = snprintf(ls->buf,ls->buflen,"%s",lc.cvec[i]);
                        ls->len = ls->pos = nwritten;
                    }
                    stop = 1;
                    break;
            }
        }
    }

    freeCompletions(&lc);
#endif
	return c; /* Return last read character */
}

/* =========================== Line editing ================================= */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
	char *b;
	int len;
};

static void abInit(struct abuf *ab)
{
	ab->b = NULL;
	ab->len = 0;
}

static void abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = sh_realloc(ab->b, ab->len + len);

	if (new == NULL)
		return;
	memcpy(new + ab->len, s, len);
	ab->b = new;
	ab->len += len;
}

static void abFree(struct abuf *ab)
{
	sh_free(ab->b);
}

/* Helper of refreshSingleLine() and refreshMultiLine() to show hints
 * to the right of the prompt. */
void refreshShowHints(struct abuf *ab, ecshell_t *sh, int plen)
{
	/**
	 * @todo Support of auto-hints would be added in future releases.
	*/
#if 0
    char seq[64];
    if (hintsCallback && plen+sh->cmd_len < sh->shell_cols) {
        int color = -1, bold = 0;
        char *hint = hintsCallback(sh->cmd_line,&color,&bold);
        if (hint) {
            int hintlen = strlen(hint);
            int hintmaxlen = sh->shell_cols-(plen+sh->cmd_len);
            if (hintlen > hintmaxlen) hintlen = hintmaxlen;
            if (bold == 1 && color == -1) color = 37;
            if (color != -1 || bold != 0)
                snprintf(seq,64,"\033[%d;%d;49m",bold,color);
            else
                seq[0] = '\0';
            abAppend(ab,seq,strlen(seq));
            abAppend(ab,hint,hintlen);
            if (color != -1 || bold != 0)
                abAppend(ab,"\033[0m",4);
            /* Call the function to free the hint returned. */
            if (freeHintsCallback) freeHintsCallback(hint);
        }
    }
#endif
}

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
static void refreshSingleLine(ecshell_t *sh)
{
	char seq[64];
	size_t plen = sh->prompt_len;
	int32_t fd = sh->stdout_fd;
	char *buf = (char *)(sh->cmd_line);
	size_t len = sh->cmd_len;
	size_t pos = sh->cmd_cursor;
	struct abuf ab;

	while ((plen + pos) >= sh->shell_cols) {
		buf++;
		len--;
		pos--;
	}
	while (plen + len > sh->shell_cols) {
		len--;
	}

	abInit(&ab);
	/* Cursor to left edge */
	snprintf(seq, 64, "\r");
	abAppend(&ab, seq, strlen(seq));
	/* Write the prompt and the current buffer content */
	abAppend(&ab, (char *)(sh->shell_prompt), sh->prompt_len);
	if (sh->echo_mask_mode) {
		while (len--)
			abAppend(&ab, "*", 1);
	}
	else {
		abAppend(&ab, buf, len);
	}
	/* Show hits if any. */
	refreshShowHints(&ab, sh, plen);
	/* Erase to right */
	snprintf(seq, 64, "\x1b[0K");
	abAppend(&ab, seq, strlen(seq));
	/* Move cursor to original position. */
	snprintf(seq, 64, "\r\x1b[%dC", (int)(pos + plen));
	abAppend(&ab, seq, strlen(seq));
	if (write(fd, ab.b, ab.len) == -1) {
	} /* Can't recover from write error. */
	abFree(&ab);
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
static void refreshMultiLine(ecshell_t *sh)
{
	char seq[64];
	int plen = sh->prompt_len;
	int rows = (plen + sh->cmd_len + sh->shell_cols - 1) / sh->shell_cols;	 /* rows used by current buf. */
	int rpos = (plen + sh->cmd_oldcursor + sh->shell_cols) / sh->shell_cols; /* cursor relative row. */
	int rpos2;																 /* rpos after refresh. */
	int col;																 /* colum position, zero-based. */
	int old_rows = sh->shell_used_rows;
	int32_t fd = sh->stdout_fd;
	int j;
	struct abuf ab;

	/* Update maxrows if needed. */
	if (rows > (int)sh->shell_used_rows)
		sh->shell_used_rows = rows;

	/* First step: clear all the lines used before. To do so start by
     * going to the last row. */
	abInit(&ab);
	if (old_rows - rpos > 0) {
		snprintf(seq, 64, "\x1b[%dB", old_rows - rpos);
		abAppend(&ab, seq, strlen(seq));
	}

	/* Now for every row clear it, go up. */
	for (j = 0; j < old_rows - 1; j++) {
		snprintf(seq, 64, "\r\x1b[0K\x1b[1A");
		abAppend(&ab, seq, strlen(seq));
	}

	/* Clean the top line. */
	snprintf(seq, 64, "\r\x1b[0K");
	abAppend(&ab, seq, strlen(seq));

	/* Write the prompt and the current buffer content */
	abAppend(&ab, (char *)(sh->shell_prompt), sh->prompt_len);
	if (sh->echo_mask_mode) {
		unsigned int i;
		for (i = 0; i < sh->cmd_len; i++)
			abAppend(&ab, "*", 1);
	}
	else {
		abAppend(&ab, (char *)(sh->cmd_line), sh->cmd_len);
	}

	/* Show hits if any. */
	refreshShowHints(&ab, sh, plen);

	/* If we are at the very end of the screen with our prompt, we need to
     * emit a newline and move the prompt to the first column. */
	if (sh->cmd_cursor &&
		sh->cmd_cursor == sh->cmd_len &&
		(sh->cmd_cursor + plen) % sh->shell_cols == 0) {
		abAppend(&ab, "\n", 1);
		snprintf(seq, 64, "\r");
		abAppend(&ab, seq, strlen(seq));
		rows++;
		if (rows > (int)sh->shell_used_rows)
			sh->shell_used_rows = rows;
	}

	/* Move cursor to right position. */
	rpos2 = (plen + sh->cmd_cursor + sh->shell_cols) / sh->shell_cols; /* current cursor relative row. */

	/* Go up till we reach the expected positon. */
	if (rows - rpos2 > 0) {
		snprintf(seq, 64, "\x1b[%dA", rows - rpos2);
		abAppend(&ab, seq, strlen(seq));
	}

	/* Set column. */
	col = (plen + (int)sh->cmd_cursor) % (int)sh->shell_cols;
	if (col)
		snprintf(seq, 64, "\r\x1b[%dC", col);
	else
		snprintf(seq, 64, "\r");
	abAppend(&ab, seq, strlen(seq));

	sh->cmd_oldcursor = sh->cmd_cursor;

	if (write(fd, ab.b, ab.len) == -1) {
	} /* Can't recover from write error. */
	abFree(&ab);
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
static void refreshLine(ecshell_t *sh)
{
	if (sh->multiline_mode)
		refreshMultiLine(sh);
	else
		refreshSingleLine(sh);
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
int linenoiseEditInsert(ecshell_t *sh, char c)
{
	if (sh->cmd_len < SHELL_LINE_MAXLEN - 1) {
		if (sh->cmd_len == sh->cmd_cursor) {
			sh->cmd_line[sh->cmd_cursor] = c;
			sh->cmd_cursor++;
			sh->cmd_len++;
			sh->cmd_line[sh->cmd_len] = '\0';
			if ((!sh->multiline_mode && sh->prompt_len + sh->cmd_len < sh->shell_cols && !hintsCallback)) {
				/* Avoid a full update of the line in the
                 * trivial case. */
				char d = (sh->echo_mask_mode) ? '*' : c;
				if (write(sh->stdout_fd, &d, 1) == -1)
					return -1;
			}
			else {
				refreshLine(sh);
			}
		}
		else {
			memmove(sh->cmd_line + sh->cmd_cursor + 1, sh->cmd_line + sh->cmd_cursor, sh->cmd_len - sh->cmd_cursor);
			sh->cmd_line[sh->cmd_cursor] = c;
			sh->cmd_len++;
			sh->cmd_cursor++;
			sh->cmd_line[sh->cmd_len] = '\0';
			refreshLine(sh);
		}
	}
	return 0;
}

/* Move cursor on the left. */
void linenoiseEditMoveLeft(ecshell_t *sh)
{
	if (sh->cmd_cursor > 0) {
		sh->cmd_cursor--;
		refreshLine(sh);
	}
}

/* Move cursor on the right. */
void linenoiseEditMoveRight(ecshell_t *sh)
{
	if (sh->cmd_cursor != sh->cmd_len) {
		sh->cmd_cursor++;
		refreshLine(sh);
	}
}

/* Move cursor to the start of the line. */
void linenoiseEditMoveHome(ecshell_t *sh)
{
	if (sh->cmd_cursor != 0) {
		sh->cmd_cursor = 0;
		refreshLine(sh);
	}
}

/* Move cursor to the end of the line. */
void linenoiseEditMoveEnd(ecshell_t *sh)
{
	if (sh->cmd_cursor != sh->cmd_len) {
		sh->cmd_cursor = sh->cmd_len;
		refreshLine(sh);
	}
}

/* This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle. */
int linenoiseHistoryAdd(ecshell_t *sh, const char *line)
{
	char *linecopy;
	int index;

	if (sh->history_used > 0) {
		if (sh->history_tail == 0) {
			index = SHELL_HISTORY_MAXNUM - 1;
		}
		else {
			index = sh->history_tail - 1;
		}

		/* Don't add duplicated lines. */
		if (!strcmp((char *)(sh->cmd_history[index]), (char *)line)) {
			return 0;
		}
	}

	if (sh->history_used >= SHELL_HISTORY_MAXNUM) {
		sh->cmd_history[sh->history_head][0] = '\0';
		if (sh->history_head == (sh->history_used - 1)) {
			sh->history_head = 0;
		}
		else {
			sh->history_head++;
		}
		sh->history_used--;
	}
	memcpy(sh->cmd_history[sh->history_tail], line, SHELL_LINE_MAXLEN);
	sh->history_used++;
	if (sh->history_tail == SHELL_HISTORY_MAXNUM - 1) {
		sh->history_tail = 0;
	}
	else {
		sh->history_tail++;
	}
	return 1;
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
void linenoiseEditHistoryNext(ecshell_t *sh, int dir)
{
	size_t index;
	if (sh->history_used > 0) {
		if (sh->history_tail <= sh->history_offset) {
			index = SHELL_HISTORY_MAXNUM - 1;
			index += sh->history_tail;
			index -= sh->history_offset;
		}
		else {
			index = sh->history_tail - 1 - sh->history_offset;
		}
		/* Update the current history entry before to
         * overwrite it with the next one. */
		memset(sh->cmd_history[index], 0, SHELL_LINE_MAXLEN);
		memcpy(sh->cmd_history[index], sh->cmd_line, SHELL_LINE_MAXLEN);
		if (dir == LINENOISE_HISTORY_PREV) {
			sh->history_offset++;
			if (sh->history_offset >= sh->history_used) {
				sh->history_offset = sh->history_used - 1;
				return;
			}
		}
		else if (sh->history_offset == 0) {
			return;
		}
		else {
			sh->history_offset--;
		}
		if (sh->history_tail <= sh->history_offset) {
			index = SHELL_HISTORY_MAXNUM - 1;
			index += sh->history_tail;
			index -= sh->history_offset;
		}
		else {
			index = sh->history_tail - 1 - sh->history_offset;
		}
		memset(sh->cmd_line, 0, SHELL_LINE_MAXLEN);
		strncpy((char *)(sh->cmd_line), (char *)(sh->cmd_history[index]), SHELL_LINE_MAXLEN);
		sh->cmd_line[SHELL_LINE_MAXLEN - 1] = '\0';
		sh->cmd_len = sh->cmd_cursor = strlen((char *)(sh->cmd_line));
		refreshLine(sh);
	}
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
void linenoiseEditDelete(ecshell_t *sh)
{
	if (sh->cmd_len > 0 && sh->cmd_cursor < sh->cmd_len) {
		memmove(sh->cmd_line + sh->cmd_cursor, sh->cmd_line + sh->cmd_cursor + 1, sh->cmd_len - sh->cmd_cursor - 1);
		sh->cmd_len--;
		sh->cmd_line[sh->cmd_len] = '\0';
		refreshLine(sh);
	}
}

/* Backspace implementation. */
void linenoiseEditBackspace(ecshell_t *sh)
{
	if (sh->cmd_cursor > 0 && sh->cmd_len > 0) {
		memmove(sh->cmd_line + sh->cmd_cursor - 1, sh->cmd_line + sh->cmd_cursor, sh->cmd_len - sh->cmd_cursor);
		sh->cmd_cursor--;
		sh->cmd_len--;
		sh->cmd_line[sh->cmd_len] = '\0';
		refreshLine(sh);
	}
}

/* Delete the previosu word, maintaining the cursor at the start of the
 * current word. */
void linenoiseEditDeletePrevWord(ecshell_t *sh)
{
	size_t old_pos = sh->cmd_cursor;
	size_t diff;

	while (sh->cmd_cursor > 0 && sh->cmd_line[sh->cmd_cursor - 1] == ' ')
		sh->cmd_cursor--;
	while (sh->cmd_cursor > 0 && sh->cmd_line[sh->cmd_cursor - 1] != ' ')
		sh->cmd_cursor--;
	diff = old_pos - sh->cmd_cursor;
	memmove(sh->cmd_line + sh->cmd_cursor, sh->cmd_line + old_pos, sh->cmd_len - old_pos + 1);
	sh->cmd_len -= diff;
	refreshLine(sh);
}

/* Clear the screen. Used to handle ctrl+l */
void linenoiseClearScreen(ecshell_t *sh)
{
	if (write(sh->stdout_fd, "\x1b[H\x1b[2J", 7) <= 0) {
		/* nothing to do, just to avoid warning. */
	}
}

/* This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer. */
int linenoiseEdit(ecshell_t *sh)
{
	//struct linenoiseState l;

	/* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
	sh->prompt_len = strlen((char *)(sh->shell_prompt));
	sh->cmd_oldcursor = sh->cmd_cursor = 0;
	sh->cmd_len = 0;
	sh->shell_cols = getColumns(sh->stdin_fd, sh->stdout_fd);
	sh->shell_used_rows = 0;

	sh->history_offset = 0;

	/* Buffer starts empty. */
	sh->cmd_line[0] = '\0';

	/* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
	linenoiseHistoryAdd(sh, "");

	if (write(sh->stdout_fd, (char *)(sh->shell_prompt), sh->prompt_len) == -1) {
		return -1;
	}
	while (1) {
		char c;
		int nread;
		char seq[3];

		nread = read(sh->stdin_fd, &c, 1);
		if (nread <= 0) {
			return sh->cmd_len;
		}

		/* Only autocomplete when the callback is set. It returns < 0 when
         * there was an error reading from fd. Otherwise it will return the
         * character that should be handled next. */
#if 0
		if (c == 9 && completionCallback != NULL) {
			c = completeLine(&l);
			/* Return on errors */
			if (c < 0)
				return sh->cmd_len;
			/* Read next character when 0 */
			if (c == 0)
				continue;
		}
#else
		/**
		 * @todo Support of tab-complete would be added in the next release.
		*/
		if (c == 9) {
			continue;
		}
#endif

		switch (c) {
		case ENTER: /* enter */
			if (sh->history_used > 0) {
				sh->history_used--;
				if (sh->history_tail == 0) {
					sh->history_tail = SHELL_HISTORY_MAXNUM - 1;
				}
				else {
					sh->history_tail--;
				}
				sh->cmd_history[sh->history_tail][0] = '\0';
			}
			if (sh->multiline_mode) {
				linenoiseEditMoveEnd(sh);
			}
			/**
			 * @todo Support of auto-hints would be added in later releases.
			*/
#if 0
			if (hintsCallback) {
				/* Force a refresh without hints to leave the previous
                 * line as the user typed it after a newline. */
				linenoiseHintsCallback *hc = hintsCallback;
				hintsCallback = NULL;
				refreshLine(&l);
				hintsCallback = hc;
			}
#endif
			return (int)sh->cmd_len;
		case CTRL_C: /* ctrl-c */
			errno = EAGAIN;
			return -1;
		case BACKSPACE: /* backspace */
		case 8:			/* ctrl-h */
			linenoiseEditBackspace(sh);
			break;
		case CTRL_D: /* ctrl-d, remove char at right of cursor, or if the
                            line is empty, act as end-of-file. */
			if (sh->cmd_len > 0) {
				linenoiseEditDelete(sh);
			}
			else {
				sh->history_used--;
				if (sh->history_tail == 0) {
					sh->history_tail = SHELL_HISTORY_MAXNUM - 1;
				}
				else {
					sh->history_tail--;
				}
				sh->cmd_history[sh->history_tail][0] = '\0';
				return -1;
			}
			break;
		case CTRL_T: /* ctrl-t, swaps current character with previous. */
			if (sh->cmd_cursor > 0 && sh->cmd_cursor < sh->cmd_len) {
				int aux = sh->cmd_line[sh->cmd_cursor - 1];
				sh->cmd_line[sh->cmd_cursor - 1] = sh->cmd_line[sh->cmd_cursor];
				sh->cmd_line[sh->cmd_cursor] = aux;
				if (sh->cmd_cursor != sh->cmd_len - 1)
					sh->cmd_cursor++;
				refreshLine(sh);
			}
			break;
		case CTRL_B: /* ctrl-b */
			linenoiseEditMoveLeft(sh);
			break;
		case CTRL_F: /* ctrl-f */
			linenoiseEditMoveRight(sh);
			break;
		case CTRL_P: /* ctrl-p */
			linenoiseEditHistoryNext(sh, LINENOISE_HISTORY_PREV);
			break;
		case CTRL_N: /* ctrl-n */
			linenoiseEditHistoryNext(sh, LINENOISE_HISTORY_NEXT);
			break;
		case ESC: /* escape sequence */
			/* Read the next two bytes representing the escape sequence.
             * Use two calls to handle slow terminals returning the two
             * chars at different times. */
			if (read(sh->stdin_fd, seq, 1) == -1)
				break;
			if (read(sh->stdin_fd, seq + 1, 1) == -1)
				break;

			/* ESC [ sequences. */
			if (seq[0] == '[') {
				if (seq[1] >= '0' && seq[1] <= '9') {
					/* Extended escape, read additional byte. */
					if (read(sh->stdin_fd, seq + 2, 1) == -1)
						break;
					if (seq[2] == '~') {
						switch (seq[1]) {
						case '3': /* Delete key. */
							linenoiseEditDelete(sh);
							break;
						}
					}
				}
				else {
					switch (seq[1]) {
					case 'A': /* Up */
						linenoiseEditHistoryNext(sh, LINENOISE_HISTORY_PREV);
						break;
					case 'B': /* Down */
						linenoiseEditHistoryNext(sh, LINENOISE_HISTORY_NEXT);
						break;
					case 'C': /* Right */
						linenoiseEditMoveRight(sh);
						break;
					case 'D': /* Left */
						linenoiseEditMoveLeft(sh);
						break;
					case 'H': /* Home */
						linenoiseEditMoveHome(sh);
						break;
					case 'F': /* End*/
						linenoiseEditMoveEnd(sh);
						break;
					}
				}
			}

			/* ESC O sequences. */
			else if (seq[0] == 'O') {
				switch (seq[1]) {
				case 'H': /* Home */
					linenoiseEditMoveHome(sh);
					break;
				case 'F': /* End*/
					linenoiseEditMoveEnd(sh);
					break;
				}
			}
			break;
		default:
			if (linenoiseEditInsert(sh, c))
				return -1;
			break;
		case CTRL_U: /* Ctrl+u, delete the whole line. */
			sh->cmd_line[0] = '\0';
			sh->cmd_cursor = sh->cmd_len = 0;
			refreshLine(sh);
			break;
		case CTRL_K: /* Ctrl+k, delete from current to end of line. */
			sh->cmd_line[sh->cmd_cursor] = '\0';
			sh->cmd_len = sh->cmd_cursor;
			refreshLine(sh);
			break;
		case CTRL_A: /* Ctrl+a, go to the start of the line */
			linenoiseEditMoveHome(sh);
			break;
		case CTRL_E: /* ctrl+e, go to the end of the line */
			linenoiseEditMoveEnd(sh);
			break;
		case CTRL_L: /* ctrl+l, clear screen */
			linenoiseClearScreen(sh);
			refreshLine(sh);
			break;
		case CTRL_W: /* ctrl+w, delete previous word */
			linenoiseEditDeletePrevWord(sh);
			break;
		}
	}
	return sh->cmd_len;
}
