/*
VS_IT Copyright(C) 2002 thejam79, 2003 minamina, 2014 msg7086

This program is free software; you can redistribute it and / or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
*/

#pragma once
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <VapourSynth.h>
#include <VSHelper.h>
#include "emmintrin.h"

#ifdef _MSC_VER
#define snprintf(a,b,c) _snprintf_s(a,b,b,c)
#define stricmp _stricmp
#define TS_ALIGN __declspec(align(16))
#define TS_FUNC_ALIGN
#else
#define TS_ALIGN __attribute__((aligned(16)))
#define TS_FUNC_ALIGN __attribute__((force_align_arg_pointer))
#endif

#define PARAM_INT(name, def) int name = int64ToIntS(vsapi->propGetInt(in, #name, 0, &err)); if (err) { name = def; }

#define IT_VERSION "0103." "0.3"

#define FAIL_IF_ERROR(cond, ...) {\
    if (cond) {\
        snprintf(msg, 200, __VA_ARGS__);\
        goto fail;\
    }\
}

#if !defined(_WIN64)
#define rax	eax
#define rbx	ebx
#define rcx	ecx
#define rdx	edx
#define rsi	esi
#define rdi	edi
#define rbp	ebp
#else
#define rax	rax
#define rbx	rbx
#define rcx	rcx
#define rdx	rdx
#define rsi	rsi
#define rdi	rdi
#define rbp	rbp
#endif

#include "IScriptEnvironment.h"
#include "vs_it.h"
