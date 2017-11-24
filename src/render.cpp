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

    int pixelFormat = ChoosePixelFormat(m_DC, &pfd);
    SetPixelFormat(m_DC, pixelFormat, &pfd);

    m_GlContext = wglCreateContext(m_DC);
    wglMakeCurrent(m_DC, m_GlContext);

}

void Render::Init(HWND hwnd)
{
    m_hWnd = hwnd;
    m_DC = GetDC(m_hWnd);
    
    InitGL();

    unsigned int width = 640, height = 480;
    m_Viewport = std::make_shared<Viewport>(width, height);
    m_Camera = std::make_shared<Camera>(m_Viewport);
    m_Scene = std::make_shared<Scene>("meshes/teapot.obj", 64);
    
    m_RandomArray = new int[width * height];

    std::ifstream input_file("src/kernel.cl");
    if (!input_file)
    {
        throw Exception("Failed to load kernel file!");
    }

    std::ostringstream buf;
    buf << "#define GRID_RES " << m_Scene->GetCellResolution() << std::endl;

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
        std::shared_ptr<ClContext> context = std::make_shared<ClContext>(all_platforms[i], source, width, height, m_Scene->GetCellResolution());
        context->SetupBuffers(m_Scene);
        m_Contexts.push_back(context);
    }
    
}

void Render::RenderFrame()
{
    glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (size_t i = 0; i < 640 * 480; ++i)
    {
        m_RandomArray[i] = rand();
    }

    std::shared_ptr<ClContext> context = m_Contexts[0];
    context->WriteRandomBuffer(640 * 480 * sizeof(int), m_RandomArray);
    m_Camera->Update();
    float3 org = m_Camera->GetOrigin();
    context->SetArgument(CL_ARG_CAM_ORIGIN, sizeof(float3), &org);

    float3 right = cross(m_Camera->GetFrontVector(), m_Camera->GetUpVector()).normalize();
    float3 up = cross(right, m_Camera->GetFrontVector());
    context->SetArgument(CL_ARG_CAM_FRONT, sizeof(float3), &m_Camera->GetFrontVector());
    context->SetArgument(CL_ARG_CAM_UP, sizeof(float3), &up);

    m_Viewport->Update(context);

    int w = 640, h = 480;
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(w, h, GL_RGBA, GL_FLOAT, m_Viewport->GetPixels());
    
    glFlush();
    SwapBuffers(m_DC);

}

void Render::Shutdown()
{
    delete[] m_RandomArray;
}
