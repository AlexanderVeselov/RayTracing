#ifndef PATH_TRACE_ESTIMATOR_HPP
#define PATH_TRACE_ESTIMATOR_HPP

#include "GpuWrappers/cl_context.hpp"

#include <memory>

class PathTraceEstimator
{
public:
    PathTraceEstimator(std::uint32_t width, std::uint32_t height, CLContext& cl_context, cl_GLuint interop_image);
    void Estimate();

private:
    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    CLContext& cl_context_;
    cl_GLuint interop_image_;

    // Kernels
    std::unique_ptr<CLKernel> render_kernel_;
    std::unique_ptr<CLKernel> copy_kernel_;


};

#endif // PATH_TRACE_ESTIMATOR_HPP
