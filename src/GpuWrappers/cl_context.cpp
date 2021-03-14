#include "cl_context.hpp"
#include "utils/cl_exception.hpp"
#include "render.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

CLContext::CLContext(const cl::Platform& platform, HDC display_context, HGLRC gl_context)
    : platform_(platform)
{
    std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    cl_context_properties props[] =
    {
        // OpenCL platform
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
        // OpenGL context
        CL_GL_CONTEXT_KHR, (cl_context_properties)gl_context,
        // HDC used to create the OpenGL context
        CL_WGL_HDC_KHR, (cl_context_properties)display_context,
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

    cl_int errCode;
    m_Context = cl::Context(devices_, props, 0, 0, &errCode);

    if (errCode)
    {
        throw CLException("Failed to create context", errCode);
    }

    m_Queue = cl::CommandQueue(m_Context, devices_[0], 0, &errCode);

    if (errCode)
    {
        throw CLException("Failed to create queue", errCode);
    }

    std::cout << "Successfully created context " << std::endl;

}

void CLContext::WriteBuffer(const cl::Buffer& buffer, const void* data, size_t size) const
{
    cl_int errCode = m_Queue.enqueueWriteBuffer(buffer, true, 0, size, data);
    if (errCode)
    {
        throw CLException("Failed to write buffer", errCode);
    }
}

void CLContext::ReadBuffer(const cl::Buffer& buffer, void* data, size_t size) const
{
    cl_int errCode = m_Queue.enqueueReadBuffer(buffer, false, 0, size, data);
    if (errCode)
    {
        throw CLException("Failed to read buffer", errCode);
    }
}

void CLContext::ExecuteKernel(std::shared_ptr<CLKernel> kernel, size_t workSize) const
{
    cl_int errCode = m_Queue.enqueueNDRangeKernel(kernel->GetKernel(), cl::NullRange, cl::NDRange(workSize), cl::NullRange, 0);
    if (errCode)
    {
        throw CLException("Failed to enqueue kernel", errCode);
    }

}

void CLContext::AcquireGLObject(cl_mem mem)
{
    clEnqueueAcquireGLObjects(m_Queue(), 1, &mem, 0, 0, NULL);
}

void CLContext::ReleaseGLObject(cl_mem mem)
{
    clEnqueueReleaseGLObjects(m_Queue(), 1, &mem, 0, 0, NULL);
}

CLKernel::CLKernel(const char* filename, const CLContext& cl_context)
{
    std::ifstream input_file(filename);

    if (!input_file)
    {
        throw std::exception("Failed to load kernel file!");
    }
    
    // std::istreambuf_iterator s should be wrapped by brackets (wat?)
    std::string source((std::istreambuf_iterator<char>(input_file)), (std::istreambuf_iterator<char>()));

    cl_int status;
    cl::Program program(cl_context.GetContext(), source, true, &status);

    if (status != CL_SUCCESS)
    {
        throw CLException("Error building" + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
            cl_context.GetDevices()[0]), status);
    }

    m_Kernel = cl::Kernel(program, "KernelEntry", &status);
    if (status)
    {
        throw CLException("Failed to create kernel", status);
    }

}

void CLKernel::SetArgument(std::uint32_t argIndex, void const* data, size_t size)
{
    cl_int errCode = m_Kernel.setArg(argIndex, size, data);

    if (errCode)
    {
        throw CLException("Failed to set kernel argument", errCode);
    }
}
