#ifndef LIGHT_H
#define LIGHT_H

#include "constants.h"

float3 Light_Sample(__global Light* analytic_lights, SceneInfo scene_info, float3 position, float3 normal, float s, float3* outgoing, float* pdf)
{
    // Fetch random light
    int light_idx = clamp((int)(s * (float)scene_info.analytic_light_count), 0, (int)scene_info.analytic_light_count - 1);
    Light light = analytic_lights[light_idx];

    // Compute light selection pdf
    *pdf = 1.0f / scene_info.analytic_light_count;

    float3 light_radiance = light.radiance;

    if (light.type == LIGHT_TYPE_POINT)
    {
        float3 to_light = light.origin - position;

        // Compute light attenuation
        float sq_length = dot(to_light, to_light);
        light_radiance /= sq_length;
        *outgoing = to_light;
    }
    else // (light.type == LIGHT_TYPE_DIRECTIONAL)
    {
        *outgoing = light.origin * MAX_RENDER_DIST;
    }

    return light_radiance;
}

#endif // LIGHT_H
