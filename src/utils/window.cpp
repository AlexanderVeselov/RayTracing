/*****************************************************************************
 MIT License

 Copyright(c) 2022 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

#include "window.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <backends/imgui_impl_win32.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <stdexcept>

#define WINDOW_CLASS "WindowClass1"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* window = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

    // sort through and find what code to run for the message given
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        window->OnKeyPressed((unsigned int)wParam);
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        window->OnKeyReleased((unsigned int)wParam);
        break;
    case WM_LBUTTONDOWN:
        window->OnMousePressed(MK_LBUTTON);
        break;
    case WM_LBUTTONUP:
        window->OnMouseReleased(MK_LBUTTON);
        break;
    case WM_RBUTTONDOWN:
        window->OnMousePressed(MK_RBUTTON);
        break;
    case WM_RBUTTONUP:
        window->OnMouseReleased(MK_RBUTTON);
        break;
    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

Window::Window(char const* title, std::uint32_t width, std::uint32_t height)
    : width_(width)
    , height_(height)
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
    HWND hwnd = CreateWindowEx(NULL,
        WINDOW_CLASS,                       // name of the window class
        title,                              // title of the window
        WS_OVERLAPPED | WS_SYSMENU,         // window style
        100,                                // x-position of the window
        100,                                // y-position of the window
        rt.right - rt.left,                 // width of the window
        rt.bottom - rt.top,                 // height of the window
        NULL,                               // we have no parent window, NULL
        NULL,                               // we aren't using menus, NULL
        hInstance,                          // application handle
        NULL);                              // used with multiple windows, NULL

    handle_ = hwnd;
    ShowWindow(hwnd, SW_SHOWNORMAL);

    ///@TODO: is this UB?
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

    // Assert the window client area matches to the provided size
    RECT hwnd_rect = {};
    GetClientRect(hwnd, &hwnd_rect);

    assert(hwnd_rect.right - hwnd_rect.left == width_);
    assert(hwnd_rect.bottom - hwnd_rect.top == height_);

    InitGL();

    // Clear key codes
    memset(keys_, 0, sizeof(keys_));
}

void Window::InitGL()
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

    HWND hwnd = (HWND)handle_;
    HDC hdc = GetDC(hwnd);
    display_context_ = hdc;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    gl_context_ = wglCreateContext(hdc);
    wglMakeCurrent(hdc, (HGLRC)gl_context_);

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

Window::~Window()
{
    //CHECK?
    //ReleaseDC((HWND)handle_, (HDC)display_context_);
}

void Window::PollEvents()
{
    MSG msg = { 0 };
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            should_close_ = true;
        }
    }
}

void Window::SwapBuffers()
{
    HWND hwnd = (HWND)handle_;

    HDC hdc = GetDC(hwnd);
    ::SwapBuffers(hdc);
    ReleaseDC(hwnd, hdc);
}

void Window::OnKeyPressed(int key)
{
    keys_[key] = true;
}

void Window::OnKeyReleased(int key)
{
    keys_[key] = false;
}

void Window::OnMousePressed(unsigned int button)
{
    mouse_ |= button;
}

void Window::OnMouseReleased(unsigned int button)
{
    mouse_ &= ~button;
}

bool Window::IsKeyPressed(int key) const
{
    return keys_[key];
}

bool Window::IsLeftMouseButtonPressed() const
{
    return (mouse_ & MK_LBUTTON) == MK_LBUTTON;
}

bool Window::IsRightMouseButtonPressed() const
{
    return (mouse_ & MK_RBUTTON) == MK_RBUTTON;
}

void Window::SetMousePos(int x, int y) const
{
    POINT point = { x, y };
    ClientToScreen((HWND)handle_, &point);
    SetCursorPos(point.x, point.y);
}

void Window::GetMousePos(int* x, int* y) const
{
    POINT point;
    GetCursorPos(&point);
    ScreenToClient((HWND)handle_, &point);

    *x = point.x;
    *y = point.y;
}
