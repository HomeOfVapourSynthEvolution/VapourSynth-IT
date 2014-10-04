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

void VS_CC itInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    IT *d = (IT*)*instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}

void VS_CC itFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    IT *d = (IT*)instanceData;
    vsapi->freeNode(d->node);
    delete d;
}

const VSFrameRef *VS_CC itGetFrame(int n, int activationReason, void **instanceData, void **frameData,
                                   VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    IT *d = (IT*)*instanceData;
    if (activationReason == arInitial) {
        d->GetFramePre(n, frameCtx, vsapi);
        return NULL;
    }
    if (activationReason != arAllFramesReady)
        return NULL;

    return d->GetFrame(n, core, vsapi);
}

static void VS_CC itCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    int err;
    char msg_buff[256] = "IT(" IT_VERSION "): ";
    char *msg = msg_buff + strlen(msg_buff);

    VSNodeRef * node = vsapi->propGetNode(in, "clip", 0, 0);
    VSVideoInfo * vi = new VSVideoInfo;
    *vi = *vsapi->getVideoInfo(node);

    if (!vi->format || vi->width == 0 || vi->height == 0) {
        vsapi->freeNode(node);
        vsapi->setError(out, "clip must be constant format");
        return;
    }

    if (vi->format->sampleType != stInteger ||
        vi->format->bitsPerSample != 8 ||
        vi->format->colorFamily != cmYUV) {
        vsapi->freeNode(node);
        vsapi->setError(out, "only YUV420P8 input supported. You can you up.");
        return;
    }

    if (vi->width & 15) {
        vsapi->freeNode(node);
        vsapi->setError(out, "width must be mod 16");
        return;
    }

    if (vi->height & 1) {
        vsapi->freeNode(node);
        vsapi->setError(out, "height must be even");
        return;
    }

    if (vi->width > MAX_WIDTH) {
        vsapi->freeNode(node);
        vsapi->setError(out, "width too large");
        return;
    }

    int fps = int64ToIntS(vsapi->propGetInt(in, "fps", 0, &err));
    if (err)
        fps = 24;

    int threshold = int64ToIntS(vsapi->propGetInt(in, "threshold", 0, &err));
    if (err)
        threshold = 20;

    int pthreshold = int64ToIntS(vsapi->propGetInt(in, "pthreshold", 0, &err));
    if (err)
        pthreshold = 75;

    IT * d = new IT(vi, node, fps, threshold, pthreshold, vsapi);

    vsapi->createFilter(in, out, "it", itInit, itGetFrame, itFree, fmParallel, 0, d, core);
    return;
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("in.7086.it",
               "it",
               "VapourSynth IVTC Filter v" IT_VERSION,
               VAPOURSYNTH_API_VERSION,
               1,
               plugin);
    registerFunc("it",
                 "clip:clip;fps:int:opt;threshold:int:opt;pthreshold:int:opt;",
                 itCreate,
                 0,
                 plugin);
}
