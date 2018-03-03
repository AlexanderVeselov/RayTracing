#define MAX_RENDER_DIST 20000.0f
#define MACHINE_EPSILON 1.192092896e-07f
#define GAMMA_3 3.576279966e-07f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define INV_PI 0.31830988618f
#define INV_TWO_PI 0.15915494309f

typedef struct
{
    float3 diffuse;
    float3 specular;
    float3 emission;
} Material;

typedef struct
{
    float3 position;
    float3 texcoord;
    float3 normal;
    float3 tangent_s;
} Vertex;

typedef struct
{
    Vertex v1, v2, v3;
    //uint mtlIndex;
} Triangle;

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
    int steps;
    const __global Triangle* object;
} IntersectData;

typedef struct
{
    float3 pos[2];
} Bounds3;

typedef struct
{
    Bounds3 bounds;
    int offset;
    ushort nPrimitives;
    uchar axis;
    uchar pad[9];
} LinearBVHNode;

Ray InitRay(float3 origin, float3 dir)
{
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

float3 SampleHemisphere(float3 normal, unsigned int* seed)
{
    float rand1 = TWO_PI * GetRandomFloat(seed);
    float rand2 = GetRandomFloat(seed);
    float rand2s = sqrt(rand2);

    float3 w = normal;
    float3 axis = fabs(w.x) > 0.1f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 u = normalize(cross(axis, w));
    float3 v = cross(w, u);

    return normalize(u * cos(rand1)*rand2s + v*sin(rand1)*rand2s + w*sqrt(1.0f - rand2));
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return (float)(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

float2 Hammersley2d(uint i, uint N)
{
    return (float2)((float)(i) / (float)(N), RadicalInverse_VdC(i));
}

float3 SampleHammersley(float3 normal, unsigned int frameNum)
{
    float2 rand12 = Hammersley2d(frameNum, 1000);

    float rand1 = TWO_PI * rand12.x;
    float rand2 = rand12.y;
    float rand2s = sqrt(rand2);

    float3 w = normal;
    float3 axis = fabs(w.x) > 0.1f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 u = normalize(cross(axis, w));
    float3 v = cross(w, u);

    return normalize(u * cos(rand1)*rand2s + v*sin(rand1)*rand2s + w*sqrt(1.0f - rand2));
}

float3 reflect(float3 v, float3 n)
{
    return v - 2.0f * dot(v, n) * n;
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

IntersectData Intersect(Ray *ray, __global Triangle* triangles, __global LinearBVHNode* nodes)
{
    IntersectData isect;
    isect.hit = false;
    isect.ray = *ray;
    isect.t = MAX_RENDER_DIST;
    isect.steps = 0;
    
    float t;
    // Follow ray through BVH nodes to find primitive intersections
    int toVisitOffset = 0, currentNodeIndex = 0;
    int nodesToVisit[64];
    while (true)
    {
        __global LinearBVHNode* node = &nodes[currentNodeIndex];
        ++isect.steps;

        if (RayBounds(&node->bounds, ray, isect.t))
        {
            // Leaf node
            if (node->nPrimitives > 0)
            {
                // Intersect ray with primitives in leaf BVH node
                for (int i = 0; i < node->nPrimitives; ++i)
                {
                    RayTriangle(ray, &triangles[node->offset + i], &isect);
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


float3 OrenNayar(float3 wo, float3 wi, float3 normal)
{
    float roughness = 0.7f;
    float albedo = 0.9f;

    float LdotV = dot(wi, wo);
    float NdotL = dot(wi, normal);
    float NdotV = dot(normal, wo);

    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0f, s));

    float sigma2 = roughness * roughness;
    float A = 1.0f + sigma2 * (albedo / (sigma2 + 0.13f) + 0.5f / (sigma2 + 0.33f));
    float B = 0.45f * sigma2 / (sigma2 + 0.09f);

    return albedo * max(0.0f, NdotL) * (A + B * s / t) / PI;
}

float3 Phong(float3 wo, float3 wi, float3 normal, float3 albedo)
{
    return pow(max(dot(reflect(wi, normal), wo), 0.0f), 32.0f) * 4.0f + albedo;
}

float3 BlinnPhong(float3 wo, float3 wi, float3 normal, float3 albedo)
{
    //return pow(min(max(dot(normalize(wo + wi), normal), 0.0f), 1.0f), 4.0f) * 25.0f + 0.5f;
    return 0.5f;
}

float3 SampleSky(__read_only image2d_t tex, float3 dir)
{
    const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;

    // Convert (normalized) dir to spherical coordinates.
    float2 coords = (float2)(atan2(dir.x, dir.y) + PI, acos(dir.z));
    coords.x = coords.x < 0.0f ? coords.x + TWO_PI : coords.x;
    coords.x *= INV_TWO_PI;
    coords.y *= INV_PI;

    return read_imagef(tex, smp, coords).xyz * 2.5f;

}

float3 Render(Ray *ray, __global Triangle* triangles, __global LinearBVHNode* nodes, unsigned int* seed, unsigned int frameNum, __read_only image2d_t tex)
{
    float3 radiance = 0.0f;
    float3 mask = 1.0f;
            
    for (int i = 0; i < 5; ++i)
    {
        if (mask.x < 0.05f) break;
        IntersectData isect = Intersect(ray, triangles, nodes);

        if (!isect.hit)
        {
            //radiance += mask * mix((float3)(0.75f, 0.75f, 0.75f), (float3)(0.5f, 0.75f, 1.0f), ray->dir.z) * 1.25f + pow(max(dot(ray->dir, normalize((float3)(0.75f, 0.25f, 1.0f))), 0.0f), 256.0f) * 100.0f * (float3)(1.0f, 0.8f, 0.7f);
            //radiance += mask * (float3)(0.8f, 0.9f, 1.0f) * 1.5f;//mix((float3)(0.75f, 0.75f, 0.75f), (float3)(0.5f, 0.75f, 1.0f), ray->dir.z) * 1.25f;
            radiance += mask * SampleSky(tex, ray->dir);
            break;
        }
        
        // Coat
        float3 newdir;
        if (GetRandomFloat(seed) < 0.98f)
            newdir = SampleHemisphere(isect.normal, seed);
        else
            newdir = reflect(ray->dir, isect.normal);

        //float3 newdir = SampleHemisphere(isect.normal, seed);

        float3 albedo = (sin(isect.texcoord.x * 64) > 0) * (sin(isect.texcoord.y * 64) > 0) + (sin(isect.texcoord.x * 64 + PI) > 0) * (sin(isect.texcoord.y * 64 + PI) > 0);
        mask *= Phong(newdir, ray->dir, isect.normal, albedo) * dot(newdir, isect.normal);

        *ray = InitRay(isect.pos + isect.normal, newdir);

    }

    return radiance;
}

Ray CreateRay(uint width, uint height, float3 cameraPos, float3 cameraFront, float3 cameraUp, unsigned int* seed)
{
    float invWidth = 1.0f / (float)(width), invHeight = 1.0f / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 90.0f * 3.1415f / 180.0f;
    float angle = tan(0.5f * fov);

    float x = (float)(get_global_id(0) % width) + GetRandomFloat(seed) - 0.5f;
    float y = (float)(get_global_id(0) / width) + GetRandomFloat(seed) - 0.5f;

    x = (2.0f * ((x + 0.5f) * invWidth) - 1) * angle * aspectratio;
    y = -(1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

    float3 dir = normalize(x * cross(cameraFront, cameraUp) + y * cameraUp + cameraFront);

    return InitRay(cameraPos, dir);
}

__kernel void main
(
    // Output
    // Input
    __global float3* result,
    __global Triangle* triangles,
    __global LinearBVHNode* nodes,
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
    const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
        

    unsigned int seed = get_global_id(0) + HashUInt32(frameCount);
    
    Ray ray = CreateRay(width, height, cameraPos, cameraFront, cameraUp, &seed);

    float3 radiance = Render(&ray, triangles, nodes, &seed, frameCount, tex);

    if (frameCount == 0)
    {
        result[get_global_id(0)] = radiance;
    }
    else
    {
        result[get_global_id(0)] = (result[get_global_id(0)] * (frameCount - 1) + radiance) / frameCount;
    }

}
