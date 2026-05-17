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

#include "common.hlsli"

// Radiance data
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Radiance : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;
    uint width;
    uint height;
    g_Radiance.GetDimensions(width, height);
    uint radiance_count = width * height;

    if (idx < radiance_count)
    {
        g_Radiance[PixelCoord(idx, width)] = 0.0f.xxxx;
    }
}
