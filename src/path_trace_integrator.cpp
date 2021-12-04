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

    kernels_ = CreateKernels();

    // Don't forget to reset frame index
    Reset();
}

PathTraceIntegrator::Kernels PathTraceIntegrator::CreateKernels()
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
    Kernels kernels;
    kernels.reset = std::make_unique<CLKernel>("src/Kernels/reset_radiance.cl", cl_context_, "ResetRadiance");
    kernels.raygen = std::make_unique<CLKernel>("src/Kernels/raygeneration.cl", cl_context_, "RayGeneration", definitions);
    kernels.miss = std::make_unique<CLKernel>("src/Kernels/miss.cl", cl_context_, "Miss", definitions);
    kernels.aov = std::make_unique<CLKernel>("src/Kernels/aov.cl", cl_context_, "GenerateAOV");
    kernels.hit_surface = std::make_unique<CLKernel>("src/Kernels/hit_surface.cl", cl_context_, "HitSurface", definitions);
    kernels.accumulate_direct_samples = std::make_unique<CLKernel>("src/Kernels/accumulate_direct_samples.cl", cl_context_, "AccumulateDirectSamples", definitions);
    kernels.clear_counter = std::make_unique<CLKernel>("src/Kernels/clear_counter.cl", cl_context_, "ClearCounter");
    kernels.increment_counter = std::make_unique<CLKernel>("src/Kernels/increment_counter.cl", cl_context_, "IncrementCounter");
    kernels.resolve = std::make_unique<CLKernel>("src/Kernels/resolve_radiance.cl", cl_context_, "ResolveRadiance", definitions);

    if (enable_denoiser_)
    {
        kernels.temporal_filter = std::make_unique<CLKernel>("src/Kernels/temporal_filter.cl", cl_context_, "TemporalFilter");
        kernels.spatial_filter = std::make_unique<CLKernel>("src/Kernels/spatial_filter.cl", cl_context_, "SpatialFilter");
    }

    // Setup kernels
    cl_mem output_image_mem = (*output_image_)();

    // Setup reset kernel
    kernels.reset->SetArgument(0, &width_, sizeof(width_));
    kernels.reset->SetArgument(1, &height_, sizeof(height_));
    kernels.reset->SetArgument(2, radiance_buffer_);

    // Setup raygen kernel
    kernels.raygen->SetArgument(args::Raygen::kWidth, &width_, sizeof(width_));
    kernels.raygen->SetArgument(args::Raygen::kHeight, &height_, sizeof(height_));
    kernels.raygen->SetArgument(args::Raygen::kSampleCounterBuffer, sample_counter_buffer_);
    kernels.raygen->SetArgument(args::Raygen::kRayBuffer, rays_buffer_[0]);
    kernels.raygen->SetArgument(args::Raygen::kRayCounterBuffer, ray_counter_buffer_[0]);
    kernels.raygen->SetArgument(args::Raygen::kPixelIndicesBuffer, pixel_indices_buffer_[0]);
    kernels.raygen->SetArgument(args::Raygen::kThroughputsBuffer, throughputs_buffer_);
    kernels.raygen->SetArgument(args::Raygen::kDiffuseAlbedoBuffer, diffuse_albedo_buffer_);
    kernels.raygen->SetArgument(args::Raygen::kDepthBuffer, depth_buffer_);
    kernels.raygen->SetArgument(args::Raygen::kNormalBuffer, normal_buffer_);
    kernels.raygen->SetArgument(args::Raygen::kVelocityBuffer, velocity_buffer_);

    if (enable_demodulation_)
    {
        kernels.raygen->SetArgument(args::Raygen::kDirectLightingBuffer, direct_lighting_buffer_);
        kernels.raygen->SetArgument(args::Raygen::kIndirectDiffuseBuffer, indirect_diffuse_buffer_);
        kernels.raygen->SetArgument(args::Raygen::kIndirectSpecularBuffer, indirect_specular_buffer_);
    }

    // Setup miss kernel
    kernels.miss->SetArgument(args::Miss::kHitsBuffer, hits_buffer_);
    kernels.miss->SetArgument(args::Miss::kThroughputsBuffer, throughputs_buffer_);
    kernels.miss->SetArgument(args::Miss::kRadianceBuffer, radiance_buffer_);

    // Setup hit surface kernel

    // Setup accumulate direct samples kernel
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kShadowHitsBuffer,
        shadow_hits_buffer_);
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kShadowRayCounterBuffer,
        shadow_ray_counter_buffer_);
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kShadowPixelIndicesBuffer,
        shadow_pixel_indices_buffer_);
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kDirectLightSamplesBuffer,
        direct_light_samples_buffer_);
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kRadianceBuffer,
        enable_demodulation_ ? direct_lighting_buffer_ : radiance_buffer_);

    // Setup resolve kernel
    kernels.resolve->SetArgument(args::Resolve::kWidth, &width_, sizeof(width_));
    kernels.resolve->SetArgument(args::Resolve::kHeight, &height_, sizeof(height_));
    kernels.resolve->SetArgument(args::Resolve::kSampleCounterBuffer, sample_counter_buffer_);
    kernels.resolve->SetArgument(args::Resolve::kRadianceBuffer, radiance_buffer_);
    kernels.resolve->SetArgument(args::Resolve::kDiffuseAlbedo, diffuse_albedo_buffer_);
    kernels.resolve->SetArgument(args::Resolve::kDepth, depth_buffer_);
    kernels.resolve->SetArgument(args::Resolve::kNormal, normal_buffer_);
    kernels.resolve->SetArgument(args::Resolve::kMotionVectors, velocity_buffer_);
    kernels.resolve->SetArgument(args::Resolve::kResolvedTexture, output_image_mem);
    std::uint32_t aov_index = aov_;
    kernels.resolve->SetArgument(args::Resolve::kAovIndex, &aov_index, sizeof(aov_index));
    if (enable_demodulation_)
    {
        kernels.resolve->SetArgument(args::Resolve::kDirectLightingBuffer, direct_lighting_buffer_);
    }

    if (enable_denoiser_)
    {
        // Setup temporal filter kernel
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kWidth, &width_, sizeof(width_));
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kHeight, &height_, sizeof(height_));
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kRadiance, radiance_buffer_);
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kPrevRadiance, prev_radiance_buffer_);
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kAccumulatedRadiance, accumulated_radiance_buffer_);
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kDepth, depth_buffer_);
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kPrevDepth, prev_depth_buffer_);
        kernels.temporal_filter->SetArgument(args::TemporalFilter::kMotionVectors, velocity_buffer_);

        // Setup spatial filter kernel
        kernels.spatial_filter->SetArgument(args::SpatialFilter::kWidth, &width_, sizeof(width_));
        kernels.spatial_filter->SetArgument(args::SpatialFilter::kHeight, &height_, sizeof(height_));
        kernels.spatial_filter->SetArgument(args::SpatialFilter::kInputRadiance, accumulated_radiance_buffer_);
        kernels.spatial_filter->SetArgument(args::SpatialFilter::kOutputRadiance, radiance_buffer_);
        kernels.spatial_filter->SetArgument(args::SpatialFilter::kDepth, depth_buffer_);
        kernels.spatial_filter->SetArgument(args::SpatialFilter::kNormals, normal_buffer_);
    }

    return kernels;
}

void PathTraceIntegrator::ReloadKernels()
{
    try
    {
        Kernels kernels = CreateKernels();
        std::swap(kernels, kernels_);
        std::cout << "Kernels have been reloaded" << std::endl;
    }
    catch (std::exception const& ex)
    {
        std::cerr << "Failed to reload kernels:\n" << ex.what() << std::endl;
    }
}

void PathTraceIntegrator::EnableWhiteFurnace(bool enable)
{
    if (enable == enable_white_furnace_)
    {
        return;
    }

    enable_white_furnace_ = enable;
    ReloadKernels();
    RequestReset();
}

void PathTraceIntegrator::SetCameraData(Camera const& camera)
{
    kernels_.raygen->SetArgument(args::Raygen::kCamera, &camera, sizeof(camera));
    kernels_.aov->SetArgument(args::Aov::kCamera, &camera, sizeof(camera));
    kernels_.aov->SetArgument(args::Aov::kPrevCamera, &prev_camera_, sizeof(prev_camera_));
    prev_camera_ = camera;
}

void PathTraceIntegrator::SetSceneData(Scene const& scene)
{
    // Set scene buffers
    SceneInfo scene_info = scene.GetSceneInfo();

    kernels_.hit_surface->SetArgument(args::HitSurface::kTrianglesBuffer, scene.GetTriangleBuffer());
    kernels_.hit_surface->SetArgument(args::HitSurface::kAnalyticLightsBuffer, scene.GetAnalyticLightBuffer());
    kernels_.hit_surface->SetArgument(args::HitSurface::kEmissiveIndicesBuffer, scene.GetEmissiveIndicesBuffer());
    kernels_.hit_surface->SetArgument(args::HitSurface::kMaterialsBuffer, scene.GetMaterialBuffer());
    kernels_.hit_surface->SetArgument(args::HitSurface::kTexturesBuffer, scene.GetTextureBuffer());
    kernels_.hit_surface->SetArgument(args::HitSurface::kTextureDataBuffer, scene.GetTextureDataBuffer());
    kernels_.hit_surface->SetArgument(args::HitSurface::kSceneInfo, &scene_info, sizeof(scene_info));
    kernels_.miss->SetArgument(args::Miss::kIblTextureBuffer, scene.GetEnvTextureBuffer());
    kernels_.aov->SetArgument(args::Aov::kTrianglesBuffer, scene.GetTriangleBuffer());
    kernels_.aov->SetArgument(args::Aov::kMaterialsBuffer, scene.GetMaterialBuffer());
    kernels_.aov->SetArgument(args::Aov::kTexturesBuffer, scene.GetTextureBuffer());
    kernels_.aov->SetArgument(args::Aov::kTextureDataBuffer, scene.GetTextureDataBuffer());
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
    ReloadKernels();
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
    kernels_.resolve->SetArgument(args::Resolve::kAovIndex, &aov_idx, sizeof(aov_idx));

    RequestReset();
}

void PathTraceIntegrator::EnableDenoiser(bool enable_denoiser)
{
    if (enable_denoiser == enable_denoiser_)
    {
        return;
    }

    enable_denoiser_ = enable_denoiser;
    ReloadKernels();
    RequestReset();
}

void PathTraceIntegrator::EnableDemodulation(bool enable_demodulation)
{
    if (enable_demodulation == enable_demodulation_)
    {
        return;
    }

    enable_demodulation_ = enable_demodulation;
    ReloadKernels();
    RequestReset();
}

void PathTraceIntegrator::Reset()
{
    if (!enable_denoiser_)
    {
        // Reset frame index
        kernels_.clear_counter->SetArgument(0, sample_counter_buffer_);
        cl_context_.ExecuteKernel(*kernels_.clear_counter, 1);
    }

    // Reset radiance buffer
    cl_context_.ExecuteKernel(*kernels_.reset, width_ * height_);
}

void PathTraceIntegrator::ClearBuffers()
{
}

void PathTraceIntegrator::AdvanceSampleCount()
{
    kernels_.increment_counter->SetArgument(0, sample_counter_buffer_);
    cl_context_.ExecuteKernel(*kernels_.increment_counter, 1);
}

void PathTraceIntegrator::GenerateRays()
{
    std::uint32_t num_rays = width_ * height_;
    cl_context_.ExecuteKernel(*kernels_.raygen, num_rays);
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
    kernels_.aov->SetArgument(args::Aov::kRayBuffer, rays_buffer_[0]);
    kernels_.aov->SetArgument(args::Aov::kRayCounterBuffer, ray_counter_buffer_[0]);
    kernels_.aov->SetArgument(args::Aov::kPixelIndicesBuffer, pixel_indices_buffer_[0]);
    kernels_.aov->SetArgument(args::Aov::kHitsBuffer, hits_buffer_);
    kernels_.aov->SetArgument(args::Aov::kWidth, &width_, sizeof(width_));
    kernels_.aov->SetArgument(args::Aov::kHeight, &height_, sizeof(height_));
    kernels_.aov->SetArgument(args::Aov::kDiffuseAlbedo, diffuse_albedo_buffer_);
    kernels_.aov->SetArgument(args::Aov::kDepth, depth_buffer_);
    kernels_.aov->SetArgument(args::Aov::kNormal, normal_buffer_);
    kernels_.aov->SetArgument(args::Aov::kVelocity, velocity_buffer_);

    cl_context_.ExecuteKernel(*kernels_.aov, max_num_rays);
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

    kernels_.miss->SetArgument(args::Miss::kRayBuffer, rays_buffer_[incoming_idx]);
    kernels_.miss->SetArgument(args::Miss::kPixelIndicesBuffer, pixel_indices_buffer_[incoming_idx]);
    kernels_.miss->SetArgument(args::Miss::kRayCounterBuffer, ray_counter_buffer_[incoming_idx]);
    cl_context_.ExecuteKernel(*kernels_.miss, max_num_rays);
}

void PathTraceIntegrator::ShadeSurfaceHits(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;

    std::uint32_t incoming_idx = bounce & 1;
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    // Incoming rays
    kernels_.hit_surface->SetArgument(args::HitSurface::kIncomingRayBuffer, rays_buffer_[incoming_idx]);
    kernels_.hit_surface->SetArgument(args::HitSurface::kIncomingPixelIndicesBuffer, pixel_indices_buffer_[incoming_idx]);
    kernels_.hit_surface->SetArgument(args::HitSurface::kIncomingRayCounterBuffer,ray_counter_buffer_[incoming_idx]);

    kernels_.hit_surface->SetArgument(args::HitSurface::kHitsBuffer, hits_buffer_);

    kernels_.hit_surface->SetArgument(args::HitSurface::kBounce, &bounce, sizeof(bounce));
    kernels_.hit_surface->SetArgument(args::HitSurface::kWidth, &width_, sizeof(width_));
    kernels_.hit_surface->SetArgument(args::HitSurface::kHeight, &height_, sizeof(height_));

    kernels_.hit_surface->SetArgument(args::HitSurface::kSampleCounterBuffer, sample_counter_buffer_);

    kernels_.hit_surface->SetArgument(args::HitSurface::kSobolBuffer, sampler_sobol_buffer_);
    kernels_.hit_surface->SetArgument(args::HitSurface::kScramblingTileBuffer, sampler_scrambling_tile_buffer_);
    kernels_.hit_surface->SetArgument(args::HitSurface::kRankingTileBuffer, sampler_ranking_tile_buffer_);

    kernels_.hit_surface->SetArgument(args::HitSurface::kThroughputsBuffer, throughputs_buffer_);

    // Outgoing rays
    kernels_.hit_surface->SetArgument(args::HitSurface::kOutgoingRayBuffer, rays_buffer_[outgoing_idx]);
    kernels_.hit_surface->SetArgument(args::HitSurface::kOutgoingPixelIndicesBuffer, pixel_indices_buffer_[outgoing_idx]);
    kernels_.hit_surface->SetArgument(args::HitSurface::kOutgoingRayCounterBuffer, ray_counter_buffer_[outgoing_idx]);

    // Shadow
    kernels_.hit_surface->SetArgument(args::HitSurface::kShadowRayBuffer, shadow_rays_buffer_);
    kernels_.hit_surface->SetArgument(args::HitSurface::kShadowRayCounterBuffer, shadow_ray_counter_buffer_);
    kernels_.hit_surface->SetArgument(args::HitSurface::kShadowPixelIndicesBuffer, shadow_pixel_indices_buffer_);
    kernels_.hit_surface->SetArgument(args::HitSurface::kDirectLightSamplesBuffer, direct_light_samples_buffer_);

    // Output radiance
    kernels_.hit_surface->SetArgument(args::HitSurface::kRadianceBuffer, radiance_buffer_);

    cl_context_.ExecuteKernel(*kernels_.hit_surface, max_num_rays);
}

void PathTraceIntegrator::AccumulateDirectSamples()
{
    std::uint32_t max_num_rays = width_ * height_;
    cl_context_.ExecuteKernel(*kernels_.accumulate_direct_samples, max_num_rays);
}

void PathTraceIntegrator::ClearOutgoingRayCounter(std::uint32_t bounce)
{
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    kernels_.clear_counter->SetArgument(0, ray_counter_buffer_[outgoing_idx]);
    cl_context_.ExecuteKernel(*kernels_.clear_counter, 1);
}

void PathTraceIntegrator::ClearShadowRayCounter()
{
    kernels_.clear_counter->SetArgument(0, shadow_ray_counter_buffer_);
    cl_context_.ExecuteKernel(*kernels_.clear_counter, 1);
}

void PathTraceIntegrator::Denoise()
{
    cl_context_.ExecuteKernel(*kernels_.temporal_filter, width_ * height_);
    cl_context_.ExecuteKernel(*kernels_.spatial_filter, width_ * height_);
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
    cl_context_.ExecuteKernel(*kernels_.resolve, width_ * height_);
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
