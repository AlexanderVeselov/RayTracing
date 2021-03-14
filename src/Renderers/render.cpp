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

    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.empty())
    {
        throw std::exception("No OpenCL platforms found");
    }

    m_CLContext = std::make_shared<CLContext>(all_platforms[0], GetDC(hwnd_), m_GLContext);

    std::vector<cl::Device> platform_devices;
    all_platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);
#ifdef BVH_INTERSECTION
    m_RenderKernel = std::make_shared<CLKernel>("src/Kernels/kernel_bvh.cl", *m_CLContext, platform_devices);
#else
    m_RenderKernel = std::make_shared<CLKernel>("src/Kernels/kernel_grid.cl", platform_devices);
#endif

    m_CopyKernel = std::make_shared<CLKernel>("src/Kernels/kernel_copy.cl", *m_CLContext, platform_devices);

#ifdef BVH_INTERSECTION
    m_Scene = std::make_shared<BVHScene>("meshes/dragon.obj", *this, 4);
#else
    m_Scene = std::make_shared<UniformGridScene>("meshes/room.obj");
#endif

    m_Framebuffer = std::make_shared<Framebuffer>(width_, height_);
    m_Camera = std::make_shared<Camera>(m_Framebuffer, *this);

    SetupBuffers();

}

Image image;
void Render::SetupBuffers()
{
    cl_int errCode;
    m_OutputImage = cl::ImageGL(GetCLContext()->GetContext(), CL_MEM_WRITE_ONLY,
        GL_TEXTURE_2D, 0, m_Framebuffer->GetGlImage(), &errCode);

    if (errCode)
    {
        throw CLException("Failed to create output image", errCode);
    }

    m_OutputBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_WRITE, width_ * height_ * sizeof(cl_float4), 0, &errCode);

    if (errCode)
    {
        throw CLException("Failed to create output buffer", errCode);
    }

    cl_mem image_mem = m_OutputImage();

    m_CopyKernel->SetArgument(0, &m_OutputBuffer, sizeof(m_OutputBuffer));
    m_CopyKernel->SetArgument(1, &image_mem, sizeof(image_mem));
    m_CopyKernel->SetArgument(2, &width_, sizeof(width_));
    m_CopyKernel->SetArgument(3, &width_, sizeof(height_));

    m_Scene->SetupBuffers();

    // Texture Buffers
    cl::ImageFormat imageFormat;
    imageFormat.image_channel_order = CL_RGBA;
    imageFormat.image_channel_data_type = CL_FLOAT;

    HDRLoader::Load("textures/Topanga_Forest_B_3k.hdr", image);

    m_Texture0 = cl::Image2D(GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        imageFormat, image.width, image.height, 0, image.colors, &errCode);
    if (errCode)
    {
        throw CLException("Failed to create image", errCode);
    }

    m_RenderKernel->SetArgument(RenderKernelArgument_t::BUFFER_OUT, &m_OutputBuffer, sizeof(cl::Buffer));

    cl_mem triangle_buffer = m_Scene->GetTriangleBuffer();
    cl_mem node_buffer = m_Scene->GetNodeBuffer();
    cl_mem material_buffer = m_Scene->GetMaterialBuffer();

    m_RenderKernel->SetArgument(RenderKernelArgument_t::BUFFER_SCENE, &triangle_buffer, sizeof(cl::Buffer));
    m_RenderKernel->SetArgument(RenderKernelArgument_t::BUFFER_NODE, &node_buffer, sizeof(cl::Buffer));
    m_RenderKernel->SetArgument(RenderKernelArgument_t::BUFFER_MATERIAL, &material_buffer, sizeof(cl::Buffer));

    m_RenderKernel->SetArgument(RenderKernelArgument_t::WIDTH, &width_, sizeof(std::uint32_t));
    m_RenderKernel->SetArgument(RenderKernelArgument_t::HEIGHT, &height_, sizeof(std::uint32_t));

    m_RenderKernel->SetArgument(RenderKernelArgument_t::TEXTURE0, &m_Texture0, sizeof(cl::Image2D));

    pixels_ = new cl_float3[width_ * height_];
}

double Render::GetCurtime() const
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

double Render::GetDeltaTime() const
{
    return GetCurtime() - m_PreviousFrameTime;
}

std::uint32_t Render::GetGlobalWorkSize() const
{
    return width_ * height_;
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

    float3 origin = m_Camera->GetOrigin();
    float3 front = m_Camera->GetFrontVector();

    m_RenderKernel->SetArgument(RenderKernelArgument_t::CAM_ORIGIN, &origin, sizeof(float3));
    m_RenderKernel->SetArgument(RenderKernelArgument_t::CAM_FRONT, &front, sizeof(float3));

    std::uint32_t frame_count = m_Camera->GetFrameCount();
    m_RenderKernel->SetArgument(RenderKernelArgument_t::FRAME_COUNT, &frame_count, sizeof(unsigned int));
    unsigned int seed = rand();
    m_RenderKernel->SetArgument(RenderKernelArgument_t::FRAME_SEED, &seed, sizeof(unsigned int));

    unsigned int globalWorksize = GetGlobalWorkSize();
    GetCLContext()->ExecuteKernel(m_RenderKernel, globalWorksize);
    GetCLContext()->ReadBuffer(m_OutputBuffer, pixels_, sizeof(cl_float4) * width_ * height_);
    GetCLContext()->Finish();

    printf("Reender\n");

    glDrawPixels(width_, height_, GL_RGBA, GL_FLOAT, pixels_);

    //GetCLContext()->AcquireGLObject(m_OutputImage());
    //GetCLContext()->ExecuteKernel(m_CopyKernel, globalWorksize);
    //GetCLContext()->Finish();
    //GetCLContext()->ReleaseGLObject(m_OutputImage());

    //glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);

    //m_Framebuffer->Present();

    /* TODO: draw GUI, debug, etc. here */

    glFinish();

    SwapBuffers(GetDC(hwnd_));

    FrameEnd();
}
