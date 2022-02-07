/*****************************************************************************
 MIT License

 Copyright(c) 2022 Alexander Veselov

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

#ifndef BXDF_H
#define BXDF_H

#include "constants.h"

// See http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

float3 SampleHemisphereCosine(float2 s, float* pdf)
{
    float phi = TWO_PI * s.x;
    float sin_theta_sqr = s.y;
    float sin_theta = sqrt(sin_theta_sqr);

    float cos_theta = sqrt(1.0f - sin_theta_sqr);
    *pdf = cos_theta * INV_PI;

    return (float3)(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// See https://en.wikipedia.org/wiki/Schlick%27s_approximation
float IorToF0(float ior_incident, float ior_transmitted)
{
    float result = (ior_transmitted - ior_incident) / (ior_transmitted + ior_incident);
    return result * result;
}

float F0ToIor(float f0)
{
    float r = sqrt(f0);
    return (1.0 + r) / (1.0 - r);
}

// Use half vector instead of normal when evaluating fresnel in microfacet models
// It doesn't matter to pass h_dot_o or h_dot_i because they are equivalent values
float3 FresnelSchlick(float3 f0, float h_dot_o)
{
    return f0 + (1.0f - f0) * pow(1.0f - h_dot_o, 5.0f);
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

float V_SmithGGXCorrelated(float n_dot_i, float n_dot_o, float alphaG)
{
    // Original formulation of G_SmithGGX Correlated
    // lambda_v = ( -1 + sqrt ( alphaG2 * (1 - NdotL2 ) / NdotL2 + 1)) * 0.5 f;
    // lambda_l = ( -1 + sqrt ( alphaG2 * (1 - NdotV2 ) / NdotV2 + 1)) * 0.5 f;
    // G_SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l );
    // V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0 f * NdotL * NdotV );

    // This is the optimize version
    float alphaG2 = alphaG * alphaG;
    // Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
    float Lambda_GGXV = n_dot_o * sqrt((-n_dot_i * alphaG2 + n_dot_i) * n_dot_i + alphaG2);
    float Lambda_GGXL = n_dot_i * sqrt((-n_dot_o * alphaG2 + n_dot_o) * n_dot_o + alphaG2);

    return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float Lambert()
{
    return INV_PI;
}

/*
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

// Eric Heitz. Sampling the GGX Distribution of Visible Normals
// http://jcgt.org/published/0007/04/01/
float3 GGXVNDF_Sample(float2 u, float3 n, float alpha, float3 incoming)
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    float3 Vh = normalize((float3)(alpha * incoming.x, alpha * incoming.y, incoming.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0 ? (float3)(-Vh.y, Vh.x, 0) / sqrt(lensq) : (float3)(1, 0, 0);
    float3 T2 = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = sqrt(u.y);
    float phi = TWO_PI * u.x;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;
    // Section 4.3: reprojection onto hemisphere
    float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    float3 Ne = normalize((float3)(alpha * Nh.x, alpha * Nh.y, max(0.0f, Nh.z)));
    return Ne;
}

#endif // BXDF_H
