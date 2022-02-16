/*****************************************************************************
 MIT License

 Copyright(c) 2022 Alexander Veselov

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

#include "cl_pt_integrator.hpp"
#include "utils/cl_exception.hpp"
#include "Scene/scene.hpp"
#include "acceleration_structure.hpp"
#include "Utils/blue_noise_sampler.hpp"
#include <GL/glew.h>

namespace args
{
    namespace Raygen
    {
        enum
        {
            kWidth,
            kHeight,
            kCamera,
            kSampleCounterBuffer,
            // Output
            kRayBuffer,
            kRayCounterBuffer,
            kPixelIndicesBuffer,
            kThroughputsBuffer,
            kDiffuseAlbedo,
            kDepth,
            kNormal,
            kVelocity,
        };
    }

    namespace Miss
    {
        enum
        {
            kRayBuffer,
            kRayCounterBuffer,
            kHitsBuffer,
            kPixelIndicesBuffer,
            kThroughputsBuffer,
            kIblTextureBuffer,
            kRadianceBuffer,
        };
    }

    namespace Aov
    {
        enum
        {
            // Input
            kRayBuffer,
            kRayCounterBuffer,
            kPixelIndicesBuffer,
            kHitsBuffer,
            kTrianglesBuffer,
            kMaterialsBuffer,
            kTexturesBuffer,
            kTextureDataBuffer,
            kWidth,
            kHeight,
            kCamera,
            kPrevCamera,
            // Output
            kDiffuseAlbedo,
            kDepth,
            kNormal,
            kVelocity,
        };
    }

    namespace HitSurface
    {
        enum
        {
            // Input
            kIncomingRayBuffer,
            kIncomingRayCounterBuffer,
            kIncomingPixelIndicesBuffer,
            kHitsBuffer,
            kTrianglesBuffer,
            kAnalyticLightsBuffer,
            kEmissiveIndicesBuffer,
            kMaterialsBuffer,
            kTexturesBuffer,
            kTextureDataBuffer,
            kBounce,
            kWidth,
            kHeight,
            kSampleCounterBuffer,
            kSceneInfo,
            kSobolBuffer,
            kScramblingTileBuffer,
            kRankingTileBuffer,
            // Output
            kThroughputsBuffer,
            kOutgoingRayBuffer,
            kOutgoingRayCounterBuffer,
            kOutgoingPixelIndicesBuffer,
            kShadowRayBuffer,
            kShadowRayCounterBuffer,
            kShadowPixelIndicesBuffer,
            kDirectLightSamplesBuffer,
            kRadianceBuffer,
        };
    }

    namespace AccumulateDirectSamples
    {
        enum
        {
            // Input
            kShadowHitsBuffer,
            kShadowRayCounterBuffer,
            kShadowPixelIndicesBuffer,
            kDirectLightSamplesBuffer,
            // Output
            kRadianceBuffer,
        };
    }

    namespace TemporalAccumulation
    {
        enum
        {
            kWidth,
            kHeight,
            kRadiance,
            kPrevRadiance,
            kDepth,
            kPrevDepth,
            kMotionVectors
        };
    }

    namespace Resolve
    {
        enum
        {
            // Input
            kWidth,
            kHeight,
            kAovIndex,
            kRadianceBuffer,
            kDiffuseAlbedo,
            kDepth,
            kNormal,
            kMotionVectors,
            kSampleCounterBuffer,
            // Output
            kResolvedTexture,
        };
    }
}

cl::Buffer CLPathTraceIntegrator::CreateBuffer(std::size_t size)
{
    cl_int status;
    cl::Buffer buffer(cl_context_.GetContext(), CL_MEM_READ_WRITE,
        size, nullptr, &status);
    ThrowIfFailed(status, "Failed to create the buffer");

    return buffer;
}

CLPathTraceIntegrator::CLPathTraceIntegrator(std::uint32_t width, std::uint32_t height,
    AccelerationStructure& acc_structure, CLContext& cl_context, unsigned int output_image)
    : Integrator(width, height, acc_structure)
    , cl_context_(cl_context)
    , gl_interop_image_(output_image)
{
    std::uint32_t num_rays = width_ * height_;

    // Create buffers and images
    cl_int status;

    radiance_buffer_ = CreateBuffer(num_rays * sizeof(cl_float4));

    // if (enable_denoiser_)
    {
        prev_radiance_buffer_ = CreateBuffer(num_rays * sizeof(cl_float4));
    }

    for (int i = 0; i < 2; ++i)
    {
        rays_buffer_[i] = CreateBuffer(num_rays * sizeof(Ray));
        pixel_indices_buffer_[i] = CreateBuffer(num_rays * sizeof(std::uint32_t));
        ray_counter_buffer_[i] = CreateBuffer(sizeof(std::uint32_t));
    }

    shadow_rays_buffer_ = CreateBuffer(num_rays * sizeof(Ray));
    shadow_pixel_indices_buffer_ = CreateBuffer(num_rays * sizeof(std::uint32_t));
    shadow_ray_counter_buffer_ = CreateBuffer(sizeof(std::uint32_t));
    hits_buffer_ = CreateBuffer(num_rays * sizeof(Hit));
    shadow_hits_buffer_ = CreateBuffer(num_rays * sizeof(std::uint32_t));
    throughputs_buffer_ = CreateBuffer(num_rays * sizeof(cl_float3));
    sample_counter_buffer_ = CreateBuffer(sizeof(std::uint32_t));
    direct_light_samples_buffer_ = CreateBuffer(num_rays * sizeof(cl_float4));

    // Sampler buffers
    {
        sampler_sobol_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(sobol_256spp_256d), (void*)sobol_256spp_256d, &status);
        ThrowIfFailed(status, "Failed to create sobol buffer");

        sampler_scrambling_tile_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(scramblingTile), (void*)scramblingTile, &status);
        ThrowIfFailed(status, "Failed to create scrambling tile buffer");

        sampler_ranking_tile_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(rankingTile), (void*)rankingTile, &status);
        ThrowIfFailed(status, "Failed to create ranking tile tile buffer");
    }

    // AOV buffers
    {
        diffuse_albedo_buffer_ = CreateBuffer(num_rays * sizeof(cl_float3));
        depth_buffer_ = CreateBuffer(num_rays * sizeof(cl_float));

        // if (enable_denoiser_)
        {
            prev_depth_buffer_ = CreateBuffer(num_rays * sizeof(cl_float));
        }

        normal_buffer_ = CreateBuffer(num_rays * sizeof(cl_float3));
        velocity_buffer_ = CreateBuffer(num_rays * sizeof(cl_float2));
    }

    output_image_ = std::make_unique<cl::ImageGL>(cl_context.GetContext(), CL_MEM_WRITE_ONLY,
        GL_TEXTURE_2D, 0, gl_interop_image_, &status);
    ThrowIfFailed(status, "Failed to create output image");

    CreateKernels();

    // Don't forget to reset frame index
    Reset();
}

void CLPathTraceIntegrator::CreateKernels()
{
    // Create kernels
    reset_kernel_ = cl_context_.CreateKernel("src/kernels/cl/reset_radiance.cl", "ResetRadiance");
    raygen_kernel_ = cl_context_.CreateKernel("src/kernels/cl/raygeneration.cl", "RayGeneration");

    std::vector<std::string> definitions;
    if (enable_white_furnace_)
    {
        definitions.push_back("ENABLE_WHITE_FURNACE");
    }

    if (sampler_type_ == SamplerType::kBlueNoise)
    {
        definitions.push_back("BLUE_NOISE_SAMPLER");
    }

    if (enable_denoiser_)
    {
        definitions.push_back("ENABLE_DENOISER");
    }

    miss_kernel_ = cl_context_.CreateKernel("src/kernels/cl/miss.cl", "Miss", definitions);
    aov_kernel_ = cl_context_.CreateKernel("src/kernels/cl/aov.cl", "GenerateAOV");
    hit_surface_kernel_ = cl_context_.CreateKernel("src/kernels/cl/hit_surface.cl", "HitSurface", definitions);
    accumulate_direct_samples_kernel_ = cl_context_.CreateKernel("src/kernels/cl/accumulate_direct_samples.cl", "AccumulateDirectSamples", definitions);
    clear_counter_kernel_ = cl_context_.CreateKernel("src/kernels/cl/clear_counter.cl", "ClearCounter");
    increment_counter_kernel_ = cl_context_.CreateKernel("src/kernels/cl/increment_counter.cl", "IncrementCounter");
    resolve_kernel_ = cl_context_.CreateKernel("src/kernels/cl/resolve_radiance.cl", "ResolveRadiance", definitions);

    if (enable_denoiser_)
    {
        temporal_accumulation_kernel_ = cl_context_.CreateKernel("src/kernels/cl/denoiser.cl", "TemporalAccumulation");
    }

    intersect_kernel_ = cl_context_.CreateKernel("src/kernels/cl/trace_bvh.cl", "TraceBvh");
    intersect_shadow_kernel_ = cl_context_.CreateKernel("src/kernels/cl/trace_bvh.cl", "TraceBvh", { "SHADOW_RAYS" });

    // Setup kernels
    cl_mem output_image_mem = (*output_image_)();

    // Setup reset kernel
    reset_kernel_->SetArgument(0, &width_, sizeof(width_));
    reset_kernel_->SetArgument(1, &height_, sizeof(height_));
    reset_kernel_->SetArgument(2, radiance_buffer_);

    // Setup raygen kernel
    raygen_kernel_->SetArgument(args::Raygen::kWidth, &width_, sizeof(width_));
    raygen_kernel_->SetArgument(args::Raygen::kHeight, &height_, sizeof(height_));
    raygen_kernel_->SetArgument(args::Raygen::kSampleCounterBuffer, sample_counter_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kRayBuffer, rays_buffer_[0]);
    raygen_kernel_->SetArgument(args::Raygen::kRayCounterBuffer, ray_counter_buffer_[0]);
    raygen_kernel_->SetArgument(args::Raygen::kPixelIndicesBuffer, pixel_indices_buffer_[0]);
    raygen_kernel_->SetArgument(args::Raygen::kThroughputsBuffer, throughputs_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kDiffuseAlbedo, diffuse_albedo_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kDepth, depth_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kNormal, normal_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kVelocity, velocity_buffer_);

    // Setup miss kernel
    miss_kernel_->SetArgument(args::Miss::kHitsBuffer, hits_buffer_);
    miss_kernel_->SetArgument(args::Miss::kThroughputsBuffer, throughputs_buffer_);
    miss_kernel_->SetArgument(args::Miss::kRadianceBuffer, radiance_buffer_);

    // Setup hit surface kernel

    // Setup accumulate direct samples kernel
    accumulate_direct_samples_kernel_->SetArgument(args::AccumulateDirectSamples::kShadowHitsBuffer,
        shadow_hits_buffer_);
    accumulate_direct_samples_kernel_->SetArgument(args::AccumulateDirectSamples::kShadowRayCounterBuffer,
        shadow_ray_counter_buffer_);
    accumulate_direct_samples_kernel_->SetArgument(args::AccumulateDirectSamples::kShadowPixelIndicesBuffer,
        shadow_pixel_indices_buffer_);
    accumulate_direct_samples_kernel_->SetArgument(args::AccumulateDirectSamples::kDirectLightSamplesBuffer,
        direct_light_samples_buffer_);
    accumulate_direct_samples_kernel_->SetArgument(args::AccumulateDirectSamples::kRadianceBuffer,
        radiance_buffer_);

    // Setup resolve kernel
    resolve_kernel_->SetArgument(args::Resolve::kWidth, &width_, sizeof(width_));
    resolve_kernel_->SetArgument(args::Resolve::kHeight, &height_, sizeof(height_));
    resolve_kernel_->SetArgument(args::Resolve::kSampleCounterBuffer, sample_counter_buffer_);
    resolve_kernel_->SetArgument(args::Resolve::kRadianceBuffer, radiance_buffer_);
    resolve_kernel_->SetArgument(args::Resolve::kDiffuseAlbedo, diffuse_albedo_buffer_);
    resolve_kernel_->SetArgument(args::Resolve::kDepth, depth_buffer_);
    resolve_kernel_->SetArgument(args::Resolve::kNormal, normal_buffer_);
    resolve_kernel_->SetArgument(args::Resolve::kMotionVectors, velocity_buffer_);
    resolve_kernel_->SetArgument(args::Resolve::kResolvedTexture, output_image_mem);
    std::uint32_t aov_index = aov_;
    resolve_kernel_->SetArgument(args::Resolve::kAovIndex, &aov_index, sizeof(aov_index));

    if (enable_denoiser_)
    {
        // Setup temporal accumulation kernel
        temporal_accumulation_kernel_->SetArgument(args::TemporalAccumulation::kWidth, &width_, sizeof(width_));
        temporal_accumulation_kernel_->SetArgument(args::TemporalAccumulation::kHeight, &height_, sizeof(height_));
        temporal_accumulation_kernel_->SetArgument(args::TemporalAccumulation::kRadiance, radiance_buffer_);
        temporal_accumulation_kernel_->SetArgument(args::TemporalAccumulation::kPrevRadiance, prev_radiance_buffer_);
        temporal_accumulation_kernel_->SetArgument(args::TemporalAccumulation::kDepth, depth_buffer_);
        temporal_accumulation_kernel_->SetArgument(args::TemporalAccumulation::kPrevDepth, prev_depth_buffer_);
        temporal_accumulation_kernel_->SetArgument(args::TemporalAccumulation::kMotionVectors, velocity_buffer_);
    }
}

void CLPathTraceIntegrator::SetCameraData(Camera const& camera)
{
    raygen_kernel_->SetArgument(args::Raygen::kCamera, &camera, sizeof(camera));
    aov_kernel_->SetArgument(args::Aov::kCamera, &camera, sizeof(camera));
    aov_kernel_->SetArgument(args::Aov::kPrevCamera, &prev_camera_, sizeof(prev_camera_));
    prev_camera_ = camera;
}

void CLPathTraceIntegrator::UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure)
{
    // Create scene buffers
    auto const& triangles = scene.GetTriangles();
    auto const& materials = scene.GetMaterials();
    auto const& emissive_indices = scene.GetEmissiveIndices();
    auto const& lights = scene.GetLights();
    auto const& textures = scene.GetTextures();
    auto const& texture_data = scene.GetTextureData();
    auto const& env_image = scene.GetEnvImage();

    cl_int status;

    assert(!triangles.empty());
    triangle_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        triangles.size() * sizeof(Triangle), (void*)triangles.data(), &status);
    ThrowIfFailed(status, "Failed to create triangle buffer");

    // Additional compressed triangle buffer
    {
        std::vector<RTTriangle> rt_triangles;
        for (auto const& triangle : triangles)
        {
            rt_triangles.emplace_back(triangle.v1.position, triangle.v2.position, triangle.v3.position);
        }

        rt_triangle_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            rt_triangles.size() * sizeof(RTTriangle), (void*)rt_triangles.data(), &status);
        ThrowIfFailed(status, "Failed to create rt triangle buffer");
    }

    assert(!materials.empty());
    material_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        materials.size() * sizeof(PackedMaterial), (void*)materials.data(), &status);
    ThrowIfFailed(status, "Failed to create material buffer");

    if (!emissive_indices.empty())
    {
        emissive_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            emissive_indices.size() * sizeof(std::uint32_t), (void*)emissive_indices.data(), &status);
        ThrowIfFailed(status, "Failed to create emissive buffer");
    }

    if (!lights.empty())
    {
        analytic_light_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            lights.size() * sizeof(Light), (void*)lights.data(), &status);
        ThrowIfFailed(status, "Failed to create analytic light buffer");
    }

    if (!textures.empty())
    {
        texture_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            textures.size() * sizeof(Texture), (void*)textures.data(), &status);
        ThrowIfFailed(status, "Failed to create texture buffer");
    }

    if (!texture_data.empty())
    {
        texture_data_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            texture_data.size() * sizeof(std::uint32_t), (void*)texture_data.data(), &status);
        ThrowIfFailed(status, "Failed to create texture data buffer");
    }

    cl::ImageFormat image_format;
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_FLOAT;

    env_texture_ = cl::Image2D(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        image_format, env_image.width, env_image.height, 0, (void*)env_image.data.data(), &status);
    ThrowIfFailed(status, "Failed to create environment image");

    scene_info_ = scene.GetSceneInfo();

    auto const& nodes = acc_structure_.GetNodes();

    // Upload BVH data
    nodes_buffer_ = cl::Buffer(cl_context_.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        nodes.size() * sizeof(LinearBVHNode), (void*)nodes.data(), &status);
    if (status != CL_SUCCESS)
    {
        throw CLException("Failed to create BVH node buffer", status);
    }
}

void CLPathTraceIntegrator::SetSamplerType(SamplerType sampler_type)
{
    if (sampler_type == sampler_type_)
    {
        return;
    }

    sampler_type_ = sampler_type;
    CreateKernels();
    RequestReset();
}

void CLPathTraceIntegrator::SetAOV(AOV aov)
{
    if (aov == aov_)
    {
        return;
    }

    aov_ = aov;

    std::uint32_t aov_idx = aov;
    resolve_kernel_->SetArgument(args::Resolve::kAovIndex, &aov_idx, sizeof(aov_idx));

    RequestReset();
}

void CLPathTraceIntegrator::EnableDenoiser(bool enable_denoiser)
{
    if (enable_denoiser == enable_denoiser_)
    {
        return;
    }

    enable_denoiser_ = enable_denoiser;
    CreateKernels();
    RequestReset();
}

void CLPathTraceIntegrator::Reset()
{
    if (!enable_denoiser_)
    {
        // Reset frame index
        clear_counter_kernel_->SetArgument(0, sample_counter_buffer_);
        cl_context_.ExecuteKernel(*clear_counter_kernel_, 1);
    }

    // Reset radiance buffer
    cl_context_.ExecuteKernel(*reset_kernel_, width_ * height_);
}

void CLPathTraceIntegrator::AdvanceSampleCount()
{
    increment_counter_kernel_->SetArgument(0, sample_counter_buffer_);
    cl_context_.ExecuteKernel(*increment_counter_kernel_, 1);
}

void CLPathTraceIntegrator::GenerateRays()
{
    std::uint32_t num_rays = width_ * height_;
    cl_context_.ExecuteKernel(*raygen_kernel_, num_rays);
}

void CLPathTraceIntegrator::IntersectRays(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;
    std::uint32_t incoming_idx = bounce & 1;

    CLKernel& kernel = *intersect_kernel_;
    kernel.SetArgument(0, rays_buffer_[incoming_idx]);
    kernel.SetArgument(1, ray_counter_buffer_[incoming_idx]);
    kernel.SetArgument(2, rt_triangle_buffer_);
    kernel.SetArgument(3, nodes_buffer_);
    kernel.SetArgument(4, hits_buffer_);

    ///@TODO: use indirect dispatch
    cl_context_.ExecuteKernel(kernel, max_num_rays);

    //acc_structure_.IntersectRays(rays_buffer_[incoming_idx], ray_counter_buffer_[incoming_idx],
    //    max_num_rays, hits_buffer_);
}

void CLPathTraceIntegrator::ComputeAOVs()
{
    std::uint32_t max_num_rays = width_ * height_;

    // Setup AOV kernel
    aov_kernel_->SetArgument(args::Aov::kRayBuffer, rays_buffer_[0]);
    aov_kernel_->SetArgument(args::Aov::kRayCounterBuffer, ray_counter_buffer_[0]);
    aov_kernel_->SetArgument(args::Aov::kPixelIndicesBuffer, pixel_indices_buffer_[0]);
    aov_kernel_->SetArgument(args::Aov::kHitsBuffer, hits_buffer_);
    aov_kernel_->SetArgument(args::Aov::kTrianglesBuffer, triangle_buffer_);
    aov_kernel_->SetArgument(args::Aov::kMaterialsBuffer, material_buffer_);
    aov_kernel_->SetArgument(args::Aov::kTexturesBuffer, texture_buffer_);
    aov_kernel_->SetArgument(args::Aov::kTextureDataBuffer, texture_data_buffer_);
    aov_kernel_->SetArgument(args::Aov::kWidth, &width_, sizeof(width_));
    aov_kernel_->SetArgument(args::Aov::kHeight, &height_, sizeof(height_));
    aov_kernel_->SetArgument(args::Aov::kDiffuseAlbedo, diffuse_albedo_buffer_);
    aov_kernel_->SetArgument(args::Aov::kDepth, depth_buffer_);
    aov_kernel_->SetArgument(args::Aov::kNormal, normal_buffer_);
    aov_kernel_->SetArgument(args::Aov::kVelocity, velocity_buffer_);

    cl_context_.ExecuteKernel(*aov_kernel_, max_num_rays);
}

void CLPathTraceIntegrator::IntersectShadowRays()
{
    std::uint32_t max_num_rays = width_ * height_;

    CLKernel& kernel = *intersect_shadow_kernel_;
    kernel.SetArgument(0, shadow_rays_buffer_);
    kernel.SetArgument(1, shadow_ray_counter_buffer_);
    kernel.SetArgument(2, rt_triangle_buffer_);
    kernel.SetArgument(3, nodes_buffer_);
    kernel.SetArgument(4, shadow_hits_buffer_);

    ///@TODO: use indirect dispatch
    cl_context_.ExecuteKernel(kernel, max_num_rays);

    //acc_structure_.IntersectRays(shadow_rays_buffer_, shadow_ray_counter_buffer_,
    //    max_num_rays, shadow_hits_buffer_, false);
}

void CLPathTraceIntegrator::ShadeMissedRays(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;
    std::uint32_t incoming_idx = bounce & 1;

    miss_kernel_->SetArgument(args::Miss::kRayBuffer, rays_buffer_[incoming_idx]);
    miss_kernel_->SetArgument(args::Miss::kPixelIndicesBuffer, pixel_indices_buffer_[incoming_idx]);
    miss_kernel_->SetArgument(args::Miss::kRayCounterBuffer, ray_counter_buffer_[incoming_idx]);
    miss_kernel_->SetArgument(args::Miss::kIblTextureBuffer, env_texture_());
    cl_context_.ExecuteKernel(*miss_kernel_, max_num_rays);
}

void CLPathTraceIntegrator::ShadeSurfaceHits(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;

    std::uint32_t incoming_idx = bounce & 1;
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    // Incoming rays
    hit_surface_kernel_->SetArgument(args::HitSurface::kIncomingRayBuffer, rays_buffer_[incoming_idx]);
    hit_surface_kernel_->SetArgument(args::HitSurface::kIncomingPixelIndicesBuffer, pixel_indices_buffer_[incoming_idx]);
    hit_surface_kernel_->SetArgument(args::HitSurface::kIncomingRayCounterBuffer,ray_counter_buffer_[incoming_idx]);

    hit_surface_kernel_->SetArgument(args::HitSurface::kHitsBuffer, hits_buffer_);

    hit_surface_kernel_->SetArgument(args::HitSurface::kTrianglesBuffer, triangle_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kAnalyticLightsBuffer, analytic_light_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kEmissiveIndicesBuffer, emissive_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kMaterialsBuffer, material_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kTexturesBuffer, texture_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kTextureDataBuffer, texture_data_buffer_);

    hit_surface_kernel_->SetArgument(args::HitSurface::kBounce, &bounce, sizeof(bounce));
    hit_surface_kernel_->SetArgument(args::HitSurface::kWidth, &width_, sizeof(width_));
    hit_surface_kernel_->SetArgument(args::HitSurface::kHeight, &height_, sizeof(height_));

    hit_surface_kernel_->SetArgument(args::HitSurface::kSampleCounterBuffer, sample_counter_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kSceneInfo, &scene_info_, sizeof(scene_info_));

    hit_surface_kernel_->SetArgument(args::HitSurface::kSobolBuffer, sampler_sobol_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kScramblingTileBuffer, sampler_scrambling_tile_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kRankingTileBuffer, sampler_ranking_tile_buffer_);

    hit_surface_kernel_->SetArgument(args::HitSurface::kThroughputsBuffer, throughputs_buffer_);

    // Outgoing rays
    hit_surface_kernel_->SetArgument(args::HitSurface::kOutgoingRayBuffer, rays_buffer_[outgoing_idx]);
    hit_surface_kernel_->SetArgument(args::HitSurface::kOutgoingPixelIndicesBuffer, pixel_indices_buffer_[outgoing_idx]);
    hit_surface_kernel_->SetArgument(args::HitSurface::kOutgoingRayCounterBuffer, ray_counter_buffer_[outgoing_idx]);

    // Shadow
    hit_surface_kernel_->SetArgument(args::HitSurface::kShadowRayBuffer, shadow_rays_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kShadowRayCounterBuffer, shadow_ray_counter_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kShadowPixelIndicesBuffer, shadow_pixel_indices_buffer_);
    hit_surface_kernel_->SetArgument(args::HitSurface::kDirectLightSamplesBuffer, direct_light_samples_buffer_);

    // Output radiance
    hit_surface_kernel_->SetArgument(args::HitSurface::kRadianceBuffer, radiance_buffer_);

    cl_context_.ExecuteKernel(*hit_surface_kernel_, max_num_rays);
}

void CLPathTraceIntegrator::AccumulateDirectSamples()
{
    std::uint32_t max_num_rays = width_ * height_;
    cl_context_.ExecuteKernel(*accumulate_direct_samples_kernel_, max_num_rays);
}

void CLPathTraceIntegrator::ClearOutgoingRayCounter(std::uint32_t bounce)
{
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    clear_counter_kernel_->SetArgument(0, ray_counter_buffer_[outgoing_idx]);
    cl_context_.ExecuteKernel(*clear_counter_kernel_, 1);
}

void CLPathTraceIntegrator::ClearShadowRayCounter()
{
    clear_counter_kernel_->SetArgument(0, shadow_ray_counter_buffer_);
    cl_context_.ExecuteKernel(*clear_counter_kernel_, 1);
}

void CLPathTraceIntegrator::Denoise()
{
    cl_context_.ExecuteKernel(*temporal_accumulation_kernel_, width_ * height_);
}

void CLPathTraceIntegrator::CopyHistoryBuffers()
{
    // Copy to the history
    cl_context_.CopyBuffer(radiance_buffer_, prev_radiance_buffer_, 0, 0, width_ * height_ * sizeof(cl_float4));
    cl_context_.CopyBuffer(depth_buffer_, prev_depth_buffer_, 0, 0, width_ * height_ * sizeof(cl_float));
}

void CLPathTraceIntegrator::ResolveRadiance()
{
    // Copy radiance to the interop image
    cl_context_.AcquireGLObject((*output_image_)());
    cl_context_.ExecuteKernel(*resolve_kernel_, width_ * height_);
    cl_context_.Finish();
    cl_context_.ReleaseGLObject((*output_image_)());
}
