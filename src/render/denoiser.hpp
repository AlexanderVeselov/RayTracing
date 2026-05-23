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

#pragma once

#include "gpu_types.hpp"

#include <cstdint>

namespace gpu
{
class CommandBuffer;
class Device;
class Image;
}  // namespace gpu

class Denoiser
{
public:
    Denoiser(uint32_t width, uint32_t height, gpu::Device& device);
    ~Denoiser();

    void SetInputs(gpu::ImagePtr const& radiance_image, gpu::ImagePtr const& depth_image,
        gpu::ImagePtr const& motion_vectors_image);
    void Denoise(gpu::CommandBuffer& command_buffer);
    gpu::Image& GetOutputImage() const;
    void Reset() { do_reset_ = true; }

private:
    gpu::Device& device_;
    uint32_t width_ = 0u;
    uint32_t height_ = 0u;

    gpu::ImagePtr radiance_image_;
    gpu::ImagePtr prev_radiance_image_;
    gpu::ImagePtr prev_depth_image_;
    gpu::ComputePipelinePtr denoiser_pipeline_;
    gpu::ComputePipelinePtr copy_history_pipeline_;
    gpu::DescriptorSetPtr denoiser_set_;
    gpu::DescriptorSetPtr copy_history_set_;

    bool do_reset_ = true;
};
