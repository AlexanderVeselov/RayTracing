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

#include "render.hpp"
#include "utils/window.hpp"

int main()
{
    try
    {
        Window window(1280, 720, "RayTracing");
        Render::RenderBackend backend = Render::RenderBackend::kOpenCL;
        //backend = Render::RenderBackend::kOpenGL;
        Render render(window, backend);

        while (!window.ShouldClose())
        {
            window.PollEvents();
            render.RenderFrame();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
    }

    return 0;
}
