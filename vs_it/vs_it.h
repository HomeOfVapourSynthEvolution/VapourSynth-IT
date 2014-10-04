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

    __forceinline const unsigned char* SYP(const VSFrameRef * pv, const VSAPI *vsapi, int y, int plane = 0)
    {
        y = std::max(0, std::min(vi->height - 1, y));
        auto rPtr = vsapi->getReadPtr(pv, plane);
        auto rStr = vsapi->getStride(pv, plane);
        if (plane == 0)
            return rPtr + y * rStr;
        else
            return rPtr + (((y >> 2) << 1) + (y % 2)) * rStr;
    }
    __forceinline unsigned char* DYP(VSFrameRef * pv, const VSAPI *vsapi, int y, int plane = 0)
    {
        y = std::max(0, std::min(vi->height - 1, y));
        auto wPtr = vsapi->getWritePtr(pv, plane);
        auto wStr = vsapi->getStride(pv, plane);
        if (plane == 0)
            return wPtr + y * wStr;
        else
            return wPtr + (((y >> 2) << 1) + (y % 2)) * wStr;
    }

public:
    const VSVideoInfo* vi;
    VSNodeRef * node;
    IT(VSVideoInfo * vi,
    VSNodeRef * node,
    int _fps,
    int _threshold,
    int _pthreshold,
    const VSAPI *vsapi);

    void VS_CC GetFramePre(int n, VSFrameContext *frameCtx, const VSAPI *vsapi);
    const VSFrameRef* VS_CC GetFrame(int n, VSCore *core, const VSAPI *vsapi);
    const VSFrameRef* VS_CC MakeOutput(int n, VSFrameRef* dst, const VSAPI *vsapi);
    bool CheckSceneChange(int n, const VSAPI *vsapi);
    void GetFrameSub(int n, const VSAPI *vsapi);
    void VS_CC EvalIV_YV12(int n, const VSFrameRef * ref, const VSAPI *vsapi, long &counter, long &counterp);
    void VS_CC MakeDEmap_YV12(const VSFrameRef * ref, const VSAPI *vsapi, int offset);
    void VS_CC MakeMotionMap_YV12(int n, bool flag, const VSAPI *vsapi);
    void VS_CC MakeMotionMap2Max_YV12(int n, const VSAPI *vsapi);
    void VS_CC MakeSimpleBlurMap_YV12(int n, const VSAPI *vsapi);
    void VS_CC CopyCPNField(int n, VSFrameRef * dst, const VSAPI *vsapi);
    void VS_CC DeintOneField_YV12(int n, VSFrameRef* dst, const VSAPI *vsapi);
    void VS_CC ChooseBest(int n, const VSAPI *vsapi);
    bool VS_CC CompCP();
    void VS_CC Decide(int n);
    void VS_CC SetFT(int base, int n, char c);
    bool VS_CC DrawPrevFrame(int n, VSFrameRef * dst, const VSAPI *vsapi);

    int AdjPara(int v)
    {
        return (((v * width) / 720) * height) / 480;
    }
    __forceinline int clipFrame(int n)
    {
        return std::max(0, std::min(n, m_iMaxFrames - 1));
    }
    __forceinline int clipX(int x)
    {
        x = std::max(0, std::min(width - 1, x));
        return x;
    }
    __forceinline int clipY(int y)
    {
        return std::max(0, std::min(height - 1, y));
    }
    __forceinline int clipYH(int y)
    {
        return std::max(0, std::min((height >> 1) - 1, y));
    }
    __forceinline unsigned char* B2YP(unsigned char *dst, int y)
    {
        y = std::max(0, std::min(height - 1, y));
        return dst + y * width * 2;
    }
    __forceinline unsigned char* BYP(unsigned char *dst, int y)
    {
        y = std::max(0, std::min(height - 1, y));
        return dst + y * width;
    }
};
