#define GLFW_DLL

#include "camera.hpp"
#include "vectors.hpp"
#include "scene.hpp"
#include "grid.hpp"

#include <GLFW/glfw3.h>
#include <CL/cl.hpp>

#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>

const int g_Width = 1280;
const int g_Height = 720;
const size_t global_work_size = g_Width * g_Height;

Camera camera(glm::vec3(0.0f), 0.001f, 1.0f, g_Width, g_Height, 90.0f);

cl::CommandQueue queue;
cl::Kernel kernel;
cl_mem pixel_buffer, random_buffer, scene_buffer, index_buffer, cell_buffer;

//std::vector<Sphere> spheres;
std::vector<Triangle> triangles;
std::vector<cl_uint> indices;
std::vector<CellData> cells;
int* random_array;
unsigned long int sample_count = 0;

const size_t cell_resolution = 16;
const size_t cell_resolution3 = cell_resolution * cell_resolution * cell_resolution;

#define PRINT_SIZE(x)   std::cout << "sizeof(" << #x << "): " << sizeof(x) << std::endl

bool InitCL()
{
    //get all platforms (drivers)
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0)
    {
        std::cerr << " No platforms found. Check OpenCL installation!" << std::endl;
        return false;
    }
    cl::Platform default_platform = all_platforms[0];
    std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if (all_devices.size() == 0)
    {
        std::cerr << " No devices found. Check OpenCL installation!" << std::endl;
        return false;
    }
    cl::Device default_device = all_devices[0];
    std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << std::endl;

    cl::Context context({default_device});
    queue = cl::CommandQueue(context, default_device, 0, 0);

    std::ifstream input_file("kernel1.cl");
    if (!input_file)
    {
        std::cerr << "Cannot load kernel file" << std::endl;
        return false;
    }

    PRINT_SIZE(int);
    PRINT_SIZE(cl_int);
    PRINT_SIZE(cl_float4);
    PRINT_SIZE(float3);
    PRINT_SIZE(Triangle);
    PRINT_SIZE(CellData);

    cl::Program::Sources sources;

    //std::ostringstream buf;
    //buf << "#define GRID_RES " << cell_resolution << std::endl;
    //sources.push_back({buf.str().c_str(), buf.str().length()});

    std::string curr_line;
    while (std::getline(input_file, curr_line))
    {
        curr_line += "\n";
    	std::cout << curr_line;
        sources.push_back({curr_line.c_str(), curr_line.length()});
    }

    cl::Program program(context, sources);
    if (program.build({default_device}) != CL_SUCCESS)
    {
        std::cerr << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
        return false;
    }

    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;

    cl_int errCode;
	kernel = cl::Kernel(program, "main", &errCode);
	std::cout << "KernelCode: " << errCode << std::endl;
	
/*
    random_array = new int[global_work_size];

	pixel_buffer  = clCreateBuffer(context(), CL_MEM_READ_WRITE, g_Width*g_Height*sizeof(cl_float4), 0, 0);
	random_buffer = clCreateBuffer(context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, g_Width*g_Height*sizeof(cl_int), random_array, 0);
    scene_buffer  = clCreateBuffer(context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Triangle) * triangles.size(), triangles.data(), 0);
	index_buffer  = clCreateBuffer(context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * indices.size(), indices.data(), 0);
	cell_buffer   = clCreateBuffer(context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(CellData) * cells.size(), cells.data(), 0);

	kernel.setArg(0, pixel_buffer);
	kernel.setArg(1, &random_buffer);
	kernel.setArg(2, &g_Width);
	kernel.setArg(3, &g_Height);
	kernel.setArg(7, &scene_buffer);
	kernel.setArg(8, &index_buffer);
	kernel.setArg(9, &cell_buffer);
*/
    std::cout << __FUNCTION__ << " : SUCCESS" << std::endl;
    return true;
}

void Update(GLFWwindow *window)
{
	camera.Update(window, 1.0f);
}

void Render()
{
	/*
	float3 pos(camera.GetPos().x, camera.GetPos().y, camera.GetPos().z);
	clSetKernelArg(kernel, 4, sizeof(float3), &pos);
	float3 front(camera.GetFrontVec().x, camera.GetFrontVec().y, camera.GetFrontVec().z);
	clSetKernelArg(kernel, 5, sizeof(float3), &front);
	float3 up(camera.GetUpVec().x, camera.GetUpVec().y, camera.GetUpVec().z);
	clSetKernelArg(kernel, 6, sizeof(float3), &up);

	clEnqueueWriteBuffer(queue, random_buffer, true, 0, sizeof(int) * global_work_size, random_array, 0, 0, 0);
	clFinish(queue);
    */
	//std::cout << queue.enqueueNDRangeKernel(kernel, cl::NDRange(1), cl::NullRange, cl::NDRange(global_work_size)) << std::endl;
	//queue.finish();

    cl_float4* ptr = 0;//(cl_float4*) clEnqueueMapBuffer(queue, pixel_buffer, CL_TRUE, CL_MAP_READ, 0, global_work_size*sizeof(cl_float4), 0, 0, 0, 0);

    if (!ptr)
	{
		//std::cout << "Could not enqueue map buffer!" << std::endl;
		return;
	}

   // clEnqueueUnmapMemObject(queue, pixel_buffer, ptr, 0, 0, 0);
	//queue.finish();
    
    if (camera.Changed())
    {
        for (size_t i = 0; i < global_work_size; ++i)
        {
            ptr[i].x = 0;
            ptr[i].y = 0;
            ptr[i].z = 0;
            sample_count = 0;
        }
       // clEnqueueWriteBuffer(queue, pixel_buffer, true, 0, sizeof(cl_float4) * global_work_size, ptr, 0, 0, 0);
       // clFinish(queue);
    }
    ++sample_count;
    for (size_t i = 0; i < global_work_size; ++i)
    {
        random_array[i] = rand();
        ptr[i].x /= sample_count;
        ptr[i].y /= sample_count;
        ptr[i].z /= sample_count;
    }
    
    if (ptr)
    {
        glDrawPixels(g_Width, g_Height, GL_RGBA, GL_FLOAT, ptr);
    }

}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	camera.KeyCallback(key, action);
}

static void CursorCallback(GLFWwindow *window, double xpos, double ypos)
{
	camera.CursorCallback(static_cast<float>(xpos), static_cast<float>(ypos));
}

int main()
{
    LoadTriangles("teapot.obj", triangles);
    
	CreateGrid(triangles, cell_resolution, indices, cells);
    
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

	InitCL();
    window = glfwCreateWindow(g_Width, g_Height, "Ray Tracing Test", NULL, NULL);

    glfwMakeContextCurrent(window);
	
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, CursorCallback);
    
    while (!glfwWindowShouldClose(window))
    {
		Update(window);
		// temp..
		glfwSetCursorPos(window, g_Width / 2, g_Height / 2);
		Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
