/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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

#include "CLI/CLI.hpp"
#include "render.hpp"
#include "utils/window.hpp"

#include <iostream>
#include <map>
#include <string>

namespace
{
std::map<std::string, Render::RenderBackend> CreateBackendMap()
{
    std::map<std::string, Render::RenderBackend> backends = {
        {"vulkan", Render::RenderBackend::kVulkan},
        {"d3d12", Render::RenderBackend::kD3D12},
    };
    return backends;
}

char const* GetBackendHelpText()
{
    return "Render backend: vulkan, d3d12";
}

char const* GetBackendTypeName()
{
    return "vulkan|d3d12";
}
}  // namespace

int main(int argc, char** argv)
{
    try
    {
        // Default parameters
        uint32_t window_width = 1280;
        uint32_t window_height = 720;
        Render::RenderBackend backend = Render::RenderBackend::kVulkan;
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
        auto backend_transform = CLI::CheckedTransformer(CreateBackendMap(), CLI::ignore_case).description("");
        cli_app.add_option("--backend", backend, GetBackendHelpText())
            ->type_name(GetBackendTypeName())
            ->transform(backend_transform);

        try
        {
            cli_app.parse(argc, argv);
        }
        catch (CLI::ParseError const& ex)
        {
            return cli_app.exit(ex);
        }

        // Create the window
        Window window(window_width, window_height, "RayTracing");

        Render render(window, backend, scene_path, scene_scale, flip_yz);

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
        return 1;
    }

    return 0;
}
