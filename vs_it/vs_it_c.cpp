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

__forceinline unsigned char make_de_map_asm(
	unsigned const char* eax,
	unsigned const char* ebx,
	unsigned const char* ecx,
	int i, int step, int offset) {
	// return abs(a - (b + c) / 2)
	auto a = eax[i*step + offset];
	auto b = ebx[i*step + offset];
	auto c = ecx[i*step + offset];
	auto bc = (short(b) + short(c)) / 2;
	auto delta = a - bc;
	return static_cast<unsigned char>(delta >= 0 ? delta : -delta);
}

void IT::C_MakeDEmap_YV12(IScriptEnvironment*env, const VSFrameRef * ref, int offset)
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

		unsigned char y0, y1, u0, v0, uv;
		for (int i = 0; i < twidth; i++)
		{
			y0 = make_de_map_asm(pC, pTT, pBB, i, 2, 0);
			y1 = make_de_map_asm(pC, pTT, pBB, i, 2, 1);
			u0 = make_de_map_asm(pC_U, pTT_U, pBB_U, i, 1, 0);
			v0 = make_de_map_asm(pC_V, pTT_V, pBB_V, i, 1, 0);
			uv = VSMAX(u0, v0);
			pED[i * 2] = VSMAX(uv, y0);
			pED[i * 2 + 1] = VSMAX(uv, y1);
		}
	}
}

void IT::C_MakeMotionMap_YV12(IScriptEnvironment*env, int n, bool flag)
{
	n = clipFrame(n);
	if (flag == false && env->m_frameInfo[n].diffP0 >= 0)
		return;


	const int twidth = width;
	const int widthminus8 = width - 8;
	const int widthminus16 = width - 16;
	int i;

	const VSFrameRef* srcP = env->GetFrame(clipFrame(n - 1));
	const VSFrameRef* srcC = env->GetFrame(n);
	short bufP0[MAX_WIDTH];
	unsigned char bufP1[MAX_WIDTH];
	int pe0 = 0, po0 = 0, pe1 = 0, po1 = 0;
	for (int yy = 16; yy < height - 16; ++yy) {
		int y = yy;
		const unsigned char *pC = env->SYP(srcC, y);
		const unsigned char *pP = env->SYP(srcP, y);

		for (i = 0; i < twidth; i++) {
			bufP0[i] = static_cast<short>(pC[i]) - static_cast<short>(pP[i]);
		}

		for (i = 8; i < widthminus8; i++) {
			auto A = bufP0[i - 1];
			auto B = bufP0[i];
			auto C = bufP0[i + 1];
			auto delta = A - B + C - B;
			// Abs(B)
			B = B >= 0 ? B : -B;
			// Abs(delta)
			delta = delta >= 0 ? delta : -delta;
			// Saturate Subtract (B-delta) To 8-bit
			bufP1[i] = VSMIN(255, VSMAX(0, B - delta));
		}

		int tsum = 0, tsum1 = 0;
		for (i = 16; i < widthminus16; i++) {
			auto A = bufP1[i - 1];
			auto B = bufP1[i + 1];
			auto C = bufP1[i];
			auto ABC = A + B + C;
			if (ABC > 36) tsum++;
			if (ABC > 18) tsum1++;
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
	env->m_frameInfo[n].diffP0 = pe0;
	env->m_frameInfo[n].diffP1 = po0;
	env->m_frameInfo[n].diffS0 = pe1;
	env->m_frameInfo[n].diffS1 = po1;
	env->FreeFrame(srcC);
	env->FreeFrame(srcP);
}
