/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

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

#define SHADED_COLOR_INDEX   0
#define DIFFUSE_INDEX        1
#define DEPTH_INDEX          2
#define NORMAL_INDEX         3
#define MOTION_VECTORS_INDEX 4

__kernel void ResolveRadiance
(
    uint width,
    uint height,
    uint aov_index,
    __global float4* radiance,
    __global float3* diffuse_albedo,
    __global float*  depth,
    __global float3* normal,
    __global float2* motion_vectors,
    __global uint*   sample_counter,
    __write_only image2d_t result
#ifdef ENABLE_DEMODULATION
    , __global float3* direct_lighting_buffer
#endif // ENABLE_DEMODULATION
)
{
    uint pixel_idx = get_global_id(0);

    uint sample_count = sample_counter[0];

    int x = pixel_idx % width;
    int y = pixel_idx / width;

    if (aov_index == DIFFUSE_INDEX)
    {
        // Diffuse albedo
        write_imagef(result, (int2)(x, y), (float4)(diffuse_albedo[pixel_idx].xyz, 1.0f));
    }
    else if (aov_index == DEPTH_INDEX)
    {
        // Depth
        float depth_value = depth[pixel_idx] * 0.1f;
        write_imagef(result, (int2)(x, y), (float4)(depth_value, depth_value, depth_value, 1.0f));
    }
    else if (aov_index == NORMAL_INDEX)
    {
        // Normal
        float3 normal_value = normal[pixel_idx] * 0.5f + 0.5f;
        write_imagef(result, (int2)(x, y), (float4)(normal_value, 1.0f));
    }
    else if (aov_index == MOTION_VECTORS_INDEX)
    {
        // Motion vectors
        write_imagef(result, (int2)(x, y), (float4)(motion_vectors[pixel_idx], 0.0f, 1.0f));
    }
    else
    {
        // Shaded color
#ifdef ENABLE_DENOISER
        float denominator = 1.0f;
#else
        float denominator = 1.0f / (float)sample_count;
#endif // ENABLE_DENOISER

        float3 hdr = radiance[pixel_idx].xyz * denominator;

#ifdef ENABLE_DEMODULATION

        // Modulate albedo
        //float3 albedo = diffuse_albedo[pixel_idx].xyz;
        // hdr *= albedo;

        //hdr = direct_lighting_buffer[pixel_idx];// *denominator;
#endif // ENABLE_DEMODULATION

        float3 ldr = hdr / (hdr + 1.0f);
        write_imagef(result, (int2)(x, y), (float4)(ldr, 1.0f));
    }
}
