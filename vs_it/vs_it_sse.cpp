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

__forceinline __m128i make_de_map_asm(
	unsigned const char* eax,
	unsigned const char* ebx,
	unsigned const char* ecx,
	int i, int step, int offset) {
	auto mma = _mm_load_si128(reinterpret_cast<const __m128i*>(eax + i * step + offset));
	auto mmb = _mm_load_si128(reinterpret_cast<const __m128i*>(ebx + i * step + offset));
	auto mmc = _mm_load_si128(reinterpret_cast<const __m128i*>(ecx + i * step + offset));
	auto mmbc = _mm_avg_epu8(mmb, mmc);
	// subs+subs+or seems to be faster than sub+abs
	auto mmabc = _mm_subs_epu8(mma, mmbc);
	auto mmbca = _mm_subs_epu8(mmbc, mma);
	return _mm_or_si128(mmabc, mmbca);
}

void IT::SSE_MakeDEmap_YV12(IScriptEnvironment*env, const VSFrameRef * ref, int offset)
{
	const int twidth = width >> 1;

	for (int yy = 0; yy < height; yy += 2) {
		int y = yy + offset;
		const unsigned char *pTT = env->SYP(ref, y - 2);
		const unsigned char *pC = env->SYP(ref, y);
		const unsigned char *pBB = env->SYP(ref, y + 2);
		const unsigned char *pTT_U = env->SYP(ref, y - 2, 1);
		const unsigned char *pC_U = env->SYP(ref, y, 1);
		const unsigned char *pBB_U = env->SYP(ref, y + 2, 1);
		const unsigned char *pTT_V = env->SYP(ref, y - 2, 2);
		const unsigned char *pC_V = env->SYP(ref, y, 2);
		const unsigned char *pBB_V = env->SYP(ref, y + 2, 2);
		unsigned char *pED = env->m_edgeMap + y * width;

		__m128i mm0, mm1, mm2, mm3, mm5;
		//		mov rdi, pED
		//		xor esi, esi
		//		align 16
		//	loopA:
		for (int i = 0; i < twidth; i += 16)
		{
			// MAKE_DE_MAP_ASM_INIT(pC, pTT, pBB);
			mm0 = make_de_map_asm(pC, pTT, pBB, i, 2, 0);
			// MAKE_DE_MAP_ASM(mm0, 2, 0);
			mm3 = make_de_map_asm(pC, pTT, pBB, i, 2, 16);
			// MAKE_DE_MAP_ASM(mm3, 2, 8);
			// MAKE_DE_MAP_ASM_INIT(pC_U, pTT_U, pBB_U);
			mm1 = make_de_map_asm(pC_U, pTT_U, pBB_U, i, 1, 0);
			// MAKE_DE_MAP_ASM(mm1, 1, 0);
			// MAKE_DE_MAP_ASM(mm4, 1, 4);
			// MAKE_DE_MAP_ASM_INIT(pC_V, pTT_V, pBB_V);
			mm2 = make_de_map_asm(pC_V, pTT_V, pBB_V, i, 1, 0);
			// MAKE_DE_MAP_ASM(mm2, 1, 0);
			// MAKE_DE_MAP_ASM(mm5, 1, 4);

			mm5 = mm2 = _mm_max_epu8(mm2, mm1);
			//	pmaxub mm2, mm1
			// mm5 = _mm_max_epu8(mm5, mm4);
			//	pmaxub mm5, mm4
			mm2 = _mm_unpacklo_epi8(mm2, mm2);
			//	punpcklbw mm2, mm2
			mm5 = _mm_unpackhi_epi8(mm5, mm5);
			//	punpcklbw mm5, mm5
			mm0 = _mm_max_epu8(mm0, mm2);
			//	pmaxub mm0, mm2
			mm3 = _mm_max_epu8(mm3, mm5);
			//	pmaxub mm3, mm5

			//	lea esi, [esi + 8]
			_mm_stream_si128(reinterpret_cast<__m128i*>(pED + i * 2), mm0);
			//	movntq[rdi + rsi * 2 - 16], mm0
			//	cmp esi, twidth
			_mm_stream_si128(reinterpret_cast<__m128i*>(pED + i * 2 + 16), mm3);
			//	movntq[rdi + rsi * 2 - 8], mm3
			//	jl loopA
		}
	}
}

void IT::SSE_MakeMotionMap_YV12(IScriptEnvironment*env, int n, bool flag)
{
	n = clipFrame(n);
	if (flag == false && env->m_frameInfo[n].diffP0 >= 0)
		return;


	const int twidth = width;
	const int widthminus8 = width - 8;
	const int widthminus16 = width - 16;
	int i;
	auto mmbTh = _mm_set1_epi8(36);
	auto mmbTh2 = _mm_set1_epi8(18);
	auto mmask1 = _mm_set1_epi8(1);

	const VSFrameRef* srcP = env->GetFrame(clipFrame(n - 1));
	const VSFrameRef* srcC = env->GetFrame(n);
	ALIGNED_ARRAY(short bufP0[MAX_WIDTH], 16);
	ALIGNED_ARRAY(unsigned char bufP1[MAX_WIDTH], 16);
	int pe0 = 0, po0 = 0, pe1 = 0, po1 = 0;
	for (int yy = 16; yy < height - 16; ++yy) {
		int y = yy;
		const unsigned char *pC = env->SYP(srcC, y);
		const unsigned char *pP = env->SYP(srcP, y);

		__m128i c, p, cx, px, zero;
		zero = _mm_setzero_si128();
		for (i = 0; i < twidth; i += 16) {
			_mm_prefetch(reinterpret_cast<const char*>(pC + i + 64), _MM_HINT_NTA);
			_mm_prefetch(reinterpret_cast<const char*>(pP + i + 64), _MM_HINT_NTA);
			c = _mm_load_si128(reinterpret_cast<const __m128i *>(pC + i));
			p = _mm_load_si128(reinterpret_cast<const __m128i *>(pP + i));
			cx = _mm_unpacklo_epi8(c, zero);
			px = _mm_unpacklo_epi8(p, zero);
			cx = _mm_sub_epi16(cx, px);
			_mm_stream_si128(reinterpret_cast<__m128i *>(bufP0 + i), cx);
			cx = _mm_unpackhi_epi8(c, zero);
			px = _mm_unpackhi_epi8(p, zero);
			cx = _mm_sub_epi16(cx, px);
			_mm_stream_si128(reinterpret_cast<__m128i *>(bufP0 + i + 8), cx);
		}

		//lea rax, bufP0
		//lea rdi, bufP1
		//mov esi, 8
		//align 16
		for (i = 8; i < widthminus8; i += 8) {
			_mm_prefetch(reinterpret_cast<const char*>(bufP0 + i + 64), _MM_HINT_NTA);
			//prefetchnta[rax + rsi + 16]
			auto mmA = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bufP0 + i - 1));
			//movq mm0, [rax + rsi * 2 - 2]
			auto mmC = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bufP0 + i + 1));
			auto delta = _mm_add_epi16(mmA, mmC);
			//paddw mm0, [rax + rsi * 2 + 2]
			auto mmB = _mm_load_si128(reinterpret_cast<const __m128i*>(bufP0 + i));
			//movq mm2, [rax + rsi * 2]
			delta = _mm_sub_epi16(delta, mmB);
			//psubw mm0, mm2
			delta = _mm_sub_epi16(delta, mmB);
			//psubw mm0, mm2
			mmB = _mm_abs_epi16(mmB);
			//movq mm3, mm7  |
			//psubw mm3, mm2  }-> abs(mm3)
			//pmaxsw mm2, mm3|
			delta = _mm_abs_epi16(delta);
			//movq mm1, mm7  |
			//psubw mm1, mm0  }-> abs(mm1)
			//pmaxsw mm0, mm1|
			//lea esi, [esi + 4]
			mmB = _mm_subs_epu16(mmB, delta);
			//psubusw mm2, mm0
			//cmp esi, widthminus8
			mmB = _mm_packus_epi16(mmB, zero);
			//packuswb mm2, mm7
			_mm_storel_epi64(reinterpret_cast<__m128i*>(bufP1 + i), mmB);
			//movd[rdi + rsi - 4], mm2
			//jl loopB
		}
		//pxor mm7, mm7
		//movq mm5, mbTh

		//lea rax, bufP1
		//mov esi, 16

		//pxor mm4, mm4
		//pxor mm3, mm3
		auto msum = zero;
		auto msum1 = zero;
		//align 16
		for (i = 16; i < widthminus16; i += 16) {
			_mm_prefetch(reinterpret_cast<const char*>(bufP1 + i + 64), _MM_HINT_NTA);
			//prefetchnta[rax + rsi + 16]
			auto mmA = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bufP1 + i - 1));
			//movq mm0, [rax + rsi - 1]
			auto mmB = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bufP1 + i + 1));
			auto mmABC = _mm_adds_epu8(mmA, mmB);
			//paddusb mm0, [rax + rsi + 1]
			auto mmC = _mm_load_si128(reinterpret_cast<const __m128i*>(bufP1 + i));
			mmABC = _mm_adds_epu8(mmABC, mmC);
			//paddusb mm0, [rax + rsi]
			auto mmABC2 = mmABC;
			//movq mm1, mm0
			mmABC = _mm_subs_epu8(mmABC, mmbTh);
			//psubusb mm0, mm5
			mmABC2 = _mm_subs_epu8(mmABC2, mmbTh2);
			//psubusb mm1, mbTh2
			mmABC = _mm_cmpeq_epi8(mmABC, zero);
			//pcmpeqb mm0, mm7
			mmABC2 = _mm_cmpeq_epi8(mmABC2, zero);
			//pcmpeqb mm1, mm7
			mmABC = _mm_cmpeq_epi8(mmABC, zero);
			//pcmpeqb mm0, mm7
			mmABC2 = _mm_cmpeq_epi8(mmABC2, zero);
			//pcmpeqb mm1, mm7

			//lea esi, [esi + 8]
			mmABC = _mm_and_si128(mmABC, mmask1);
			//pand mm0, mask1
			mmABC2 = _mm_and_si128(mmABC2, mmask1);
			//pand mm1, mask1
			//cmp esi, widthminus16
			msum = _mm_add_epi8(msum, mmABC);
			//paddb mm4, mm0
			msum1 = _mm_add_epi8(msum1, mmABC2);
			//paddb mm3, mm1
			//jl loopC
		}

		msum = _mm_sad_epu8(msum, zero);
		msum = _mm_hadd_epi32(msum, zero);
		msum = _mm_hadd_epi32(msum, zero);
		//psadbw mm4, mm7
		int tsum = _mm_cvtsi128_si32(msum);
		//movd tsum, mm4

		msum1 = _mm_sad_epu8(msum1, zero);
		msum1 = _mm_hadd_epi32(msum1, zero);
		msum1 = _mm_hadd_epi32(msum1, zero);
		//psadbw mm3, mm7
		int tsum1 = _mm_cvtsi128_si32(msum1);
		//movd tsum1, mm3

		if ((y & 1) == 0) {
			pe0 += tsum;
			pe1 += tsum1;
		}
		else {
			po0 += tsum;
			po1 += tsum1;
		}
	}
	env->m_frameInfo[n].diffP0 = pe0;
	env->m_frameInfo[n].diffP1 = po0;
	env->m_frameInfo[n].diffS0 = pe1;
	env->m_frameInfo[n].diffS1 = po1;
	env->FreeFrame(srcC);
	env->FreeFrame(srcP);
}
