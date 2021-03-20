#ifndef BXDF_H
#define BXDF_H

#include "constants.h"

float3 SampleHemisphereCosine(float3 n, float2 s)
{
    float phi = TWO_PI * s.x;
    float sinThetaSqr = s.y;
    float sinTheta = sqrt(sinThetaSqr);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);

    return normalize(b * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * sqrt(1.0f - sinThetaSqr));
}

#endif // BXDF_H
