#define MAX_RENDER_DIST 20000.0f
#define MACHINE_EPSILON 1.192092896e-07f
#define GAMMA_3 3.576279966e-07f

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
    float3 tangent_t;
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


bool RayBounds(const __global Bounds3* bounds, const Ray* ray, float* tNear, float* tFar)
{
    float t0 = 0.0f, t1 = MAX_RENDER_DIST;
    float nearx = (bounds->pos[ray->sign[0]].x - ray->origin.x) * ray->invDir.x;    
    float farx = (bounds->pos[1 - ray->sign[0]].x - ray->origin.x) * ray->invDir.x;
    t0 = max(t0, nearx);
    t1 = min(t1, farx);
    if (t0 > t1) return false;

    float neary = (bounds->pos[ray->sign[1]].y - ray->origin.y) * ray->invDir.y;
    float fary = (bounds->pos[1 - ray->sign[1]].y - ray->origin.y) * ray->invDir.y;
    t0 = max(t0, neary);
    t1 = min(t1, fary);
    if (t0 > t1) return false;

    float nearz = (bounds->pos[ray->sign[2]].z - ray->origin.z) * ray->invDir.z;
    float farz = (bounds->pos[1 - ray->sign[2]].z - ray->origin.z) * ray->invDir.z;
    t0 = max(t0, nearz);
    t1 = min(t1, farz);
    if (t0 > t1) return false;

    *tNear = t0;
    *tFar = t1;    
    return true;

}

IntersectData IntersectBVH(Ray *ray, __global Triangle* triangles, __global LinearBVHNode* nodes)
{
    IntersectData isect;
    isect.hit = false;
    isect.ray = *ray;
    isect.t = MAX_RENDER_DIST;
    
    // Follow ray through BVH nodes to find primitive intersections
    int toVisitOffset = 0, currentNodeIndex = 0;
    int nodesToVisit[8];
    for (int i = 0; i < 64; ++i)
    {
        const __global LinearBVHNode* node = &nodes[currentNodeIndex];
        float tNear, tFar;
        if (RayBounds(&node->bounds, ray, &tNear, &tFar))
        {
            // Leaf node
            if (node->nPrimitives > 0)
            {
                // Intersect ray with primitives in leaf BVH node
                for (int i = 0; i < node->nPrimitives; ++i)
                {
                    RayTriangle(ray, &triangles[node->offset + i], &isect);
                }

                //if (tNear < isect.t)
                //{
                //    isect.t = tNear;
                //}

                if (toVisitOffset == 0) break;
                currentNodeIndex = nodesToVisit[--toVisitOffset];
            }
            else
            {
                // Put far BVH node on _nodesToVisit_ stack, advance to near
                // node
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

#define TRACE_DEPTH 3

//float3 RaytraceBVH(Ray *ray, __global Triangle* triangles, __global LinearBVHNode* nodes)
//{
//    IntersectData data[TRACE_DEPTH];
//
//    float3 Radiance = 0.0f;
//    int intersections = 0;
//    for (int i = 0; i < TRACE_DEPTH; ++i)
//    {
//        ++intersections;
//        data[i] = IntersectBVH(ray, triangles, nodes);
//        if (!data[i].hit) break;
//        
//        float3 pos = data[i].pos + ray->dir * 0.0001f;
//        float3 dir = reflect(ray->dir, data[i].normal);
//        *ray = InitRay(pos, dir);
//
//    }
//
//    for (int i = intersections - 1; i >= 0; --i)
//    {
//        if (!data[i].hit)
//        {
//            Radiance += mix((float3)(0.75f, 0.75f, 0.75f), (float3)(0.5f, 0.75f, 1.0f), data[i].ray.dir.z) * 1.25f + pow(max(dot(data[i].ray.dir, normalize((float3)(1.0f, 0.0f, 1.0f))), 0.0f), 64.0f) * 50.0f * (float3)(1.0f, 0.7f, 0.6f);
//            //break;
//        }
//        else
//        {
//            Radiance *= 0.75f;
//        }
//    }
//    return Radiance;
//
//    //IntersectData data = IntersectBVH(ray, triangles, nodes);
//
//
//    //return data.pos / 64.0f;
//
//}


float3 RaytraceBVH(Ray *ray, __global Triangle* triangles, __global LinearBVHNode* nodes)
{
    IntersectData data = IntersectBVH(ray, triangles, nodes);
    float3 sss;
    float3 pos = ray->origin + ray->dir * data.t;
    return fract(pos / 4.0f, &sss);
}

Ray CreateRay(uint width, uint height, float3 cameraPos, float3 cameraFront, float3 cameraUp)
{
    float invWidth = 1.0f / (float)(width), invHeight = 1.0f / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 90.0f * 3.1415f / 180.0f;
    float angle = tan(0.5f * fov);

    float x = (float)(get_global_id(0) % width);
    float y = (float)(get_global_id(0) / width);

    x = (2.0f * ((x + 0.5f) * invWidth) - 1) * angle * aspectratio;
    y = -(1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

    float3 dir = normalize(x * cross(cameraFront, cameraUp) + y * cameraUp + cameraFront);

    return InitRay(cameraPos, dir);
}

__kernel void main
(
    // Output
    __global float3* result,
    // Input
    __global Triangle* triangles,
    __global LinearBVHNode* nodes,
    uint width,
    uint height,
    float3 cameraPos,
    float3 cameraFront,
    float3 cameraUp,
    __read_only image2d_t tex
)
{
    const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
        
    Ray ray = CreateRay(width, height, cameraPos, cameraFront, cameraUp);

    //result[get_global_id(0)] = RaytraceBVH(&ray, triangles, nodes) / 32;
    result[get_global_id(0)] = RaytraceBVH(&ray, triangles, nodes);
    //float2 coords = RaytraceBVH(&ray, triangles, nodes).xy;
        
    //result[get_global_id(0)] = read_imagef(tex, smp, coords).xyz;
 


}
