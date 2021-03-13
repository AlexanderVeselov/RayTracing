#include "render.hpp"
#include "mathlib/mathlib.hpp"
#include "io/inputsystem.hpp"
#include "io/image_loader.hpp"
#include "utils/cl_exception.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <GL/wglew.h>

#define WINDOW_CLASS "WindowClass1"
#define WINDOW_TITLE "Ray Tracing"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        input->KeyDown((unsigned int)wParam);
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        input->KeyUp((unsigned int)wParam);
        break;
    case WM_LBUTTONDOWN:
        input->MousePressed(MK_LBUTTON);
        break;
    case WM_LBUTTONUP:
        input->MouseReleased(MK_LBUTTON);
        break;
    case WM_RBUTTONDOWN:
        input->MousePressed(MK_RBUTTON);
        break;
    case WM_RBUTTONUP:
        input->MouseReleased(MK_RBUTTON);
        break;
    default:
        // Handle any messages the switch statement didn't
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;

}

void Render::InitWindow()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = WINDOW_CLASS;

    RegisterClassEx(&wc);

    RECT rt = { 0, 0, width_, height_ };
    AdjustWindowRect(&rt, WS_OVERLAPPEDWINDOW, FALSE);

    // create the window and use the result as the handle
    hwnd_ = CreateWindowEx(NULL,
        WINDOW_CLASS,                       // name of the window class
        WINDOW_TITLE,                       // title of the window
        WS_OVERLAPPED | WS_SYSMENU,         // window style
        100,                                // x-position of the window
        100,                                // y-position of the window
        rt.right - rt.left,                 // width of the window
        rt.bottom - rt.top,                 // height of the window
        NULL,                               // we have no parent window, NULL
        NULL,                               // we aren't using menus, NULL
        hInstance,                          // application handle
        NULL);                              // used with multiple windows, NULL

    ShowWindow(hwnd_, SW_SHOWNORMAL);

    // Assert the window client area matches to the provided size
    RECT hwnd_rect = {};
    GetClientRect(hwnd_, &hwnd_rect);

    assert(hwnd_rect.right - hwnd_rect.left == width_);
    assert(hwnd_rect.bottom - hwnd_rect.top == height_);

}

void Render::InitGL()
{
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    HDC hdc = GetDC(hwnd_);

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    m_GLContext = wglCreateContext(hdc);
    wglMakeCurrent(hdc, m_GLContext);

    printf("GL version: %s\n", (char*)glGetString(GL_VERSION));

    GLenum glew_status = glewInit();

    if (glew_status != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));

        throw std::runtime_error("Failed to init GLEW");
    }

    // Disable VSync
    wglSwapIntervalEXT(0);
}

Render::Render(std::uint32_t width, std::uint32_t height)
    : width_(width)
    , height_(height)
{
    InitWindow();
    InitGL();


    m_Framebuffer = std::make_shared<Framebuffer>(width_, height_);
    //m_Camera = std::make_shared<Camera>(m_Framebuffer);
#ifdef BVH_INTERSECTION
    //m_Scene = std::make_shared<BVHScene>("meshes/dragon.obj", 4);
#else
    m_Scene = std::make_shared<UniformGridScene>("meshes/room.obj");
#endif
    
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.empty())
    {
        throw std::exception("No OpenCL platforms found");
    }
    
    //m_CLContext = std::make_shared<CLContext>(all_platforms[0]);

    std::vector<cl::Device> platform_devices;
    all_platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);
#ifdef BVH_INTERSECTION
//    m_RenderKernel = std::make_shared<CLKernel>("src/Kernels/kernel_bvh.cl", platform_devices);
#else
    m_RenderKernel = std::make_shared<CLKernel>("src/Kernels/kernel_grid.cl", platform_devices);
#endif

    SetupBuffers();

}

void Render::SetupBuffers()
{

    //GetCLKernel()->SetArgument(RenderKernelArgument_t::WIDTH, &m_Viewport->width, sizeof(unsigned int));
    //GetCLKernel()->SetArgument(RenderKernelArgument_t::HEIGHT, &m_Viewport->height, sizeof(unsigned int));

    //cl_int errCode;
    //m_OutputBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_WRITE, GetGlobalWorkSize() * sizeof(float3), 0, &errCode);
    //if (errCode)
    //{
    //    throw CLException("Failed to create output buffer", errCode);
    //}
    //GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_OUT, &m_OutputBuffer, sizeof(cl::Buffer));
    //
    //m_Scene->SetupBuffers();

    //// Texture Buffers
    //cl::ImageFormat imageFormat;
    //imageFormat.image_channel_order = CL_RGBA;
    //imageFormat.image_channel_data_type = CL_FLOAT;

    //HDRLoader::Load("textures/Topanga_Forest_B_3k.hdr", image);
    ////HDRLoader::Load("textures/studio.hdr", image);

    //m_Texture0 = cl::Image2D(GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, image.width, image.height, 0, image.colors, &errCode);
    //if (errCode)
    //{
    //    throw CLException("Failed to create image", errCode);
    //}
    //GetCLKernel()->SetArgument(RenderKernelArgument_t::TEXTURE0, &m_Texture0, sizeof(cl::Image2D));

}

//const HWND Render::GetHWND() const
//{
//    return m_hWnd;
//}

double Render::GetCurtime() const
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

double Render::GetDeltaTime() const
{
    return GetCurtime() - m_PreviousFrameTime;
}

//unsigned int Render::GetGlobalWorkSize() const
//{
//    if (m_Viewport)
//    {
//        return m_Viewport->width * m_Viewport->height;
//    }
//    else
//    {
//        return 0;
//    }
//}

//std::shared_ptr<CLContext> Render::GetCLContext() const
//{
//    return m_CLContext;
//}
//
//std::shared_ptr<CLKernel> Render::GetCLKernel() const
//{
//    return m_RenderKernel;
//}

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


    //m_Camera->Update();

    //if (m_Camera->GetFrameCount() > 64) return;

    /*
    unsigned int globalWorksize = GetGlobalWorkSize();
    GetCLContext()->ExecuteKernel(GetCLKernel(), globalWorksize);
    GetCLContext()->ReadBuffer(m_OutputBuffer, m_Viewport->pixels, sizeof(float3) * globalWorksize);
    GetCLContext()->Finish();

    glDrawPixels(m_Viewport->width, m_Viewport->height, GL_RGBA, GL_FLOAT, m_Viewport->pixels);
        
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, 1280.0 / 720.0, 1, 1024);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float3 eye = m_Camera->GetOrigin();
    float3 center = m_Camera->GetFrontVector() + eye;
    gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, m_Camera->GetUpVector().x, m_Camera->GetUpVector().y, m_Camera->GetUpVector().z);

    //m_Scene->DrawDebug();
    */

    glFinish();

    SwapBuffers(GetDC(hwnd_));

    FrameEnd();
}
