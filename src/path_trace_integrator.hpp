#ifndef PATH_TRACE_INTEGRATOR_HPP
#define PATH_TRACE_INTEGRATOR_HPP

#include "GpuWrappers/cl_context.hpp"
#include <memory>

class Scene;
class Camera;
class AccelerationStructure;

class PathTraceIntegrator
{
public:
    enum class SamplerType
    {
        kRandom,
        kBlueNoise
    };

    PathTraceIntegrator(std::uint32_t width, std::uint32_t height,
        CLContext& cl_context, AccelerationStructure& acc_structure, cl_GLuint interop_image);
    void Integrate();
    void SetSceneData(Scene const& scene);
    void SetCameraData(Camera const& camera);
    void RequestReset() { request_reset_ = true; };
    void ReloadKernels();
    void EnableWhiteFurnace(bool enable);
    void SetMaxBounces(std::uint32_t max_bounces);
    void SetSamplerType(SamplerType sampler_type);

private:
    struct Kernels
    {
        std::unique_ptr<CLKernel> raygen;
        std::unique_ptr<CLKernel> miss;
        std::unique_ptr<CLKernel> hit_surface;
        std::unique_ptr<CLKernel> accumulate_direct_samples;
        std::unique_ptr<CLKernel> reset;
        std::unique_ptr<CLKernel> clear_counter;
        std::unique_ptr<CLKernel> increment_counter;
        std::unique_ptr<CLKernel> resolve;
    };

    Kernels CreateKernels();
    void Reset();
    void AdvanceSampleCount();
    void GenerateRays();
    void IntersectRays(std::uint32_t bounce);
    void ShadeMissedRays(std::uint32_t bounce);
    void ShadeSurfaceHits(std::uint32_t bounce);
    void IntersectShadowRays();
    void AccumulateDirectSamples();
    void ClearOutgoingRayCounter(std::uint32_t bounce);
    void ClearShadowRayCounter();
    void ResolveRadiance();

    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    std::uint32_t max_bounces_ = 5u;
    SamplerType sampler_type_ = SamplerType::kRandom;

    bool request_reset_ = false;
    // For debugging
    bool enable_white_furnace_ = false;

    CLContext& cl_context_;
    cl_GLuint gl_interop_image_;

    // Acceleration structure
    AccelerationStructure& acc_structure_;

    // Kernels
    Kernels kernels_;

    // Internal buffers
    cl::Buffer rays_buffer_[2]; // 2 buffers for incoming-outgoing rays
    cl::Buffer shadow_rays_buffer_;
    cl::Buffer pixel_indices_buffer_[2];
    cl::Buffer shadow_pixel_indices_buffer_;
    cl::Buffer ray_counter_buffer_[2];
    cl::Buffer shadow_ray_counter_buffer_;
    cl::Buffer hits_buffer_;
    cl::Buffer shadow_hits_buffer_;
    cl::Buffer throughputs_buffer_;
    cl::Buffer sample_counter_buffer_;
    cl::Buffer radiance_buffer_;
    cl::Buffer direct_light_samples_buffer_;

    // Sampler buffers
    cl::Buffer sampler_sobol_buffer_;
    cl::Buffer sampler_scrambling_tile_buffer_;
    cl::Buffer sampler_ranking_tile_buffer_;

    std::unique_ptr<cl::Image> output_image_;

};

#endif // PATH_TRACE_INTEGRATOR_HPP
