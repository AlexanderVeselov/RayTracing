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

#include "simple_accumulator.hpp"

#include "gpu_command_buffer.hpp"
#include "gpu_descriptor_set.hpp"
#include "gpu_device.hpp"
#include "gpu_image.hpp"
#include "gpu_pipeline.hpp"
#include "gpu_queue.hpp"

#include <cassert>
#include <utility>

namespace
{
uint32_t DivideAndRoundUp(uint32_t value, uint32_t divisor)
{
    return (value + divisor - 1) / divisor;
}
}  // namespace

SimpleAccumulator::SimpleAccumulator(uint32_t width, uint32_t height, gpu::Device& device)
    : device_(device), width_(width), height_(height)
{
    accumulated_color_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    accumulate_radiance_pipeline_ = device_.CreateComputePipeline("accumulate_radiance.cs");

    gpu::Queue& queue = device_.GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr command_buffer = queue.CreateCommandBuffer();
    command_buffer->TransitionBarrier(accumulated_color_image_, gpu::ImageLayout::kUndefined,
        gpu::ImageLayout::kShaderReadWrite);
    queue.Submit(std::move(command_buffer));
    queue.WaitIdle();
}

SimpleAccumulator::~SimpleAccumulator() = default;

void SimpleAccumulator::SetInput(gpu::ImagePtr const& radiance_image)
{
    assert(accumulate_radiance_pipeline_);
    assert(radiance_image);

    accumulate_radiance_set_ = accumulate_radiance_pipeline_->CreateDescriptorSet();
    accumulate_radiance_set_->BindImage(*accumulated_color_image_, 0);
    accumulate_radiance_set_->BindImage(*radiance_image, 1);
}

void SimpleAccumulator::Accumulate(gpu::CommandBuffer& command_buffer)
{
    assert(accumulate_radiance_pipeline_ && accumulate_radiance_set_);
    ++num_samples_;

    command_buffer.BindPipeline(accumulate_radiance_pipeline_);
    command_buffer.BindDescriptorSet(accumulate_radiance_set_);
    command_buffer.SetRootConstants(&num_samples_, sizeof(num_samples_));
    command_buffer.Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    command_buffer.StorageBarrier(accumulated_color_image_);
}

gpu::Image& SimpleAccumulator::GetOutputImage() const
{
    return *accumulated_color_image_;
}
