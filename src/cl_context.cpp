#include "cl_context.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

ClContext::ClContext(const cl::Platform& platform, const std::string& source, size_t width, size_t height, size_t cell_resolution) : m_Width(width), m_Height(height), m_Valid(true)
{
    std::string platformName(platform.getInfo<CL_PLATFORM_NAME>());
    std::cout << "Platform: " << platformName << std::endl;
    //std::ofstream output("log/" + platformName.substr(0, platformName.size() - 1) + ".txt");

    std::vector<cl::Device> platform_devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);

    if (platform_devices.size() == 0)
    {
        std::cerr << "No devices found!" << std::endl;
        m_Valid = false;
    }

    for (int i = 0; i < platform_devices.size(); ++i)
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
        std::cerr << "Failed to create context! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }
    
    cl::Program program(m_Context, source);

    errCode = program.build(platform_devices);
    if (errCode != CL_SUCCESS)
    {
        std::cerr << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]) << " (" << errCode << ")" << std::endl;
        m_Valid = false;
    }
    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]) << std::endl;

    m_Kernel = cl::Kernel(program, "main", &errCode);
    if (errCode)
    {
	    std::cerr << "Failed to create kernel! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }
    
    m_Queue = cl::CommandQueue(m_Context, platform_devices[0], 0, &errCode);
    if (errCode)
    {
	    std::cerr << "Failed to create queue! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }

	std::cout << "Context successfully created" << std::endl;

}

void ClContext::SetupBuffers(const Scene& scene)
{
    size_t global_work_size = m_Width * m_Height;

    cl_int errCode;
    m_PixelBuffer  = cl::Buffer(m_Context, CL_MEM_READ_WRITE, global_work_size * sizeof(cl_float4), 0, &errCode);
    if (errCode)
    {
        std::cerr << "Failed to create pixel buffer! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }

    m_RandomBuffer = cl::Buffer(m_Context, CL_MEM_READ_ONLY, global_work_size * sizeof(cl_int), 0, &errCode);
    if (errCode)
    {
        std::cerr << "Failed to create random buffer! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }

    m_SceneBuffer  = cl::Buffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.triangles.size() * sizeof(Triangle), (void*) scene.triangles.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Failed to create scene buffer! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }

    m_IndexBuffer  = cl::Buffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.indices.size() * sizeof(cl_uint), (void*) scene.indices.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Failed to create index buffer! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }

    m_CellBuffer   = cl::Buffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.cells.size() * sizeof(CellData), (void*) scene.cells.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Failed to create cell buffer! (" << errCode << ")" << std::endl;
        m_Valid = false;
    }
    
	SetArgument(0, sizeof(cl::Buffer), &m_PixelBuffer);
	SetArgument(1, sizeof(cl::Buffer), &m_RandomBuffer);
	
	SetArgument(2, sizeof(size_t), &m_Width);
	SetArgument(3, sizeof(size_t), &m_Height);
	
	SetArgument(7, sizeof(cl::Buffer), &m_SceneBuffer);
	SetArgument(8, sizeof(cl::Buffer), &m_IndexBuffer);
	SetArgument(9, sizeof(cl::Buffer), &m_CellBuffer);
    
}

void ClContext::SetArgument(size_t index, size_t size, const void* argPtr)
{
    m_Kernel.setArg(index, size, argPtr);
}

void ClContext::WriteRandomBuffer(size_t size, const void* ptr)
{
    m_Queue.enqueueWriteBuffer(m_RandomBuffer, true, 0, size, ptr);
}

void ClContext::ExecuteKernel(cl_float4* ptr, size_t size, size_t offset)
{
    std::vector<cl::Event> events(1);
    m_Queue.enqueueNDRangeKernel(m_Kernel, cl::NDRange(offset), cl::NDRange(size), cl::NullRange, 0, &events[0]);
    m_Queue.enqueueReadBuffer(m_PixelBuffer, false, offset * sizeof(cl_float4), size * sizeof(cl_float4), ptr + offset, &events);
    m_Queue.finish();

}
