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

__kernel void ResolveRadiance
(
    uint width,
    uint height,
    uint aov_index,
    __global float4* radiance,
    __global float3* diffuse_albedo,
    __global float*  depth,
    __global float2* motion_vectors,
    __global uint*   sample_counter,
    __write_only image2d_t result
)
{
    uint global_id = get_global_id(0);

    uint sample_count = sample_counter[0];

    int x = global_id % width;
    int y = global_id / width;

    if (aov_index == 1)
    {
        // Diffuse albedo
        write_imagef(result, (int2)(x, y), (float4)(diffuse_albedo[global_id].xyz, 1.0f));
    }
    else if (aov_index == 2)
    {
        // Depth
        float depth_value = depth[global_id] * 0.1f;
        write_imagef(result, (int2)(x, y), (float4)(depth_value, depth_value, depth_value, 1.0f));
    }
    else if (aov_index == 3)
    {
        // Motion vectors
        write_imagef(result, (int2)(x, y), (float4)(1.0f, 0.5f, 0.5f, 1.0f));
    }
    else
    {
        // Shaded color
        float3 hdr = radiance[global_id].xyz / (float)sample_count;
        float3 ldr = hdr / (hdr + 1.0f);

        write_imagef(result, (int2)(x, y), (float4)(ldr, 1.0f));
    }
}
