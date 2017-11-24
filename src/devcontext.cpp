#include "devcontext.hpp"
#include "exception.hpp"
#include "render.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <Windows.h>

ClContext::ClContext(const cl::Platform& platform, const std::string& source, size_t width, size_t height, size_t cell_resolution) : m_Width(width), m_Height(height), m_Valid(true)
{
    std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    //cl_context_properties props[] =
    //{
    //    //OpenCL platform
    //    CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
    //    //OpenGL context
    //    CL_GL_CONTEXT_KHR, (cl_context_properties)hRC,
    //    //HDC used to create the OpenGL context
    //    CL_WGL_HDC_KHR, (cl_context_properties)GetDC(render),
    //    0
    //};

    std::vector<cl::Device> platform_devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);
    
    if (platform_devices.empty())
    {
        throw Exception("No devices found!");
    }

    for (unsigned int i = 0; i < platform_devices.size(); ++i)
    {
        std::cout << "Device: " << std::endl;
        std::cout << platform_devices[i].getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "Status: " << (platform_devices[i].getInfo<CL_DEVICE_AVAILABLE>() ? "Available" : "Not available") << std::endl;
        std::cout << "Max compute units: " << platform_devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
        std::cout << "Max workgroup size: " << platform_devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
    }
    
    cl_int errCode;
    m_Context = cl::Context(platform_devices, 0, 0, 0, &errCode);
    if (errCode)
    {
        throw ClException("Failed to create context", errCode);
    }
    
    cl::Program program(m_Context, source);

    errCode = program.build(platform_devices);
    if (errCode != CL_SUCCESS)
    {
        throw ClException("Error building" + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]), errCode);
    }
    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]) << std::endl;

    m_Kernel = cl::Kernel(program, "main", &errCode);
    if (errCode)
    {
        throw ClException("Failed to create kernel", errCode);
    }
    
    m_Queue = cl::CommandQueue(m_Context, platform_devices[0], 0, &errCode);
    if (errCode)
    {
        throw ClException("Failed to create queue", errCode);
    }

    std::cout << "Context successfully created" << std::endl;

}

void ClContext::SetupBuffers(std::shared_ptr<Scene> scene)
{
    size_t global_work_size = m_Width * m_Height;

    cl_int errCode;
    m_PixelBuffer = cl::Buffer(m_Context, CL_MEM_READ_ONLY, global_work_size * sizeof(cl_uint3), 0, &errCode);
    if (errCode)
    {
        throw ClException("Failed to pixel buffer", errCode);
    }

    m_RandomBuffer = cl::Buffer(m_Context, CL_MEM_READ_ONLY, global_work_size * sizeof(cl_int), 0, &errCode);
    if (errCode)
    {
        throw ClException("Failed to random buffer", errCode);
    }

    m_SceneBuffer = cl::Buffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene->triangles.size() * sizeof(Triangle), (void*)scene->triangles.data(), &errCode);
    if (errCode)
    {
        throw ClException("Failed to scene buffer", errCode);
    }

    m_IndexBuffer = cl::Buffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene->indices.size() * sizeof(cl_uint), (void*)scene->indices.data(), &errCode);
    if (errCode)
    {
        throw ClException("Failed to index buffer", errCode);
    }

    m_CellBuffer = cl::Buffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene->cells.size() * sizeof(CellData), (void*)scene->cells.data(), &errCode);
    if (errCode)
    {
        throw ClException("Failed to cell buffer", errCode);
    }
    
    SetArgument(CL_ARG_PIXEL_BUFFER, sizeof(cl::Buffer), &m_PixelBuffer);
    SetArgument(CL_ARG_RANDOM_BUFFER, sizeof(cl::Buffer), &m_RandomBuffer);
    
    SetArgument(CL_ARG_WIDTH, sizeof(size_t), &m_Width);
    SetArgument(CL_ARG_HEIGHT, sizeof(size_t), &m_Height);
    
    SetArgument(CL_ARG_SCENE_BUFFER, sizeof(cl::Buffer), &m_SceneBuffer);
    SetArgument(CL_ARG_INDEX_BUFFER, sizeof(cl::Buffer), &m_IndexBuffer);
    SetArgument(CL_ARG_CELL_BUFFER, sizeof(cl::Buffer), &m_CellBuffer);
        
}

void ClContext::SetArgument(size_t index, size_t size, const void* argPtr)
{
    cl_int errCode = m_Kernel.setArg(index, size, argPtr);
    if (errCode)
    {
        throw ClException("Failed to set kernel argument", errCode);
    }
}

void ClContext::WriteRandomBuffer(size_t size, const void* ptr)
{
    cl_int errCode = m_Queue.enqueueWriteBuffer(m_RandomBuffer, true, 0, size, ptr);
    if (errCode)
    {
        throw ClException("Failed to write random buffer", errCode);
    }
}

void ClContext::ExecuteKernel(float3* ptr, size_t size)
{
    cl_int errCode = m_Queue.enqueueNDRangeKernel(m_Kernel, cl::NullRange, cl::NDRange(size), cl::NullRange, 0);
    if (errCode)
    {
        throw ClException("Failed to enqueue kernel", errCode);
    }
    errCode = m_Queue.enqueueReadBuffer(m_PixelBuffer, false, 0, size * sizeof(float3), ptr);
    if (errCode)
    {
        throw ClException("Failed to read pixel buffer", errCode);
    }
    m_Queue.finish();

}
