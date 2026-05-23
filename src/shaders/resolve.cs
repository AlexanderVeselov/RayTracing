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
#include "frame_data.hlsli"

struct RootConstants
{
    uint sample_count;
};

ROOT_CONSTANTS
ConstantBuffer<RootConstants> g_RootConstants;

// Linear resolved color. Tonemapping is applied by a separate pass.
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_ResolvedColor : register(u1);

// Radiance data
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Radiance : register(u2);

// AOV data
IMAGE_FORMAT("rgba8")
RWTexture2D<float4> g_DiffuseAlbedo : register(u4);
RWTexture2D<float> g_Depth : register(u5);
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_Normal : register(u6);
IMAGE_FORMAT("rgba16f")
RWTexture2D<float4> g_MotionVectors : register(u7);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_ResolvedColor.GetDimensions(width, height);
    if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
    {
        return;
    }

    uint2 pixel_coord = dispatch_thread_id.xy;
    uint aov_index = g_RenderParams.x;
    if (aov_index == DIFFUSE_INDEX)
    {
        g_ResolvedColor[pixel_coord] = float4(saturate(g_DiffuseAlbedo[pixel_coord].xyz), 1.0f);
    }
    else if (aov_index == DEPTH_INDEX)
    {
        float depth_value = g_Depth[pixel_coord] * 0.1f;
        g_ResolvedColor[pixel_coord] = float4(depth_value.xxx, 1.0f);
    }
    else if (aov_index == NORMAL_INDEX)
    {
        float3 normal_value = g_Normal[pixel_coord].xyz * 0.5f + 0.5f;
        g_ResolvedColor[pixel_coord] = float4(normal_value, 1.0f);
    }
    else if (aov_index == MOTION_VECTORS_INDEX)
    {
        g_ResolvedColor[pixel_coord] = float4(g_MotionVectors[pixel_coord].xy, 0.0f, 1.0f);
    }
    else
    {
        float sample_count = max(float(g_RootConstants.sample_count), 1.0f);
        float3 color = (g_RenderParams.y & RENDER_FLAG_DENOISER) != 0u
                           ? g_Radiance[pixel_coord].xyz
                           : g_Radiance[pixel_coord].xyz / sample_count;
        g_ResolvedColor[pixel_coord] = float4(color, 1.0f);
    }
}
