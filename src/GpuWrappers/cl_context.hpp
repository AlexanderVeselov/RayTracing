#ifndef CL_CONTEXT_HPP
#define CL_CONTEXT_HPP

#include "scene/scene.hpp"
#include <CL/cl.hpp>
#include <memory>
#include <Windows.h>

// OpenCL & OpenGL interop
// https://software.intel.com/en-us/articles/opencl-and-opengl-interoperability-tutorial

class CLKernel;

class CLContext
{
public:
    CLContext(const cl::Platform& platform, HDC display_context, HGLRC gl_context);

    void WriteBuffer(const cl::Buffer& buffer, const void* data, size_t size) const;
    void ReadBuffer(const cl::Buffer& buffer, void* ptr, size_t size) const;
    void ExecuteKernel(CLKernel const& kernel, std::size_t work_size) const;
    void Finish() const { m_Queue.finish(); }
    void AcquireGLObject(cl_mem mem);
    void ReleaseGLObject(cl_mem mem);

    const cl::Context& GetContext() const { return m_Context; }
    std::vector<cl::Device> const& GetDevices() const { return devices_; }

private:
    cl::Platform platform_;
    std::vector<cl::Device> devices_;
    cl::Context m_Context;
    cl::CommandQueue m_Queue;

};

class CLKernel
{
public:
    CLKernel(const char* filename, const CLContext& cl_context);
    void SetArgument(std::uint32_t argIndex, void const* data, size_t size);
    const cl::Kernel& GetKernel() const { return m_Kernel; }

private:
    cl::Kernel  m_Kernel;
    cl::Program m_Program;

};

#endif // CL_CONTEXT_HPP
