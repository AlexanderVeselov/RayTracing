#include "cl_context.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

ClContext::ClContext(size_t global_work_size, size_t cell_resolution)
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
        std::cerr << " No devices found. Check OpenCL installation!" << std::endl;

    }
    cl::Device default_device = all_devices[0];
    std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << std::endl;

    cl::Context context = cl::Context(default_device);
    
    std::ifstream input_file("src/kernel.cl");
    if (!input_file)
    {
        std::cerr << "Cannot load kernel file" << std::endl;

    }
    
    cl::Program::Sources sources;

    std::ostringstream buf;
    buf << "#define GRID_RES " << cell_resolution << std::endl;

    std::string curr_line;
    std::string source;
    source += buf.str() + "\n";
    while (std::getline(input_file, curr_line))
    {
        source += curr_line + "\n";
    }
    sources.push_back(std::make_pair(source.c_str(), source.length()));
    
    cl::Program program(context, sources);
    if (program.build(all_devices) != CL_SUCCESS)
    {
        std::cerr << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;

    }

    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;

    cl_int errCode;
	kernel = cl::Kernel(program, "main", &errCode);
	std::cout << "KernelCode: " << errCode << std::endl;
	
    queue = cl::CommandQueue(context, default_device, 0, 0);
    
}

void ClContext::SetupBuffers(size_t width, size_t height, const Scene& scene)
{
    random_array = new int[width * height];

    for (size_t i = 0; i < width * height; ++i)
    {
        random_array[i] = rand();
    }

    pixel_buffer  = cl::Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(cl_float4));
    random_buffer = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, width * height * sizeof(cl_int), random_array);
    
    scene_buffer  = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Triangle) * scene.triangles.size(), (void* ) scene.triangles.data());
    index_buffer  = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * scene.indices.size(), (void* ) scene.indices.data());
    cell_buffer   = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(CellData) * scene.cells.size(), (void* ) scene.cells.data());
    
	kernel.setArg(0, sizeof(cl::Buffer), &pixel_buffer);
	kernel.setArg(1, sizeof(cl::Buffer), &random_buffer);
	
	kernel.setArg(2, sizeof(unsigned int), &width);
	kernel.setArg(3, sizeof(unsigned int), &height);
	
	kernel.setArg(7, sizeof(cl::Buffer), &scene_buffer);
	kernel.setArg(8, sizeof(cl::Buffer), &index_buffer);
	kernel.setArg(9, sizeof(cl::Buffer), &cell_buffer);
}
