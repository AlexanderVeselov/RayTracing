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

#ifndef UTILS_H
#define UTILS_H

float2 InterpolateAttributes2(float2 attr1, float2 attr2, float2 attr3, float2 uv)
{
    return attr1 * (1.0f - uv.x - uv.y) + attr2 * uv.x + attr3 * uv.y;
}

float3 InterpolateAttributes(float3 attr1, float3 attr2, float3 attr3, float2 uv)
{
    return attr1 * (1.0f - uv.x - uv.y) + attr2 * uv.x + attr3 * uv.y;
}

float3 reflect(float3 v, float3 n)
{
    return -v + 2.0f * dot(v, n) * n;
}

float3 TangentToWorld(float3 dir, float3 n)
{
    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);

    return normalize(b * dir.x + t * dir.y + n * dir.z);
}

float Luma(float3 rgb)
{
    return dot(rgb, (float3)(0.299f, 0.587f, 0.114f));
}

int To1D(int2 pos, int width)
{
    return pos.y * width + pos.x;
}

float frac(float x)
{
    return x - floor(x);
}

float4 SampleBilinear(float2 uv, __global float4* buffer, uint width, uint height)
{
    uv.x -= 0.5f / (float)width;
    uv.y -= 0.5f / (float)height;

    int2 texel_pos = (int2)(uv.x * width, uv.y * height);

    // Compute sample offsets
    int2 xy00 = clamp(texel_pos + (int2)(0, 0), (int2)(0, 0), (int2)(width - 1, height - 1));
    int2 xy01 = clamp(texel_pos + (int2)(0, 1), (int2)(0, 0), (int2)(width - 1, height - 1));
    int2 xy10 = clamp(texel_pos + (int2)(1, 0), (int2)(0, 0), (int2)(width - 1, height - 1));
    int2 xy11 = clamp(texel_pos + (int2)(1, 1), (int2)(0, 0), (int2)(width - 1, height - 1));

    // Fetch samples
    float4 val00 = buffer[To1D(xy00, width)];
    float4 val01 = buffer[To1D(xy01, width)];
    float4 val10 = buffer[To1D(xy10, width)];
    float4 val11 = buffer[To1D(xy11, width)];

    // Bilinear weights
    float wx = frac(uv.x * width);
    float wy = frac(uv.y * height);

    // Bilinear interpolation
    return mix(mix(val00, val01, wx), mix(val10, val11, wx), wy);
}

// From "Reconstruction filters in Computer Graphics", Don P. Mitchell, Arun N. Netravali
// https://www.cs.utexas.edu/~fussell/courses/cs384g-fall2013/lectures/mitchell/Mitchell.pdf
float FilterCubic(float x, float b, float c)
{
    float y = 0.0f;
    float x2 = x * x;
    float x3 = x * x * x;

    if (x < 1.0f)
    {
        y = (12.0f - 9.0f * b - 6.0f * c) * x3 + (-18.0f + 12.0f * b + 6.0f * c) * x2 + (6.0f - 2.0f * b);
    }
    else if (x <= 2.0f)
    {
        y = (-b - 6.0f * c) * x3 + (6.0f * b + 30.0f * c) * x2 + (-12.0f * b - 48.0f * c) * x + (8.0f * b + 24.0f * c);
    }

    return y / 6.0f;
}

float4 SampleBicubic(float2 uv, __global float4* buffer, uint width, uint height)
{
    float2 texel_pos = (float2)(uv.x * width, uv.y * height);
    float total_weight = 0.0f;
    float4 sum = 0.0f;

    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            float2 current_sample_pos = floor(texel_pos + (float2)(i, j)) + 0.5f;

            int2 current_pos = clamp((int2)(current_sample_pos.x, current_sample_pos.y), (int2)(0, 0), (int2)(width - 1, height - 1));

            float4 current_sample = buffer[To1D(current_pos, width)];
            float2 sample_dist = fabs(current_sample_pos - texel_pos);
            float filter_weight = FilterCubic(sample_dist.x, 0, 0.5f) * FilterCubic(sample_dist.y, 0, 0.5f);

            float sample_lum = Luma(current_sample.xyz);
            filter_weight *= 1.0f / (1.0f + sample_lum);

            sum += current_sample * filter_weight;
            total_weight += filter_weight;
        }
    }

    return (total_weight > 0.0f) ? (sum / total_weight) : 0.0f;
}

unsigned int WangHash(unsigned int x)
{
    x = (x ^ 61) ^ (x >> 16);
    x = x + (x << 3);
    x = x ^ (x >> 4);
    x = x * 0x27d4eb2d;
    x = x ^ (x >> 15);
    return x;
}

float4 UnpackRGBA8(uint data)
{
    float r = (float)( data        & 0xFF);
    float g = (float)((data >>  8) & 0xFF);
    float b = (float)((data >> 16) & 0xFF);
    float a = (float)((data >> 24) & 0xFF);

    return (float4)(r, g, b, a) / 255.0f;
}

float3 UnpackRGBTex(uint data, uint* texture_idx)
{
    float r = (float)(data & 0xFF);
    float g = (float)((data >> 8) & 0xFF);
    float b = (float)((data >> 16) & 0xFF);
    *texture_idx =   ((data >> 24) & 0xFF);

    return (float3)(r, g, b) / 255.0f;
}

float3 UnpackRGBE(uint rgbe)
{
    int r = (int)(rgbe >> 0 ) & 0xFF;
    int g = (int)(rgbe >> 8 ) & 0xFF;
    int b = (int)(rgbe >> 16) & 0xFF;
    int e = (int)(rgbe >> 24);

    float f = ldexp(1.0f, e - (128 + 8));
    return (float3)(r, g, b) * f;
}

void UnpackRoughnessMetalness(uint data, float* roughness, uint* roughness_idx,
    float* metalness, uint* metalness_idx)
{
    *roughness = (float)((data >> 0 ) & 0xFF) / 255.0f;
    *roughness_idx =    ((data >> 8 ) & 0xFF);
    *metalness = (float)((data >> 16) & 0xFF) / 255.0f;
    *metalness_idx =    ((data >> 24) & 0xFF);
}

void UnpackIorEmissionIdxTransparency(uint data, float* ior, uint* emission_idx,
    float* transparency, uint* transparency_idx)
{
    *ior = (float)((data >> 0) & 0xFF) / 25.5f;
    *emission_idx = ((data >> 8) & 0xFF);
    *transparency = (float)((data >> 16) & 0xFF) / 255.0f;
    *transparency_idx = ((data >> 24) & 0xFF);
}

#endif // UTILS_H
