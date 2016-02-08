#define kMaxRenderDist 20000.0f;

#define GRID_RES 10
#define GRID_MAX 400
#define CELL_SIZE (GRID_MAX / GRID_RES)
#define CELL_INDEX(x, y, z) (((x) + (y) * GRID_RES + (z) * GRID_RES * GRID_RES))
#define POS_TO_CELL(pos) (CELL_INDEX((int)(pos.x) / CELL_SIZE, (int)(pos.y) / CELL_SIZE, (int)(pos.z) / CELL_SIZE))
#define CELL_X(ind) ((float)(((ind) % GRID_RES) * CELL_SIZE))
#define CELL_Y(ind) ((float)((((ind) / GRID_RES) % GRID_RES) * CELL_SIZE))
#define CELL_Z(ind) ((float)(((ind) / (GRID_RES * GRID_RES)) * CELL_SIZE))
#define CELL_TO_POS(ind) ((float3)(CELL_X(ind), CELL_Y(ind), CELL_Z(ind)))

typedef struct Sphere
{
	float3 Position;
	float3 Color;
	float Radius;

} Sphere;

typedef struct
{
	float3 Position;
	float3 Normal;

} Plane;

typedef struct
{
	float3 Position;
	float3 Color;

} Light;
/*
typedef struct
{
	Sphere spheres[64];
	int spheresCount;

	Plane planes[10];
	int planesCount;
	
	Light lights[10];
	int lightsCount;

} Scene;
*/
typedef struct{
	float3 origin;
	float3 dir;
} Ray;

typedef struct{
	bool hit;
	float dist;
	__global Sphere* object;
	int objectType;
} IntersectData;

typedef struct
{
	uint start_index;
	uint count;

} CellData;

bool rayPlane(Plane* p, Ray* r, float* t)
{
	float dotProduct = dot(r->dir, p->Normal);
	if (dotProduct == 0){
		return false;
	}
	*t = dot(p->Normal,p->Position-r->origin) / dotProduct ;

	return *t >= 0;
}
/*
IntersectData intersect(Ray* ray, Scene* scene)
{
	float minT = kMaxRenderDist;
	IntersectData out;
	out.hit = false;

	for(int i = 0; i < scene->spheresCount; i++){
		float t;
		if ( raySphere( &scene->spheres[i], ray, &t ) ){
			if ( t < minT ){
				minT = t;
				out.dist = t;
				out.objectType = 1;
				out.object = &scene->spheres[i];
				out.hit = true;
			}
		}
	}
	
	for(int i = 0; i < scene->planesCount; i++){
		float t;
		if ( rayPlane( &scene->planes[i], ray, &t ) ){
			if (t < minT){
				minT = t;
				out.dist = t;
				out.objectType = 2;
				out.object = &scene->planes[i];
				out.hit = true;
			}
		}
	}
	
	return out;
}
*/
float3 reflect(float3 V, float3 N)
{
	return V - 2.0f * dot( V, N ) * N;
}
/*
float4 raytrace(Ray* ray, Scene* scene, int traceDepth)
{
	IntersectData data = intersect(ray, scene);

	if (data.hit)
	{
		float3 Normal;
		float4 diffuseColor = 0.0f;
		float4 albedo = 0.0f;
		float3 intersectPos = ray->origin+ray->dir*data.dist;

		if (data.objectType == 1)
		{
			Normal = normalize(intersectPos-((Sphere*)data.object)->Position);
			albedo = (float4)(((Sphere*)data.object)->Color, 1.0f);
		}
		else if (data.objectType == 2)
		{
			Normal = ((Plane*)data.object)->Normal;
			albedo = (round(sin(intersectPos.x) * 0.5f + 0.5f) == round(sin(intersectPos.y) * 0.5f + 0.5f)) * 0.75f + 0.25f;
		}
		
		diffuseColor = (float4)(0.25f, 0.75f, 1.0f, 0) * albedo * 0.25f;
		
		for (int i = 0; i < scene->lightsCount; ++i)
		{
			Ray lightRay;

			float3 lightDir = normalize(scene->lights[i].Position);
			lightRay.origin = intersectPos + lightDir*0.001f;
			lightRay.dir = lightDir;
			data = intersect(&lightRay, scene);

			if (!data.hit)
			{
				diffuseColor += albedo * max(0.0f, dot(Normal, lightDir));
			}
		}
		
		if (traceDepth == 0)
		{
			Ray reflectedRay;

			reflectedRay.dir = reflect(ray->dir, Normal);
			reflectedRay.origin = intersectPos + reflectedRay.dir * 0.0001f;
			diffuseColor += raytrace(&reflectedRay, scene, traceDepth + 1) * 0.5f;
		}
		
		return diffuseColor;
	}
	else
	{
		return (float4)(0.25f, 0.75f, 1.0f, 0);
	}
}
*/

bool raySphere(const __global Sphere* s, Ray* r, float* t)
{
	float3 rayToCenter = s->Position.xyz - r->origin;
	float dotProduct = dot(r->dir, rayToCenter);
	float d = dotProduct * dotProduct - dot(rayToCenter, rayToCenter) + s->Radius * s->Radius;

	if (d < 0)
	{
		return false;
	}
	*t = (dotProduct - sqrt(d));

	if (*t < 0)
	{
		*t = (dotProduct + sqrt(d));
		if (*t < 0)
		{
			return false;
		}
	}

	return true;
}

void DDA_prepare(Ray ray, float3* tmax, float3* step, float3* tdelta)
{
	float3 cell_min = floor(ray.origin / CELL_SIZE) * CELL_SIZE;
	float3 cell_max = cell_min + CELL_SIZE;
	float3 tmax_neg = (cell_min - ray.origin) / ray.dir;
	float3 tmax_pos = (cell_max - ray.origin) / ray.dir;
	*tmax			= (ray.dir < 0) ? tmax_neg : tmax_pos;
	*step			= (ray.dir < 0) ? (float3)(-CELL_SIZE, -CELL_SIZE, -CELL_SIZE) : (float3)(CELL_SIZE, CELL_SIZE, CELL_SIZE);
	*tdelta			= fabs((float3)(CELL_SIZE, CELL_SIZE, CELL_SIZE) / ray.dir);
}

void DDA_step(float3* current_pos, float3* tmax, float3 step, float3 tdelta)
{
	if ((*tmax).x < (*tmax).y)
	{
		if ((*tmax).x < (*tmax).z)
		{
			(*tmax).x += tdelta.x;
			(*current_pos).x += step.x;
		}
		else
		{
			(*tmax).z += tdelta.z;
			(*current_pos).z += step.z;
		}
	}
	else
	{
		if ((*tmax).y < (*tmax).z)
		{
			(*tmax).y += tdelta.y;
			(*current_pos).y += step.y;
		}
		else
		{
			(*tmax).z += tdelta.z;
			(*current_pos).z += step.z;
		}
	}
}

IntersectData Intersect(Ray *ray, __global Sphere* spheres, __global uint* indices, __global CellData* cells)
{
	IntersectData out;
	out.hit = false;

	float3 tmax, step, tdelta;
	DDA_prepare(*ray, &tmax, &step, &tdelta);
	
	int start_index = POS_TO_CELL(ray->origin);
	float3 current_pos = CELL_TO_POS(start_index);
	
	float t;
	float minT = kMaxRenderDist;
	while (current_pos.x >= 0 && current_pos.x < GRID_MAX
		&& current_pos.y >= 0 && current_pos.y < GRID_MAX
		&& current_pos.z >= 0 && current_pos.z < GRID_MAX)
	{
		int current_index = POS_TO_CELL(current_pos);
		CellData current_cell = cells[current_index];
		for (uint i = current_cell.start_index; i < current_cell.start_index+current_cell.count; ++i)
		{
			if (raySphere(&spheres[indices[i]], ray, &t))
			{
				if (t < minT)
				{
					minT = t;
					out.dist = t;
					out.objectType = 1;
					out.object = &spheres[indices[i]];
					out.hit = true;
				}
			}
		}
		if (out.hit)
		{
			break;
		}
		DDA_step(&current_pos, &tmax, step, tdelta);
	}
	
	return out;
}


float4 raytrace(Ray *ray, __global Sphere* spheres, __global uint* indices, __global CellData* cells, int traceDepth)
{
	IntersectData data = Intersect(ray, spheres, indices, cells);
	
	const int light_count = 1;
	Light lights[2];
	lights[0].Position = (float3)(0.5f, 0.5f, 0.75f);
	lights[1].Position = (float3)(48, 48, 12);

	if (data.hit)
	{
		float3 Normal;
		float4 diffuseColor = 0.0f;
		float4 albedo = 0.0f;
		float3 intersectPos = ray->origin+ray->dir*data.dist;

		Normal = normalize(intersectPos-data.object->Position);
		albedo = (float4)(Normal * 0.5f + 0.5f, 1.0f);
		
		if (traceDepth == 0)
		{
			Ray reflectedRay;

			reflectedRay.dir = reflect(ray->dir, Normal);
			reflectedRay.origin = intersectPos + reflectedRay.dir * 0.0001f;
			float fresnel = 1.0f - clamp(dot(-ray->dir, Normal), 0.0f, 1.0f);
			diffuseColor += raytrace(&reflectedRay, spheres, indices, cells, traceDepth + 1) * fresnel;
		}
		
		
		for (int i = 0; i < light_count; ++i)
		{
			Ray lightRay;
			float3 lightDir = normalize(lights[i].Position);
			lightRay.origin = intersectPos + lightDir*0.001f;
			lightRay.dir = lightDir;
            data = Intersect(&lightRay, spheres, indices, cells);

			if (!data.hit)
			{
				float att = 1.0f;//1.0f - clamp(distance(intersectPos, lights[0].Position) / 64.0f, 0.0f, 1.0f);
				diffuseColor += albedo * max(0.0f, dot(Normal, lightDir)) * att;// + pow(dot(reflect(-lightDir, Normal), ray->dir), 32.0f);
			}
		}
		return diffuseColor;
	}
	else
	{
		float zenith = ray->dir.z;
		float4 color1 = (float4)(0.75f, 0.75f, 0.75f, 1.0f);
		float4 color2 = (float4)(0.5f, 0.75f, 1.0f, 1.0f);

		return mix(color1, color2, zenith) + pow(max(0.0f, dot(normalize(lights[0].Position), ray->dir)), 32.0f);
	}
}

__kernel void main(__global float4* result, uint width, uint height, float3 cameraPos, float3 cameraFront, float3 cameraUp,
					__global Sphere* spheres, __global uint* indices, __global CellData* cells)
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
	float3 dir = normalize(x * cross(cameraUp, cameraFront) + y * cameraUp + cameraFront);
	r.dir = dir;

	result[get_global_id(0)] = raytrace(&r, spheres, indices, cells, 0);
	
}
