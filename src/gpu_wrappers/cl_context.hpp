/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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
#include <GL/glew.h>
#include <CL/cl.hpp>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

class CLKernel;

class CLContext
{
public:
    CLContext(const cl::Platform& platform);
    std::shared_ptr<CLKernel> CreateKernel(const char* filename, char const* kernel_name,
        std::vector<std::string> const& definitions = std::vector<std::string>());

    void WriteBuffer(const cl::Buffer& buffer, const void* data, size_t size) const;
    void ReadBuffer(const cl::Buffer& buffer, void* ptr, size_t size) const;
    void CopyBuffer(const cl::Buffer& src_buffer, const cl::Buffer& dst_buffer,
        std::size_t src_offset, std::size_t dst_offset, std::size_t size) const;
    void ExecuteKernel(CLKernel const& kernel, std::size_t global_size, std::size_t local_size = 0) const;
    void Finish() const { queue_.finish(); }
    void AcquireGLObject(cl_mem mem);
    void ReleaseGLObject(cl_mem mem);

    const cl::Context& GetContext() const { return context_; }
    std::vector<cl::Device> const& GetDevices() const { return devices_; }
    void ReloadKernels();

private:
    cl::Platform platform_;
    std::vector<cl::Device> devices_;
    cl::Context context_;
    cl::CommandQueue queue_;
    std::vector<std::weak_ptr<CLKernel>> kernels_;
    std::string kernels_path_;

};

class CLKernel
{
public:
    CLKernel(CLContext const& cl_context, const char* filename, char const* kernel_name,
        std::vector<std::string> const& definitions = std::vector<std::string>());
    void Reload();

    void SetArgument(std::uint32_t arg_index, cl_mem buffer);
    void SetArgument(std::uint32_t arg_index, cl::Buffer buffer);
    void SetArgument(std::uint32_t arg_index, void const* data, std::size_t size);
    const cl::Kernel& GetKernel() const { return kernel_; }
    std::string const& GetName() const { return kernel_name_; }

private:
    struct KernelArg
    {
        void const* data;
        std::size_t size;

        enum class ArgType
        {
            kBuffer,
            kConstant
        } arg_type;
    };

    CLContext const& context_;
    std::string filename_;
    std::string kernel_name_;
    std::vector<std::string> definitions_;
    cl::Kernel kernel_;
    std::unordered_map<std::uint32_t, KernelArg> kernel_args_;
};
