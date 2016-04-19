#include "raytracer.hpp"
#include "vectors.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

RayTracer::RayTracer(const char* filename) :
    width_(1024),
    height_(768),
    camera_(width_, height_, 0.0, 90.0f, 0.0005f, 1.0f),
    scene_(filename, 64)
{
    glfwInit();

    // Load kernel file
    std::ifstream input_file("src/kernel.cl");
    if (!input_file)
    {
        std::cerr << "Cannot load kernel file" << std::endl;
    }
    
    std::ostringstream buf;
    buf << "#define GRID_RES " << scene_.getCellResolution() << std::endl;

    std::string curr_line;
    std::string source;
    source += buf.str() + "\n";
    while (std::getline(input_file, curr_line))
    {
        source += curr_line + "\n";
    }

    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    for (size_t i = 0; i < all_platforms.size(); ++i)
    {
        ClContext context = ClContext(all_platforms[i], source, width_, height_, scene_.getCellResolution());
        context.setupBuffers(scene_);

        if (context.isValid())
        {
            contexts_.push_back(context);
        }
    }

    window_ = glfwCreateWindow(width_, height_, "Ray Tracing Test", NULL, NULL);
    glfwMakeContextCurrent(window_);

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
    for (size_t i = 0; i < contexts_.size(); ++i)
    {
        std::cout << "Using context " << i << std::endl;
	    contexts_[i].setArgument(4, sizeof(float3), &camera_.getPos());
	    contexts_[i].setArgument(5, sizeof(float3), &camera_.getFrontVector());
	    contexts_[i].setArgument(6, sizeof(float3), &camera_.getUpVector());
        contexts_[i].executeKernel();

        cl_float4* pixels = contexts_[i].getPixels();
    
        if (pixels)
        {
            glDrawPixels(width_, height_, GL_RGBA, GL_FLOAT, pixels);
        }

        contexts_[i].unmapPixels(pixels);
    }
}
