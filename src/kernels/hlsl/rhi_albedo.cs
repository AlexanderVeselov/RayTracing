static const float EPS = 0.001f;
static const float PI = 3.141592654f;
static const float INV_PI = 0.318309886f;
static const float MAX_RENDER_DIST = 1000000.0f;
static const uint INVALID_ID = 0xFFFFFFFFu;
static const uint INVALID_TEXTURE_IDX = 0xFFu;
static const uint LIGHT_TYPE_POINT = 0u;
static const uint LIGHT_TYPE_DIRECTIONAL = 1u;

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

struct Hit
{
    float2 bc;
    uint primitive_id;
    float t;
};

RWTexture2D<float4> g_Output : register(u0);

cbuffer CameraData : register(b1)
{
    float4 g_CameraPositionFov;
    float4 g_CameraFrontAspect;
    float4 g_CameraUpPadding;
    uint4 g_SceneCounts;
};

StructuredBuffer<RhiTriangle> g_Triangles : register(t2);
StructuredBuffer<LinearBVHNode> g_Nodes : register(t3);
StructuredBuffer<PackedMaterial> g_Materials : register(t4);
StructuredBuffer<Light> g_Lights : register(t5);
StructuredBuffer<TextureInfo> g_Textures : register(t6);
StructuredBuffer<uint> g_TextureData : register(t7);

float3 InterpolateAttributes(float3 a, float3 b, float3 c, float2 uv)
{
    return a * (1.0f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

float2 InterpolateAttributes2(float2 a, float2 b, float2 c, float2 uv)
{
    return a * (1.0f - uv.x - uv.y) + b * uv.x + c * uv.y;
}

float3 UnpackRGBTex(uint data, out uint texture_idx)
{
    texture_idx = (data >> 24) & 0xFFu;
    return float3((data >> 0) & 0xFFu, (data >> 8) & 0xFFu, (data >> 16) & 0xFFu) / 255.0f;
}

float3 UnpackRGBE(uint rgbe)
{
    int r = int((rgbe >> 0) & 0xFFu);
    int g = int((rgbe >> 8) & 0xFFu);
    int b = int((rgbe >> 16) & 0xFFu);
    int e = int(rgbe >> 24);

    if (e == 0)
    {
        return 0.0f.xxx;
    }

    return float3(r, g, b) * exp2(float(e - (128 + 8)));
}

void UnpackRoughnessMetalness(uint data, out float roughness, out uint roughness_idx,
    out float metalness, out uint metalness_idx)
{
    roughness = float((data >> 0) & 0xFFu) / 255.0f;
    roughness_idx = (data >> 8) & 0xFFu;
    metalness = float((data >> 16) & 0xFFu) / 255.0f;
    metalness_idx = (data >> 24) & 0xFFu;
}

void UnpackIorEmissionIdxTransparency(uint data, out float ior, out uint emission_idx,
    out float transparency, out uint transparency_idx)
{
    ior = float((data >> 0) & 0xFFu) / 25.5f;
    emission_idx = (data >> 8) & 0xFFu;
    transparency = float((data >> 16) & 0xFFu) / 255.0f;
    transparency_idx = (data >> 24) & 0xFFu;
}

float4 UnpackRGBA8(uint data)
{
    return float4(
        float((data >> 0) & 0xFFu),
        float((data >> 8) & 0xFFu),
        float((data >> 16) & 0xFFu),
        float((data >> 24) & 0xFFu)) / 255.0f;
}

float3 SampleTexture(uint texture_index, float2 uv)
{
    if (texture_index == INVALID_TEXTURE_IDX || texture_index >= g_SceneCounts.w)
    {
        return 1.0f.xxx;
    }

    TextureInfo texture_info = g_Textures[texture_index];
    uv = frac(uv);
    uv.y = 1.0f - uv.y;

    int texel_x = clamp(int(uv.x * texture_info.width), 0, texture_info.width - 1);
    int texel_y = clamp(int(uv.y * texture_info.height), 0, texture_info.height - 1);
    int texel_addr = texture_info.data_start + texel_y * texture_info.width + texel_x;

    return saturate(UnpackRGBA8(g_TextureData[texel_addr]).xyz);
}

Material ApplyTextures(PackedMaterial packed_material, float2 uv)
{
    Material material;

    uint diffuse_albedo_idx;
    material.diffuse_albedo = UnpackRGBTex(packed_material.diffuse_albedo, diffuse_albedo_idx);
    if (diffuse_albedo_idx != INVALID_TEXTURE_IDX)
    {
        material.diffuse_albedo = pow(SampleTexture(diffuse_albedo_idx, uv), 2.2f.xxx);
    }

    uint specular_albedo_idx;
    material.specular_albedo = UnpackRGBTex(packed_material.specular_albedo, specular_albedo_idx);
    if (specular_albedo_idx != INVALID_TEXTURE_IDX)
    {
        material.specular_albedo = pow(SampleTexture(specular_albedo_idx, uv), 2.2f.xxx);
    }

    material.emission = UnpackRGBE(packed_material.emission);

    uint roughness_idx;
    uint metalness_idx;
    UnpackRoughnessMetalness(packed_material.roughness_metalness,
        material.roughness, roughness_idx, material.metalness, metalness_idx);

    if (roughness_idx != INVALID_TEXTURE_IDX)
    {
        material.roughness = SampleTexture(roughness_idx, uv).x;
    }

    if (metalness_idx != INVALID_TEXTURE_IDX)
    {
        material.metalness = SampleTexture(metalness_idx, uv).x;
    }

    uint emission_idx;
    uint transparency_idx;
    UnpackIorEmissionIdxTransparency(packed_material.ior_emission_idx_transparency,
        material.ior, emission_idx, material.transparency, transparency_idx);

    if (emission_idx != INVALID_TEXTURE_IDX)
    {
        material.emission *= pow(SampleTexture(emission_idx, uv), 2.2f.xxx);
    }

    if (transparency_idx != INVALID_TEXTURE_IDX)
    {
        material.transparency *= SampleTexture(transparency_idx, uv).x;
    }

    return material;
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
    float f0_dielectric = IorToF0(1.0f, material.ior);
    float3 f0 = lerp(f0_dielectric.xxx, material.specular_albedo, material.metalness.xxx);
    float3 diffuse_color = (1.0f - material.metalness) * material.diffuse_albedo;

    float3 fresnel = FresnelSchlick(f0, h_dot_o);
    float specular = GGX_D(alpha, n_dot_h) * V_SmithGGXCorrelated(n_dot_i, n_dot_o, alpha);
    float3 diffuse = diffuse_color * INV_PI;

    return fresnel * specular + (1.0f - fresnel) * diffuse;
}

bool IntersectTriangle(float3 origin, float3 direction, float t_min, inout float t_max,
    RhiTriangle tri, out float2 bc)
{
    float3 p0 = tri.v1.position.xyz;
    float3 p1 = tri.v2.position.xyz;
    float3 p2 = tri.v3.position.xyz;
    float3 e1 = p1 - p0;
    float3 e2 = p2 - p0;
    float3 p = cross(direction, e2);
    float det = dot(e1, p);

    if (abs(det) < 1.0e-8f)
    {
        bc = 0.0f.xx;
        return false;
    }

    float inv_det = 1.0f / det;
    float3 s = origin - p0;
    float u = dot(s, p) * inv_det;
    if (u < 0.0f || u > 1.0f)
    {
        bc = 0.0f.xx;
        return false;
    }

    float3 q = cross(s, e1);
    float v = dot(direction, q) * inv_det;
    if (v < 0.0f || u + v > 1.0f)
    {
        bc = 0.0f.xx;
        return false;
    }

    float t = dot(e2, q) * inv_det;
    if (t < t_min || t > t_max)
    {
        bc = 0.0f.xx;
        return false;
    }

    bc = float2(u, v);
    t_max = t;
    return true;
}

bool IntersectBounds(Bounds3 bounds, float3 ray_origin, float3 ray_inv_dir, float t_min, float t_max)
{
    float3 t0 = (bounds.pmin.xyz - ray_origin) * ray_inv_dir;
    float3 t1 = (bounds.pmax.xyz - ray_origin) * ray_inv_dir;
    float3 tsmaller = min(t0, t1);
    float3 tbigger = max(t0, t1);
    float near_t = max(max(tsmaller.x, tsmaller.y), max(tsmaller.z, t_min));
    float far_t = min(min(tbigger.x, tbigger.y), min(tbigger.z, t_max));
    return far_t >= near_t;
}

Hit TraceBVH(float3 ray_origin, float3 ray_direction, float t_min, float t_max, bool any_hit)
{
    Hit hit;
    hit.bc = 0.0f.xx;
    hit.primitive_id = INVALID_ID;
    hit.t = t_max;

    if (g_SceneCounts.y == 0)
    {
        return hit;
    }

    float3 ray_inv_dir = 1.0f.xxx / ray_direction;
    uint3 ray_sign = ray_inv_dir < 0.0f.xxx;
    uint nodes_to_visit[64];
    uint to_visit_offset = 0;
    uint current_node_index = 0;

    [loop]
    while (true)
    {
        LinearBVHNode node = g_Nodes[current_node_index];
        if (IntersectBounds(node.bounds, ray_origin, ray_inv_dir, t_min, hit.t))
        {
            uint num_primitives = node.num_primitives_axis >> 16;
            if (num_primitives > 0)
            {
                [loop]
                for (uint i = 0; i < num_primitives; ++i)
                {
                    float2 bc;
                    float t = hit.t;
                    if (IntersectTriangle(ray_origin, ray_direction, t_min, t,
                        g_Triangles[node.offset + i], bc))
                    {
                        hit.bc = bc;
                        hit.primitive_id = node.offset + i;
                        hit.t = t;

                        if (any_hit)
                        {
                            return hit;
                        }
                    }
                }

                if (to_visit_offset == 0)
                {
                    break;
                }
                current_node_index = nodes_to_visit[--to_visit_offset];
            }
            else
            {
                uint axis = node.num_primitives_axis & 0xFFFFu;
                bool sign = axis == 0 ? ray_sign.x != 0 : (axis == 1 ? ray_sign.y != 0 : ray_sign.z != 0);
                if (sign)
                {
                    nodes_to_visit[to_visit_offset++] = current_node_index + 1;
                    current_node_index = node.offset;
                }
                else
                {
                    nodes_to_visit[to_visit_offset++] = node.offset;
                    current_node_index = current_node_index + 1;
                }
            }
        }
        else
        {
            if (to_visit_offset == 0)
            {
                break;
            }
            current_node_index = nodes_to_visit[--to_visit_offset];
        }
    }

    return hit;
}

float3 SampleFirstLight(float3 position, out float3 outgoing, out float pdf, out float distance_to_light)
{
    if (g_SceneCounts.z == 0)
    {
        outgoing = 0.0f.xxx;
        pdf = 0.0f;
        distance_to_light = 0.0f;
        return 0.0f.xxx;
    }

    Light light = g_Lights[0];
    pdf = 1.0f;

    if (light.type == LIGHT_TYPE_POINT)
    {
        outgoing = light.origin.xyz - position;
        float sq_length = max(dot(outgoing, outgoing), EPS);
        distance_to_light = sqrt(sq_length);
        return light.radiance.xyz / sq_length;
    }

    outgoing = light.origin.xyz * MAX_RENDER_DIST;
    distance_to_light = MAX_RENDER_DIST;
    return light.radiance.xyz;
}

float3 Tonemap(float3 color)
{
    color = color / (1.0f.xxx + color);
    return pow(saturate(color), 1.0f / 2.2f);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_Output.GetDimensions(width, height);

    if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
    {
        return;
    }

    float2 uv = (float2(dispatch_thread_id.xy) + 0.5f) / float2(width, height);
    float2 ndc = uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    float3 camera_position = g_CameraPositionFov.xyz;
    float3 camera_front = normalize(g_CameraFrontAspect.xyz);
    float3 camera_up = normalize(g_CameraUpPadding.xyz);
    float3 camera_right = normalize(cross(camera_front, camera_up));
    float tan_half_fov = tan(0.5f * g_CameraPositionFov.w);
    float3 ray_direction = normalize(camera_front +
        camera_right * ndc.x * tan_half_fov * g_CameraFrontAspect.w +
        camera_up * ndc.y * tan_half_fov);

    Hit hit = TraceBVH(camera_position, ray_direction, EPS, MAX_RENDER_DIST, false);

    float3 color = float3(0.02f, 0.02f, 0.025f);
    if (hit.primitive_id != INVALID_ID)
    {
        RhiTriangle tri = g_Triangles[hit.primitive_id];
        float3 position = InterpolateAttributes(
            tri.v1.position.xyz, tri.v2.position.xyz, tri.v3.position.xyz, hit.bc);
        float2 texcoord = InterpolateAttributes2(
            tri.v1.texcoord.xy, tri.v2.texcoord.xy, tri.v3.texcoord.xy, hit.bc);
        float3 geometry_normal = normalize(cross(
            tri.v2.position.xyz - tri.v1.position.xyz, tri.v3.position.xyz - tri.v1.position.xyz));
        float3 normal = normalize(InterpolateAttributes(
            tri.v1.normal.xyz, tri.v2.normal.xyz, tri.v3.normal.xyz, hit.bc));

        if (dot(normal, -ray_direction) < 0.0f)
        {
            normal = -normal;
        }
        if (dot(geometry_normal, normal) < 0.0f)
        {
            geometry_normal = -geometry_normal;
        }

        Material material = ApplyTextures(g_Materials[tri.material_index], texcoord);
        float3 incoming = -ray_direction;
        color = material.emission;

        float3 outgoing_to_light;
        float light_pdf;
        float distance_to_light;
        float3 light_radiance = SampleFirstLight(position, outgoing_to_light, light_pdf, distance_to_light);

        if (light_pdf > 0.0f && dot(light_radiance, light_radiance) > 0.0f)
        {
            float3 outgoing = normalize(outgoing_to_light);
            Hit shadow_hit = TraceBVH(position + geometry_normal * EPS, outgoing, EPS,
                distance_to_light - EPS, true);

            if (shadow_hit.primitive_id == INVALID_ID)
            {
                float3 brdf = EvaluateMaterial(material, normal, incoming, outgoing);
                color += light_radiance * brdf * max(dot(outgoing, normal), 0.0f) / light_pdf;
            }
        }
    }

    g_Output[dispatch_thread_id.xy] = float4(Tonemap(color), 1.0f);
}
