/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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

#include "frame_data.hlsli"

float3 SampleTexture(uint texture_index, float2 uv, uint texture_count)
{
    if (texture_index == INVALID_TEXTURE_IDX || texture_index >= texture_count)
    {
        return 1.0f.xxx;
    }

    uv = frac(uv);
    uv.y = 1.0f - uv.y;

    return saturate(g_TextureImages[NonUniformResourceIndex(texture_index)].SampleLevel(g_TextureSampler, uv, 0.0f).xyz);
}

Material ApplyTextures(PackedMaterial packed_material, float2 uv, uint texture_count)
{
    Material material;

    uint diffuse_albedo_idx;
    material.diffuse_albedo = UnpackRGBTex(packed_material.diffuse_albedo, diffuse_albedo_idx);
    if (diffuse_albedo_idx != INVALID_TEXTURE_IDX)
    {
        material.diffuse_albedo =
            pow(SampleTexture(diffuse_albedo_idx, uv, texture_count), 2.2f.xxx);
    }

    uint specular_albedo_idx;
    material.specular_albedo = UnpackRGBTex(packed_material.specular_albedo, specular_albedo_idx);
    if (specular_albedo_idx != INVALID_TEXTURE_IDX)
    {
        material.specular_albedo =
            pow(SampleTexture(specular_albedo_idx, uv, texture_count), 2.2f.xxx);
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

float3 SampleDiffuse(float2 s, float3 albedo, float3 normal, out float3 outgoing, out float pdf)
{
    float3 tbn_outgoing = SampleHemisphereCosine(s, pdf);
    outgoing = TangentToWorld(tbn_outgoing, normal);
    return albedo * INV_PI;
}

float3 SampleSpecular(
    float2 s, float alpha, float3 normal, float3 incoming, out float3 outgoing, out float pdf)
{
    if (alpha <= 1.0e-4f)
    {
        outgoing = reflect(-incoming, normal);
        pdf = 1.0f;
        float n_dot_o = max(dot(outgoing, normal), EPS);
        return 1.0f.xxx / n_dot_o;
    }

    float3 wh = GGX_Sample(s, normal, alpha);
    outgoing = reflect(-incoming, wh);

    float n_dot_o = max(dot(normal, outgoing), EPS);
    float n_dot_h = max(dot(normal, wh), EPS);
    float n_dot_i = max(dot(normal, incoming), EPS);
    float wh_dot_o = max(dot(wh, outgoing), EPS);

    float d = GGX_D(alpha, n_dot_h);
    float g = V_SmithGGXCorrelated(n_dot_i, n_dot_o, alpha);
    pdf = d * n_dot_h / (4.0f * wh_dot_o);
    return d.xxx * g;
}

float3 SampleTransparency(float3 incoming, out float3 outgoing, out float pdf)
{
    pdf = 1.0f;
    outgoing = -incoming;
    return 1.0f.xxx;
}

float3 SampleBxdf(float s1, float2 s, Material material, float3 normal, float3 incoming,
    bool white_furnace,
    out float3 outgoing, out float pdf, out float offset)
{
    if (white_furnace)
    {
        material.diffuse_albedo = 1.0f.xxx;
        material.specular_albedo = 1.0f.xxx;
    }

    float alpha = material.roughness * material.roughness;
    float f0_dielectric = IorToF0(1.0f, material.ior);
    float3 f0 = lerp(f0_dielectric.xxx, material.specular_albedo, material.metalness.xxx);
    float3 diffuse_albedo = (1.0f - material.metalness) * material.diffuse_albedo;
    float3 specular_albedo = lerp(material.specular_albedo, 1.0f.xxx, material.metalness.xxx);
    float3 fresnel = FresnelSchlick(f0, dot(normal, incoming)) * specular_albedo;

    float specular_weight = Luma(specular_albedo * fresnel);
    float diffuse_weight = Luma(diffuse_albedo * (1.0f - fresnel));
    float weight_sum = diffuse_weight + specular_weight;
    float specular_sampling_pdf = weight_sum > 0.0f ? specular_weight / weight_sum : 0.0f;
    float diffuse_sampling_pdf = weight_sum > 0.0f ? diffuse_weight / weight_sum : 0.0f;

    offset = 1.0f;
    if (material.transparency < 0.5f)
    {
        offset = -1.0f;
        return SampleTransparency(incoming, outgoing, pdf);
    }

    float3 bxdf;
    if (s1 <= specular_sampling_pdf)
    {
        bxdf = fresnel * SampleSpecular(s, alpha, normal, incoming, outgoing, pdf) *
               max(dot(outgoing, normal), 0.0f);
        pdf *= specular_sampling_pdf;
    }
    else
    {
        bxdf = (1.0f - fresnel) * SampleDiffuse(s, diffuse_albedo, normal, outgoing, pdf) *
               max(dot(outgoing, normal), 0.0f);
        pdf *= diffuse_sampling_pdf;
    }

    return bxdf;
}
