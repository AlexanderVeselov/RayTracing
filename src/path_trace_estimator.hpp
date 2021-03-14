#ifndef PATH_TRACE_ESTIMATOR_HPP
#define PATH_TRACE_ESTIMATOR_HPP

#include "GpuWrappers/cl_context.hpp"
#include <memory>

class Scene;
class Camera;

class PathTraceEstimator
{
public:
    PathTraceEstimator(std::uint32_t width, std::uint32_t height, CLContext& cl_context, cl_GLuint interop_image);
    void Estimate();
    void SetSceneData(Scene const& scene);
    void SetCameraData(Camera const& camera);
    void Reset();

private:
    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    CLContext& cl_context_;
    cl_GLuint gl_interop_image_;

    // Kernels
    std::unique_ptr<CLKernel> render_kernel_;
    std::unique_ptr<CLKernel> copy_kernel_;

    std::unique_ptr<cl::Buffer> radiance_buffer_;
    std::unique_ptr<cl::Image> output_image_;

};

#endif // PATH_TRACE_ESTIMATOR_HPP
