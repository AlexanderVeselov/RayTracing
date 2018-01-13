#ifndef DEVCONTEXT_HPP
#define DEVCONTEXT_HPP

#include "scene.hpp"
#include <CL/cl.hpp>
#include <memory>

// OpenCL & OpenGL interloperability
// https://software.intel.com/en-us/articles/opencl-and-opengl-interoperability-tutorial

class CLKernel;

class CLContext
{
public:
    CLContext(const cl::Platform& platform);

    void WriteBuffer(const cl::Buffer& buffer, const void* data, size_t size) const;
    void ReadBuffer(const cl::Buffer& buffer, void* ptr, size_t size) const;
    void ExecuteKernel(std::shared_ptr<CLKernel> kernel, size_t workSize) const;
    void Finish() const { m_Queue.finish(); }

    const cl::Context& GetContext() const { return m_Context; }

private:
    cl::Context m_Context;
    cl::CommandQueue m_Queue;

};

class CLKernel
{
public:
    CLKernel(const char* filename, const std::vector<cl::Device>& devices);
    void SetArgument(size_t argIndex, const void* data, size_t size);
    const cl::Kernel& GetKernel() const { return m_Kernel; }

private:
    cl::Kernel  m_Kernel;
    cl::Program m_Program;

};

#endif // DEVCONTEXT_HPP
