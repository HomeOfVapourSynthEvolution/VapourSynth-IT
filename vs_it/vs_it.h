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

const int MAX_WIDTH = 8192;

#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(z, y, x, w) (z<<6) | (y<<4) | (x<<2) | w
#endif

#define USE_MMX2  _asm { emms } _asm { sfence }

class IT {
private:
	int m_iPThreshold;
	int m_iThreshold;
	int m_iCounter;
	int m_iFPS;
	const VSFrameRef* m_PVOut[8];
	int m_iPVOutIndex[8];

	int width;
	int height;
	VSFrameRef* m_pvf[32];
	int m_ipvfIndex[32];
	int m_iMaxFrames;
public:
	const VSVideoInfo* vi;
	VSNodeRef * node;
	IT(VSVideoInfo * vi,
		VSNodeRef * node,
		int _fps,
		int _threshold,
		int _pthreshold,
		const VSAPI *vsapi);
	int AdjPara(int v) {
		return (((v * width) / 720) * height) / 480;
	}
    __forceinline int clipFrame(int n) {
		return max(0, min(n, m_iMaxFrames - 1));
	}
    __forceinline int clipX(int x) {
		x = max(0, min(width - 1, x));
		return x;
	}
    __forceinline int clipY(int y) {
		return max(0, min(height - 1, y));
	}
    __forceinline int clipYH(int y) {
		return max(0, min((height >> 1) - 1, y));
	}
    __forceinline unsigned char* B2YP(unsigned char *dst, int y) {
		y = max(0, min(height - 1, y));
		return dst + y * width * 2;
	}
    __forceinline unsigned char* BYP(unsigned char *dst, int y) {
		y = max(0, min(height - 1, y));
		return dst + y * width;
	}
    void VS_CC GetFramePre(IScriptEnvironment* env, int n);
    const VSFrameRef* VS_CC GetFrame(IScriptEnvironment* env, int n);
    const VSFrameRef* VS_CC MakeOutput(IScriptEnvironment* env, VSFrameRef* dst, int n);
	bool CheckSceneChange(IScriptEnvironment* env, int n);
	void GetFrameSub(IScriptEnvironment* env, int n);
    void VS_CC EvalIV_YV12(IScriptEnvironment* env, int n, const VSFrameRef * ref, long &counter, long &counterp);
    void VS_CC MakeDEmap_YV12(IScriptEnvironment* env, const VSFrameRef * ref, int offset);
    void VS_CC MakeMotionMap_YV12(IScriptEnvironment* env, int fno, bool flag);
	// void __stdcall MakeMotionMap2_YV12(IScriptEnvironment* env, int fno, bool flag);
    void VS_CC MakeMotionMap2Max_YV12(IScriptEnvironment* env, int fno);
    void VS_CC MakeSimpleBlurMap_YV12(IScriptEnvironment* env, int fno);
    void VS_CC CopyCPNField(IScriptEnvironment* env, VSFrameRef * dst, int n);
	// void __stdcall Deinterlace_YV12(IScriptEnvironment* env, VSFrameRef * dst, int n, int nParameterMode = DI_MODE_DEINTERLACE);
	// void __stdcall SimpleBlur_YV12(IScriptEnvironment* env, VSFrameRef * dst, int n);
    void VS_CC DeintOneField_YV12(IScriptEnvironment* env, VSFrameRef * dst, int n);
	// void __stdcall BlendFrame_YV12(IScriptEnvironment* env, VSFrameRef * dst, int base, int n);
	// void __stdcall ShowInterlaceArea(const VSAPI * vsapi, VSFrameRef * dst, int n);
	// void __stdcall ShowDifference();
    void VS_CC ChooseBest(IScriptEnvironment* env, int n);
    bool VS_CC CompCP(IScriptEnvironment* env);
	// bool __stdcall CompCN();
    void VS_CC Decide(IScriptEnvironment* env, int n);
    void VS_CC SetFT(IScriptEnvironment* env, int base, int n, char c);
	// void __stdcall ReadLog();
	// void __stdcall WriteLog();
    bool VS_CC DrawPrevFrame(IScriptEnvironment* env, VSFrameRef * dst, int n);

};
