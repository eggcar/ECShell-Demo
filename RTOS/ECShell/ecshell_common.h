/**
 * @file	ecshell_common.h
 * @brief	Common definitions used in ECShell internally.
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

/**
 * Choose memory pool wrapper type of EClayer.
 * Or implement your own memory pool functions.
*/
#define USE_EXT_RAM 0

#if USE_EXT_RAM
#	define sh_malloc(x)	 	ecmalloc_ext(x)
#	define sh_free(x)		ecfree_ext(x)
#	define sh_calloc(n, s)	eccalloc_ext(n, s)
#	define sh_realloc(p, s)	ecrealloc_ext(p, s)
#else
#	define sh_malloc(x) ecmalloc(x)
#	define sh_free(x)	 ecfree(x)
#	define sh_calloc(n, s)	eccalloc(n, s)
#	define sh_realloc(p, s)	ecrealloc(p, s)
#endif

