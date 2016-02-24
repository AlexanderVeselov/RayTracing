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
#include <sstream>

const int g_Width = 1280;
const int g_Height = 720;
const size_t global_work_size = g_Width * g_Height;

Camera camera(glm::vec3(0.0f), 0.001f, 1.0f, g_Width, g_Height, 90.0f);

cl_command_queue queue;
cl_kernel kernel;
cl_mem pixel_buffer, random_buffer, scene_buffer, index_buffer, cell_buffer;

//std::vector<Sphere> spheres;
std::vector<Triangle> triangles;
std::vector<cl_uint> indices;
std::vector<CellData> cells;
int* random_array;
unsigned long int sample_count = 0;

const size_t cell_resolution = 64;
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
    source += "#define GRID_RES " + static_cast<std::ostringstream*>( &(std::ostringstream() << cell_resolution) )->str();
    source += "\n";
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

    random_array = new int[global_work_size];

    for (size_t i = 0; i < global_work_size; ++i)
    {
        random_array[i] = rand();
    }

	pixel_buffer  = clCreateBuffer(context, CL_MEM_READ_WRITE, g_Width*g_Height*sizeof(cl_float4), 0, 0);
	random_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, g_Width*g_Height*sizeof(cl_int), random_array, 0);
    scene_buffer  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Triangle) * triangles.size(), triangles.begin()._Ptr, 0);
	index_buffer  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * indices.size(), indices.begin()._Ptr, 0);
	cell_buffer   = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(CellData) * cells.size(), cells.begin()._Ptr, 0);
	
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &pixel_buffer);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &random_buffer);
	clSetKernelArg(kernel, 2, sizeof(cl_uint), &g_Width);
	clSetKernelArg(kernel, 3, sizeof(cl_uint), &g_Height);
	clSetKernelArg(kernel, 7, sizeof(cl_mem), &scene_buffer);
	clSetKernelArg(kernel, 8, sizeof(cl_mem), &index_buffer);
	clSetKernelArg(kernel, 9, sizeof(cl_mem), &cell_buffer);

}

void Update(GLFWwindow *window)
{
	camera.Update(window, 1.0f);
}

void Render()
{
	float3 pos(camera.GetPos().x, camera.GetPos().y, camera.GetPos().z);
	clSetKernelArg(kernel, 4, sizeof(float3), &pos);
	float3 front(camera.GetFrontVec().x, camera.GetFrontVec().y, camera.GetFrontVec().z);
	clSetKernelArg(kernel, 5, sizeof(float3), &front);
	float3 up(camera.GetUpVec().x, camera.GetUpVec().y, camera.GetUpVec().z);
	clSetKernelArg(kernel, 6, sizeof(float3), &up);
    
	clEnqueueWriteBuffer(queue, random_buffer, true, 0, sizeof(int) * global_work_size, random_array, 0, 0, 0);
	clFinish(queue);
    
	clEnqueueNDRangeKernel(queue, kernel, 1, 0, &global_work_size, 0, 0, 0, 0);
	clFinish(queue);

    cl_float4* ptr = (cl_float4*) clEnqueueMapBuffer(queue, pixel_buffer, CL_TRUE, CL_MAP_READ, 0, global_work_size*sizeof(cl_float4), 0, 0, 0, 0);

    clEnqueueUnmapMemObject(queue, pixel_buffer, ptr, 0, 0, 0);
    
    
    if (camera.Changed())
    {
        for (size_t i = 0; i < global_work_size; ++i)
        {
            ptr[i].x = 0;
            ptr[i].y = 0;
            ptr[i].z = 0;
            sample_count = 0;
        }
        clEnqueueWriteBuffer(queue, pixel_buffer, true, 0, sizeof(cl_float4) * global_work_size, ptr, 0, 0, 0);
        clFinish(queue);
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
