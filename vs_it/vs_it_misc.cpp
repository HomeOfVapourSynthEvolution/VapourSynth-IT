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
#include "IScriptEnvironment.h"

void IT::SetFT(IScriptEnvironment*env, int base, int n, char c)
{
	FI(mflag, base + n) = c;
	env->m_blockInfo[base / 5].cfi = n;
	env->m_blockInfo[base / 5].level = '0';
}

void IT::ChooseBest(IScriptEnvironment* env, int n)
{
	PVideoFrame srcC = GetChildFrame(n);
	//	MakeMotionMap(m_iCurrentFrame - 1, false);
	MakeMotionMap_YV12(env, env->m_iCurrentFrame, false);
	MakeMotionMap_YV12(env, env->m_iCurrentFrame + 1, false);
	MakeDEmap_YV12(env, srcC, 0);
	EvalIV_YV12(env, n, srcC, env->m_iSumC, env->m_iSumPC);
	PVideoFrame srcP = GetChildFrame(n - 1);
	EvalIV_YV12(env, n, srcP, env->m_iSumP, env->m_iSumPP);
	CompCP(env);
	FreeFrame(srcC);
	FreeFrame(srcP);
}

#define EVAL_IV_ASM_INIT(C, T, B) \
	__asm mov eax, C \
	__asm mov ebx, T \
	__asm mov ecx, B

#define EVAL_IV_ASM(mmm, step) \
	__asm movq mmm, [eax + esi*step] \
	__asm movq mm1, [ebx + esi*step] \
	__asm movq mm2, mmm \
	__asm movq mm4, mmm \
	__asm psubusb mmm, [ebx + esi*step] \
	__asm psubusb mm1, mm2 \
	__asm movq mm3, [ecx + esi*step] \
	__asm por mmm, mm1 \
	__asm movq mm1, [ebx + esi*step] \
	__asm psubusb mm3, mm2 \
	__asm pavgb mm1, [ecx + esi*step] \
	__asm psubusb mm2, [ecx + esi*step] \
	__asm psubusb mm4, mm1 \
	__asm por mm2, mm3 \
	__asm psubusb mm1, [eax + esi*step] \
	__asm pminub mmm, mm2 \
	__asm por mm1, mm4 \
	__asm pminub mmm, mm1

///////////////////////////////////////////////////////////////////////////
void IT::EvalIV_YV12(IScriptEnvironment *env, int n, const VSFrameRef * ref, long &counter, long &counterp)
{
	const __int64 mask1 = 0x0101010101010101i64;
	unsigned char th[8], th2[8];
	unsigned char rsum[8], psum[8];
	unsigned short psum0[4], psum1[4];

	PVideoFrame srcC = GetChildFrame(n);
	for (int i = 0; i < 8; ++i) {
		th[i] = 40;
		th2[i] = 6;
	}

	MakeDEmap_YV12(env, ref, 1);

	const int widthminus16 = (width - 16) >> 1;
	int sum = 0, sum2 = 0; //, sumS = 0;
	for (int yy = 16; yy < height - 16; yy += 2) {
		int y;
		if (m_iField == 0) {
			y = yy + 1;
		}
		else {
			y = yy + 0;
		}
		const unsigned char *pT = SYP(env, srcC, y - 1);
		const unsigned char *pC = SYP(env, ref, y);
		const unsigned char *pB = SYP(env, srcC, y + 1);
		const unsigned char *pT_U = SYP(env, srcC, y - 1, PLANAR_U);
		const unsigned char *pC_U = SYP(env, ref, y, PLANAR_U);
		const unsigned char *pB_U = SYP(env, srcC, y + 1, PLANAR_U);
		const unsigned char *pT_V = SYP(env, srcC, y - 1, PLANAR_V);
		const unsigned char *pC_V = SYP(env, ref, y, PLANAR_V);
		const unsigned char *pB_V = SYP(env, srcC, y + 1, PLANAR_V);

		const unsigned char *peT = &env->m_edgeMap[clipY(y - 1) * width];
		const unsigned char *peC = &env->m_edgeMap[clipY(y) * width];
		const unsigned char *peB = &env->m_edgeMap[clipY(y + 1) * width];

		//		unsigned char *pmW = m_pImap + width * y;
		__asm {
			pxor mm7, mm7
				//			mov edi,pmW
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

				mov edx, peC
				movq mm3, [edx + esi * 2]
				mov edx, peT
				pmaxub mm3, [edx + esi * 2]
				mov edx, peB
				pmaxub mm3, [edx + esi * 2]; mm3 <-max(peC[x], max(peT[x], peB[x]))

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
		//		sum2 += psum0[0] + psum0[1] + psum0[2] + psum0[3]; 
		//		sum2 += psum1[0] + psum1[1] + psum1[2] + psum1[3]; 
		if (sum > m_iPThreshold) {
			sum = m_iPThreshold;
			break;
		}
	}
	counter = sum;
	counterp = sum2;

	FreeFrame(srcC);
	USE_MMX2;
	return;
}

#define MAKE_DE_MAP_ASM_INIT(C, TT, BB) \
	__asm mov eax, C \
	__asm mov ebx, TT \
	__asm mov ecx, BB

#define MAKE_DE_MAP_ASM(mmm, step, offset) \
	__asm movq mm7, [ebx + esi*step + offset] \
	__asm movq mmm, [eax + esi*step + offset] \
	__asm pavgb mm7, [ecx + esi*step + offset] \
	__asm psubusb mmm, mm7 \
	__asm psubusb mm7, [eax + esi*step + offset] \
	__asm por mmm, mm7

///////////////////////////////////////////////////////////////////////////
void IT::MakeDEmap_YV12(IScriptEnvironment*env, const VSFrameRef * ref, int offset)
{
	const int twidth = width >> 1;

	for (int yy = 0; yy < height; yy += 2) {
		int y = yy + offset;
		const unsigned char *pTT = SYP(env, ref, y - 2);
		const unsigned char *pC = SYP(env, ref, y);
		const unsigned char *pBB = SYP(env, ref, y + 2);
		const unsigned char *pTT_U = SYP(env, ref, y - 2, PLANAR_U);
		const unsigned char *pC_U = SYP(env, ref, y, PLANAR_U);
		const unsigned char *pBB_U = SYP(env, ref, y + 2, PLANAR_U);
		const unsigned char *pTT_V = SYP(env, ref, y - 2, PLANAR_V);
		const unsigned char *pC_V = SYP(env, ref, y, PLANAR_V);
		const unsigned char *pBB_V = SYP(env, ref, y + 2, PLANAR_V);
		unsigned char *pED = env->m_edgeMap + y * width;
		__asm {
			mov edi, pED
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
				movntq[edi + esi * 2 - 16], mm0
				cmp esi, twidth
				movntq[edi + esi * 2 - 8], mm3
				jl loopA
		}
	}
	USE_MMX2
}

void IT::MakeMotionMap_YV12(IScriptEnvironment*env, int n, bool flag)
{
	n = clipFrame(n);
	if (flag == false && env->m_frameInfo[n].diffP0 >= 0) {
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


	PVideoFrame srcP = GetChildFrame(n - 1);
	PVideoFrame srcC = GetChildFrame(n);
	short bufP0[2048];
	unsigned char bufP1[2048];
	int pe0 = 0, po0 = 0, pe1 = 0, po1 = 0;
	for (int yy = 16; yy < height - 16; ++yy) {
		int y = yy;
		const unsigned char *pC = SYP(env, srcC, y);
		const unsigned char *pP = SYP(env, srcP, y);
		{
			_asm {
				pxor mm7, mm7
					mov eax, pC
					mov ebx, pP
					lea edi, bufP0
					xor esi, esi
					align 16
				loopA:
				prefetchnta[eax + esi + 16]
					prefetchnta[ebx + esi + 16]
					movd mm0, [eax + esi]
					movd mm1, [ebx + esi]
					punpcklbw mm0, mm7
					punpcklbw mm1, mm7
					lea esi, [esi + 4]
					psubw mm0, mm1
					cmp esi, twidth
					movntq[edi + esi * 2 - 8], mm0
					jl loopA
			}
		}
		{
			_asm {
				lea eax, bufP0
					lea edi, bufP1
					mov esi, 8
					align 16
				loopB:
				prefetchnta[eax + esi + 16]
					movq mm0, [eax + esi * 2 - 2]
					movq mm1, mm7
					paddw mm0, [eax + esi * 2 + 2]
					movq mm2, [eax + esi * 2]
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
					movd[edi + esi - 4], mm2
					jl loopB
			}
		}
		int tsum = 0, tsum1 = 0;
		{
			//			unsigned char sum[8];
			//			unsigned char sum[8];
			_asm {
				movq mm5, mbTh
					//				movq mm4,mask1
					lea eax, bufP1
					mov esi, 16
					//				mov edi,pD
					pxor mm4, mm4
					pxor mm3, mm3
					align 16
				loopC:
				prefetchnta[eax + esi + 16]
					movq mm0, [eax + esi - 1]
					paddusb mm0, [eax + esi + 1]
					paddusb mm0, [eax + esi]
					movq mm1, mm0
					psubusb mm0, mm5
					psubusb mm1, mbTh2
					pcmpeqb mm0, mm7
					pcmpeqb mm1, mm7
					pcmpeqb mm0, mm7
					pcmpeqb mm1, mm7
					//				movntq [edi+esi],mm0
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
	env->m_frameInfo[n].diffP0 = pe0;
	env->m_frameInfo[n].diffP1 = po0;
	env->m_frameInfo[n].diffS0 = pe1;
	env->m_frameInfo[n].diffS1 = po1;
	USE_MMX2;
	FreeFrame(srcC);
	FreeFrame(srcP);
}

bool IT::CompCP(IScriptEnvironment*env)
{
	int n = env->m_iCurrentFrame;
	int p0 = env->m_frameInfo[n].diffP0;
	int p1 = env->m_frameInfo[n].diffP1;
	int n0 = env->m_frameInfo[clipFrame(n + 1)].diffP0;
	int n1 = env->m_frameInfo[clipFrame(n + 1)].diffP1;
	int ps0 = env->m_frameInfo[n].diffS0;
	int ps1 = env->m_frameInfo[n].diffS1;
	int ns0 = env->m_frameInfo[clipFrame(n + 1)].diffS0;
	int ns1 = env->m_frameInfo[clipFrame(n + 1)].diffS1;
	//	int th = 5;
	//	int thm = 5;

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
		if ((env->m_iSumC < thcomb && env->m_iSumP < thcomb) || abs(env->m_iSumC - env->m_iSumP) * 10 < env->m_iSumC + env->m_iSumP) {
			if (abs(env->m_iSumC - env->m_iSumP) > AdjPara(8)) {
				if (env->m_iSumP >= env->m_iSumC) {
					env->m_iUseFrame = 'c';
					return true;
				}
				else {
					env->m_iUseFrame = 'p';
					return true;
				}
			}
			if (abs(env->m_iSumPC - env->m_iSumPP) > AdjPara(10)) {
				if (env->m_iSumPP >= env->m_iSumPC) {
					env->m_iUseFrame = 'c';
					return true;
				}
				else {
					env->m_iUseFrame = 'p';
					return true;
				}
			}

			if (spe && mpo) {
				env->m_iUseFrame = 'p';
				return true;
			}
			if (mpe && spo) {
				env->m_iUseFrame = 'c';
				return true;
			}
			if (mne && sno) {
				env->m_iUseFrame = 'p';
				return true;
			}
			if (sne && mno) {
				env->m_iUseFrame = 'c';
				return true;
			}
			if (spe && spo) {
				env->m_iUseFrame = 'c';
				return false;
			}
			if (sne && sno) {
				env->m_iUseFrame = 'c';
				return false;
			}
			if (mpe && mpo && mne && mno) {
				env->m_iUseFrame = 'c';
				return false;
			}

			//			return (env->m_iSumPC > env->m_iSumPP);
			if (env->m_iSumPC > env->m_iSumPP) {
				env->m_iUseFrame = 'p';
				return true;
			}
			else {
				env->m_iUseFrame = 'c';
				return false;
			}
		}
	}
	//	env->m_frameInfo[clipFrame(n)].matchAcc = '0';
	//	if (env->m_iSumC > env->m_iSumP) {
	//	} else {
	//		env->m_iUseFrame = 'C';
	//		return false;
	//	}
	env->m_frameInfo[n].pos = '.';
	if (env->m_iSumP >= env->m_iSumC) {
		env->m_iUseFrame = 'C';
		if (!spe) {
			env->m_frameInfo[n].pos = '.';
		}
		return true;
	}
	else {
		env->m_iUseFrame = 'P';
		if (spe && !sno) {
			env->m_frameInfo[n].pos = '2';
		}
		if (!spe && sno) {
			env->m_frameInfo[n].pos = '3';
		}
		return true;
	}
}

bool IT::DrawPrevFrame(IScriptEnvironment*env, VSFrameRef * dst, int n)
{
	bool bResult = false;

	int nPrevFrame = clipFrame(n - 1);
	int nNextFrame = clipFrame(n + 1);

	int nOldCurrentFrame = env->m_iCurrentFrame;
	int nOldUseFrame = env->m_iUseFrame;

	GetFrameSub(env, nPrevFrame);
	GetFrameSub(env, nNextFrame);

	env->m_iCurrentFrame = nOldCurrentFrame;

	if (env->m_frameInfo[nPrevFrame].ip == 'P' && env->m_frameInfo[nNextFrame].ip == 'P')
	{
		bResult = CheckSceneChange(env, n);
	}

	if (bResult)
	{
		env->m_iUseFrame = env->m_frameInfo[nPrevFrame].match;

		CopyCPNField(env, dst, nPrevFrame);
	}

	env->m_iUseFrame = nOldUseFrame;

	return bResult;
}

void IT::CopyCPNField(IScriptEnvironment* env, VSFrameRef * dst, int n)
{
	PVideoFrame srcC = GetChildFrame(n);
	PVideoFrame srcR;
	switch (toupper(env->m_iUseFrame)) {
	default:
	case 'C':
		srcR = srcC;
		break;
	case 'P':
		srcR = GetChildFrame(n - 1);
		break;
	case 'N':
		srcR = GetChildFrame(n + 1);
		break;
	}

	int nPitch = env->vsapi->getStride(dst, 0);
	int nRowSize = width;
	int nPitchU = env->vsapi->getStride(dst, 1);
	int nRowSizeU = width >> vi->format->subSamplingW;

	for (int yy = 0; yy < height; yy += 2) {
		int y, yo;
		if (m_iField == 0) {
			y = yy + 1;
			yo = yy + 0;
		}
		else {
			y = yy + 0;
			yo = yy + 1;
		}
		env->BitBlt(DYP(env, dst, yo), nPitch, SYP(env, srcC, yo), nPitch, nRowSize, 1);
		env->BitBlt(DYP(env, dst, y), nPitch, SYP(env, srcR, y), nPitch, nRowSize, 1);

		if ((yy >> 1) % 2)
		{
			env->BitBlt(DYP(env, dst, yo, PLANAR_U), nPitchU, SYP(env, srcC, yo, PLANAR_U), nPitchU, nRowSizeU, 1);
			env->BitBlt(DYP(env, dst, y, PLANAR_U), nPitchU, SYP(env, srcR, y, PLANAR_U), nPitchU, nRowSizeU, 1);
			env->BitBlt(DYP(env, dst, yo, PLANAR_V), nPitchU, SYP(env, srcC, yo, PLANAR_V), nPitchU, nRowSizeU, 1);
			env->BitBlt(DYP(env, dst, y, PLANAR_V), nPitchU, SYP(env, srcR, y, PLANAR_V), nPitchU, nRowSizeU, 1);
		}
	}
	USE_MMX2;
	if (srcC != srcR)
		FreeFrame(srcR);
	FreeFrame(srcC);
}

bool IT::CheckSceneChange(IScriptEnvironment*env, int n)
{
	PVideoFrame srcP = GetChildFrame(n - 1);
	PVideoFrame srcC = GetChildFrame(n);

	int rowSize = env->vsapi->getStride(srcC, 0);

	int sum3 = 0;
	int x, y;

	int startY = 0;
	if (m_iField == 0)
	{
		startY = 1;
	}

	for (y = startY; y < height; y += 2)
	{
		const unsigned char *pC = SYP(env, srcC, y);
		const unsigned char *pP = SYP(env, srcP, y);

		for (x = 0; x < rowSize; x++)
		{
			int a = abs(pC[x] - pP[x]);
			if (a > 50) sum3 += 1;
		}
	}
	FreeFrame(srcP);
	FreeFrame(srcC);
	return sum3 > height * rowSize / 8;
}

