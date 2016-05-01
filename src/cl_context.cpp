#include "cl_context.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

ClContext::ClContext(const cl::Platform& platform, const std::string& source, size_t width, size_t height, size_t cell_resolution) : width_(width), height_(height), valid_(true)
{
    std::string platformName(platform.getInfo<CL_PLATFORM_NAME>());
    std::cout << "Platform: " << platformName << std::endl;
    std::ofstream output("log/" + platformName.substr(0, platformName.size() - 1) + ".txt");

    std::vector<cl::Device> platform_devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);

    if (platform_devices.size() == 0)
    {
        std::cerr << "No devices found!" << std::endl;
        valid_ = false;
    }

    for (int i = 0; i < platform_devices.size(); ++i)
    {
        output << "Device: " << std::endl;
        output << platform_devices[i].getInfo<CL_DEVICE_NAME>() << std::endl;
        output << "Status: " << (platform_devices[i].getInfo<CL_DEVICE_AVAILABLE>() ? "Available" : "Not available") << std::endl;
        output << "Max compute units: " << platform_devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
        output << "Max workgroup size: " << platform_devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
    }

    cl_int errCode;
    context_ = cl::Context(platform_devices, 0, 0, 0, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create context! (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    
    cl::Program program(context_, source);

    errCode = program.build(platform_devices);
    if (errCode != CL_SUCCESS)
    {
        std::cerr << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]) << " (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]) << std::endl;

    kernel_ = cl::Kernel(program, "main", &errCode);
    if (errCode)
    {
	    std::cerr << "Cannot create kernel! (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    
    queue_ = cl::CommandQueue(context_, platform_devices[0], 0, &errCode);
    if (errCode)
    {
	    std::cerr << "Cannot create queue! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

}

void ClContext::setupBuffers(const Scene& scene)
{
    size_t global_work_size = width_ * height_;

    cl_int errCode;
    pixel_buffer_  = cl::Buffer(context_, CL_MEM_READ_WRITE, global_work_size * sizeof(cl_float4), 0, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create pixel buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    random_buffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY, global_work_size * sizeof(cl_int), 0, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create random buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    scene_buffer_  = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.triangles.size() * sizeof(Triangle), (void*) scene.triangles.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create scene buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    index_buffer_  = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.indices.size() * sizeof(cl_uint), (void*) scene.indices.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create index buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    cell_buffer_   = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.cells.size() * sizeof(CellData), (void*) scene.cells.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create cell buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    
	setArgument(0, sizeof(cl::Buffer), &pixel_buffer_);
	setArgument(1, sizeof(cl::Buffer), &random_buffer_);
	
	setArgument(2, sizeof(size_t), &width_);
	setArgument(3, sizeof(size_t), &height_);
	
	setArgument(7, sizeof(cl::Buffer), &scene_buffer_);
	setArgument(8, sizeof(cl::Buffer), &index_buffer_);
	setArgument(9, sizeof(cl::Buffer), &cell_buffer_);
    
}

void ClContext::setArgument(size_t index, size_t size, const void* argPtr)
{
    kernel_.setArg(index, size, argPtr);
}

void ClContext::writeRandomBuffer(size_t size, const void* ptr)
{
    queue_.enqueueWriteBuffer(random_buffer_, true, 0, size, ptr);

}

void ClContext::executeKernel(cl_float4* ptr, size_t size, size_t offset)
{
    std::vector<cl::Event> events(1);
    queue_.enqueueNDRangeKernel(kernel_, cl::NDRange(offset), cl::NDRange(size), cl::NullRange, 0, &events[0]);
    queue_.enqueueReadBuffer(pixel_buffer_, false, offset * sizeof(cl_float4), size * sizeof(cl_float4), ptr + offset, &events);
    queue_.finish();

}
