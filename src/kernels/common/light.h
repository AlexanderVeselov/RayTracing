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

#ifndef LIGHT_H
#define LIGHT_H

#include "src/kernels/common/constants.h"

float3 Light_Sample(
#ifdef GLSL
    SceneInfo scene_info, float3 position, float3 normal, float s, out float3 outgoing, out float pdf)
#else
    __global Light* analytic_lights, SceneInfo scene_info, float3 position, float3 normal, float s, float3* outgoing, float* pdf)
#endif
{
    // Fetch random light
#ifdef GLSL
    int light_idx = clamp(int(s * float(scene_info.analytic_light_count)), 0, int(scene_info.analytic_light_count) - 1);
#else
    int light_idx = clamp((int)(s * (float)scene_info.analytic_light_count), 0, (int)scene_info.analytic_light_count - 1);
#endif
    Light light = analytic_lights[light_idx];

    // Compute light selection pdf
    OUT(pdf) = 1.0f / scene_info.analytic_light_count;

    float3 light_radiance = light.radiance;

    if (light.type == LIGHT_TYPE_POINT)
    {
        float3 to_light = light.origin - position;

        // Compute light attenuation
        float sq_length = dot(to_light, to_light);
        light_radiance /= sq_length;
        OUT(outgoing) = to_light;
    }
    else // (light.type == LIGHT_TYPE_DIRECTIONAL)
    {
        OUT(outgoing) = light.origin * MAX_RENDER_DIST;
    }

    return light_radiance;
}

#endif // LIGHT_H
