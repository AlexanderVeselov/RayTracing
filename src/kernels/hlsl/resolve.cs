#include "common.hlsli"

RWTexture2D<float4> g_Output : register(u0);
RWStructuredBuffer<float4> g_DiffuseAlbedo : register(u16);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_Output.GetDimensions(width, height);
    if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
    {
        return;
    }

    uint pixel_idx = dispatch_thread_id.y * width + dispatch_thread_id.x;
    float3 albedo = g_DiffuseAlbedo[pixel_idx].xyz;
    g_Output[dispatch_thread_id.xy] = float4(pow(saturate(albedo), 1.0f / 2.2f), 1.0f);
}
