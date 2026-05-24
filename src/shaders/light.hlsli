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

#ifndef LIGHT_HLSLI
#define LIGHT_HLSLI

// Samples one analytic light. Matches the OpenCL Light_Sample helper shape:
// returns radiance, writes the unnormalized direction to outgoing and selection pdf to pdf.
float3 Light_Sample(float3 position, float3 normal, float s, out float3 outgoing, out float pdf)
{
    if (g_SceneInfo.light_count == 0u)
    {
        outgoing = 0.0f.xxx;
        pdf = 0.0f;
        return 0.0f.xxx;
    }

    uint light_idx = min(uint(s * float(g_SceneInfo.light_count)), g_SceneInfo.light_count - 1u);
    Light light = g_Lights[light_idx];

    pdf = 1.0f / float(g_SceneInfo.light_count);
    float3 light_radiance = light.radiance;

    if (light.type == LIGHT_TYPE_POINT)
    {
        outgoing = light.origin - position;
        float sq_length = max(dot(outgoing, outgoing), EPS);
        return light_radiance / sq_length;
    }

    outgoing = light.origin * MAX_RENDER_DIST;
    return light_radiance;
}

#endif  // LIGHT_HLSLI
