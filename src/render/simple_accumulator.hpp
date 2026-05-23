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

class SimpleAccumulator
{
public:
    SimpleAccumulator(uint32_t width, uint32_t height, gpu::Device& device);
    ~SimpleAccumulator();

    void SetInput(gpu::ImagePtr const& radiance_image);
    void Accumulate(gpu::CommandBuffer& command_buffer);
    void Reset() { num_samples_ = 0; }
    gpu::Image& GetOutputImage() const;

private:
    gpu::Device& device_;
    uint32_t width_ = 0u;
    uint32_t height_ = 0u;

    gpu::ImagePtr accumulated_color_image_;
    gpu::ComputePipelinePtr accumulate_radiance_pipeline_;
    gpu::DescriptorSetPtr accumulate_radiance_set_;

    uint32_t num_samples_ = 0u;
};
