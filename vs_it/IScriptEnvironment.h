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

enum REFTYPE;

class CFrameInfo {
public:
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

class CTFblockInfo {
public:
	int cfi;
	char level;
	char itype;
};

class IScriptEnvironment
{
public:
	int m_iRealFrame;
	CFrameInfo *m_frameInfo;
	CTFblockInfo *m_blockInfo;
	unsigned char *m_edgeMap, *m_motionMap4DI, *m_motionMap4DIMax;

	long m_iSumC, m_iSumP, m_iSumN, m_iSumM;
	long m_iSumPC, m_iSumPP, m_iSumPN, m_iSumPM;
	int m_iCurrentFrame;
	bool m_bRefP;
	int m_iUsePrev, m_iUseNext;
	int m_iUseFrame;

	VSFrameContext *frameCtx;
	VSCore *core;
	const VSAPI *vsapi;
	IScriptEnvironment(VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi);
	~IScriptEnvironment();
	VSFrameRef *IScriptEnvironment::NewVideoFrame(const VSVideoInfo * vi);
	void BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) {
		if ((!height) || (!row_size)) return;

		if (height == 1 || (dst_pitch == src_pitch && src_pitch == row_size)) {
			memcpy(dstp, srcp, row_size*height);
		}
		else {
			for (int y = height; y > 0; --y) {
				memcpy(dstp, srcp, row_size);
				dstp += dst_pitch;
				srcp += src_pitch;
			}
		}
	}
};

