/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

#ifndef FRAME_DATA_HLSLI
#define FRAME_DATA_HLSLI

#include "shared_structures.h"

cbuffer FrameData : register(b0)
{
    Camera g_Camera;
    Camera g_PrevCamera;
    uint4 g_RenderSize;
    uint4 g_SceneCounts;
    uint4 g_RenderParams;
};

static const uint SHADED_COLOR_INDEX = 0u;
static const uint DIFFUSE_INDEX = 1u;
static const uint DEPTH_INDEX = 2u;
static const uint NORMAL_INDEX = 3u;
static const uint MOTION_VECTORS_INDEX = 4u;

static const uint RENDER_FLAG_WHITE_FURNACE = 1u;
static const uint RENDER_FLAG_DENOISER = 2u;

#endif
