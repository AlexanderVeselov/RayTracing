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

#pragma once

#include "GpuWrappers/cl_context.hpp"

class Denoiser
{
public:
    Denoiser(CLContext& context, std::uint32_t width, std::uint32_t height);
    void Denoise(cl::Buffer const& radiance_buffer, cl::Buffer const& depth_buffer,
        cl::Buffer const& normal_buffer, cl::Buffer const& prev_depth_buffer,
        cl::Buffer const& motion_vectors_buffer);

private:
    CLContext& context_;
    std::uint32_t width_;
    std::uint32_t height_;

    std::shared_ptr<CLKernel> temporal_filter_kernel_;
    std::shared_ptr<CLKernel> spatial_filter_kernel_;

    // Internal buffers
    cl::Buffer prev_radiance_buffer_;
    cl::Buffer accumulated_radiance_buffer_;
};
