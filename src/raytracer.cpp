#include "raytracer.hpp"
#include "vectors.hpp"
#include <glm/glm.hpp>
#include <iostream>

RayTracer::RayTracer() :
    width(1280),
    height(720),
    context(1280 * 720, 16),
    camera(glm::vec3(), 0.0005f, 1.0f, width, height, 90.0f),
    scene("meshes/teapot.obj", 16)
{
    glfwInit();
    window = glfwCreateWindow(width, height, "Ray Tracing Test", NULL, NULL);
    glfwMakeContextCurrent(window);
	
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    std::cout << scene.triangles.size() << std::endl;
    context.SetupBuffers(width, height, scene);
	//glfwSetKeyCallback(window, (GLFWkeyfun) RayTracer::KeyCallback);
	//glfwSetCursorPosCallback(window, RayTracer::CursorCallback);
}

void RayTracer::MainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
	    camera.Update(window, 1.0f);
		// temp..
		glfwSetCursorPos(window, width / 2, height / 2);
		Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}

void RayTracer::Render()
{
    float3 pos(camera.GetPos().x, camera.GetPos().y, camera.GetPos().z);
	context.kernel.setArg(4, sizeof(float3), &pos);
	float3 front(camera.GetFrontVec().x, camera.GetFrontVec().y, camera.GetFrontVec().z);
	context.kernel.setArg(5, sizeof(float3), &front);
	float3 up(camera.GetUpVec().x, camera.GetUpVec().y, camera.GetUpVec().z);
	context.kernel.setArg(6, sizeof(float3), &up);

	context.queue.enqueueWriteBuffer(context.random_buffer, true, 0, sizeof(int) * width * height, context.random_array);
	context.queue.finish();

	context.queue.enqueueNDRangeKernel(context.kernel, cl::NullRange, cl::NDRange(width * height));
	context.queue.finish();

    cl_float4* ptr = (cl_float4* ) context.queue.enqueueMapBuffer(context.pixel_buffer, CL_TRUE, CL_MAP_READ, 0, width * height * sizeof(cl_float4));
    
    if (ptr)
    {
        glDrawPixels(width, width, GL_RGBA, GL_FLOAT, ptr);
    }

    context.queue.enqueueUnmapMemObject(context.pixel_buffer, ptr);
	context.queue.finish();
}

void RayTracer::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	camera.KeyCallback(key, action);
}

void RayTracer::CursorCallback(GLFWwindow *window, double xpos, double ypos)
{
	camera.CursorCallback(static_cast<float>(xpos), static_cast<float>(ypos));
}
