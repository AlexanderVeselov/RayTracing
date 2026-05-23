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

#include "denoiser.hpp"

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

Denoiser::Denoiser(uint32_t width, uint32_t height, gpu::Device& device)
    : device_(device), width_(width), height_(height)
{
    prev_radiance_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    prev_depth_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kR32_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    denoiser_pipeline_ = device_.CreateComputePipeline("denoiser.cs");
    copy_history_pipeline_ = device_.CreateComputePipeline("copy_history.cs");

    gpu::Queue& queue = device_.GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr command_buffer = queue.CreateCommandBuffer();
    command_buffer->TransitionBarrier({ prev_radiance_image_, prev_depth_image_ }, gpu::ImageLayout::kUndefined,
        gpu::ImageLayout::kShaderReadWrite);
    queue.Submit(std::move(command_buffer));
    queue.WaitIdle();
}

Denoiser::~Denoiser() = default;

void Denoiser::SetInputs(gpu::ImagePtr const& radiance_image, gpu::ImagePtr const& depth_image,
    gpu::ImagePtr const& motion_vectors_image)
{
    assert(denoiser_pipeline_ && copy_history_pipeline_);
    assert(radiance_image && depth_image && motion_vectors_image);

    radiance_image_ = radiance_image;

    denoiser_set_ = denoiser_pipeline_->CreateDescriptorSet();
    denoiser_set_->BindImage(*radiance_image, 1);
    denoiser_set_->BindImage(*prev_radiance_image_, 2);
    denoiser_set_->BindImage(*depth_image, 3);
    denoiser_set_->BindImage(*prev_depth_image_, 4);
    denoiser_set_->BindImage(*motion_vectors_image, 5);

    copy_history_set_ = copy_history_pipeline_->CreateDescriptorSet();
    copy_history_set_->BindImage(*radiance_image, 0);
    copy_history_set_->BindImage(*prev_radiance_image_, 1);
    copy_history_set_->BindImage(*depth_image, 2);
    copy_history_set_->BindImage(*prev_depth_image_, 3);
}

void Denoiser::Denoise(gpu::CommandBuffer& command_buffer)
{
    assert(radiance_image_);
    assert(denoiser_pipeline_ && denoiser_set_);
    assert(copy_history_pipeline_ && copy_history_set_);

    struct DenoiserRootConstants
    {
        uint32_t width;
        uint32_t height;
        uint32_t reset;
    } root_constants = { width_, height_, do_reset_ ? 1u : 0u };

    command_buffer.BindPipeline(denoiser_pipeline_);
    command_buffer.BindDescriptorSet(denoiser_set_);
    command_buffer.SetRootConstants(&root_constants, sizeof(root_constants));
    command_buffer.Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer.StorageBarrier(radiance_image_);

    command_buffer.BindPipeline(copy_history_pipeline_);
    command_buffer.BindDescriptorSet(copy_history_set_);
    command_buffer.Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    command_buffer.StorageBarrier(prev_radiance_image_);
    command_buffer.StorageBarrier(prev_depth_image_);

    do_reset_ = false;
}

gpu::Image& Denoiser::GetOutputImage() const
{
    assert(radiance_image_);
    return *radiance_image_;
}
