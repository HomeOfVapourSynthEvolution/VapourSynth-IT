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

#include "IScriptEnvironment.h"

IScriptEnvironment::IScriptEnvironment(VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
: frameCtx(_frameCtx), core(_core), vsapi(_vsapi)
{
	m_iSumC = m_iSumP = m_iSumN = 0;
	m_iUsePrev = m_iUseNext = 0;
}

IScriptEnvironment::~IScriptEnvironment()
{
}

VSFrameRef *IScriptEnvironment::NewVideoFrame(const VSVideoInfo *vi)
{
	return vsapi->newVideoFrame(vi->format, vi->width, vi->height, nullptr, core);
}
