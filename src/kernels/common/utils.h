/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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

#ifdef GLSL
float to_float(uint x)
{
    return float(x);
}
int to_int(uint x)
{
    return int(x);
}
float3 to_float3(float x)
{
    return float3(x, x, x);
}
float3 make_float3(float x, float y, float z)
{
    return float3(x, y, z);
}
float4 make_float4(float x, float y, float z, float w)
{
    return float4(x, y, z, w);
}
#define OUT(x) x
#else
float to_float(uint x)
{
    return (float)(x);
}
int to_int(uint x)
{
    return (int)(x);
}
float3 to_float3(float x)
{
    return (float3)(x, x, x);
}
float3 make_float3(float x, float y, float z)
{
    return (float3)(x, y, z);
}
float4 make_float4(float x, float y, float z, float w)
{
    return (float4)(x, y, z, w);
}
#define OUT(x) *x
#endif

#ifdef GLSL
float fabs(float x)
{
    return abs(x);
}
#endif // #ifdef GLSL

#ifndef GLSL
// OpenCL
float3 reflect(float3 v, float3 n)
{
    return v - 2.0f * dot(v, n) * n;
}
#endif // #ifndef GLSL

float2 InterpolateAttributes2(float2 attr1, float2 attr2, float2 attr3, float2 uv)
{
    return attr1 * (1.0f - uv.x - uv.y) + attr2 * uv.x + attr3 * uv.y;
}

float3 InterpolateAttributes(float3 attr1, float3 attr2, float3 attr3, float2 uv)
{
    return attr1 * (1.0f - uv.x - uv.y) + attr2 * uv.x + attr3 * uv.y;
}

float3 TangentToWorld(float3 dir, float3 n)
{
    float3 axis = fabs(n.x) > 0.001f ? make_float3(0.0f, 1.0f, 0.0f) : make_float3(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);

    return normalize(b * dir.x + t * dir.y + n * dir.z);
}

float Luma(float3 rgb)
{
    return dot(rgb, make_float3(0.299f, 0.587f, 0.114f));
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
    float r = to_float(data & 0xFF);
    float g = to_float((data >> 8) & 0xFF);
    float b = to_float((data >> 16) & 0xFF);
    float a = to_float((data >> 24) & 0xFF);

    return make_float4(r, g, b, a) / 255.0f;
}

float3 UnpackRGBTex(uint data,
#ifdef GLSL
    out uint texture_idx
#else
    uint* texture_idx
#endif
)
{
    float r = to_float(data & 0xFF);
    float g = to_float((data >> 8) & 0xFF);
    float b = to_float((data >> 16) & 0xFF);
    OUT(texture_idx) = ((data >> 24) & 0xFF);

    return make_float3(r, g, b) / 255.0f;
}

float3 UnpackRGBE(uint rgbe)
{
    int r = to_int((rgbe >> 0) & 0xFF);
    int g = to_int((rgbe >> 8) & 0xFF);
    int b = to_int((rgbe >> 16) & 0xFF);
    int e = to_int(rgbe >> 24);

    float f = ldexp(1.0f, e - (128 + 8));
    return make_float3(r, g, b) * f;
}

void UnpackRoughnessMetalness(uint data,
#ifdef GLSL
    out float roughness, out uint roughness_idx,
    out float metalness, out uint metalness_idx
#else
    float* roughness, uint* roughness_idx,
    float* metalness, uint* metalness_idx
#endif
)
{
    OUT(roughness) = to_float((data >> 0) & 0xFF) / 255.0f;
    OUT(roughness_idx) = ((data >> 8) & 0xFF);
    OUT(metalness) = to_float((data >> 16) & 0xFF) / 255.0f;
    OUT(metalness_idx) = ((data >> 24) & 0xFF);
}

void UnpackIorEmissionIdxTransparency(uint data,
#ifdef GLSL
    out float ior, out uint emission_idx,
    out float transparency, out uint transparency_idx
#else
    float* ior, uint* emission_idx,
    float* transparency, uint* transparency_idx
#endif
)
{
    OUT(ior) = to_float((data >> 0) & 0xFF) / 25.5f;
    OUT(emission_idx) = ((data >> 8) & 0xFF);
    OUT(transparency) = to_float((data >> 16) & 0xFF) / 255.0f;
    OUT(transparency_idx) = ((data >> 24) & 0xFF);
}

#endif // UTILS_H
