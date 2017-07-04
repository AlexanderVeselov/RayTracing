#include "raytracer.hpp"
#include "vectors.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

RayTracer::RayTracer(const char* filename) :
    m_Width(128),
    m_Height(128),
    m_Camera(m_Width, m_Height, 0.0, 90.0f, 0.0005f, 1.0f),
    m_Scene(filename, 64),
    m_Viewport(m_Camera, m_Width, m_Height, 1)
{
    glfwInit();
    // Load kernel file
    std::ifstream input_file("src/kernel.cl");
    if (!input_file)
    {
        std::cerr << "Failed to load kernel file!" << std::endl;
    }
    
    std::ostringstream buf;
    buf << "#define GRID_RES " << m_Scene.GetCellResolution() << std::endl;

    std::string curr_line;
    std::string source;
    source += buf.str() + "\n";
    while (std::getline(input_file, curr_line))
    {
        source += curr_line + "\n";
    }

    int* random_array = new int[m_Width * m_Height];

    for (size_t i = 0; i < m_Width * m_Height; ++i)
    {
        random_array[i] = rand();
    }

    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    for (size_t i = 0; i < all_platforms.size(); ++i)
    {
        ClContext context = ClContext(all_platforms[i], source, m_Width, m_Height, m_Scene.GetCellResolution());
        context.SetupBuffers(m_Scene);

        if (context.IsValid())
        {
            m_Contexts.push_back(context);
        }
    }

    m_Window = glfwCreateWindow(m_Width, m_Height, "Ray Tracing Test", NULL, NULL);
    glfwMakeContextCurrent(m_Window);
}

void RayTracer::Start()
{
    
    int* random_array = new int[m_Width * m_Height];

    //for (size_t i = 0; i < m_Contexts.size(); ++i)
    //{
    //    boost::thread thread(&RayTracer::testThread, this, 0);
    //}
    //
	time_t t = clock();
    while (!glfwWindowShouldClose(m_Window))
    {
        //boost::unique_lock<boost::mutex> lock(mutex_);
        //cond_.wait(lock);
        //std::cout << "Draw thread" << std::endl;
		for (size_t i = 0; i < m_Width * m_Height; ++i)
		{
			random_array[i] = rand();
		}
		m_Contexts[0].WriteRandomBuffer(m_Width * m_Height * sizeof(int), random_array);
	    m_Camera.Update(m_Window, 1.0f);
		m_Viewport.Update(m_Contexts[0]);
		m_Viewport.Draw();
        glfwSwapBuffers(m_Window);
        glfwPollEvents();

		glfwSetWindowTitle(m_Window, ("Ray Tracing Test FPS: " + std::to_string(1.0 / ((float)(clock() - t) / CLOCKS_PER_SEC))).c_str());
		t = clock();
    }
    glfwTerminate();

}

//void RayTracer::testThread(size_t i)
//{
//    while (!glfwWindowShouldClose(m_Window))
//    {
//        m_Viewport.Update(m_Contexts[i], i, cond_);
//        if (i == 0) ++gpu;
//        else ++cpu;
//    }
//}

void RayTracer::Render()
{

}
