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

void IT::SetFT(int base, int n, char c)
{
    m_frameInfo[clipFrame(base + n)].mflag = c;
	m_blockInfo[base / 5].cfi = n;
	m_blockInfo[base / 5].level = '0';
}

void IT::ChooseBest(int n, const VSAPI *vsapi)
{
    const VSFrameRef* srcC = vsapi->getFrame(clipFrame(n), node, nullptr, 0);
	MakeMotionMap_YV12(m_iCurrentFrame, false, vsapi);
	MakeMotionMap_YV12(m_iCurrentFrame + 1, false, vsapi);
	MakeDEmap_YV12(srcC, vsapi, 0);
	EvalIV_YV12(n, srcC, vsapi, m_iSumC, m_iSumPC);
    const VSFrameRef* srcP = vsapi->getFrame(clipFrame(n - 1), node, nullptr, 0);
	EvalIV_YV12(n, srcP, vsapi, m_iSumP, m_iSumPP);
	CompCP();
    vsapi->freeFrame(srcC);
    vsapi->freeFrame(srcP);
}

#define EVAL_IV_ASM_INIT(C, T, B) \
	__asm mov rax, C \
	__asm mov rbx, T \
	__asm mov rcx, B

#define EVAL_IV_ASM(mmm, step) \
	__asm movq mmm, [rax + rsi*step] \
	__asm movq mm1, [rbx + rsi*step] \
	__asm movq mm2, mmm \
	__asm movq mm4, mmm \
	__asm psubusb mmm, [rbx + rsi*step] \
	__asm psubusb mm1, mm2 \
	__asm movq mm3, [rcx + rsi*step] \
	__asm por mmm, mm1 \
	__asm movq mm1, [rbx + rsi*step] \
	__asm psubusb mm3, mm2 \
	__asm pavgb mm1, [rcx + rsi*step] \
	__asm psubusb mm2, [rcx + rsi*step] \
	__asm psubusb mm4, mm1 \
	__asm por mm2, mm3 \
	__asm psubusb mm1, [rax + rsi*step] \
	__asm pminub mmm, mm2 \
	__asm por mm1, mm4 \
	__asm pminub mmm, mm1

///////////////////////////////////////////////////////////////////////////
void IT::EvalIV_YV12(int n, const VSFrameRef * ref, const VSAPI *vsapi, long &counter, long &counterp)
{
	const __int64 mask1 = 0x0101010101010101i64;
	unsigned char th[8], th2[8];
	unsigned char rsum[8], psum[8];
	unsigned short psum0[4], psum1[4];

    const VSFrameRef* srcC = vsapi->getFrame(clipFrame(n), node, nullptr, 0);
	for (int i = 0; i < 8; ++i) {
		th[i] = 40;
		th2[i] = 6;
	}

	MakeDEmap_YV12(ref, vsapi, 1);

	const int widthminus16 = (width - 16) >> 1;
	int sum = 0, sum2 = 0;
	for (int yy = 16; yy < height - 16; yy += 2) {
		int y;
		y = yy + 1;
        const unsigned char *pT = SYP(srcC, vsapi, y - 1);
        const unsigned char *pC = SYP(ref, vsapi, y);
        const unsigned char *pB = SYP(srcC, vsapi, y + 1);
        const unsigned char *pT_U = SYP(srcC, vsapi, y - 1, 1);
        const unsigned char *pC_U = SYP(ref, vsapi, y, 1);
        const unsigned char *pB_U = SYP(srcC, vsapi, y + 1, 1);
        const unsigned char *pT_V = SYP(srcC, vsapi, y - 1, 2);
        const unsigned char *pC_V = SYP(ref, vsapi, y, 2);
        const unsigned char *pB_V = SYP(srcC, vsapi, y + 1, 2);

		const unsigned char *peT = &m_edgeMap[clipY(y - 1) * width];
		const unsigned char *peC = &m_edgeMap[clipY(y) * width];
		const unsigned char *peB = &m_edgeMap[clipY(y + 1) * width];

		__asm {
			pxor mm7, mm7

			mov esi, 16

			movq rsum, mm7
			movq psum, mm7
			movq psum0, mm7
			movq psum1, mm7
			align 16
			loopB:
			EVAL_IV_ASM_INIT(pC, pT, pB)
				EVAL_IV_ASM(mm0, 2)

				EVAL_IV_ASM_INIT(pC_U, pT_U, pB_U)
				EVAL_IV_ASM(mm5, 1)

				EVAL_IV_ASM_INIT(pC_V, pT_V, pB_V)
				EVAL_IV_ASM(mm6, 1)

				pmaxub mm5, mm6
				punpcklbw mm5, mm5
				pmaxub mm0, mm5; mm0 < -max(y, max(u, v))

				mov rdx, peC
				movq mm3, [rdx + rsi * 2]
				mov rdx, peT
				pmaxub mm3, [rdx + rsi * 2]
				mov rdx, peB
				pmaxub mm3, [rdx + rsi * 2]; mm3 <-max(peC[x], max(peT[x], peB[x]))

				psubusb mm0, mm3
				psubusb mm0, mm3
				movq mm1, mm0

				psubusb mm0, th
				pcmpeqb mm0, mm7
				pcmpeqb mm0, mm7
				pand mm0, mask1
				paddusb mm0, rsum; if (max - maxpe * 2 > 40) sum++
				movq rsum, mm0

				psubusb mm1, th2
				pcmpeqb mm1, mm7
				pcmpeqb mm1, mm7
				pand mm1, mask1
				paddusb mm1, psum; if (max - maxpe * 2 > 6) sum2++
				movq psum, mm1

				lea esi, [esi + 4]
				cmp esi, widthminus16
				jl loopB
		}
		sum += rsum[0] + rsum[1] + rsum[2] + rsum[3] + rsum[4] + rsum[5] + rsum[6] + rsum[7];
		sum2 += psum[0] + psum[1] + psum[2] + psum[3] + psum[4] + psum[5] + psum[6] + psum[7];
		if (sum > m_iPThreshold) {
			sum = m_iPThreshold;
			break;
		}
	}
	counter = sum;
	counterp = sum2;

    vsapi->freeFrame(srcC);
	USE_MMX2;
	return;
}

#define MAKE_DE_MAP_ASM_INIT(C, TT, BB) \
	__asm mov rax, C \
	__asm mov rbx, TT \
	__asm mov rcx, BB

#define MAKE_DE_MAP_ASM(mmm, step, offset) \
	__asm movq mm7, [rbx + rsi*step + offset] \
	__asm movq mmm, [rax + rsi*step + offset] \
	__asm pavgb mm7, [rcx + rsi*step + offset] \
	__asm psubusb mmm, mm7 \
	__asm psubusb mm7, [rax + rsi*step + offset] \
	__asm por mmm, mm7

///////////////////////////////////////////////////////////////////////////
void IT::MakeDEmap_YV12(const VSFrameRef * ref, const VSAPI *vsapi, int offset)
{
	const int twidth = width >> 1;

	for (int yy = 0; yy < height; yy += 2) {
		int y = yy + offset;
		const unsigned char *pTT = SYP(ref, vsapi, y - 2);
        const unsigned char *pC = SYP(ref, vsapi, y);
        const unsigned char *pBB = SYP(ref, vsapi, y + 2);
        const unsigned char *pTT_U = SYP(ref, vsapi, y - 2, 1);
        const unsigned char *pC_U = SYP(ref, vsapi, y, 1);
        const unsigned char *pBB_U = SYP(ref, vsapi, y + 2, 1);
        const unsigned char *pTT_V = SYP(ref, vsapi, y - 2, 2);
        const unsigned char *pC_V = SYP(ref, vsapi, y, 2);
        const unsigned char *pBB_V = SYP(ref, vsapi, y + 2, 2);
		unsigned char *pED = m_edgeMap + y * width;
		__asm {
			mov rdi, pED
				xor esi, esi
				align 16
			loopA:
			MAKE_DE_MAP_ASM_INIT(pC, pTT, pBB)
				MAKE_DE_MAP_ASM(mm0, 2, 0)
				MAKE_DE_MAP_ASM(mm3, 2, 8)
				MAKE_DE_MAP_ASM_INIT(pC_U, pTT_U, pBB_U)
				MAKE_DE_MAP_ASM(mm1, 1, 0)
				MAKE_DE_MAP_ASM(mm4, 1, 4)
				MAKE_DE_MAP_ASM_INIT(pC_V, pTT_V, pBB_V)
				MAKE_DE_MAP_ASM(mm2, 1, 0)
				MAKE_DE_MAP_ASM(mm5, 1, 4)

				pmaxub mm2, mm1
				pmaxub mm5, mm4
				punpcklbw mm2, mm2
				punpcklbw mm5, mm5
				pmaxub mm0, mm2
				pmaxub mm3, mm5

				lea esi, [esi + 8]
				movntq[rdi + rsi * 2 - 16], mm0
				cmp esi, twidth
				movntq[rdi + rsi * 2 - 8], mm3
				jl loopA
		}
	}
	USE_MMX2
}

void IT::MakeMotionMap_YV12(int n, bool flag, const VSAPI *vsapi)
{
	n = clipFrame(n);
	if (flag == false && m_frameInfo[n].diffP0 >= 0) {
		return;
	}

	const __int64 mask1 = 0x0101010101010101i64;

	const int twidth = width;
	const int widthminus8 = width - 8;
	const int widthminus16 = width - 16;
	unsigned short th[4], th2[4];
	unsigned char mbTh[8], mbTh2[8];
	int i;
	for (i = 0; i < 4; ++i) {
		th[i] = 12 * 3;
		th2[i] = 6 * 3;
	}
	for (i = 0; i < 8; ++i) {
		mbTh[i] = 12 * 3;
		mbTh2[i] = 6 * 3;
	}

    const VSFrameRef* srcP = vsapi->getFrame(clipFrame(n - 1), node, nullptr, 0);
    const VSFrameRef* srcC = vsapi->getFrame(n, node, nullptr, 0);
    ALIGNED_ARRAY(short bufP0[MAX_WIDTH], 16);
    ALIGNED_ARRAY(unsigned char bufP1[MAX_WIDTH], 16);
	int pe0 = 0, po0 = 0, pe1 = 0, po1 = 0;
	for (int yy = 16; yy < height - 16; ++yy) {
		int y = yy;
		const unsigned char *pC = SYP(srcC, vsapi, y);
		const unsigned char *pP = SYP(srcP, vsapi, y);
		{
			_asm {
				pxor mm7, mm7
					mov rax, pC
					mov rcx, pP
					lea rdi, bufP0
					xor esi, esi
					align 16
				loopA:
				prefetchnta[rax + rsi + 16]
					prefetchnta[rcx + rsi + 16]
					movd mm0, [rax + rsi]
					movd mm1, [rcx + rsi]
					punpcklbw mm0, mm7
					punpcklbw mm1, mm7
					lea esi, [esi + 4]
					psubw mm0, mm1
					cmp esi, twidth
					movntq[rdi + rsi * 2 - 8], mm0
					jl loopA
			}
		}
		{
			_asm {
				lea rax, bufP0
					lea rdi, bufP1
					mov esi, 8
					align 16
				loopB:
				prefetchnta[rax + rsi + 16]
					movq mm0, [rax + rsi * 2 - 2]
					movq mm1, mm7
					paddw mm0, [rax + rsi * 2 + 2]
					movq mm2, [rax + rsi * 2]
					psubw mm0, mm2
					movq mm3, mm7
					psubw mm0, mm2
					psubw mm3, mm2
					psubw mm1, mm0
					pmaxsw mm2, mm3
					pmaxsw mm0, mm1
					lea esi, [esi + 4]
					psubusw mm2, mm0
					cmp esi, widthminus8
					packuswb mm2, mm7
					movd[rdi + rsi - 4], mm2
					jl loopB
			}
		}
		int tsum = 0, tsum1 = 0;
		{
			_asm {
				movq mm5, mbTh

				lea rax, bufP1
				mov esi, 16

				pxor mm4, mm4
				pxor mm3, mm3
				align 16
				loopC:
				prefetchnta[rax + rsi + 16]
				movq mm0, [rax + rsi - 1]
				paddusb mm0, [rax + rsi + 1]
				paddusb mm0, [rax + rsi]
				movq mm1, mm0
				psubusb mm0, mm5
				psubusb mm1, mbTh2
				pcmpeqb mm0, mm7
				pcmpeqb mm1, mm7
				pcmpeqb mm0, mm7
				pcmpeqb mm1, mm7

				lea esi, [esi + 8]
				pand mm0, mask1
				pand mm1, mask1
				cmp esi, widthminus16
				paddb mm4, mm0
				paddb mm3, mm1
				jl loopC

				psadbw mm4, mm7
				movd tsum, mm4

				psadbw mm3, mm7
				movd tsum1, mm3
			}
			if ((y & 1) == 0) {
				pe0 += tsum;
				pe1 += tsum1;
			}
			else {
				po0 += tsum;
				po1 += tsum1;
			}
		}
	}
	m_frameInfo[n].diffP0 = pe0;
	m_frameInfo[n].diffP1 = po0;
	m_frameInfo[n].diffS0 = pe1;
	m_frameInfo[n].diffS1 = po1;
	USE_MMX2;
    vsapi->freeFrame(srcC);
    vsapi->freeFrame(srcP);
}

bool IT::CompCP()
{
	int n = m_iCurrentFrame;
	int p0 = m_frameInfo[n].diffP0;
	int p1 = m_frameInfo[n].diffP1;
	int n0 = m_frameInfo[clipFrame(n + 1)].diffP0;
	int n1 = m_frameInfo[clipFrame(n + 1)].diffP1;
	int ps0 = m_frameInfo[n].diffS0;
	int ps1 = m_frameInfo[n].diffS1;
	int ns0 = m_frameInfo[clipFrame(n + 1)].diffS0;
	int ns1 = m_frameInfo[clipFrame(n + 1)].diffS1;

	int th = AdjPara(5);
	int thm = AdjPara(5);
	int ths = AdjPara(200);

	bool spe = p0 < th && ps0 < ths;
	bool spo = p1 < th && ps1 < ths;
	bool sne = n0 < th && ns0 < ths;
	bool sno = n1 < th && ns1 < ths;

	bool mpe = p0 > thm;
	bool mpo = p1 > thm;
	bool mne = n0 > thm;
	bool mno = n1 > thm;

	//1773
	int thcomb = AdjPara(20);
	if (n != 0) {
		if ((m_iSumC < thcomb && m_iSumP < thcomb) || std::abs(m_iSumC - m_iSumP) * 10 < m_iSumC + m_iSumP) {
			if (std::abs(m_iSumC - m_iSumP) > AdjPara(8)) {
				if (m_iSumP >= m_iSumC) {
					m_iUseFrame = 'c';
					return true;
				}
				else {
					m_iUseFrame = 'p';
					return true;
				}
			}
			if (abs(m_iSumPC - m_iSumPP) > AdjPara(10)) {
				if (m_iSumPP >= m_iSumPC) {
					m_iUseFrame = 'c';
					return true;
				}
				else {
					m_iUseFrame = 'p';
					return true;
				}
			}

			if (spe && mpo) {
				m_iUseFrame = 'p';
				return true;
			}
			if (mpe && spo) {
				m_iUseFrame = 'c';
				return true;
			}
			if (mne && sno) {
				m_iUseFrame = 'p';
				return true;
			}
			if (sne && mno) {
				m_iUseFrame = 'c';
				return true;
			}
			if (spe && spo) {
				m_iUseFrame = 'c';
				return false;
			}
			if (sne && sno) {
				m_iUseFrame = 'c';
				return false;
			}
			if (mpe && mpo && mne && mno) {
				m_iUseFrame = 'c';
				return false;
			}

			if (m_iSumPC > m_iSumPP) {
				m_iUseFrame = 'p';
				return true;
			}
			else {
				m_iUseFrame = 'c';
				return false;
			}
		}
	}

	m_frameInfo[n].pos = '.';
	if (m_iSumP >= m_iSumC) {
		m_iUseFrame = 'C';
		if (!spe) {
			m_frameInfo[n].pos = '.';
		}
		return true;
	} else {
		m_iUseFrame = 'P';
		if (spe && !sno) {
			m_frameInfo[n].pos = '2';
		}
		if (!spe && sno) {
			m_frameInfo[n].pos = '3';
		}
		return true;
	}
}

bool IT::DrawPrevFrame(int n, VSFrameRef * dst, const VSAPI *vsapi)
{
	bool bResult = false;

	int nPrevFrame = clipFrame(n - 1);
	int nNextFrame = clipFrame(n + 1);

	int nOldCurrentFrame = m_iCurrentFrame;
	int nOldUseFrame = m_iUseFrame;

	GetFrameSub(nPrevFrame, vsapi);
	GetFrameSub(nNextFrame, vsapi);

	m_iCurrentFrame = nOldCurrentFrame;

	if (m_frameInfo[nPrevFrame].ip == 'P' && m_frameInfo[nNextFrame].ip == 'P')
    {
		bResult = CheckSceneChange(n, vsapi);
	}

	if (bResult)
    {
		m_iUseFrame = m_frameInfo[nPrevFrame].match;
        CopyCPNField(nPrevFrame, dst, vsapi);
	}

	m_iUseFrame = nOldUseFrame;

	return bResult;
}

void IT::CopyCPNField(int n, VSFrameRef * dst, const VSAPI *vsapi)
{
    const VSFrameRef* srcC = vsapi->getFrame(clipFrame(n), node, nullptr, 0);
	const VSFrameRef* srcR;
	switch (toupper(m_iUseFrame)) {
	default:
	case 'C':
		srcR = srcC;
		break;
	case 'P':
        srcR = vsapi->getFrame(clipFrame(n - 1), node, nullptr, 0);
		break;
	case 'N':
        srcR = vsapi->getFrame(clipFrame(n + 1), node, nullptr, 0);
		break;
	}

	int nPitch = vsapi->getStride(dst, 0);
	int nRowSize = width;
	int nPitchU = vsapi->getStride(dst, 1);
	int nRowSizeU = width >> vi->format->subSamplingW;

	for (int yy = 0; yy < height; yy += 2) {
		int y, yo;
		y = yy + 1;
		yo = yy + 0;
        vs_bitblt(DYP(dst, vsapi, yo), nPitch, SYP(srcC, vsapi, yo), nPitch, nRowSize, 1);
        vs_bitblt(DYP(dst, vsapi, y), nPitch, SYP(srcR, vsapi, y), nPitch, nRowSize, 1);

		if ((yy >> 1) % 2)
		{
            vs_bitblt(DYP(dst, vsapi, yo, 1), nPitchU, SYP(srcC, vsapi, yo, 1), nPitchU, nRowSizeU, 1);
            vs_bitblt(DYP(dst, vsapi, y, 1), nPitchU, SYP(srcR, vsapi, y, 1), nPitchU, nRowSizeU, 1);
            vs_bitblt(DYP(dst, vsapi, yo, 2), nPitchU, SYP(srcC, vsapi, yo, 2), nPitchU, nRowSizeU, 1);
            vs_bitblt(DYP(dst, vsapi, y, 2), nPitchU, SYP(srcR, vsapi, y, 2), nPitchU, nRowSizeU, 1);
		}
	}
	USE_MMX2;
	if (srcC != srcR)
        vsapi->freeFrame(srcR);
    vsapi->freeFrame(srcC);
}

bool IT::CheckSceneChange(int n, const VSAPI *vsapi)
{
    const VSFrameRef* srcP = vsapi->getFrame(clipFrame(n - 1), node, nullptr, 0);
    const VSFrameRef* srcC = vsapi->getFrame(clipFrame(n), node, nullptr, 0);

	int rowSize = vsapi->getStride(srcC, 0);

	int sum3 = 0;
	int x, y;

	int startY = 1;

	for (y = startY; y < height; y += 2)
	{
		const unsigned char *pC = SYP(srcC, vsapi, y);
		const unsigned char *pP = SYP(srcP, vsapi, y);

		for (x = 0; x < rowSize; x++)
		{
			int a = abs(pC[x] - pP[x]);
			if (a > 50) sum3 += 1;
		}
	}
    vsapi->freeFrame(srcP);
    vsapi->freeFrame(srcC);
	return sum3 > height * rowSize / 8;
}
