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

float FresnelShlick(float f0, float n_dot_o)
{
    return f0 + (1.0f - f0) * pow(1.0f - n_dot_o, 5.0f);
}

float DistributionBlinn(float3 normal, float3 wh, float alpha)
{
    return (alpha + 2.0f) * pow(max(0.0f, dot(normal, wh)), alpha) * INV_TWO_PI;
}

float DistributionBeckmann(float3 normal, float3 wh, float alpha)
{
    float cosTheta2 = dot(normal, wh);
    cosTheta2 *= cosTheta2;
    float alpha2 = alpha * alpha;

    return exp(-(1.0f / cosTheta2 - 1.0f) / alpha2) * INV_PI / (alpha2 * cosTheta2 * cosTheta2);
}

float DistributionGGX(float cosTheta, float alpha)
{
    float alpha2 = alpha * alpha;
    return alpha2 * INV_PI / pow(cosTheta * cosTheta * (alpha2 - 1.0f) + 1.0f, 2.0f);
}

/*
float3 SampleBlinn(float3 n, float alpha, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float cosTheta = pow(GetRandomFloat(seed), 1.0f / (alpha + 1.0f));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * cosTheta);

}

float3 SampleBeckmann(float3 n, float alpha, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float cosTheta = sqrt(1.0f / (1.0f - alpha * alpha * log(GetRandomFloat(seed))));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * cosTheta);

}
*/

float3 SampleGGX(float2 s, float3 n, float alpha, float* cosTheta)
{
    float phi = TWO_PI * s.x;
    float xi = s.y;
    *cosTheta = sqrt((1.0f - xi) / (xi * (alpha * alpha - 1.0f) + 1.0f));
    float sinTheta = sqrt(max(0.0f, 1.0f - (*cosTheta) * (*cosTheta)));

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);

    return normalize(b * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * (*cosTheta));
}

#endif // BXDF_H
