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

#ifndef __VS_IT_H__
#define __VS_IT_H__

#include "vs_it_interface.h"
#include "IScriptEnvironment.h"

#define PVideoFrame const VSFrameRef *
#define GetChildFrame(n) env->vsapi->getFrame(clipFrame(n), node, nullptr, 0)
#define FreeFrame(f) env->vsapi->freeFrame(f)

#define _MM_SHUFFLE(z, y, x, w) (z<<6) | (y<<4) | (x<<2) | w

#define USE_MMX2  _asm { emms } _asm { sfence }

#define FALSE 0
#define TRUE 1

#define FI(name, n) env->m_frameInfo[clipFrame(n)].##name
#define FII(n) env->m_frameInfo[clipFrame(n)]

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define MAX_WIDTH 8192

static double GetF(double x) {
	x = fabs(x);
	return (x < 1.0) ? 1.0 - x : 0.0;
}

enum {
	PLANAR_Y,
	PLANAR_U,
	PLANAR_V
};

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
	int m_bSwap;
	int m_iField;
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
	int GetDiffVal(IScriptEnvironment*env, int n, int p = 0) {
		if (p == 0)
			return env->m_frameInfo[clipFrame(n)].diffP0;
		else
			return env->m_frameInfo[clipFrame(n)].diffS0;
	}
	inline int clipFrame(int n) {
		return max(0, min(n, m_iMaxFrames - 1));
	}
	inline int clipOutFrame(int n) {
		return max(0, min(n, vi->numFrames - 1));
	}
	inline int clipX(int x) {
		x = max(0, min(width - 1, x));
		return x;
	}
	inline int clipY(int y) {
		return max(0, min(height - 1, y));
	}
	inline int clipYH(int y) {
		return max(0, min((height >> 1) - 1, y));
	}
	inline const unsigned char* SYP(IScriptEnvironment* env, const VSFrameRef * pv, int y, int plane = PLANAR_Y) {
		y = max(0, min(height - 1, y));
		if (m_bSwap)
			y = y ^ 1;
		auto rPtr = env->vsapi->getReadPtr(pv, plane);
		auto rStr = env->vsapi->getStride(pv, plane);
		if (plane == PLANAR_Y)
			return rPtr + y * rStr;
		else
			return rPtr + (((y >> 2) << 1) + (y % 2)) * rStr;
		//if (plane == PLANAR_Y)
		// return &pv->GetReadPtr()[y * pv->GetPitch()];
		//else
		// return &pv->GetReadPtr(plane)[(((y >> 2) << 1) + (y % 2)) * pv->GetPitch(plane)];
	}
	inline unsigned char* DYP(IScriptEnvironment* env, VSFrameRef * pv, int y, int plane = PLANAR_Y) {
		y = max(0, min(height - 1, y));
		auto wPtr = env->vsapi->getWritePtr(pv, plane);
		auto wStr = env->vsapi->getStride(pv, plane);
		if (plane == PLANAR_Y)
			return wPtr + y * wStr;
		else
			return wPtr + (((y >> 2) << 1) + (y % 2)) * wStr;
		return env->vsapi->getWritePtr(pv, plane) + y * env->vsapi->getStride(pv, plane);
		//if (plane == PLANAR_Y)
		// return pv->GetWritePtr() + y * pv->GetPitch();
		//else
		// return pv->GetWritePtr(plane) + (((y >> 2) << 1) + (y % 2)) * pv->GetPitch(plane);
	}
	inline unsigned char* B2YP(unsigned char *dst, int y) {
		y = max(0, min(height - 1, y));
		return dst + y * width * 2;
	}
	inline unsigned char* BYP(unsigned char *dst, int y) {
		y = max(0, min(height - 1, y));
		return dst + y * width;
	}
	void __stdcall GetFramePre(IScriptEnvironment* env, int n);
	const VSFrameRef* __stdcall GetFrame(IScriptEnvironment* env, int n);
	const VSFrameRef* __stdcall MakeOutput(IScriptEnvironment* env, VSFrameRef* dst, int n);
	bool CheckSceneChange(IScriptEnvironment* env, int n);
	void GetFrameSub(IScriptEnvironment* env, int n);
	void __stdcall EvalIV_YV12(IScriptEnvironment* env, int n, const VSFrameRef * ref, long &counter, long &counterp);
	void __stdcall MakeDEmap_YV12(IScriptEnvironment* env, const VSFrameRef * ref, int offset);
	void __stdcall MakeMotionMap_YV12(IScriptEnvironment* env, int fno, bool flag);
	// void __stdcall MakeMotionMap2_YV12(IScriptEnvironment* env, int fno, bool flag);
	void __stdcall MakeMotionMap2Max_YV12(IScriptEnvironment* env, int fno);
	void __stdcall MakeSimpleBlurMap_YV12(IScriptEnvironment* env, int fno);
	void __stdcall CopyCPNField(IScriptEnvironment* env, VSFrameRef * dst, int n);
	// void __stdcall Deinterlace_YV12(IScriptEnvironment* env, VSFrameRef * dst, int n, int nParameterMode = DI_MODE_DEINTERLACE);
	// void __stdcall SimpleBlur_YV12(IScriptEnvironment* env, VSFrameRef * dst, int n);
	void __stdcall DeintOneField_YV12(IScriptEnvironment* env, VSFrameRef * dst, int n);
	// void __stdcall BlendFrame_YV12(IScriptEnvironment* env, VSFrameRef * dst, int base, int n);
	// void __stdcall ShowInterlaceArea(const VSAPI * vsapi, VSFrameRef * dst, int n);
	// void __stdcall ShowDifference();
	void __stdcall ChooseBest(IScriptEnvironment* env, int n);
	bool __stdcall CompCP(IScriptEnvironment* env);
	// bool __stdcall CompCN();
	void __stdcall Decide(IScriptEnvironment* env, int n);
	void __stdcall SetFT(IScriptEnvironment* env, int base, int n, char c);
	// void __stdcall ReadLog();
	// void __stdcall WriteLog();
	bool __stdcall DrawPrevFrame(IScriptEnvironment* env, VSFrameRef * dst, int n);

};

void VS_CC
itInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi);

void VS_CC
itFree(void *instanceData, VSCore *core, const VSAPI *vsapi);

const VSFrameRef *VS_CC
itGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi);

#endif