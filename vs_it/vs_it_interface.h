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
#include <VapourSynth.h>
#include <VSHelper.h>
#include "emmintrin.h"
#include <algorithm>

#ifdef _MSC_VER
#define alignas(x) __declspec(align(x))
#define ALIGNED_ARRAY(decl, alignment) alignas(alignment) decl
#else
#define __forceinline inline
#define ALIGNED_ARRAY(decl, alignment) __attribute__((aligned(16))) decl
#endif

#define IT_VERSION "0103." "0.3"

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

struct CFrameInfo {
    char pos;
    char match;
    char matchAcc;
    char ip;
    char out;
    char mflag;
    int diffP0;
    int diffP1;
    int diffS0;
    int diffS1;
    long ivC, ivP, ivN, ivM;
    long ivPC, ivPP, ivPN;
};

struct CTFblockInfo {
    int cfi;
    char level;
    char itype;
};

#include "vs_it.h"
