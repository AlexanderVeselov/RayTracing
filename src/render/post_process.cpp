/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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

#include "post_process.hpp"

#include "gpu_command_buffer.hpp"
#include "gpu_descriptor_set.hpp"
#include "gpu_device.hpp"
#include "gpu_image.hpp"
#include "gpu_pipeline.hpp"

#include <cassert>

namespace
{
uint32_t DivideAndRoundUp(uint32_t value, uint32_t divisor)
{
    return (value + divisor - 1) / divisor;
}
}  // namespace

PostProcess::PostProcess(uint32_t width, uint32_t height, gpu::Device& device, gpu::ImageFormat output_format)
    : device_(device), width_(width), height_(height)
{
    output_image_ = device_.CreateImage(width_, height_, output_format,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    tonemap_pipeline_ = device_.CreateComputePipeline("tonemap.cs");
}

PostProcess::~PostProcess() = default;

void PostProcess::SetInput(gpu::Image& input_color_image)
{
    assert(tonemap_pipeline_);

    if (tonemap_set_)
    {
        // WIP! These descriptor sets are not cleaned up, we need to keep them alive until
        // the GPU finishes using them
        retired_descriptor_sets_.push_back(std::move(tonemap_set_));
    }

    tonemap_set_ = tonemap_pipeline_->CreateDescriptorSet();
    tonemap_set_->BindImage(*output_image_, 0);
    tonemap_set_->BindImage(input_color_image, 1);
}

void PostProcess::Tonemap(gpu::CommandBuffer& command_buffer)
{
    assert(tonemap_pipeline_ && tonemap_set_);

    command_buffer.TransitionBarrier(output_image_, output_layout_, gpu::ImageLayout::kShaderReadWrite);
    command_buffer.BindPipeline(tonemap_pipeline_);
    command_buffer.BindDescriptorSet(tonemap_set_);
    command_buffer.Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    command_buffer.TransitionBarrier(output_image_, gpu::ImageLayout::kShaderReadWrite, gpu::ImageLayout::kCopySrc);
    output_layout_ = gpu::ImageLayout::kCopySrc;
}
