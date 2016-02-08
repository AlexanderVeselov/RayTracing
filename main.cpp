#define GLFW_DLL
#include "camera.hpp"
#include "vectors.hpp"
#include "scene.hpp"
#include "grid.hpp"
#include <GLFW/glfw3.h>
#include <CL/cl.h>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

const int g_Width = 1920;
const int g_Height = 1080;
const size_t global_work_size = g_Width * g_Height;

Camera camera(glm::vec3(0.0f), 0.001f, 1.0f, g_Width, g_Height, 90.0f);

cl_command_queue queue;
cl_kernel kernel;
cl_mem pixel_buffer, scene_buffer, index_buffer, cell_buffer;

std::vector<Sphere> spheres;
std::vector<cl_uint> indices;
std::vector<CellData> cells;
const int spheres_x = 10;
const int spheres_y = 10;
const int spheres_z = 10;
const int sphere_count = spheres_x * spheres_y * spheres_z;
const size_t cell_resolution = 10;
const size_t cell_resolution3 = cell_resolution * cell_resolution * cell_resolution;

void InitCL()
{
	cl_platform_id platform_id;
	clGetPlatformIDs(1, &platform_id, 0);
	
	cl_device_id device_id;
	clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, 0);
	cl_context context = clCreateContext(0, 1, &device_id, 0, 0, 0);
	queue = clCreateCommandQueue(context, device_id, 0, 0);
	
	std::ifstream input_file("kernel.cl");
	if (!input_file)
	{
		std::cerr << "Cannot load kernel file" << std::endl;
		return;
	}
	
	std::string curr_line, source;
	while (std::getline(input_file, curr_line))
	{
		source += curr_line + "\n";
	}

	const char *str = source.c_str();
	cl_program program = clCreateProgramWithSource(context, 1, &str, 0, 0);
	cl_int result = clBuildProgram(program, 1, &device_id, 0, 0, 0);
	
    size_t len;
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, 0, &len);
    char *log = new char[len];
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, log, 0);
    std::cout << log << std::endl;
    delete[] log;

	if (result)
	{
		std::cerr << "Error during compilation! (" << result << ")" << std::endl;
	}

	kernel = clCreateKernel(program, "main", 0);

	pixel_buffer   = clCreateBuffer(context, CL_MEM_WRITE_ONLY, g_Width*g_Height*sizeof(cl_float4), 0, 0);
	scene_buffer   = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Sphere) * sphere_count, spheres.begin()._Ptr, 0);
	index_buffer   = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * indices.size(), indices.begin()._Ptr, 0);
	cell_buffer   = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(CellData) * cells.size(), cells.begin()._Ptr, 0);
	
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &pixel_buffer);
	clSetKernelArg(kernel, 1, sizeof(cl_uint), &g_Width);
	clSetKernelArg(kernel, 2, sizeof(cl_uint), &g_Height);
	clSetKernelArg(kernel, 6, sizeof(cl_mem), &scene_buffer);
	clSetKernelArg(kernel, 7, sizeof(cl_mem), &index_buffer);
	clSetKernelArg(kernel, 8, sizeof(cl_mem), &cell_buffer);

}

void Update(GLFWwindow *window)
{
	camera.Update(window, 1.0f);
}

void Render()
{
	cl_float4* ptr = (cl_float4*) clEnqueueMapBuffer(queue, pixel_buffer, CL_TRUE, CL_MAP_READ, 0, global_work_size*sizeof(cl_float4), 0, 0, 0, 0); 
	clEnqueueNDRangeKernel(queue, kernel, 1, 0, &global_work_size, 0, 0, 0, 0);

    clEnqueueUnmapMemObject(queue, pixel_buffer, ptr, 0, 0, 0);

	float3 pos(camera.GetPos().x, camera.GetPos().y, camera.GetPos().z);
	clSetKernelArg(kernel, 3, sizeof(float3), &pos);
	float3 front(camera.GetFrontVec().x, camera.GetFrontVec().y, camera.GetFrontVec().z);
	clSetKernelArg(kernel, 4, sizeof(float3), &front);
	float3 up(camera.GetUpVec().x, camera.GetUpVec().y, camera.GetUpVec().z);
	clSetKernelArg(kernel, 5, sizeof(float3), &up);



    //if (ptr)
    {
        glDrawPixels(g_Width, g_Height, GL_RGBA, GL_FLOAT, ptr);
    }	
	clFinish(queue);

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
	for (int x = 0; x < spheres_x; ++x)
	for (int y = 0; y < spheres_y; ++y)
	for (int z = 0; z < spheres_z; ++z)
	{
		/*
        spheres.push_back(Sphere(float3(
			x * 4, y * 4, z * 4)+2.0f,
			1.75f));
        */

        spheres.push_back(Sphere(float3(
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 80.0f,
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 80.0f,
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 80.0f),
            2.0f));

	}
	CreateGrid(spheres, cell_resolution, indices, cells);

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
