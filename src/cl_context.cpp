#include "cl_context.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

ClContext::ClContext(size_t width, size_t height, size_t cell_resolution) : width_(width), height_(height)
{
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0)
    {
        std::cerr << "No platforms found. Check OpenCL installation!" << std::endl;

    }
    cl::Platform default_platform = all_platforms[0];
    std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if (all_devices.size() == 0)
    {
        std::cerr << "No devices found. Check OpenCL installation!" << std::endl;
    }
    cl::Device default_device = all_devices[0];
    std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << std::endl;

    context_ = cl::Context(default_device);
    
    std::ifstream input_file("src/kernel.cl");
    if (!input_file)
    {
        std::cerr << "Cannot load kernel file" << std::endl;
    }
    
    std::ostringstream buf;
    buf << "#define GRID_RES " << cell_resolution << std::endl;

    std::string curr_line;
    std::string source;
    source += buf.str() + "\n";
    while (std::getline(input_file, curr_line))
    {
        source += curr_line + "\n";
    }

    cl::Program::Sources sources;
    sources.push_back(std::make_pair(source.c_str(), source.length()));
    
    cl::Program program(context_, sources);
    if (program.build(all_devices) != CL_SUCCESS)
    {
        std::cerr << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
    }

    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;

    cl_int errCode;
	kernel_ = cl::Kernel(program, "main", &errCode);
    if (errCode)
    {
	    std::cout << "Cannot create kernel! (" << errCode << ")" << std::endl;
    }
	
    queue_ = cl::CommandQueue(context_, default_device, 0, &errCode);
    if (errCode)
    {
	    std::cout << "Cannot create queue! (" << errCode << ")" << std::endl;
    }
}

void ClContext::setupBuffers(const Scene& scene)
{
    // Memory leak!
    size_t global_work_size = width_ * height_;
    int* random_array = new int[global_work_size];

    for (size_t i = 0; i < width_ * height_; ++i)
    {
        random_array[i] = rand();
    }

    cl_int errCode;
    pixel_buffer_  = cl::Buffer(context_, CL_MEM_READ_WRITE, global_work_size * sizeof(cl_float4), 0, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create pixel buffer! (" << errCode << ")" << std::endl;
    }

    random_buffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, global_work_size * sizeof(cl_int), random_array, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create random buffer! (" << errCode << ")" << std::endl;
    }

    scene_buffer_  = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.triangles.size() * sizeof(Triangle), (void* ) scene.triangles.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create scene buffer! (" << errCode << ")" << std::endl;
    }

    index_buffer_  = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.indices.size() * sizeof(cl_uint), (void* ) scene.indices.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create index buffer! (" << errCode << ")" << std::endl;
    }

    cell_buffer_   = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.cells.size() * sizeof(CellData), (void* ) scene.cells.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create cell buffer! (" << errCode << ")" << std::endl;
    }
    
	setArgument(0, sizeof(cl::Buffer), &pixel_buffer_);
	setArgument(1, sizeof(cl::Buffer), &random_buffer_);
	
	setArgument(2, sizeof(unsigned int), &width_);
	setArgument(3, sizeof(unsigned int), &height_);
	
	setArgument(7, sizeof(cl::Buffer), &scene_buffer_);
	setArgument(8, sizeof(cl::Buffer), &index_buffer_);
	setArgument(9, sizeof(cl::Buffer), &cell_buffer_);
    
    writeBuffer(random_buffer_, global_work_size * sizeof(int), random_array);

}

void ClContext::setArgument(size_t index, size_t size, const void* argPtr)
{
    kernel_.setArg(index, size, argPtr);
}

void ClContext::writeBuffer(const cl::Buffer& buffer, size_t size, const void* ptr)
{
    queue_.enqueueWriteBuffer(buffer, true, 0, size, ptr);
	queue_.finish();

}

void ClContext::executeKernel()
{
    queue_.enqueueNDRangeKernel(kernel_, cl::NullRange, cl::NDRange(width_ * height_));
	queue_.finish();

}

cl_float4* ClContext::getPixels()
{
    cl_float4* ptr = static_cast<cl_float4*>(queue_.enqueueMapBuffer(pixel_buffer_, CL_TRUE, CL_MAP_READ, 0, width_ * height_ * sizeof(cl_float4)));
    queue_.finish();
    return ptr;

}

void ClContext::unmapPixels(cl_float4* ptr)
{
    queue_.enqueueUnmapMemObject(pixel_buffer_, ptr);
	queue_.finish();

}
