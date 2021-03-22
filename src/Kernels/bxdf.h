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

// See http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

float FresnelShlick(float f0, float n_dot_o)
{
    return f0 + (1.0f - f0) * pow(1.0f - n_dot_o, 5.0f);
}

float Blinn_D(float3 normal, float3 wh, float alpha)
{
    return (alpha + 2.0f) * pow(max(0.0f, dot(normal, wh)), alpha) * INV_TWO_PI;
}

float Beckmann_D(float3 normal, float3 wh, float alpha)
{
    float cosTheta2 = dot(normal, wh);
    cosTheta2 *= cosTheta2;
    float alpha2 = alpha * alpha;

    return exp(-(1.0f / cosTheta2 - 1.0f) / alpha2) * INV_PI / (alpha2 * cosTheta2 * cosTheta2);
}

float GGX_D(float alpha, float n_dot_h)
{
    float alpha2 = alpha * alpha;
    float denom = n_dot_h * n_dot_h * (alpha2 - 1.0f) + 1.0f;
    return alpha2 * INV_PI / (denom * denom);
}

float Schlick_G(float alpha, float n_dot_i)
{
    // Schlick-GGX from UE4
    float k = alpha * 0.5f;
    return n_dot_i / (n_dot_i * (1.0f - k) + k);
}

float3 Blinn_Sample(float3 n, float alpha, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float cosTheta = pow(GetRandomFloat(seed), 1.0f / (alpha + 1.0f));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * cosTheta);

}
/*

float3 Beckmann_Sample(float2 s, float3 n, float alpha)
{
    float phi = TWO_PI * s.x;
    float cosTheta = sqrt(1.0f / (1.0f - alpha * alpha * log(s.y)));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * cosTheta);

}
*/

float3 GGX_Sample(float2 s, float3 n, float alpha)
{
    float phi = TWO_PI * s.x;
    float cos_theta = 1.0f / sqrt(1.0 + (alpha * alpha * s.y) / (1.0 - s.y));
    float sin_theta = sqrt(max(0.0f, 1.0f - cos_theta * cos_theta));

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);

    return normalize(b * cos(phi) * sin_theta + t * sin(phi) * sin_theta + n * cos_theta);
}

#endif // BXDF_H
