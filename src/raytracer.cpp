#include "raytracer.hpp"
#include "vectors.hpp"
#include <glm/glm.hpp>
#include <iostream>

RayTracer::RayTracer(const char* filename) :
    width_(1280),
    height_(720),
    context_(1280, 720, 64),
    camera_(width_, height_, 0.0, 90.0f, 0.0005f, 2.0f),
    scene_(filename, 64)
{
    glfwInit();
    window_ = glfwCreateWindow(width_, height_, "Ray Tracing Test", NULL, NULL);
    glfwMakeContextCurrent(window_);
	
	glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    context_.setupBuffers(scene_);
    std::cout << "Triangle count: " << scene_.triangles.size() << std::endl;

}

void RayTracer::Start()
{
    while (!glfwWindowShouldClose(window_))
    {
	    camera_.Update(window_, 1.0f);
		// temp..
		Render();

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
    glfwTerminate();
}

void RayTracer::Render()
{
	context_.setArgument(4, sizeof(float3), &camera_.getPos());
	context_.setArgument(5, sizeof(float3), &camera_.getFrontVector());
	context_.setArgument(6, sizeof(float3), &camera_.getUpVector());
    context_.executeKernel();

    cl_float4* ptr = context_.getPixels();
    
    if (ptr)
    {
        glDrawPixels(width_, height_, GL_RGBA, GL_FLOAT, ptr);
    }

    context_.unmapPixels(ptr);

}
