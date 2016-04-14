#include "raytracer.hpp"
#include "vectors.hpp"
#include <glm/glm.hpp>
#include <iostream>

RayTracer::RayTracer(const char* filename) :
    width_(512),
    height_(512),
    context_(512, 512, 64),
    camera_(width_, height_, 0.0, 90.0f, 0.0005f, 1.0f),
    scene_(filename, 64)
{
    glfwInit();
    window_ = glfwCreateWindow(width_, height_, "Ray Tracing Test", NULL, NULL);
    glfwMakeContextCurrent(window_);
	
    context_.setupBuffers(scene_);
    std::cout << "Triangle count: " << scene_.triangles.size() << std::endl;

}

void RayTracer::Start()
{
    while (!glfwWindowShouldClose(window_))
    {
	    camera_.Update(window_, 1.0f);
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

    cl_float4* pixels = context_.getPixels();
    
    if (pixels)
    {
        glDrawPixels(width_, height_, GL_RGBA, GL_FLOAT, pixels);
    }

    context_.unmapPixels(pixels);

}
