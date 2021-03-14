#include "path_trace_estimator.hpp"

PathTraceEstimator::PathTraceEstimator(std::uint32_t width, std::uint32_t height,
    CLContext& cl_context, cl_GLuint interop_image)
    : width_(width)
    , height_(height)
    , cl_context_(cl_context)
    , interop_image_(interop_image)
{

}

void PathTraceEstimator::Estimate()
{

}
