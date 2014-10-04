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
#include "vs_it_interface.h"

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
