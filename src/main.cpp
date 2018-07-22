#include "renderers/render.hpp"
#include "io/inputsystem.hpp"
#include "utils/cl_exception.hpp"
#include <Windows.h>

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

HWND InitMainWindow()
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

    // create the window and use the result as the handle
    HWND hWnd = CreateWindowEx(NULL,
        WINDOW_CLASS,                       // name of the window class
        WINDOW_TITLE,                       // title of the window
        WS_OVERLAPPED | WS_SYSMENU,         // window style
        100,                                // x-position of the window
        100,                                // y-position of the window
        1280,                               // width of the window
        720,                                // height of the window
        NULL,                               // we have no parent window, NULL
        NULL,                               // we aren't using menus, NULL
        hInstance,                          // application handle
        NULL);                              // used with multiple windows, NULL

    ShowWindow(hWnd, SW_SHOWNORMAL);

    return hWnd;
}

int main()
{
    HWND hwnd = InitMainWindow();

    try
    {
        render->Init(hwnd);
    }
    catch (std::exception& ex)
    {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
        return 0;
    }

    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        try
        {
            render->RenderFrame();
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Caught exception: " << ex.what() << std::endl;
            system("PAUSE");
            return 0;
        }
        
    }

    render->Shutdown();

    return msg.wParam;

}
