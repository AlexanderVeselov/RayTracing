#define kMaxRenderDist 20000.0f

#define GRID_RES 64
#define GRID_MAX 64
#define CELL_SIZE (GRID_MAX / GRID_RES)
#define CELL_INDEX(x, y, z) (((x) + (y) * GRID_RES + (z) * GRID_RES * GRID_RES))
#define POS_TO_CELL(pos) (CELL_INDEX((int)(pos.x) / CELL_SIZE, (int)(pos.y) / CELL_SIZE, (int)(pos.z) / CELL_SIZE))
#define CELL_X(ind) ((float)(((ind) % GRID_RES) * CELL_SIZE))
#define CELL_Y(ind) ((float)((((ind) / GRID_RES) % GRID_RES) * CELL_SIZE))
#define CELL_Z(ind) ((float)(((ind) / (GRID_RES * GRID_RES)) * CELL_SIZE))
#define CELL_TO_POS(ind) ((float3)(CELL_X(ind), CELL_Y(ind), CELL_Z(ind)))

typedef struct
{
    float3 diffuse;
    float3 specular;
    float3 emission;
} Material;

typedef struct
{
    float3 p1, p2, p3;
    float3 n1, n2, n3;
    Material material;
} Triangle;

typedef struct
{
    float3 origin;
    float3 dir;
} Ray;


typedef struct
{
    bool hit;
    float3 pos;
    float3 normal;
    __global Triangle* object;
    Ray ray;
} IntersectData;

typedef struct
{
    uint start_index;
    uint count;
} CellData;

typedef struct
{
    ulong a;
    ulong b;
    ulong c;
} random_state;

float3 reflect(float3 V, float3 N)
{
    return V - 2.0f * dot( V, N ) * N;
}

/*
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
*/

bool RayTriangle(const __global Triangle* triangle, const Ray* r, float* t, float3* n)
{
    float3 e1 = triangle->p2 - triangle->p1;
    float3 e2 = triangle->p3 - triangle->p1;
    // Calculate planes normal vector
    float3 pvec = cross(r->dir, e2);
    float det = dot(e1, pvec);

    // Ray is parallel to plane
    if (det < 1e-8f || -det > 1e-8f)
    {
        return false;
    }
    float inv_det = 1.0 / det;
    float3 tvec = r->origin - triangle->p1;
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

    *t = dot(e2, qvec) * inv_det;
    *n = normalize(u * triangle->n2 + v * triangle->n3 + (1.0f - u - v) * triangle->n1);
    
    return true;
}

void DDA_prepare(Ray ray, float3* tmax, float3* step, float3* tdelta)
{
    float3 cell_min = floor(ray.origin / CELL_SIZE) * CELL_SIZE;
    float3 cell_max = cell_min + CELL_SIZE;
    float3 tmaxneg = (cell_min - ray.origin) / ray.dir;
    float3 tmaxpos = (cell_max - ray.origin) / ray.dir;
    *tmax            = (ray.dir < 0) ? tmaxneg : tmaxpos;
    *step            = (ray.dir < 0) ? (float3)(-CELL_SIZE, -CELL_SIZE, -CELL_SIZE) : (float3)(CELL_SIZE, CELL_SIZE, CELL_SIZE);
    *tdelta            = fabs((float3)(CELL_SIZE, CELL_SIZE, CELL_SIZE) / ray.dir);
}

void DDA_step(float3* current_pos, float3* tmax, float3 step, float3 tdelta)
{
    if (tmax->x < tmax->y)
    {
        if (tmax->x < tmax->z)
        {
            tmax->x += tdelta.x;
            current_pos->x += step.x;
        }
        else
        {
            tmax->z += tdelta.z;
            current_pos->z += step.z;
        }
    }
    else
    {
        if (tmax->y < tmax->z)
        {
            tmax->y += tdelta.y;
            current_pos->y += step.y;
        }
        else
        {
            tmax->z += tdelta.z;
            current_pos->z += step.z;
        }
    }
}

IntersectData Intersect(Ray *ray, __global Triangle* triangles, __global uint* indices, __global CellData* cells)
{
    IntersectData out;
    out.hit = false;
    out.ray = *ray;

    float3 tmax, step, tdelta;
    DDA_prepare(*ray, &tmax, &step, &tdelta);
    
    float3 current_pos = CELL_TO_POS(POS_TO_CELL(ray->origin));
    
    float t;
    float3 n;
    float minT = kMaxRenderDist;
    
    while (current_pos.x >= 0 && current_pos.x < GRID_MAX
        && current_pos.y >= 0 && current_pos.y < GRID_MAX
        && current_pos.z >= 0 && current_pos.z < GRID_MAX)
    {
        int current_index = POS_TO_CELL(current_pos);
        CellData current_cell = cells[current_index];
        
        for (uint i = current_cell.start_index; i < current_cell.start_index + current_cell.count; ++i)
        {
            if (RayTriangle(&triangles[indices[i]], ray, &t, &n))
            {
                if (t < minT)
                {
                    minT = t;
                    out.pos = ray->origin + ray->dir * t;
                    out.object = &triangles[indices[i]];
                    out.normal = n;
                    out.hit = true;
                }
            }
        }
        if (out.hit && POS_TO_CELL(out.pos) == current_index)
        {
            break;
        }
        DDA_step(&current_pos, &tmax, step, tdelta);
    }
    
    return out;
}


float3 Raytrace(Ray *ray, __global Triangle* triangles, __global uint* indices, __global CellData* cells, int traceDepth)
{
    IntersectData data = Intersect(ray, triangles, indices, cells);
    return data.normal;    
}

__kernel void main
(
    __global float3* result,
    __global Triangle* triangles,
    __global uint* indices,
    __global CellData* cells,
    uint width,
    uint height,
    float3 cameraPos,
    float3 cameraFront,
    float3 cameraUp
)
{
    float invWidth = 1 / (float)(width), invHeight = 1 / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 90.0f;
    float angle = tan(3.1415f * 0.5f * fov / 180.0f);
    
    float x = (float)(get_global_id(0) % width);

    float y = (float)(get_global_id(0) / width);
    x = (2 * ((x + 0.5f) * invWidth) - 1) * angle * aspectratio; 
    y = -(1 - 2 * ((y + 0.5f) * invHeight)) * angle;
    
    Ray r;
    r.origin = cameraPos;
    float3 dir = normalize(-x * cross(cameraFront, cameraUp) + y * cameraUp + cameraFront);
    r.dir = dir;

    result[get_global_id(0)] = Raytrace(&r, triangles, indices, cells, 0);
 
}
