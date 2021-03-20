#ifndef BXDF_H
#define BXDF_H

float3 SampleHemisphereCosine(float3 n, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float sinThetaSqr = GetRandomFloat(seed);
    float sinTheta = sqrt(sinThetaSqr);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s * cos(phi) * sinTheta + t * sin(phi) * sinTheta + n * sqrt(1.0f - sinThetaSqr));
}


#endif // BXDF_H
