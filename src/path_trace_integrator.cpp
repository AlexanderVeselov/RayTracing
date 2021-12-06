/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

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

#include "path_trace_integrator.hpp"
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
            // AOVs
            kDiffuseAlbedoBuffer,
            kDepthBuffer,
            kNormalBuffer,
            kVelocityBuffer,
            // Demodulation
            kDirectLightingBuffer,
            kIndirectDiffuseBuffer,
            kIndirectSpecularBuffer,
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
            kDiffuseThroughputTerm,
            kSpecularThroughputTerm,
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

    namespace TemporalFilter
    {
        enum
        {
            kWidth,
            kHeight,
            kRadiance,
            kPrevRadiance,
            kAccumulatedRadiance,
            kDepth,
            kPrevDepth,
            kMotionVectors
        };
    }

    namespace SpatialFilter
    {
        enum
        {
            kWidth,
            kHeight,
            kInputRadiance,
            kOutputRadiance,
            kDepth,
            kNormals,
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
            // Modulation
            kDirectLightingBuffer,
        };
    }
}

PathTraceIntegrator::PathTraceIntegrator(std::uint32_t width, std::uint32_t height,
    CLContext& cl_context, AccelerationStructure& acc_structure, cl_GLuint gl_interop_image)
    : width_(width)
    , height_(height)
    , cl_context_(cl_context)
    , acc_structure_(acc_structure)
    , gl_interop_image_(gl_interop_image)
{
    std::uint32_t num_rays = width_ * height_;

    // Create buffers and images
    cl_int status;

    radiance_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        width_ * height_ * sizeof(cl_float4), nullptr, &status);
    ThrowIfFailed(status, "Failed to create radiance buffer");

    for (int i = 0; i < 2; ++i)
    {
        rays_buffer_[i] = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            num_rays * sizeof(Ray), nullptr, &status);
        ThrowIfFailed(status, "Failed to create ray buffer");

        pixel_indices_buffer_[i] = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            num_rays * sizeof(std::uint32_t), nullptr, &status);
        ThrowIfFailed(status, "Failed to create pixel indices buffer");

        ray_counter_buffer_[i] = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            sizeof(std::uint32_t), nullptr, &status);
        ThrowIfFailed(status, "Failed to create ray counter buffer");
    }

    shadow_rays_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        num_rays * sizeof(Ray), nullptr, &status);
    ThrowIfFailed(status, "Failed to create shadow ray buffer");

    shadow_pixel_indices_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        num_rays * sizeof(std::uint32_t), nullptr, &status);
    ThrowIfFailed(status, "Failed to create shadow pixel indices buffer");

    shadow_ray_counter_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        sizeof(std::uint32_t), nullptr, &status);
    ThrowIfFailed(status, "Failed to create shadow ray counter buffer");

    hits_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        num_rays * sizeof(Hit), nullptr, &status);
    ThrowIfFailed(status, "Failed to create hits buffer");

    shadow_hits_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        num_rays * sizeof(std::uint32_t), nullptr, &status);
    ThrowIfFailed(status, "Failed to create shadow hits buffer");

    throughputs_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        num_rays * sizeof(cl_float3), nullptr, &status);
    ThrowIfFailed(status, "Failed to create throughputs buffer");

    sample_counter_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        sizeof(std::uint32_t), nullptr, &status);
    ThrowIfFailed(status, "Failed to create sample counter buffer");

    direct_light_samples_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
        width_ * height_ * sizeof(cl_float4), nullptr, &status);
    ThrowIfFailed(status, "Failed to create direct light samples buffer");

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
        // if (enable_demodulation_)
        {
            direct_lighting_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
                width_ * height_ * sizeof(cl_float3), nullptr, &status);
            ThrowIfFailed(status, "Failed to create direct lighting buffer");

            indirect_diffuse_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
                width_ * height_ * sizeof(cl_float3), nullptr, &status);
            ThrowIfFailed(status, "Failed to create indirect diffuse buffer");

            indirect_specular_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
                width_ * height_ * sizeof(cl_float3), nullptr, &status);
            ThrowIfFailed(status, "Failed to create indirect specular buffer");
        }

        diffuse_albedo_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            width_ * height_ * sizeof(cl_float3), nullptr, &status);
        ThrowIfFailed(status, "Failed to create diffuse albedo buffer");

        depth_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            width_ * height_ * sizeof(cl_float), nullptr, &status);
        ThrowIfFailed(status, "Failed to create depth buffer");

        normal_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            width_ * height_ * sizeof(cl_float3), nullptr, &status);
        ThrowIfFailed(status, "Failed to create normal buffer");

        velocity_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            width_ * height_ * sizeof(cl_float2), nullptr, &status);
        ThrowIfFailed(status, "Failed to create velocity buffer");
    }

    // if (enable_denoiser_)
    {
        prev_radiance_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            width_ * height_ * sizeof(cl_float4), nullptr, &status);
        ThrowIfFailed(status, "Failed to create prev radiance buffer");

        prev_depth_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            width_ * height_ * sizeof(cl_float), nullptr, &status);
        ThrowIfFailed(status, "Failed to create prev depth buffer");

        accumulated_radiance_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_WRITE,
            width_ * height_ * sizeof(cl_float4), nullptr, &status);
        ThrowIfFailed(status, "Failed to create accumulated radiance buffer");
    }

    output_image_ = std::make_unique<cl::ImageGL>(cl_context.GetContext(), CL_MEM_WRITE_ONLY,
        GL_TEXTURE_2D, 0, gl_interop_image_, &status);
    ThrowIfFailed(status, "Failed to create output image");

    CreateKernels();

    // Don't forget to reset frame index
    Reset();
}

void PathTraceIntegrator::CreateKernels()
{
    // Fill definitions
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

    if (enable_demodulation_)
    {
        definitions.push_back("ENABLE_DEMODULATION");
    }

    // Create kernels
    reset_kernel_ = cl_context_.CreateKernel("src/Kernels/reset_radiance.cl", "ResetRadiance");
    raygen_kernel_ = cl_context_.CreateKernel("src/Kernels/raygeneration.cl", "RayGeneration", definitions);
    miss_kernel_ = cl_context_.CreateKernel("src/Kernels/miss.cl", "Miss", definitions);
    aov_kernel_ = cl_context_.CreateKernel("src/Kernels/aov.cl", "GenerateAOV");
    hit_surface_kernel_ = cl_context_.CreateKernel("src/Kernels/hit_surface.cl", "HitSurface", definitions);
    accumulate_direct_samples_kernel_ = cl_context_.CreateKernel("src/Kernels/accumulate_direct_samples.cl", "AccumulateDirectSamples", definitions);
    clear_counter_kernel_ = cl_context_.CreateKernel("src/Kernels/clear_counter.cl", "ClearCounter");
    increment_counter_kernel_ = cl_context_.CreateKernel("src/Kernels/increment_counter.cl", "IncrementCounter");
    resolve_kernel_ = cl_context_.CreateKernel("src/Kernels/resolve_radiance.cl", "ResolveRadiance", definitions);

    if (enable_denoiser_)
    {
        temporal_filter_kernel_ = cl_context_.CreateKernel("src/Kernels/temporal_filter.cl", "TemporalFilter");
        spatial_filter_kernel_ = cl_context_.CreateKernel("src/Kernels/spatial_filter.cl", "SpatialFilter");
    }

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
    raygen_kernel_->SetArgument(args::Raygen::kDiffuseAlbedoBuffer, diffuse_albedo_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kDepthBuffer, depth_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kNormalBuffer, normal_buffer_);
    raygen_kernel_->SetArgument(args::Raygen::kVelocityBuffer, velocity_buffer_);

    if (enable_demodulation_)
    {
        raygen_kernel_->SetArgument(args::Raygen::kDirectLightingBuffer, direct_lighting_buffer_);
        raygen_kernel_->SetArgument(args::Raygen::kIndirectDiffuseBuffer, indirect_diffuse_buffer_);
        raygen_kernel_->SetArgument(args::Raygen::kIndirectSpecularBuffer, indirect_specular_buffer_);
    }

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
        enable_demodulation_ ? direct_lighting_buffer_ : radiance_buffer_);

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

    if (enable_demodulation_)
    {
        resolve_kernel_->SetArgument(args::Resolve::kDirectLightingBuffer, direct_lighting_buffer_);
    }

    if (enable_denoiser_)
    {
        // Setup temporal filter kernel
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kWidth, &width_, sizeof(width_));
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kHeight, &height_, sizeof(height_));
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kRadiance, radiance_buffer_);
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kPrevRadiance, prev_radiance_buffer_);
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kAccumulatedRadiance, accumulated_radiance_buffer_);
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kDepth, depth_buffer_);
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kPrevDepth, prev_depth_buffer_);
        temporal_filter_kernel_->SetArgument(args::TemporalFilter::kMotionVectors, velocity_buffer_);

        // Setup spatial filter kernel
        spatial_filter_kernel_->SetArgument(args::SpatialFilter::kWidth, &width_, sizeof(width_));
        spatial_filter_kernel_->SetArgument(args::SpatialFilter::kHeight, &height_, sizeof(height_));
        spatial_filter_kernel_->SetArgument(args::SpatialFilter::kInputRadiance, accumulated_radiance_buffer_);
        spatial_filter_kernel_->SetArgument(args::SpatialFilter::kOutputRadiance, radiance_buffer_);
        spatial_filter_kernel_->SetArgument(args::SpatialFilter::kDepth, depth_buffer_);
        spatial_filter_kernel_->SetArgument(args::SpatialFilter::kNormals, normal_buffer_);
    }

}

void PathTraceIntegrator::EnableWhiteFurnace(bool enable)
{
    if (enable == enable_white_furnace_)
    {
        return;
    }

    enable_white_furnace_ = enable;
    CreateKernels();
    RequestReset();
}

void PathTraceIntegrator::SetCameraData(Camera const& camera)
{
    raygen_kernel_->SetArgument(args::Raygen::kCamera, &camera, sizeof(camera));
    aov_kernel_->SetArgument(args::Aov::kCamera, &camera, sizeof(camera));
    aov_kernel_->SetArgument(args::Aov::kPrevCamera, &prev_camera_, sizeof(prev_camera_));
    prev_camera_ = camera;
}

void PathTraceIntegrator::SetSceneData(Scene const& scene)
{
    // Set scene buffers
    SceneInfo scene_info = scene.GetSceneInfo();

    hit_surface_kernel_->SetArgument(args::HitSurface::kTrianglesBuffer, scene.GetTriangleBuffer());
    hit_surface_kernel_->SetArgument(args::HitSurface::kAnalyticLightsBuffer, scene.GetAnalyticLightBuffer());
    hit_surface_kernel_->SetArgument(args::HitSurface::kEmissiveIndicesBuffer, scene.GetEmissiveIndicesBuffer());
    hit_surface_kernel_->SetArgument(args::HitSurface::kMaterialsBuffer, scene.GetMaterialBuffer());
    hit_surface_kernel_->SetArgument(args::HitSurface::kTexturesBuffer, scene.GetTextureBuffer());
    hit_surface_kernel_->SetArgument(args::HitSurface::kTextureDataBuffer, scene.GetTextureDataBuffer());
    hit_surface_kernel_->SetArgument(args::HitSurface::kSceneInfo, &scene_info, sizeof(scene_info));
    miss_kernel_->SetArgument(args::Miss::kIblTextureBuffer, scene.GetEnvTextureBuffer());
    aov_kernel_->SetArgument(args::Aov::kTrianglesBuffer, scene.GetTriangleBuffer());
    aov_kernel_->SetArgument(args::Aov::kMaterialsBuffer, scene.GetMaterialBuffer());
    aov_kernel_->SetArgument(args::Aov::kTexturesBuffer, scene.GetTextureBuffer());
    aov_kernel_->SetArgument(args::Aov::kTextureDataBuffer, scene.GetTextureDataBuffer());
}

void PathTraceIntegrator::SetMaxBounces(std::uint32_t max_bounces)
{
    max_bounces_ = max_bounces;
    RequestReset();
}

void PathTraceIntegrator::SetSamplerType(SamplerType sampler_type)
{
    if (sampler_type == sampler_type_)
    {
        return;
    }

    sampler_type_ = sampler_type;
    CreateKernels();
    RequestReset();
}

void PathTraceIntegrator::SetAOV(AOV aov)
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

void PathTraceIntegrator::EnableDenoiser(bool enable_denoiser)
{
    if (enable_denoiser == enable_denoiser_)
    {
        return;
    }

    enable_denoiser_ = enable_denoiser;
    CreateKernels();
    RequestReset();
}

void PathTraceIntegrator::EnableDemodulation(bool enable_demodulation)
{
    if (enable_demodulation == enable_demodulation_)
    {
        return;
    }

    enable_demodulation_ = enable_demodulation;
    CreateKernels();
    RequestReset();
}

void PathTraceIntegrator::Reset()
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

void PathTraceIntegrator::ClearBuffers()
{
}

void PathTraceIntegrator::AdvanceSampleCount()
{
    increment_counter_kernel_->SetArgument(0, sample_counter_buffer_);
    cl_context_.ExecuteKernel(*increment_counter_kernel_, 1);
}

void PathTraceIntegrator::GenerateRays()
{
    std::uint32_t num_rays = width_ * height_;
    cl_context_.ExecuteKernel(*raygen_kernel_, num_rays);
}

void PathTraceIntegrator::IntersectRays(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;
    std::uint32_t incoming_idx = bounce & 1;

    acc_structure_.IntersectRays(rays_buffer_[incoming_idx], ray_counter_buffer_[incoming_idx],
        max_num_rays, hits_buffer_);
}

void PathTraceIntegrator::ComputeAOVs()
{
    std::uint32_t max_num_rays = width_ * height_;

    // Setup AOV kernel
    aov_kernel_->SetArgument(args::Aov::kRayBuffer, rays_buffer_[0]);
    aov_kernel_->SetArgument(args::Aov::kRayCounterBuffer, ray_counter_buffer_[0]);
    aov_kernel_->SetArgument(args::Aov::kPixelIndicesBuffer, pixel_indices_buffer_[0]);
    aov_kernel_->SetArgument(args::Aov::kHitsBuffer, hits_buffer_);
    aov_kernel_->SetArgument(args::Aov::kWidth, &width_, sizeof(width_));
    aov_kernel_->SetArgument(args::Aov::kHeight, &height_, sizeof(height_));
    aov_kernel_->SetArgument(args::Aov::kDiffuseAlbedo, diffuse_albedo_buffer_);
    aov_kernel_->SetArgument(args::Aov::kDepth, depth_buffer_);
    aov_kernel_->SetArgument(args::Aov::kNormal, normal_buffer_);
    aov_kernel_->SetArgument(args::Aov::kVelocity, velocity_buffer_);

    cl_context_.ExecuteKernel(*aov_kernel_, max_num_rays);
}

void PathTraceIntegrator::IntersectShadowRays()
{
    std::uint32_t max_num_rays = width_ * height_;

    acc_structure_.IntersectRays(shadow_rays_buffer_, shadow_ray_counter_buffer_,
        max_num_rays, shadow_hits_buffer_, false);
}

void PathTraceIntegrator::ShadeMissedRays(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;
    std::uint32_t incoming_idx = bounce & 1;

    miss_kernel_->SetArgument(args::Miss::kRayBuffer, rays_buffer_[incoming_idx]);
    miss_kernel_->SetArgument(args::Miss::kPixelIndicesBuffer, pixel_indices_buffer_[incoming_idx]);
    miss_kernel_->SetArgument(args::Miss::kRayCounterBuffer, ray_counter_buffer_[incoming_idx]);
    cl_context_.ExecuteKernel(*miss_kernel_, max_num_rays);
}

void PathTraceIntegrator::ShadeSurfaceHits(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;

    std::uint32_t incoming_idx = bounce & 1;
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    // Incoming rays
    hit_surface_kernel_->SetArgument(args::HitSurface::kIncomingRayBuffer, rays_buffer_[incoming_idx]);
    hit_surface_kernel_->SetArgument(args::HitSurface::kIncomingPixelIndicesBuffer, pixel_indices_buffer_[incoming_idx]);
    hit_surface_kernel_->SetArgument(args::HitSurface::kIncomingRayCounterBuffer,ray_counter_buffer_[incoming_idx]);

    hit_surface_kernel_->SetArgument(args::HitSurface::kHitsBuffer, hits_buffer_);

    hit_surface_kernel_->SetArgument(args::HitSurface::kBounce, &bounce, sizeof(bounce));
    hit_surface_kernel_->SetArgument(args::HitSurface::kWidth, &width_, sizeof(width_));
    hit_surface_kernel_->SetArgument(args::HitSurface::kHeight, &height_, sizeof(height_));

    hit_surface_kernel_->SetArgument(args::HitSurface::kSampleCounterBuffer, sample_counter_buffer_);

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

void PathTraceIntegrator::AccumulateDirectSamples()
{
    std::uint32_t max_num_rays = width_ * height_;
    cl_context_.ExecuteKernel(*accumulate_direct_samples_kernel_, max_num_rays);
}

void PathTraceIntegrator::ClearOutgoingRayCounter(std::uint32_t bounce)
{
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    clear_counter_kernel_->SetArgument(0, ray_counter_buffer_[outgoing_idx]);
    cl_context_.ExecuteKernel(*clear_counter_kernel_, 1);
}

void PathTraceIntegrator::ClearShadowRayCounter()
{
    clear_counter_kernel_->SetArgument(0, shadow_ray_counter_buffer_);
    cl_context_.ExecuteKernel(*clear_counter_kernel_, 1);
}

void PathTraceIntegrator::Denoise()
{
    cl_context_.ExecuteKernel(*temporal_filter_kernel_, width_ * height_);
    cl_context_.ExecuteKernel(*spatial_filter_kernel_, width_ * height_);
}

void PathTraceIntegrator::CopyHistoryBuffers()
{
    // Copy to the history
    cl_context_.CopyBuffer(radiance_buffer_, prev_radiance_buffer_, 0, 0, width_ * height_ * sizeof(cl_float4));
    cl_context_.CopyBuffer(depth_buffer_, prev_depth_buffer_, 0, 0, width_ * height_ * sizeof(cl_float));
}

void PathTraceIntegrator::ResolveRadiance()
{
    // Copy radiance to the interop image
    cl_context_.AcquireGLObject((*output_image_)());
    cl_context_.ExecuteKernel(*resolve_kernel_, width_ * height_);
    cl_context_.Finish();
    cl_context_.ReleaseGLObject((*output_image_)());
}

void PathTraceIntegrator::Integrate()
{
    if (request_reset_ || enable_denoiser_)
    {
        Reset();
        request_reset_ = false;
    }

    GenerateRays();

    for (std::uint32_t bounce = 0; bounce <= max_bounces_; ++bounce)
    {
        IntersectRays(bounce);
        if (bounce == 0)
        {
            ComputeAOVs();
        }
        ShadeMissedRays(bounce);
        ClearOutgoingRayCounter(bounce);
        ClearShadowRayCounter();
        ShadeSurfaceHits(bounce);
        IntersectShadowRays();
        AccumulateDirectSamples();
    }

    AdvanceSampleCount();
    if (enable_denoiser_)
    {
        Denoise();
        CopyHistoryBuffers();
    }
    ResolveRadiance();
}
