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

#include "cl_context.hpp"
#include "utils/cl_exception.hpp"
#include "render.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

CLContext::CLContext(const cl::Platform& platform)
    : platform_(platform)
    , kernels_path_("src/kernels/cl/")
{
    std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    cl_context_properties props[] =
    {
        // OpenCL platform
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
        // OpenGL context
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
        // HDC used to create the OpenGL context
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
        0
    };

    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices_);
    if (devices_.empty())
    {
        throw std::runtime_error("No devices found!");
    }

    for (unsigned int i = 0; i < devices_.size(); ++i)
    {
        std::cout << "Device: " << std::endl;
        std::cout << devices_[i].getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "Status: " << (devices_[i].getInfo<CL_DEVICE_AVAILABLE>() ? "Available" : "Not available") << std::endl;
        std::cout << "Max compute units: " << devices_[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
        std::cout << "Max workgroup size: " << devices_[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
        std::cout << "Max constant buffer size: " << devices_[i].getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() << std::endl;
        //std::cout << "Extensions: " << platform_devices[i].getInfo<CL_DEVICE_EXTENSIONS>() << std::endl;
        std::cout << "Image support: " << (devices_[i].getInfo<CL_DEVICE_IMAGE_SUPPORT>() ? "Yes" : "No") << std::endl;
        std::cout << "2D Image max width: " << devices_[i].getInfo<CL_DEVICE_IMAGE2D_MAX_WIDTH>() << std::endl;
        std::cout << "2D Image max height: " << devices_[i].getInfo<CL_DEVICE_IMAGE2D_MAX_HEIGHT>() << std::endl;
        std::cout << "Preferred vector width: " << devices_[i].getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>() << std::endl;
    }

    cl_int status;
    context_ = cl::Context(devices_, props, 0, 0, &status);
    ThrowIfFailed(status, "Failed to create OpenCL context");

    queue_ = cl::CommandQueue(context_, devices_[0], 0, &status);
    ThrowIfFailed(status, "Failed to create queue");

    std::cout << "Successfully created context " << std::endl;

}

void CLContext::WriteBuffer(const cl::Buffer& buffer, const void* data, size_t size) const
{
    cl_int status = queue_.enqueueWriteBuffer(buffer, true, 0, size, data);
    ThrowIfFailed(status, "Failed to write buffer");
}

void CLContext::ReadBuffer(const cl::Buffer& buffer, void* data, size_t size) const
{
    cl_int status = queue_.enqueueReadBuffer(buffer, false, 0, size, data);
    ThrowIfFailed(status, "Failed to read buffer");
}

void CLContext::CopyBuffer(const cl::Buffer& src_buffer, const cl::Buffer& dst_buffer,
    std::size_t src_offset, std::size_t dst_offset, std::size_t size) const
{
    cl_int status = queue_.enqueueCopyBuffer(src_buffer, dst_buffer, src_offset, dst_offset, size);
    ThrowIfFailed(status, "Failed to copy buffer");
}

void CLContext::ExecuteKernel(CLKernel const& kernel, std::size_t work_size) const
{
    cl_int status = queue_.enqueueNDRangeKernel(kernel.GetKernel(), cl::NullRange, cl::NDRange(work_size), cl::NullRange, 0);
    ThrowIfFailed(status, ("Failed to enqueue kernel " + kernel.GetName()).c_str());
}

void CLContext::AcquireGLObject(cl_mem mem)
{
    cl_int status = clEnqueueAcquireGLObjects(queue_(), 1, &mem, 0, 0, NULL);
    ThrowIfFailed(status, "Failed to acquire GL object");
}

void CLContext::ReleaseGLObject(cl_mem mem)
{
    cl_int status = clEnqueueReleaseGLObjects(queue_(), 1, &mem, 0, 0, NULL);
    ThrowIfFailed(status, "Failed to release GL object");
}

std::shared_ptr<CLKernel> CLContext::CreateKernel(const char* filename, char const* kernel_name,
    std::vector<std::string> const& definitions)
{
    std::shared_ptr<CLKernel> kernel = std::make_shared<CLKernel>(*this, (kernels_path_ + filename).c_str(), kernel_name, definitions);
    kernels_.push_back(kernel);
    return kernel;
}

void CLContext::ReloadKernels()
{
    try
    {
        // Erase expired kernels
        kernels_.erase(std::remove_if(kernels_.begin(), kernels_.end(),
            [](std::weak_ptr<CLKernel> ptr) { return ptr.expired(); }), kernels_.end());

        // Reload remaining
        for (auto kernel : kernels_)
        {
            kernel.lock()->Reload();
        }

        std::cout << "Kernels have been reloaded" << std::endl;
    }
    catch (std::exception const& ex)
    {
        std::cerr << "Failed to reload kernels:\n" << ex.what() << std::endl;
    }
}

CLKernel::CLKernel(CLContext const& cl_context, const char* filename, char const* kernel_name,
    std::vector<std::string> const& definitions)
    : context_(cl_context)
    , filename_(filename)
    , kernel_name_(kernel_name)
    , definitions_(definitions)
{
    Reload();
}

void CLKernel::Reload()
{
    std::ifstream input_file(filename_);

    if (!input_file)
    {
        throw std::runtime_error("Failed to load kernel file!");
    }

    // std::istreambuf_iterator s should be wrapped by brackets (wat?)
    std::string source((std::istreambuf_iterator<char>(input_file)), (std::istreambuf_iterator<char>()));

    cl_int status;
    cl::Program program(context_.GetContext(), source, false, &status);
    ThrowIfFailed(status, ("Failed to create program from file " + std::string(filename_)).c_str());

    std::string build_options = "-I . -I src/kernels/cl/";

    for (auto const& definition : definitions_)
    {
        build_options += " -D " + definition;
    }

    status = program.build(build_options.c_str());
    ThrowIfFailed(status, ("Error building " + std::string(filename_) + ": " + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(context_.GetDevices()[0])).c_str());

    kernel_ = cl::Kernel(program, kernel_name_.c_str(), &status);
    ThrowIfFailed(status, ("Failed to create kernel " + std::string(kernel_name_)).c_str());

    // Bind previously bound data
    for (auto const& arg : kernel_args_)
    {
        bool is_buffer = arg.second.arg_type == KernelArg::ArgType::kBuffer;
        // For cl_mem, this function takes a reference
        cl_int status = kernel_.setArg(arg.first, arg.second.size, (void*)(is_buffer ? &arg.second.data : arg.second.data));
        ThrowIfFailed(status, (kernel_name_ + ": failed to set kernel argument #" + std::to_string(arg.first)).c_str());
    }
}

void CLKernel::SetArgument(std::uint32_t arg_index, void const* data, std::size_t size)
{
    kernel_args_[arg_index] = KernelArg{ data, size, KernelArg::ArgType::kConstant };
    cl_int status = kernel_.setArg(arg_index, size, (void*)data);
    ThrowIfFailed(status, (kernel_name_ + ": failed to set kernel argument #" + std::to_string(arg_index)).c_str());
}

void CLKernel::SetArgument(std::uint32_t arg_index, cl_mem buffer)
{
    kernel_args_[arg_index] = KernelArg{ buffer, sizeof(cl_mem), KernelArg::ArgType::kBuffer };
    cl_int status = kernel_.setArg(arg_index, sizeof(cl_mem), &buffer);
    ThrowIfFailed(status, (kernel_name_ + ": failed to set kernel argument #" + std::to_string(arg_index)).c_str());
}

void CLKernel::SetArgument(std::uint32_t arg_index, cl::Buffer buffer)
{
    SetArgument(arg_index, buffer());
}
