#include "src/utils/shared_structs.hpp"

#define MAX_RENDER_DIST 20000.0f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define INV_PI 0.31830988618f
#define INV_TWO_PI 0.15915494309f

typedef struct
{
    float3 origin;
    float3 dir;
    float3 invDir;
    int sign[3];
} Ray;

typedef struct
{
    bool hit;
    Ray ray;
    float t;
    float3 pos;
    float3 texcoord;
    float3 normal;
    const __global Triangle* object;
} IntersectData;

typedef struct
{
    __global Triangle* triangles;
    __global LinearBVHNode* nodes;
    __global Material* materials;
} Scene;

Ray InitRay(float3 origin, float3 dir)
{
    dir = normalize(dir);

    Ray r;
    r.origin = origin;
    r.dir = dir;
    r.invDir = 1.0f / dir;
    r.sign[0] = r.invDir.x < 0;
    r.sign[1] = r.invDir.y < 0;
    r.sign[2] = r.invDir.z < 0;

    return r;
}

unsigned int HashUInt32(unsigned int x)
{
#if 0
    x = (x ^ 61) ^ (x >> 16);
    x = x + (x << 3);
    x = x ^ (x >> 4);
    x = x * 0x27d4eb2d;
    x = x ^ (x >> 15);
    return x;
#else
    return 1103515245 * x + 12345;
#endif
}

float GetRandomFloat(unsigned int* seed)
{
    *seed = (*seed ^ 61) ^ (*seed >> 16);
    *seed = *seed + (*seed << 3);
    *seed = *seed ^ (*seed >> 4);
    *seed = *seed * 0x27d4eb2d;
    *seed = *seed ^ (*seed >> 15);
    *seed = 1103515245 * (*seed) + 12345;

    return (float)(*seed) * 2.3283064365386963e-10f;
}

float3 reflect(float3 v, float3 n)
{
    return -v + 2.0f * dot(v, n) * n;
}

float3 SampleHemisphereCosine(float3 n, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float sinThetaSqr = GetRandomFloat(seed);
    float sinTheta = sqrt(sinThetaSqr);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s*cos(phi)*sinTheta + t*sin(phi)*sinTheta + n*sqrt(1.0f - sinThetaSqr));
}

#define ALGORITHM2

#ifdef ALGORITHM1
bool RayTriangle(const __global Triangle* triangle, const Ray* r, float* t, float3* n)
{
    // Vertices
    float3 A = triangle->p1;
    float3 B = triangle->p2;
    float3 C = triangle->p3;

    // Figure out triangle plane
    float3 triangle_ab = B - A;
    float3 triangle_ac = C - A;
    float3 triangle_nn = cross(triangle_ab, triangle_ac);
    float3 triangle_n = normalize(triangle_nn);
    float triangle_support = dot(A, triangle_n);

    // Compute intersection distance, bail if infinite or negative
    float intersection_det = dot(triangle_n, r->dir);
    if(fabs(intersection_det) <= 0.0001f) {
        return false;
    }
    float intersection_dist = (triangle_support - dot(triangle_n, r->origin)) / intersection_det;
    if(intersection_dist <= 0.0f)
    {
        return false;
    }

    // Compute intersection point
    float3 Q = r->origin + r->dir * intersection_dist;

    // Test inside-ness
    float3 triangle_bc = C - B;
    float3 triangle_ca = A - C;
    float3 triangle_aq = Q - A;
    float3 triangle_bq = Q - B;
    float3 triangle_cq = Q - C;

    float baryA = dot(cross(triangle_bc, triangle_bq), triangle_n);
    float baryB = dot(cross(triangle_ca, triangle_cq), triangle_n);
    float baryC = dot(cross(triangle_ab, triangle_aq), triangle_n);

    if(baryA < 0.0f || baryB < 0.0f || baryC < 0.0f)
    {
        return false;
    }
    
    // Perform barycentric interpolation of normals
    float triangle_den = dot(triangle_nn, triangle_n);
    baryA /= triangle_den;
    baryB /= triangle_den;
    baryC /= triangle_den;

    float3 N = normalize(
        triangle->n1 * baryA + 
        triangle->n2 * baryB + 
        triangle->n3 * baryC
    );
    *t = intersection_dist;
    *n = N;

    return true;
}

#else
bool RayTriangle(const Ray* r, const __global Triangle* triangle, IntersectData* isect)
{
    float3 e1 = triangle->v2.position - triangle->v1.position;
    float3 e2 = triangle->v3.position - triangle->v1.position;
    // Calculate planes normal vector
    float3 pvec = cross(r->dir, e2);
    float det = dot(e1, pvec);

    // Ray is parallel to plane
    if (det < 1e-8f || -det > 1e-8f)
    {
        return false;
    }
    float inv_det = 1.0f / det;
    float3 tvec = r->origin - triangle->v1.position;
    float u = dot(tvec, pvec) * inv_det;
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }

    float3 qvec = cross(tvec, e1);
    float v = dot(r->dir, qvec) * inv_det;
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    float t = dot(e2, qvec) * inv_det;

    if (t < isect->t)
    {
        isect->hit = true;
        isect->t = t;
        isect->pos = isect->ray.origin + isect->ray.dir * t;
        isect->object = triangle;
        isect->normal = normalize(u * triangle->v2.normal + v * triangle->v3.normal + (1.0f - u - v) * triangle->v1.normal);
        isect->texcoord = u * triangle->v2.texcoord + v * triangle->v3.texcoord + (1.0f - u - v) * triangle->v1.texcoord;
    }

    return true;
}
#endif


bool RayBounds(const __global Bounds3* bounds, const Ray* ray, float t)
{
    float t0 = max(0.0f, (bounds->pos[ray->sign[0]].x - ray->origin.x) * ray->invDir.x);
    float t1 = min(t, (bounds->pos[1 - ray->sign[0]].x - ray->origin.x) * ray->invDir.x);

    t0 = max(t0, (bounds->pos[ray->sign[1]].y - ray->origin.y) * ray->invDir.y);
    t1 = min(t1, (bounds->pos[1 - ray->sign[1]].y - ray->origin.y) * ray->invDir.y);

    t0 = max(t0, (bounds->pos[ray->sign[2]].z - ray->origin.z) * ray->invDir.z);
    t1 = min(t1, (bounds->pos[1 - ray->sign[2]].z - ray->origin.z) * ray->invDir.z);

    return (t1 >= t0);

}

IntersectData Intersect(Ray *ray, const Scene* scene)
{
    IntersectData isect;
    isect.hit = false;
    isect.ray = *ray;
    isect.t = MAX_RENDER_DIST;
    
    float t;
    // Follow ray through BVH nodes to find primitive intersections
    int toVisitOffset = 0, currentNodeIndex = 0;
    int nodesToVisit[64];
    while (true)
    {
        __global LinearBVHNode* node = &scene->nodes[currentNodeIndex];

        if (RayBounds(&node->bounds, ray, isect.t))
        {
            // Leaf node
            if (node->nPrimitives > 0)
            {
                // Intersect ray with primitives in leaf BVH node
                for (int i = 0; i < node->nPrimitives; ++i)
                {
                    RayTriangle(ray, &scene->triangles[node->offset + i], &isect);
                }

                if (toVisitOffset == 0) break;
                currentNodeIndex = nodesToVisit[--toVisitOffset];
            }
            else
            {
                // Put far BVH node on _nodesToVisit_ stack, advance to near node
                if (ray->sign[node->axis])
                {
                    nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
                    currentNodeIndex = node->offset;
                }
                else
                {
                    nodesToVisit[toVisitOffset++] = node->offset;
                    currentNodeIndex = currentNodeIndex + 1;
                }
            }
        }
        else
        {
            if (toVisitOffset == 0) break;
            currentNodeIndex = nodesToVisit[--toVisitOffset];
        }
    }

    return isect;
}

float3 SampleSky(__read_only image2d_t tex, float3 dir)
{
    //return 0.0f;
    const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;

    // Convert (normalized) dir to spherical coordinates.
    float2 coords = (float2)(atan2(dir.x, dir.y) + PI, acos(dir.z));
    coords.x = coords.x < 0.0f ? coords.x + TWO_PI : coords.x;
    coords.x *= INV_TWO_PI;
    coords.y *= INV_PI;

    return read_imagef(tex, smp, coords).xyz;

}

float3 saturate(float3 value)
{
    return min(max(value, 0.0f), 1.0f);
}


float DistributionBlinn(float3 normal, float3 wh, float alpha)
{
    return (alpha + 2.0f) * pow(max(0.0f, dot(normal, wh)), alpha) * INV_TWO_PI;    
}

float DistributionBeckmann(float3 normal, float3 wh, float alpha)
{
    float cosTheta2 = dot(normal, wh);
    cosTheta2 *= cosTheta2;
    float alpha2 = alpha*alpha;

    return exp(-(1.0f / cosTheta2 - 1.0f) / alpha2) * INV_PI / (alpha2 * cosTheta2 * cosTheta2);
}

float DistributionGGX(float cosTheta, float alpha)
{
    float alpha2 = alpha*alpha;
    return alpha2 * INV_PI / pow(cosTheta * cosTheta * (alpha2 - 1.0f) + 1.0f, 2.0f);
}

float3 SampleBlinn(float3 n, float alpha, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float cosTheta = pow(GetRandomFloat(seed), 1.0f / (alpha + 1.0f));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s*cos(phi)*sinTheta + t*sin(phi)*sinTheta + n*cosTheta);

}

float3 SampleBeckmann(float3 n, float alpha, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float cosTheta = sqrt(1.0f / (1.0f - alpha * alpha * log(GetRandomFloat(seed))));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s*cos(phi)*sinTheta + t*sin(phi)*sinTheta + n*cosTheta);

}

float3 SampleGGX(float3 n, float alpha, float* cosTheta, unsigned int* seed)
{
    float phi = TWO_PI * GetRandomFloat(seed);
    float xi = GetRandomFloat(seed);
    *cosTheta = sqrt((1.0f - xi) / (xi * (alpha * alpha - 1.0f) + 1.0f));
    float sinTheta = sqrt(max(0.0f, 1.0f - (*cosTheta) * (*cosTheta)));

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s*cos(phi)*sinTheta + t*sin(phi)*sinTheta + n*(*cosTheta));;
}

float FresnelShlick(float f0, float nDotWi)
{
    return f0 + (1.0f - f0) * pow(1.0f - nDotWi, 5.0f);
}

float3 SampleDiffuse(float3 wo, float3* wi, float* pdf, float3 texcoord, float3 normal, const __global Material* material, unsigned int* seed)
{
    *wi = SampleHemisphereCosine(normal, seed);
    *pdf = dot(*wi, normal) * INV_PI;

    float3 albedo = (sin(texcoord.x * 64) > 0) * (sin(texcoord.y * 64) > 0) + (sin(texcoord.x * 64 + PI) > 0) * (sin(texcoord.y * 64 + PI) > 0) * 2.0f;
    return albedo * material->diffuse * INV_PI;
}

//#define BLINN
float3 SampleSpecular(float3 wo, float3* wi, float* pdf, float3 normal, const __global Material* material, unsigned int* seed)
{
#ifdef BLINN
    float alpha = 2.0f / pow(material->roughness, 2.0f) - 2.0f;
    float3 wh = SampleBlinn(normal, alpha, seed);
#else
    float alpha = material->roughness;
    float cosTheta;
    float3 wh = SampleGGX(normal, alpha, &cosTheta, seed);
#endif
        *wi = reflect(wo, wh);
    if (dot(*wi, normal) * dot(wo, normal) < 0.0f) return 0.0f;
#ifdef BLINN
    float D = DistributionBlinn(normal, wh, alpha);
#else
    float D = DistributionGGX(cosTheta, alpha);
#endif
    *pdf = D * cosTheta / (4.0f * dot(wo, wh));
    // Actually, _material->ior_ isn't ior value, this is f0 value for now
    return D / (4.0f * dot(*wi, normal) * dot(wo, normal)) * material->specular;
}

float3 SampleBrdf(float3 wo, float3* wi, float* pdf, float3 texcoord, float3 normal, const __global Material* material, unsigned int* seed)
{
    bool doSpecular = dot(material->specular, (float3)(1.0f, 1.0f, 1.0f)) > 0.0f;
    bool doDiffuse = dot(material->diffuse, (float3)(1.0f, 1.0f, 1.0f)) > 0.0f;

    if (doSpecular && !doDiffuse)
    {
        return SampleSpecular(wo, wi, pdf, normal, material, seed);
    }
    else if (!doSpecular && doDiffuse)
    {
        return SampleDiffuse(wo, wi, pdf, texcoord, normal, material, seed);
    }
    else if (doSpecular && doDiffuse)
    {
        if (GetRandomFloat(seed) > 0.5f)
        {
            return SampleSpecular(wo, wi, pdf, normal, material, seed) * 2.0f;
        }
        else
        {
            return SampleDiffuse(wo, wi, pdf, texcoord, normal, material, seed) * 2.0f;
        }
    }
    else
    {
        return 0.0f;
    }

}

float3 Render(Ray* ray, const Scene* scene, unsigned int* seed, __read_only image2d_t tex)
{
    float3 radiance = 0.0f;
    float3 beta = 1.0f;
            
    for (int i = 0; i < 5; ++i)
    {
        IntersectData isect = Intersect(ray, scene);

        if (!isect.hit)
        {
            float3 val = beta * max(SampleSky(tex, ray->dir), 0.0f);
            radiance += val;
            break;
        }
        
        const __global Material* material = &scene->materials[isect.object->mtlIndex];
        radiance += beta * material->emission * 50.0f;

        float3 wi;
        float3 wo = -ray->dir;
        float pdf = 0.0f;
        float3 f = SampleBrdf(wo, &wi, &pdf, isect.texcoord, isect.normal, material, seed);
        if (pdf <= 0.0f) break;

        float3 mul = f * dot(wi, isect.normal) / pdf;
        beta *= mul;
        *ray = InitRay(isect.pos + wi * 0.01f, wi);

    }
    
    return max(radiance, 0.0f);
}

float2 PointInHexagon(unsigned int* seed)
{
    float2 hexPoints[3] = { (float2)(-1.0f, 0.0f), (float2)(0.5f, 0.866f), (float2)(0.5f, -0.866f) };
    int x = floor(GetRandomFloat(seed) * 3.0f);
    float2 v1 = hexPoints[x];
    float2 v2 = hexPoints[(x + 1) % 3];
    float p1 = GetRandomFloat(seed);
    float p2 = GetRandomFloat(seed);
    return (float2)(p1 * v1.x + p2 * v2.x, p1 * v1.y + p2 * v2.y);
}

Ray CreateRay(uint width, uint height, float3 cameraPos, float3 cameraFront, float3 cameraUp, unsigned int* seed)
{
    float invWidth = 1.0f / (float)(width), invHeight = 1.0f / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 45.0f * 3.1415f / 180.0f;
    float angle = tan(0.5f * fov);

    float x = (float)(get_global_id(0) % width) + GetRandomFloat(seed) - 0.5f;
    float y = (float)(get_global_id(0) / width) + GetRandomFloat(seed) - 0.5f;

    x = (2.0f * ((x + 0.5f) * invWidth) - 1) * angle * aspectratio;
    y = -(1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

    float3 dir = normalize(x * cross(cameraFront, cameraUp) + y * cameraUp + cameraFront);

    // Simple Depth of Field
    float3 pointAimed = cameraPos + 60.0f * dir;
    //float2 dofDir = (float2)(GetRandomFloat(seed), GetRandomFloat(seed));
    //dofDir = normalize(dofDir * 2.0f - 1.0f) * GetRandomFloat(seed);
    float2 dofDir = PointInHexagon(seed);
    float r = 1.0f;
    float3 newPos = cameraPos + dofDir.x * r * cross(cameraFront, cameraUp) + dofDir.y * r * cameraUp;
    
    return InitRay(newPos, normalize(pointAimed - newPos));
    //return InitRay(cameraPos, dir);
}

#define GAMMA_CORRECTION

float3 ToGamma(float3 value)
{
#ifdef GAMMA_CORRECTION
    return pow(value, 1.0f / 2.2f);
#else
    return value;
#endif
}

float3 FromGamma(float3 value)
{
#ifdef GAMMA_CORRECTION
    return pow(value, 2.2f);
#else
    return value;
#endif
}

__kernel void KernelEntry
(
    // Output
    // Input
    __global float3* result,
    __global Triangle* triangles,
    __global LinearBVHNode* nodes,
    __global Material* materials,
    uint width,
    uint height,
    float3 cameraPos,
    float3 cameraFront,
    float3 cameraUp,
    unsigned int frameCount,
    unsigned int frameSeed,
    __read_only image2d_t tex
)
{
    Scene scene = { triangles, nodes, materials };

    unsigned int seed = get_global_id(0) + HashUInt32(frameCount);
    
    Ray ray = CreateRay(width, height, cameraPos, cameraFront, cameraUp, &seed);
    float3 radiance = Render(&ray, &scene, &seed, tex);
        
    if (frameCount == 0)
    {
        result[get_global_id(0)] = ToGamma(radiance);
    }
    else
    {
        result[get_global_id(0)] = ToGamma((FromGamma(result[get_global_id(0)]) * (frameCount - 1) + radiance) / frameCount);
    }

}
