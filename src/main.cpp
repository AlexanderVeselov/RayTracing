/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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
#include "CLI/CLI.hpp"

int main(int argc, char** argv)
{
    try
    {
        // Default parameters
        std::uint32_t window_width = 1280;
        std::uint32_t window_height = 720;
        bool use_opengl = false;
        std::string scene_path = "assets/ShaderBalls.obj";
        float scene_scale = 1.0f;
        bool flip_yz = false;

        // Parse the command line
        CLI::App cli_app("RayTracing");
        cli_app.allow_extras(true);

        cli_app.set_help_flag("--help", "Print this help");
        cli_app.add_option("-w", window_width, "Window width");
        cli_app.add_option("-h", window_height, "Window height");
        cli_app.add_option("--scene", scene_path, "Scene path");
        cli_app.add_option("--scale", scene_scale, "Scene scale");
        cli_app.add_option("--flip_yz", flip_yz, "Flip Y and Z axis");
        cli_app.add_option("--opengl", use_opengl, "Use OpenGL");

        cli_app.parse(argc, argv);

        // Load the scene
        Scene scene(scene_path.c_str(), scene_scale, flip_yz);
        // Add a directional light since obj format doesn't support lights
        scene.AddDirectionalLight({ -0.6f, -1.5f, 3.5f }, { 15.0f, 10.0f, 5.0f });

        // Create the window
        Window window(window_width, window_height, "RayTracing");

        // Create the renderer
        Render::RenderBackend backend = use_opengl ? Render::RenderBackend::kOpenGL : Render::RenderBackend::kOpenCL;
        Render render(window, backend, scene);

        // Render loop
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
