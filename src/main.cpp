#include "renderers/render.hpp"
#include "io/inputsystem.hpp"
#include "utils/cl_exception.hpp"
#include <Windows.h>

int main()
{
    try
    {
        Render render(1280, 720);

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
                render.RenderFrame();
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Caught exception: " << ex.what() << std::endl;
                system("PAUSE");
                return 0;
            }
        
        }

        return msg.wParam;
    }
    catch (std::exception& ex)
    {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
        return 0;
    }

}
