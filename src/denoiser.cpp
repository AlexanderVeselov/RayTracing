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

#include "denoiser.hpp"
#include "utils/cl_exception.hpp"

namespace args
{
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
}

Denoiser::Denoiser(CLContext& context, std::uint32_t width, std::uint32_t height)
    : context_(context)
    , width_(width)
    , height_(height)
{
    temporal_filter_kernel_ = context_.CreateKernel("src/Kernels/temporal_filter.cl", "TemporalFilter");
    spatial_filter_kernel_ = context_.CreateKernel("src/Kernels/spatial_filter.cl", "SpatialFilter");

    cl_int status;
    prev_radiance_buffer_ = cl::Buffer(context.GetContext(), CL_MEM_READ_WRITE,
        width_ * height_ * sizeof(cl_float4), nullptr, &status);
    ThrowIfFailed(status, "Failed to create prev radiance buffer");

    accumulated_radiance_buffer_ = cl::Buffer(context.GetContext(), CL_MEM_READ_WRITE,
        width_ * height_ * sizeof(cl_float4), nullptr, &status);
    ThrowIfFailed(status, "Failed to create accumulated radiance buffer");
}

void Denoiser::Denoise(cl::Buffer const& radiance_buffer, cl::Buffer const& depth_buffer,
    cl::Buffer const& normal_buffer, cl::Buffer const& prev_depth_buffer,
    cl::Buffer const& motion_vectors_buffer)
{
    // Setup temporal filter kernel
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kWidth, &width_, sizeof(width_));
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kHeight, &height_, sizeof(height_));
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kRadiance, radiance_buffer);
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kPrevRadiance, prev_radiance_buffer_);
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kAccumulatedRadiance, accumulated_radiance_buffer_);
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kDepth, depth_buffer);
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kPrevDepth, prev_depth_buffer);
    temporal_filter_kernel_->SetArgument(args::TemporalFilter::kMotionVectors, motion_vectors_buffer);

    // Setup spatial filter kernel
    spatial_filter_kernel_->SetArgument(args::SpatialFilter::kWidth, &width_, sizeof(width_));
    spatial_filter_kernel_->SetArgument(args::SpatialFilter::kHeight, &height_, sizeof(height_));
    spatial_filter_kernel_->SetArgument(args::SpatialFilter::kInputRadiance, accumulated_radiance_buffer_);
    spatial_filter_kernel_->SetArgument(args::SpatialFilter::kOutputRadiance, radiance_buffer);
    spatial_filter_kernel_->SetArgument(args::SpatialFilter::kDepth, depth_buffer);
    spatial_filter_kernel_->SetArgument(args::SpatialFilter::kNormals, normal_buffer);

    // Execute
    context_.ExecuteKernel(*temporal_filter_kernel_, width_ * height_);
    context_.ExecuteKernel(*spatial_filter_kernel_, width_ * height_);

    // Copy to the history
    context_.CopyBuffer(radiance_buffer, prev_radiance_buffer_, 0, 0, width_ * height_ * sizeof(cl_float4));
}
