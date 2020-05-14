/**
 * @file	console_codes.h
 * @brief	Console control codes definition
 * @author 	EggCar
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

#include "va_macros.h"

#define CC_ETX "\x03" /**< ^C */
#define CC_BEL "\x07" /**< Beeps */
#define CC_BS  "\x08" /**< Backspaces one column (but not past the beginning of the line) */
#define CC_HT  "\x09" /**< Goes to the next stop to the end of the line if there is no earlier tab stop */
#define CC_LF  "\x0A" /**< Give a linefeed, and if LF/NL (new-line mode) is set also a carriage return */
#define CC_CR  "\x0D" /**< Gives a carriage return */
#define CC_SO  "\x0E" /**< Activates the G1 character set */
#define CC_SI  "\x0F" /**< Activates the G0 character set */
#define CC_CAN "\x18" /**< Interrupt escape sequences */
#define CC_SUB "\x1A" /**< Interrupt escape sequences */
#define CC_ESC "\033" /**< Escape */
#define CC_DEL "\x7F" /**< DEL ignored */

#define SQ_RIS	"\033c"	 /**< Reset */
#define SQ_IND	"\033D"	 /**< Linefeed */
#define SQ_NEL	"\033E"	 /**< New line */
#define SQ_HTS	"\033H"	 /**< Set tab stop at current column */
#define SQ_RI	"\033M"	 /**< Reverse line feed */
#define SQ_CHSU "\033%G" /**< Select UTF-8 character set */
#define SQ_CHSD "\033%@" /**< Select default character set (ISO 646 / ISO8859-1) */

#define CSI_CUB(n) "\033[" #n "D" /**< Cursor backward. The distance moved is determined by the parameter. */
#define CSI_CUD(n) "\033[" #n "B" /**< Cursor down. The number of lines moved is determined by the parameter. */
#define CSI_CUF(n) "\033[" #n "C" /**< Cursor forward. The distance moved is determined by the parameter. */
#define CSI_CUU(n) "\033[" #n "A" /**< Cursor up. The number of lines moved is determined by the parameter. */

#define CSI_CUP(x, y) "\033[" #x ";" #y "H" /**< Cursor position. Moves the active position to the position specified by the parameters. */

/**
 * This sequence erases some or all of the characters in the display according to the parameter. 
 * 0 	Erase from the active position to the end of the screen, inclusive (default)
 * 1 	Erase from start of the screen to the active position, inclusive
 * 2 	Erase all of the display – all lines are erased, changed to single-width, and the cursor does not move.
 */
#define CSI_ED(s) "\033[" #s "J"

/**
 * Erases some or all characters in the active line according to the parameter.
 * 0 	Erase from the active position to the end of the line, inclusive (default)
 * 1 	Erase from the start of the screen to the active position, inclusive
 * 2 	Erase all of the line, inclusive
*/
#define CSI_EL(s) "\033[" #s "K"

/**
 * Moves the active position to the position specified by the parameters. 
 * This sequence has two parameter values, the first specifying the line position and the second specifying the column.
 * A parameter value of either zero or one causes the active position to move to the first line or column in the display, respectively. 
 * The default condition with no parameters present moves the active position to the home position. In the VT100, this control behaves 
 * identically with its editor function counterpart, CUP. The numbering of lines and columns depends on the reset or set state of the 
 * origin mode (DECOM).
*/
#define CSI_HVP(x, y) "\033[" #x ";" #y "f"

/**
 * Form a CSI-SGR sequence using this macro.
 * The ECMA-48 SGR sequence ESC [ parameters m sets display attributes.  Several attributes can be set in the same sequence, 
 * separated by semicolons.  An empty parameter (between semicolons or string initiator or terminator) is interpreted as a zero.
 *
 *	param     result
 *	"0"       reset all attributes to their defaults
 *	"1"       set bold
 *	"2"       set half-bright (simulated with color on a color display)
 *	"4"       set underscore (simulated with color on a color display) (the colors used to sim‐
 *	          ulate dim or underline are set using ESC ] ...)
 *	"5"       set blink
 *	"7"       set reverse video
 *	"10"      reset  selected mapping, display control flag, and toggle meta flag (ECMA-48 says
 *	          "primary font").
 *	"11"      select null mapping, set display control flag, reset toggle  meta  flag  (ECMA-48
 *	          says "first alternate font").
 *	"12"      select null mapping, set display control flag, set toggle meta flag (ECMA-48 says
 *	          "second alternate font").  The toggle meta flag causes the high bit of a byte  to
 *	          be toggled before the mapping table translation is done.
 *	"21"      set normal intensity (ECMA-48 says "doubly underlined")
 *	"22"      set normal intensity
 *	"24"      underline off
 *	"25"      blink off
 *	"27"      reverse video off
 *	"30"      set black foreground
 *	"31"      set red foreground
 *	"32"      set green foreground
 *	"33"      set brown foreground
 *	"34"      set blue foreground
 *	"35"      set magenta foreground
 *	"36"      set cyan foreground
 *	"37"      set white foreground
 *	"38"      set underscore on, set default foreground color
 *	"39"      set underscore off, set default foreground color
 *	"40"      set black background
 *	"41"      set red background
 *	"42"      set green background
 *	"43"      set brown background
 *	"44"      set blue background
 *	"45"      set magenta background
 *	"46"      set cyan background
 *	"47"      set white background
 *	"49"      set default background color
*/
#define CSI_SGR(...)                                \
	CONCAT2(CSI_SGR_, COUNT_ARGUMENTS(__VA_ARGS__)) \
	(__VA_ARGS__)

#define _CODE_TO_STRING(x) #x

#define CSI_SGR_0(s)		  "\033[m"
#define CSI_SGR_1(s)		  "\033[" s "m"
#define CSI_SGR_2(s, t)		  "\033[" s ";" t "m"
#define CSI_SGR_3(s, t, u)	  "\033[" s ";" t ";" u "m"
#define CSI_SGR_4(s, t, u, v) "\033[" s ";" t ";" u ";" v "m"

#define COL_BLACK	0 /**< ANSI defined color code: Black */
#define COL_RED		1 /**< ANSI defined color code: Red */
#define COL_GREEN	2 /**< ANSI defined color code: Green */
#define COL_YELLOW	3 /**< ANSI defined color code: Yellow */
#define COL_BLUE	4 /**< ANSI defined color code: Blue */
#define COL_MAGENTA 5 /**< ANSI defined color code: Magenta */
#define COL_CYAN	6 /**< ANSI defined color code: Cyan */
#define COL_WHITE	7 /**< ANSI defined color code: White */
#define COL_DEFAULT 9 /**< Set to default color */
// clang-format off
#define COL_8BIT(s) 8;5;s /**< 8-bit pre-defined 256 color code. */

/**
 * True RGB 24-bit color code
 * RGB is not supported by some popular terminal.
 * So we provide static macro only.
*/
#define COL_RGB(r,g,b) 8;2;r;g;b
// clang-format on
#define SGR_COL_FRONT(s)		"3" _CODE_TO_STRING(s)
#define SGR_COL_BACK(s)			"4" _CODE_TO_STRING(s)
#define SGR_COL_BRIGHT_FRONT(s) "9" _CODE_TO_STRING(s)
#define SGR_COL_BRIGHT_BACK(s)	"10" _CODE_TO_STRING(s)

#define SGR_FONT_BOLD		"1"
#define SGR_FONT_FAINT		"2"
#define SGR_FONT_ITALIC		"3" /**< Not widely supported */
#define SGR_FONT_UNDERLINE	"4"
#define SGR_FONT_BLINK_S	"5"	 /**< Blink slowly */
#define SGR_FONT_BLINK_F	"6"	 /**< Blink fastly, not widely supported. */
#define SGR_FONT_INVERSE	"7"	 /**< Inverse front and background color. */
#define SGR_FONT_HIDE		"8"	 /**< Hide text, not widely supported. */
#define SGR_FONT_STRIKE		"9"	 /**< Delete line, not widely supported. */
#define SGR_FONT_DEFAULT	"10" /**< Default font. */
#define SGR_FONT_ALT1		"11" /**< Select null mapping, set display control flag, reset toggle meta flag (ECMA-48 says "first alternate font"). */
#define SGR_FONT_ALT2		"12" /**< Select null mapping, set display control flag, set toggle meta flag (ECMA-48 says "second alternate font"). */
#define SGR_FONT_DOUBLELINE "21" /**< Set normal intensity (ECMA-48 says "doubly underlined") */
#define SGR_FONT_BOLDOFF	"22" /**< Set normal intensity, clear bold or faint. */
#define SGR_FONT_ITALICOFF	"23" /**< Clear italic, not widely supported. */
#define SGR_FONT_LINEOFF	"24" /**< Clear under line. */
#define SGR_FONT_BLINKOFF	"25" /**< Clear blink. */
#define SGR_FONT_INVERSEOFF "27" /**< Inverse color off. */
