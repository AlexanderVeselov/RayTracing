#include "render.hpp"
#include "mathlib.hpp"
#include "inputsystem.hpp"
#include "exception.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <gl/GL.h>
#include <gl/GLU.h>

static Render g_Render;
Render* render = &g_Render;

void Render::InitGL()
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(m_DisplayContext, &pfd);
    SetPixelFormat(m_DisplayContext, pixelFormat, &pfd);

    m_GLContext = wglCreateContext(m_DisplayContext);
    wglMakeCurrent(m_DisplayContext, m_GLContext);

}

void Render::Init(HWND hwnd)
{
    m_hWnd = hwnd;
    m_DisplayContext = GetDC(m_hWnd);
    
    InitGL();

    unsigned int width = 640, height = 480;
    m_Viewport = std::make_shared<Viewport>(0, 0, width, height);
    m_Camera = std::make_shared<Camera>(m_Viewport);
    m_Scene = std::make_shared<Scene>("meshes/city.obj", 64);
            
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.empty())
    {
        throw Exception("No OpenCL platforms found");
    }
    
    m_CLContext = std::make_shared<CLContext>(all_platforms[0]);

    std::vector<cl::Device> platform_devices;
    all_platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);
    m_RenderKernel = std::make_shared<CLKernel>("src/kernel.cl", platform_devices);

    SetupBuffers();

}

void Render::SetupBuffers()
{
    GetCLKernel()->SetArgument(RenderKernelArgument_t::WIDTH, &m_Viewport->width, sizeof(size_t));
    GetCLKernel()->SetArgument(RenderKernelArgument_t::HEIGHT, &m_Viewport->height, sizeof(size_t));
    size_t globalWorkSize = m_Viewport->width * m_Viewport->height;

    cl_int errCode;
    m_OutputBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_ONLY, globalWorkSize * sizeof(float3), 0, &errCode);
    if (errCode)
    {
        throw CLException("Failed to output buffer", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_OUT, &m_OutputBuffer, sizeof(cl::Buffer));
    
    m_SceneBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Scene->triangles.size() * sizeof(Triangle), m_Scene->triangles.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to scene buffer", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_SCENE, &m_SceneBuffer, sizeof(cl::Buffer));

    m_IndexBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Scene->indices.size() * sizeof(cl_uint), m_Scene->indices.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to index buffer", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_INDEX, &m_IndexBuffer, sizeof(cl::Buffer));

    m_CellBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Scene->cells.size() * sizeof(CellData), m_Scene->cells.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to cell buffer", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_CELL, &m_CellBuffer, sizeof(cl::Buffer));
    
}

void Render::FrameBegin()
{
    m_StartFrameTime = GetCurtime();
}

void Render::FrameEnd()
{
    m_PreviousFrameTime = m_StartFrameTime;
}

void Render::RenderFrame()
{
    FrameBegin();

    glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_Camera->Update();    
    unsigned int globalWorksize = m_Viewport->width * m_Viewport->height;
    GetCLContext()->ExecuteKernel(GetCLKernel(), globalWorksize);
    GetCLContext()->ReadBuffer(m_OutputBuffer, m_Viewport->pixels, sizeof(float3) * globalWorksize);
    glDrawPixels(m_Viewport->width, m_Viewport->height, GL_RGBA, GL_FLOAT, m_Viewport->pixels);
    
    glFlush();
    SwapBuffers(m_DisplayContext);

    FrameEnd();
}

void Render::Shutdown()
{

}
