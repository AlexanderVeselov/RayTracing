#include "path_trace_integrator.hpp"
#include "utils/cl_exception.hpp"
#include "Scene/scene.hpp"
#include "Scene/camera.hpp"
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
            kCameraPos,
            kCameraFront,
            kCameraUp,
            kFrameCount,
            kFrameSeed,
            kAperture,
            kFocusDistance,
            // Output
            kRayBuffer,
            kRayCounterBuffer,
            kPixelIndicesBuffer,
            kThroughputsBuffer,
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

    namespace Resolve
    {
        enum
        {
            // Input
            kWidth,
            kHeight,
            kRadianceBuffer,
            kSampleCounterBuffer,
            // Output
            kResolvedTexture,
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
    sampler_sobol_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(sobol_256spp_256d), (void*)sobol_256spp_256d, &status);
    ThrowIfFailed(status, "Failed to create sobol buffer");

    sampler_scrambling_tile_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(scramblingTile), (void*)scramblingTile, &status);
    ThrowIfFailed(status, "Failed to create scrambling tile buffer");

    sampler_ranking_tile_buffer_ = cl::Buffer(cl_context.GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(rankingTile), (void*)rankingTile, &status);
    ThrowIfFailed(status, "Failed to create ranking tile tile buffer");

    output_image_ = std::make_unique<cl::ImageGL>(cl_context.GetContext(), CL_MEM_WRITE_ONLY,
        GL_TEXTURE_2D, 0, gl_interop_image_, &status);
    ThrowIfFailed(status, "Failed to create output image");

    kernels_ = CreateKernels();

    // Don't forget to reset frame index
    Reset();
}

PathTraceIntegrator::Kernels PathTraceIntegrator::CreateKernels()
{
    Kernels kernels;

    // Create kernels
    kernels.reset = std::make_unique<CLKernel>("src/Kernels/reset_radiance.cl", cl_context_, "ResetRadiance");
    kernels.raygen = std::make_unique<CLKernel>("src/Kernels/raygeneration.cl", cl_context_, "RayGeneration");

    std::vector<std::string> definitions;
    if (enable_white_furnace_)
    {
        definitions.push_back("ENABLE_WHITE_FURNACE");
    }

    if (sampler_type_ == SamplerType::kBlueNoise)
    {
        definitions.push_back("BLUE_NOISE_SAMPLER");
    }

    kernels.miss = std::make_unique<CLKernel>("src/Kernels/miss.cl", cl_context_, "Miss", definitions);
    kernels.hit_surface = std::make_unique<CLKernel>("src/Kernels/hit_surface.cl", cl_context_, "HitSurface", definitions);
    kernels.accumulate_direct_samples = std::make_unique<CLKernel>("src/Kernels/accumulate_direct_samples.cl", cl_context_, "AccumulateDirectSamples", definitions);
    kernels.clear_counter = std::make_unique<CLKernel>("src/Kernels/clear_counter.cl", cl_context_, "ClearCounter");
    kernels.increment_counter = std::make_unique<CLKernel>("src/Kernels/increment_counter.cl", cl_context_, "IncrementCounter");
    kernels.resolve = std::make_unique<CLKernel>("src/Kernels/resolve_radiance.cl", cl_context_, "ResolveRadiance");

    // Setup kernels
    cl_mem output_image_mem = (*output_image_)();

    // Setup reset kernel
    kernels.reset->SetArgument(0, &width_, sizeof(width_));
    kernels.reset->SetArgument(1, &height_, sizeof(height_));
    kernels.reset->SetArgument(2, &radiance_buffer_, sizeof(radiance_buffer_));

    // Setup raygen kernel
    kernels.raygen->SetArgument(args::Raygen::kWidth, &width_, sizeof(width_));
    kernels.raygen->SetArgument(args::Raygen::kHeight, &height_, sizeof(height_));
    kernels.raygen->SetArgument(args::Raygen::kRayBuffer, &rays_buffer_[0], sizeof(rays_buffer_[0]));
    kernels.raygen->SetArgument(args::Raygen::kRayCounterBuffer, &ray_counter_buffer_[0], sizeof(ray_counter_buffer_[0]));
    kernels.raygen->SetArgument(args::Raygen::kPixelIndicesBuffer, &pixel_indices_buffer_[0], sizeof(pixel_indices_buffer_[0]));
    kernels.raygen->SetArgument(args::Raygen::kThroughputsBuffer, &throughputs_buffer_, sizeof(throughputs_buffer_));

    // Setup miss kernel
    kernels.miss->SetArgument(args::Miss::kHitsBuffer, &hits_buffer_, sizeof(hits_buffer_));
    kernels.miss->SetArgument(args::Miss::kThroughputsBuffer, &throughputs_buffer_, sizeof(throughputs_buffer_));
    kernels.miss->SetArgument(args::Miss::kRadianceBuffer, &radiance_buffer_, sizeof(radiance_buffer_));

    // Setup hit surface kernel

    // Setup accumulate direct samples kernel
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kShadowHitsBuffer,
        &shadow_hits_buffer_, sizeof(shadow_hits_buffer_));
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kShadowRayCounterBuffer,
        &shadow_ray_counter_buffer_, sizeof(shadow_ray_counter_buffer_));
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kShadowPixelIndicesBuffer,
        &shadow_pixel_indices_buffer_, sizeof(shadow_pixel_indices_buffer_));
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kDirectLightSamplesBuffer,
        &direct_light_samples_buffer_, sizeof(direct_light_samples_buffer_));
    kernels.accumulate_direct_samples->SetArgument(args::AccumulateDirectSamples::kRadianceBuffer,
        &radiance_buffer_, sizeof(radiance_buffer_));

    // Setup resolve kernel
    kernels.resolve->SetArgument(args::Resolve::kWidth, &width_, sizeof(width_));
    kernels.resolve->SetArgument(args::Resolve::kHeight, &height_, sizeof(height_));
    kernels.resolve->SetArgument(args::Resolve::kSampleCounterBuffer, &sample_counter_buffer_, sizeof(sample_counter_buffer_));
    kernels.resolve->SetArgument(args::Resolve::kRadianceBuffer, &radiance_buffer_, sizeof(radiance_buffer_));
    kernels.resolve->SetArgument(args::Resolve::kResolvedTexture, &output_image_mem, sizeof(output_image_mem));

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
    float3 origin = camera.GetOrigin();
    float3 front = camera.GetFrontVector();

    float3 right = Cross(front, camera.GetUpVector()).Normalize();
    float3 up = Cross(right, front);
    kernels_.raygen->SetArgument(args::Raygen::kCameraPos, &origin, sizeof(origin));
    kernels_.raygen->SetArgument(args::Raygen::kCameraFront, &front, sizeof(front));
    kernels_.raygen->SetArgument(args::Raygen::kCameraUp, &up, sizeof(up));

    ///@TODO: move frame count to here
    std::uint32_t frame_count = camera.GetFrameCount();
    kernels_.raygen->SetArgument(args::Raygen::kFrameCount, &frame_count, sizeof(frame_count));
    unsigned int seed = rand();
    kernels_.raygen->SetArgument(args::Raygen::kFrameSeed, &seed, sizeof(seed));

    float aperture = camera.GetAperture();
    float focus_distance = camera.GetFocusDistance();
    kernels_.raygen->SetArgument(args::Raygen::kAperture, &aperture, sizeof(aperture));
    kernels_.raygen->SetArgument(args::Raygen::kFocusDistance, &focus_distance, sizeof(focus_distance));
}

void PathTraceIntegrator::SetSceneData(Scene const& scene)
{
    // Set scene buffers
    cl_mem triangle_buffer = scene.GetTriangleBuffer();
    cl_mem analytic_lights_buffer = scene.GetAnalyticLightBuffer();
    cl_mem emissive_indices_buffer = scene.GetEmissiveIndicesBuffer();
    cl_mem material_buffer = scene.GetMaterialBuffer();
    cl_mem env_texture = scene.GetEnvTextureBuffer();
    SceneInfo scene_info = scene.GetSceneInfo();

    kernels_.hit_surface->SetArgument(args::HitSurface::kTrianglesBuffer, &triangle_buffer, sizeof(cl_mem));
    kernels_.hit_surface->SetArgument(args::HitSurface::kAnalyticLightsBuffer, &analytic_lights_buffer, sizeof(cl_mem));
    kernels_.hit_surface->SetArgument(args::HitSurface::kEmissiveIndicesBuffer,
        &emissive_indices_buffer, sizeof(cl_mem));
    kernels_.hit_surface->SetArgument(args::HitSurface::kMaterialsBuffer, &material_buffer, sizeof(cl_mem));
    kernels_.hit_surface->SetArgument(args::HitSurface::kSceneInfo, &scene_info, sizeof(scene_info));
    kernels_.miss->SetArgument(args::Miss::kIblTextureBuffer, &env_texture, sizeof(cl_mem));
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

void PathTraceIntegrator::Reset()
{
    // Reset frame index
    kernels_.clear_counter->SetArgument(0, &sample_counter_buffer_,
        sizeof(sample_counter_buffer_));
    cl_context_.ExecuteKernel(*kernels_.clear_counter, 1);

    // Reset radiance buffer
    cl_context_.ExecuteKernel(*kernels_.reset, width_ * height_);
}

void PathTraceIntegrator::AdvanceSampleCount()
{
    kernels_.increment_counter->SetArgument(0, &sample_counter_buffer_,
        sizeof(sample_counter_buffer_));
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

    kernels_.miss->SetArgument(args::Miss::kRayBuffer,
        &rays_buffer_[incoming_idx], sizeof(rays_buffer_[incoming_idx]));
    kernels_.miss->SetArgument(args::Miss::kPixelIndicesBuffer,
        &pixel_indices_buffer_[incoming_idx], sizeof(pixel_indices_buffer_[incoming_idx]));
    kernels_.miss->SetArgument(args::Miss::kRayCounterBuffer,
        &ray_counter_buffer_[incoming_idx], sizeof(ray_counter_buffer_[incoming_idx]));
    cl_context_.ExecuteKernel(*kernels_.miss, max_num_rays);
}

void PathTraceIntegrator::ShadeSurfaceHits(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;

    std::uint32_t incoming_idx = bounce & 1;
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    // Incoming rays
    kernels_.hit_surface->SetArgument(args::HitSurface::kIncomingRayBuffer,
        &rays_buffer_[incoming_idx], sizeof(rays_buffer_[incoming_idx]));
    kernels_.hit_surface->SetArgument(args::HitSurface::kIncomingPixelIndicesBuffer,
        &pixel_indices_buffer_[incoming_idx], sizeof(pixel_indices_buffer_[incoming_idx]));
    kernels_.hit_surface->SetArgument(args::HitSurface::kIncomingRayCounterBuffer,
        &ray_counter_buffer_[incoming_idx], sizeof(ray_counter_buffer_[incoming_idx]));

    kernels_.hit_surface->SetArgument(args::HitSurface::kHitsBuffer,
        &hits_buffer_, sizeof(hits_buffer_));

    kernels_.hit_surface->SetArgument(args::HitSurface::kBounce, &bounce, sizeof(bounce));
    kernels_.hit_surface->SetArgument(args::HitSurface::kWidth,
        &width_, sizeof(width_));
    kernels_.hit_surface->SetArgument(args::HitSurface::kHeight,
        &height_, sizeof(height_));

    kernels_.hit_surface->SetArgument(args::HitSurface::kSampleCounterBuffer,
        &sample_counter_buffer_, sizeof(sample_counter_buffer_));

    kernels_.hit_surface->SetArgument(args::HitSurface::kSobolBuffer,
        &sampler_sobol_buffer_, sizeof(sampler_sobol_buffer_));
    kernels_.hit_surface->SetArgument(args::HitSurface::kScramblingTileBuffer,
        &sampler_scrambling_tile_buffer_, sizeof(sampler_scrambling_tile_buffer_));
    kernels_.hit_surface->SetArgument(args::HitSurface::kRankingTileBuffer,
        &sampler_ranking_tile_buffer_, sizeof(sampler_ranking_tile_buffer_));

    kernels_.hit_surface->SetArgument(args::HitSurface::kThroughputsBuffer,
        &throughputs_buffer_, sizeof(throughputs_buffer_));

    // Outgoing rays
    kernels_.hit_surface->SetArgument(args::HitSurface::kOutgoingRayBuffer,
        &rays_buffer_[outgoing_idx], sizeof(rays_buffer_[outgoing_idx]));
    kernels_.hit_surface->SetArgument(args::HitSurface::kOutgoingPixelIndicesBuffer,
        &pixel_indices_buffer_[outgoing_idx], sizeof(pixel_indices_buffer_[outgoing_idx]));
    kernels_.hit_surface->SetArgument(args::HitSurface::kOutgoingRayCounterBuffer,
        &ray_counter_buffer_[outgoing_idx], sizeof(ray_counter_buffer_[outgoing_idx]));

    // Shadow
    kernels_.hit_surface->SetArgument(args::HitSurface::kShadowRayBuffer,
        &shadow_rays_buffer_, sizeof(shadow_rays_buffer_));
    kernels_.hit_surface->SetArgument(args::HitSurface::kShadowRayCounterBuffer,
        &shadow_ray_counter_buffer_, sizeof(shadow_ray_counter_buffer_));
    kernels_.hit_surface->SetArgument(args::HitSurface::kShadowPixelIndicesBuffer,
        &shadow_pixel_indices_buffer_, sizeof(shadow_pixel_indices_buffer_));
    kernels_.hit_surface->SetArgument(args::HitSurface::kDirectLightSamplesBuffer,
        &direct_light_samples_buffer_, sizeof(direct_light_samples_buffer_));

    // Output radiance
    kernels_.hit_surface->SetArgument(args::HitSurface::kRadianceBuffer,
        &radiance_buffer_, sizeof(radiance_buffer_));

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

    kernels_.clear_counter->SetArgument(0, &ray_counter_buffer_[outgoing_idx],
        sizeof(ray_counter_buffer_[outgoing_idx]));
    cl_context_.ExecuteKernel(*kernels_.clear_counter, 1);
}

void PathTraceIntegrator::ClearShadowRayCounter()
{
    kernels_.clear_counter->SetArgument(0, &shadow_ray_counter_buffer_,
        sizeof(shadow_ray_counter_buffer_));
    cl_context_.ExecuteKernel(*kernels_.clear_counter, 1);
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
    if (request_reset_)
    {
        Reset();
        request_reset_ = false;
    }

    GenerateRays();

    for (std::uint32_t bounce = 0; bounce <= max_bounces_; ++bounce)
    {
        IntersectRays(bounce);
        ShadeMissedRays(bounce);
        ClearOutgoingRayCounter(bounce);
        ClearShadowRayCounter();
        ShadeSurfaceHits(bounce);
        IntersectShadowRays();
        AccumulateDirectSamples();
    }

    AdvanceSampleCount();
    ResolveRadiance();
}
