#include "common.hlsli"

StructuredBuffer<TextureInfo> g_Textures : register(t24);
StructuredBuffer<uint> g_TextureData : register(t25);

float3 SampleTexture(uint texture_index, float2 uv, uint texture_count)
{
    if (texture_index == INVALID_TEXTURE_IDX || texture_index >= texture_count)
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

Material ApplyTextures(PackedMaterial packed_material, float2 uv, uint texture_count)
{
    Material material;

    uint diffuse_albedo_idx;
    material.diffuse_albedo = UnpackRGBTex(packed_material.diffuse_albedo, diffuse_albedo_idx);
    if (diffuse_albedo_idx != INVALID_TEXTURE_IDX)
    {
        material.diffuse_albedo = pow(SampleTexture(diffuse_albedo_idx, uv, texture_count), 2.2f.xxx);
    }

    uint specular_albedo_idx;
    material.specular_albedo = UnpackRGBTex(packed_material.specular_albedo, specular_albedo_idx);
    if (specular_albedo_idx != INVALID_TEXTURE_IDX)
    {
        material.specular_albedo = pow(SampleTexture(specular_albedo_idx, uv, texture_count), 2.2f.xxx);
    }

    material.emission = UnpackRGBE(packed_material.emission);

    uint roughness_idx;
    uint metalness_idx;
    material.roughness = float((packed_material.roughness_metalness >> 0) & 0xFFu) / 255.0f;
    roughness_idx = (packed_material.roughness_metalness >> 8) & 0xFFu;
    material.metalness = float((packed_material.roughness_metalness >> 16) & 0xFFu) / 255.0f;
    metalness_idx = (packed_material.roughness_metalness >> 24) & 0xFFu;

    if (roughness_idx != INVALID_TEXTURE_IDX)
    {
        material.roughness = SampleTexture(roughness_idx, uv, texture_count).x;
    }
    if (metalness_idx != INVALID_TEXTURE_IDX)
    {
        material.metalness = SampleTexture(metalness_idx, uv, texture_count).x;
    }

    uint emission_idx = (packed_material.ior_emission_idx_transparency >> 8) & 0xFFu;
    uint transparency_idx = (packed_material.ior_emission_idx_transparency >> 24) & 0xFFu;
    material.ior = float((packed_material.ior_emission_idx_transparency >> 0) & 0xFFu) / 25.5f;
    material.transparency =
        float((packed_material.ior_emission_idx_transparency >> 16) & 0xFFu) / 255.0f;

    if (emission_idx != INVALID_TEXTURE_IDX)
    {
        material.emission *= pow(SampleTexture(emission_idx, uv, texture_count), 2.2f.xxx);
    }
    if (transparency_idx != INVALID_TEXTURE_IDX)
    {
        material.transparency *= SampleTexture(transparency_idx, uv, texture_count).x;
    }

    return material;
}
