#include "common.hlsli"
#include "frame_data.hlsli"

RWTexture2D<float4> g_Output : register(u0);
RWStructuredBuffer<float4> g_Radiance : register(u13);
RWStructuredBuffer<uint> g_SampleCounter : register(u15);
RWStructuredBuffer<float4> g_DiffuseAlbedo : register(u16);
RWStructuredBuffer<float> g_Depth : register(u17);
RWStructuredBuffer<float4> g_Normal : register(u18);
RWStructuredBuffer<float4> g_MotionVectors : register(u26);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_Output.GetDimensions(width, height);
    if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
    {
        return;
    }

    uint pixel_idx = dispatch_thread_id.y * width + dispatch_thread_id.x;
    uint aov_index = g_RenderParams.x;
    if (aov_index == DIFFUSE_INDEX)
    {
        g_Output[dispatch_thread_id.xy] = float4(saturate(g_DiffuseAlbedo[pixel_idx].xyz), 1.0f);
    }
    else if (aov_index == DEPTH_INDEX)
    {
        float depth_value = g_Depth[pixel_idx] * 0.1f;
        g_Output[dispatch_thread_id.xy] = float4(depth_value.xxx, 1.0f);
    }
    else if (aov_index == NORMAL_INDEX)
    {
        float3 normal_value = g_Normal[pixel_idx].xyz * 0.5f + 0.5f;
        g_Output[dispatch_thread_id.xy] = float4(normal_value, 1.0f);
    }
    else if (aov_index == MOTION_VECTORS_INDEX)
    {
        g_Output[dispatch_thread_id.xy] = float4(g_MotionVectors[pixel_idx].xy, 0.0f, 1.0f);
    }
    else
    {
        float sample_count = max(float(g_SampleCounter[0]), 1.0f);
        float3 color = (g_RenderParams.y & RENDER_FLAG_DENOISER) != 0u
                           ? g_Radiance[pixel_idx].xyz
                           : g_Radiance[pixel_idx].xyz / sample_count;
        g_Output[dispatch_thread_id.xy] = float4(Tonemap(color), 1.0f);
    }
}
