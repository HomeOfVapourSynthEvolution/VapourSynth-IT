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

#include "vs_it.h"

IT::IT(VSVideoInfo * vi, VSNodeRef * node, int _fps, int _threshold, int _pthreshold, const VSAPI *vsapi) :
    vi(vi),
    node(node),
    m_iFPS(_fps),
    m_iThreshold(_threshold),
    m_iPThreshold(_pthreshold)
{
    m_iMaxFrames = vi->numFrames;
    m_iCounter = 0;
    width = vi->width;
    height = vi->height;

    for (int i = 0; i < 32; ++i) {
        m_pvf[i] = 0;
        m_ipvfIndex[i] = -1;
    }

    m_iPThreshold = AdjPara(m_iPThreshold);

    if (m_iFPS == 24) {
        vi->numFrames = vi->numFrames * 4 / 5;
        vi->fpsNum *= 4;
        if (vi->fpsNum % 5 == 0)
            vi->fpsNum /= 5;
        else
            vi->fpsDen *= 5;
    }
}


void IT::GetFramePre(int n, VSFrameContext *frameCtx, const VSAPI *vsapi)
{
    if (m_iFPS == 24) {
        int base = n + n / 4;
        base = (base / 5) * 5;
        for (int i = 0; i < 6; ++i)
            vsapi->requestFrameFilter(base + i, node, frameCtx);
    } else
        vsapi->requestFrameFilter(n, node, frameCtx);
}

const VSFrameRef *IT::GetFrame(int n, VSCore *core, const VSAPI *vsapi)
{
    ++m_iCounter;
    m_iRealFrame = n;
    m_frameInfo = new CFrameInfo[m_iMaxFrames + 6];

    for (int i = 0; i < 8; ++i) {
        m_PVOut[i] = 0;
        m_iPVOutIndex[i] = -1;
    }

    for (int i = 0; i < m_iMaxFrames + 6; ++i) {
        m_frameInfo[i].match = 'U';
        m_frameInfo[i].matchAcc = 'U';
        m_frameInfo[i].pos = 'U';
        m_frameInfo[i].ip = 'U';
        m_frameInfo[i].mflag = 'U';
        m_frameInfo[i].diffP0 = -1;
        m_frameInfo[i].diffP1 = -1;
    }

    m_blockInfo = new CTFblockInfo[m_iMaxFrames / 5 + 6];
    for (int i = 0; i < m_iMaxFrames / 5 + 1; ++i) {
        m_blockInfo[i].level = 'U';
        m_blockInfo[i].itype = 'U';
    }

    m_edgeMap = new unsigned char[width * height];
    memset(m_edgeMap, width * height, 0);

    m_motionMap4DI = new unsigned char[width * height];
    memset(m_motionMap4DI, width * height, 0);

    m_motionMap4DIMax = new unsigned char[width * height];
    memset(m_motionMap4DIMax, width * height, 0);

    int tfFrame;
    if (m_iFPS == 24) {
        tfFrame = n + n / (5 - 1);

        int base = (tfFrame / 5) * 5;
        int i;

        for (i = 0; i < 5; ++i)
            GetFrameSub(base + i, vsapi);
        Decide(base);

        bool iflag = true;
        for (i = 0; i < 5; ++i)
            if (m_frameInfo[clipFrame(base + i)].ivC >= m_iPThreshold)
                iflag = false;

        if (iflag)
            m_blockInfo[base / 5].itype = '3';
        else
            m_blockInfo[base / 5].itype = '2';

        int no = tfFrame - base;
        for (i = 0; i < 5; ++i) {
            char f = m_frameInfo[clipFrame(base + i)].mflag;
            if (f != 'D' && f != 'd' && f != 'X' && f != 'x' && f != 'y' && f != 'z' && f != 'R') {
                if (no == 0)
                    break;
                --no;
            }
        }
        if (m_iFPS != 30)
            n = clipFrame(i + base);
    } else
        GetFrameSub(n, vsapi);

    VSFrameRef * dst = vsapi->newVideoFrame(vi->format, vi->width, vi->height, nullptr, core);
    MakeOutput(n, dst, vsapi);
    delete[] m_frameInfo;
    delete[] m_blockInfo;
    delete[] m_edgeMap;
    delete[] m_motionMap4DI;
    delete[] m_motionMap4DIMax;
    return dst;
}

void IT::GetFrameSub(int n, const VSAPI *vsapi)
{
    if (n >= m_iMaxFrames)
        return;
    if (m_frameInfo[n].ip != 'U')
        return;

    m_iCurrentFrame = n;

    m_iUseFrame = 'C';
    m_iSumC = m_iSumP = m_iSumN = m_iSumM = 720 * 480;
    m_bRefP = true;

    ChooseBest(n, vsapi);
    m_frameInfo[n].match = (unsigned char)m_iUseFrame;
    switch (toupper(m_iUseFrame)) {
    case 'C':
        m_iSumM = m_iSumC;
        m_iSumPM = m_iSumPC;
        break;
    case 'P':
        m_iSumM = m_iSumP;
        m_iSumPM = m_iSumPP;
        break;
    case 'N':
        m_iSumM = m_iSumN;
        m_iSumPM = m_iSumPN;
        break;
    }

    m_frameInfo[n].ivC = m_iSumC;
    m_frameInfo[n].ivP = m_iSumP;
    m_frameInfo[n].ivN = m_iSumN;
    m_frameInfo[n].ivM = m_iSumM;
    m_frameInfo[n].ivPC = m_iSumPC;
    m_frameInfo[n].ivPP = m_iSumPP;
    m_frameInfo[n].ivPN = m_iSumPN;
    if (m_iSumM < m_iPThreshold && m_iSumPM < m_iPThreshold * 3)
        m_frameInfo[n].ip = 'P';
    else
        m_frameInfo[n].ip = 'I';

    return;
}

const VSFrameRef* IT::MakeOutput(int n, VSFrameRef* dst, const VSAPI *vsapi)
{
    m_iCurrentFrame = n;

    m_iSumC = m_frameInfo[n].ivC;
    m_iSumP = m_frameInfo[n].ivP;
    m_iSumN = m_frameInfo[n].ivN;
    m_iSumM = m_frameInfo[n].ivM;
    m_iSumPC = m_frameInfo[n].ivPC;
    m_iSumPP = m_frameInfo[n].ivPP;
    m_iSumPN = m_frameInfo[n].ivPN;

    m_bRefP = true;

    m_iUseFrame = toupper(m_frameInfo[n].match);

#ifdef DEBUG_SHOW_INTERLACE
    USE_MMX2
        return dst;
#endif // DEBUG_SHOW_INTERLACE

    if (m_frameInfo[n].ip == 'P')
        CopyCPNField(n, dst, vsapi);
    else
        if (!DrawPrevFrame(n, dst, vsapi))
            DeintOneField_YV12(n, dst, vsapi);

    return dst;
}

