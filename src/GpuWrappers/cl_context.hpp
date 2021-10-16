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

#include "scene/scene.hpp"
#include <CL/cl.hpp>
#include <memory>
#include <vector>
#include <string>

// OpenCL & OpenGL interop
// https://software.intel.com/en-us/articles/opencl-and-opengl-interoperability-tutorial

class CLKernel;

class CLContext
{
public:
    CLContext(const cl::Platform& platform, void* display_context, void* gl_context);

    void WriteBuffer(const cl::Buffer& buffer, const void* data, size_t size) const;
    void ReadBuffer(const cl::Buffer& buffer, void* ptr, size_t size) const;
    void ExecuteKernel(CLKernel const& kernel, std::size_t work_size) const;
    void Finish() const { queue_.finish(); }
    void AcquireGLObject(cl_mem mem);
    void ReleaseGLObject(cl_mem mem);

    const cl::Context& GetContext() const { return context_; }
    std::vector<cl::Device> const& GetDevices() const { return devices_; }


private:
    cl::Platform platform_;
    std::vector<cl::Device> devices_;
    cl::Context context_;
    cl::CommandQueue queue_;

};

class CLKernel
{
public:
    CLKernel(const char* filename, const CLContext& cl_context, char const* kernel_name,
        std::vector<std::string> const& definitions = std::vector<std::string>());
    void SetArgument(std::uint32_t argIndex, void const* data, size_t size);
    const cl::Kernel& GetKernel() const { return m_Kernel; }

private:
    cl::Kernel  m_Kernel;
    cl::Program m_Program;

};
