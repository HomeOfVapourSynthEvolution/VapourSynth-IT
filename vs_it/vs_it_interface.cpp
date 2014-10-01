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

#include "vs_it_interface.h"
#include "vs_it.h"

static IT *d = NULL;

static void VS_CC
itCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core,
const VSAPI *vsapi) {
	int err;
	char msg_buff[256] = "IT(" IT_VERSION "): ";
	char *msg = msg_buff + strlen(msg_buff);

	VSNodeRef * node = vsapi->propGetNode(in, "clip", 0, 0);
	VSVideoInfo * vi = new VSVideoInfo;
	*vi = *vsapi->getVideoInfo(node);

	FAIL_IF_ERROR(!vi->format || vi->width == 0 || vi->height == 0,
		"clip must be constant format");

	FAIL_IF_ERROR(vi->format->sampleType != stInteger ||
		vi->format->bitsPerSample != 8 ||
		vi->format->colorFamily != cmYUV,
		"only YUV420P8 input supported. You can you up.");

	FAIL_IF_ERROR(vi->width & 15,
		"width must be mod 16");

	FAIL_IF_ERROR(vi->height & 1,
		"height must be even")
	{
		int fps = vsapi->propGetInt(in, "fps", 0, &err);
		if (err) { fps = 24; }

		int threshold = vsapi->propGetInt(in, "threshold", 0, &err);
		if (err) { threshold = 20; }

		int pthreshold = vsapi->propGetInt(in, "pthreshold", 0, &err);
		if (err) { pthreshold = 75; }

		d = new IT(vi, node, fps, threshold, pthreshold, vsapi);
	}
	vsapi->createFilter(in, out, "it", itInit,
		itGetFrame, itFree, fmParallel,
		0, NULL, core);
	return;

fail:
	vsapi->freeNode(node);
	vsapi->setError(out, msg_buff);
}

void VS_CC
itInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
	vsapi->setVideoInfo(d->vi, 1, node);
}

void VS_CC
itFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
	vsapi->freeNode(d->node);
	delete d;
}

const VSFrameRef *VS_CC
itGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
	IScriptEnvironment env(frameCtx, core, vsapi);
	if (activationReason == arInitial) {
		d->GetFramePre(&env, n);
		return NULL;
	}
	if (activationReason != arAllFramesReady) {
		return NULL;
	}

	return d->GetFrame(&env, n);
}

VS_EXTERNAL_API(void)
VapourSynthPluginInit(VSConfigPlugin configFunc,
VSRegisterFunction registerFunc, VSPlugin *plugin)
{
	configFunc("in.7086.it", "it",
		"VapourSynth IVTC Filter v" IT_VERSION,
		VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("it",
		"clip:clip;fps:int:opt;threshold:int:opt;pthreshold:int:opt;",
		itCreate, 0, plugin);
}
