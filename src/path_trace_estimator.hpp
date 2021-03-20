#ifndef PATH_TRACE_ESTIMATOR_HPP
#define PATH_TRACE_ESTIMATOR_HPP

#include "GpuWrappers/cl_context.hpp"
#include <memory>

class Scene;
class Camera;
class AccelerationStructure;

class PathTraceEstimator
{
public:
    PathTraceEstimator(std::uint32_t width, std::uint32_t height,
        CLContext& cl_context, AccelerationStructure& acc_structure, cl_GLuint interop_image);
    void Estimate();
    void SetSceneData(Scene const& scene);
    void SetCameraData(Camera const& camera);
    void Reset();

private:
    void AdvanceFrameIndex();
    void GenerateRays();
    void IntersectRays(std::uint32_t bounce);
    void ShadeMissedRays(std::uint32_t bounce);
    void ShadeSurfaceHits(std::uint32_t bounce);
    void ClearOutgoingRayCounter(std::uint32_t bounce);
    void ResolveRadiance();

    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    std::uint32_t max_bounces_ = 3u;

    CLContext& cl_context_;
    cl_GLuint gl_interop_image_;

    // Acceleration structure
    AccelerationStructure& acc_structure_;

    // Kernels
    std::unique_ptr<CLKernel> raygen_kernel_;
    std::unique_ptr<CLKernel> miss_kernel_;
    std::unique_ptr<CLKernel> hit_surface_kernel_;
    std::unique_ptr<CLKernel> clear_counter_kernel_;
    std::unique_ptr<CLKernel> increment_counter_kernel_;
    std::unique_ptr<CLKernel> resolve_kernel_;

    // Internal buffers
    cl::Buffer rays_buffer_[2]; // 2 buffers for incoming-outgoing rays
    cl::Buffer pixel_indices_buffer_[2];
    cl::Buffer ray_counter_buffer_[2];
    cl::Buffer hits_buffer_;
    cl::Buffer frame_index_buffer_;
    cl::Buffer radiance_buffer_;

    // Sampler buffers
    cl::Buffer sampler_sobol_buffer_;
    cl::Buffer sampler_scrambling_tile_buffer_;
    cl::Buffer sampler_ranking_tile_buffer_;

    std::unique_ptr<cl::Image> output_image_;

};

#endif // PATH_TRACE_ESTIMATOR_HPP
