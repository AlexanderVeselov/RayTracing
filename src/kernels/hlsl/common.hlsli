static const float EPS = 0.001f;
static const float INV_PI = 0.318309886f;
static const float INV_TWO_PI = 0.159154943f;
static const float TWO_PI = 6.283185307f;
static const float MAX_RENDER_DIST = 1000000.0f;
static const uint INVALID_ID = 0xFFFFFFFFu;
static const uint INVALID_TEXTURE_IDX = 0xFFu;
static const uint LIGHT_TYPE_POINT = 0u;

static const uint SAMPLE_TYPE_SUBPIXEL = 0u;
static const uint SAMPLE_TYPE_BXDF_LAYER = 1u;
static const uint SAMPLE_TYPE_BXDF_U = 2u;
static const uint SAMPLE_TYPE_BXDF_V = 3u;
static const uint SAMPLE_TYPE_LIGHT = 4u;
static const uint SAMPLE_TYPE_MAX = 5u;

struct Ray
{
    float4 origin;
    float4 direction;
};

struct Hit
{
    float2 bc;
    uint primitive_id;
    float t;
};

struct Vertex
{
    float4 position;
    float4 texcoord;
    float4 normal;
};

struct RhiTriangle
{
    Vertex v1;
    Vertex v2;
    Vertex v3;
    uint material_index;
    uint3 padding;
};

struct Bounds3
{
    float4 pmin;
    float4 pmax;
};

struct LinearBVHNode
{
    Bounds3 bounds;
    uint offset;
    uint num_primitives_axis;
    uint2 padding;
};

struct PackedMaterial
{
    uint diffuse_albedo;
    uint specular_albedo;
    uint emission;
    uint roughness_metalness;
    uint ior_emission_idx_transparency;
};

struct Material
{
    float3 diffuse_albedo;
    float roughness;
    float3 specular_albedo;
    float metalness;
    float3 emission;
    float ior;
    float transparency;
};

struct Light
{
    float4 origin;
    float4 radiance;
    uint type;
    uint3 padding;
};

struct TextureInfo
{
    int data_start;
    int width;
    int height;
    int padding;
};

float3 InterpolateAttributes(float3 a, float3 b, float3 c, float2 uv)
{
    return a * (1.0f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

float2 InterpolateAttributes2(float2 a, float2 b, float2 c, float2 uv)
{
    return a * (1.0f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

uint WangHash(uint x)
{
    x = (x ^ 61u) ^ (x >> 16u);
    x = x + (x << 3u);
    x = x ^ (x >> 4u);
    x = x * 0x27d4eb2du;
    x = x ^ (x >> 15u);
    return x;
}

float RandomFloat(inout uint seed)
{
    seed = WangHash(seed);
    return float(seed) * 2.3283064365386963e-10f;
}

float3 UnpackRGBTex(uint data, out uint texture_idx)
{
    texture_idx = (data >> 24) & 0xFFu;
    return float3((data >> 0) & 0xFFu, (data >> 8) & 0xFFu, (data >> 16) & 0xFFu) / 255.0f;
}

float3 UnpackRGBE(uint rgbe)
{
    int e = int(rgbe >> 24);
    if (e == 0)
    {
        return 0.0f.xxx;
    }

    return float3((rgbe >> 0) & 0xFFu, (rgbe >> 8) & 0xFFu, (rgbe >> 16) & 0xFFu) *
           exp2(float(e - (128 + 8)));
}

float4 UnpackRGBA8(uint data)
{
    return float4(float((data >> 0) & 0xFFu), float((data >> 8) & 0xFFu),
               float((data >> 16) & 0xFFu), float((data >> 24) & 0xFFu)) /
           255.0f;
}

float IorToF0(float ior_incident, float ior_transmitted)
{
    float result = (ior_transmitted - ior_incident) / (ior_transmitted + ior_incident);
    return result * result;
}

float3 FresnelSchlick(float3 f0, float h_dot_o)
{
    return f0 + (1.0f - f0) * pow(1.0f - h_dot_o, 5.0f);
}

float Luma(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float GGX_D(float alpha, float n_dot_h)
{
    float alpha2 = alpha * alpha;
    float denom = n_dot_h * n_dot_h * (alpha2 - 1.0f) + 1.0f;
    return alpha2 * INV_PI / (denom * denom);
}

float V_SmithGGXCorrelated(float n_dot_i, float n_dot_o, float alpha)
{
    float alpha2 = alpha * alpha;
    float lambda_v = n_dot_o * sqrt((-n_dot_i * alpha2 + n_dot_i) * n_dot_i + alpha2);
    float lambda_l = n_dot_i * sqrt((-n_dot_o * alpha2 + n_dot_o) * n_dot_o + alpha2);
    return 0.5f / max(lambda_v + lambda_l, EPS);
}

float3 TangentToWorld(float3 dir, float3 n)
{
    float3 axis = abs(n.x) > 0.001f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);
    return normalize(b * dir.x + t * dir.y + n * dir.z);
}

float3 GGX_Sample(float2 s, float3 n, float alpha)
{
    float phi = TWO_PI * s.x;
    float cos_theta = 1.0f / sqrt(1.0f + (alpha * alpha * s.y) / max(1.0f - s.y, EPS));
    float sin_theta = sqrt(max(0.0f, 1.0f - cos_theta * cos_theta));

    float3 axis = abs(n.x) > 0.001f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 b = cross(n, t);

    return normalize(b * cos(phi) * sin_theta + t * sin(phi) * sin_theta + n * cos_theta);
}

float3 SampleHemisphereCosine(float2 s, out float pdf)
{
    float phi = TWO_PI * s.x;
    float sin_theta_sqr = s.y;
    float sin_theta = sqrt(sin_theta_sqr);
    float cos_theta = sqrt(1.0f - sin_theta_sqr);
    pdf = cos_theta * INV_PI;
    return float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

float3 EvaluateMaterial(Material material, float3 normal, float3 incoming, float3 outgoing)
{
    if (material.transparency < 0.5f)
    {
        return 0.0f.xxx;
    }

    float3 half_vec = normalize(incoming + outgoing);
    float n_dot_i = max(dot(normal, incoming), EPS);
    float n_dot_o = max(dot(normal, outgoing), EPS);
    float n_dot_h = max(dot(normal, half_vec), EPS);
    float h_dot_o = max(dot(half_vec, outgoing), EPS);
    float alpha = material.roughness * material.roughness;
    float3 f0 =
        lerp(IorToF0(1.0f, material.ior).xxx, material.specular_albedo, material.metalness.xxx);
    float3 diffuse_color = (1.0f - material.metalness) * material.diffuse_albedo;
    float3 fresnel = FresnelSchlick(f0, h_dot_o);
    return fresnel * GGX_D(alpha, n_dot_h) * V_SmithGGXCorrelated(n_dot_i, n_dot_o, alpha) +
           (1.0f - fresnel) * diffuse_color * INV_PI;
}

float SampleRandom(uint pixel_i, uint pixel_j, uint sample_index, uint bounce, uint sample_type)
{
    uint sample_dimension = bounce * SAMPLE_TYPE_MAX + sample_type;
    uint seed = WangHash(pixel_i);
    seed = WangHash(seed + WangHash(pixel_j));
    seed = WangHash(seed + WangHash(sample_index));
    seed = WangHash(seed + WangHash(sample_dimension));
    return float(seed) * 2.3283064365386963e-10f;
}

float3 Tonemap(float3 color)
{
    color = color / (1.0f.xxx + color);
    return pow(saturate(color), 1.0f / 2.2f);
}
