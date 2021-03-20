#include "render.hpp"
#include "mathlib/mathlib.hpp"
#include "io/inputsystem.hpp"
#include "utils/cl_exception.hpp"
#include "bvh.hpp"
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_win32.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <GL/wglew.h>

#define WINDOW_CLASS "WindowClass1"
#define WINDOW_TITLE "Ray Tracing"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

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
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);

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

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplWin32_Init(hwnd_);

    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.empty())
    {
        throw std::runtime_error("No OpenCL platforms found");
    }

    cl_context_ = std::make_shared<CLContext>(all_platforms[0], GetDC(hwnd_), m_GLContext);

    scene_ = std::make_unique<Scene>("meshes/dragon.obj", *cl_context_);

    framebuffer_ = std::make_unique<Framebuffer>(width_, height_);
    camera_ = std::make_shared<Camera>(*framebuffer_, *this);

    // Create acc structure
    acc_structure_ = std::make_unique<Bvh>(*cl_context_);
    // Build it right here
    acc_structure_->BuildCPU(scene_->GetTriangles());

    // TODO, NOTE: this is done after building the acc structure because it reorders triangles
    // Need to get rid of reordering
    scene_->UploadBuffers();

    // Create estimator
    integrator_ = std::make_unique<PathTraceIntegrator>(width_, height_, *cl_context_,
        *acc_structure_, framebuffer_->GetGLImage());
    integrator_->SetSceneData(*scene_);

}

Render::~Render()
{
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Render::FrameEnd()
{
    m_PreviousFrameTime = m_StartFrameTime;
}

void Render::DrawGUI()
{
    ImGui::Begin("PerformanceStats", nullptr,
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowPos(ImVec2(10, 10));
    ImGui::SetWindowSize(ImVec2(350, 30));
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Begin("Useless window");
    static float aperture = 0.0f;
    ImGui::SliderFloat("Camera aperture", &aperture, 0.0, 1.0);
    camera_->SetAperture(aperture);
    static float focus_distance = 10.0f;
    ImGui::SliderFloat("Camera focus distance", &focus_distance, 0.0, 100.0);
    camera_->SetFocusDistance(focus_distance);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Render::RenderFrame()
{
    FrameBegin();

    glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ///@TODO: need to fix this hack
    bool need_to_reset = false;

    if (input->IsKeyDown('R'))
    {
        integrator_->ReloadKernels();
        integrator_->SetSceneData(*scene_);
        need_to_reset = true;
    }

    camera_->Update();
    integrator_->SetCameraData(*camera_);

    need_to_reset = need_to_reset || (camera_->GetFrameCount() == 1);
    if (need_to_reset)
    {
        integrator_->Reset();
    }

    integrator_->Integrate();
    framebuffer_->Present();

    // Draw GUI
    DrawGUI();

    glFinish();

    HDC hdc = GetDC(hwnd_);
    SwapBuffers(hdc);
    ReleaseDC(hwnd_, hdc);

    FrameEnd();
}
