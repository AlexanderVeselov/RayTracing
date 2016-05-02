#include "raytracer.hpp"
#include "vectors.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

RayTracer::RayTracer(const char* filename) :
    width_(1280),
    height_(720),
    camera_(width_, height_, 0.0, 90.0f, 0.0005f, 1.0f),
    scene_(filename, 64),
    viewport_(camera_, width_, height_, 4)
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

    int* random_array = new int[width_ * height_];

    for (size_t i = 0; i < width_ * height_; ++i)
    {
        random_array[i] = rand();
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
    
    int* random_array = new int[width_ * height_];

    for (size_t i = 0; i < width_ * height_; ++i)
    {
        random_array[i] = rand();
    }
    for (size_t i = 0; i < contexts_.size(); ++i)
    {
        contexts_[i].writeRandomBuffer(width_ * height_ * sizeof(int), random_array);
    }

    //for (size_t i = 0; i < contexts_.size(); ++i)
    {
        boost::thread thread(&RayTracer::testThread, this, 0);
    }
    
    while (!glfwWindowShouldClose(window_))
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        cond_.wait(lock);
        //std::cout << "Draw thread" << std::endl;
	    camera_.Update(window_, 1.0f);
        Render();
        
        for (size_t i = 0; i < width_ * height_; ++i)
        {
            random_array[i] = rand();
        }

        for (size_t i = 0; i < contexts_.size(); ++i)
        {
            contexts_[i].writeRandomBuffer(width_ * height_ * sizeof(int), random_array);
        }

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
    glfwTerminate();

}

void RayTracer::testThread(size_t i)
{
    while (!glfwWindowShouldClose(window_))
    {
        viewport_.Update(contexts_[i], i, cond_);
    }
}

void RayTracer::Render()
{
    viewport_.Draw();

}
