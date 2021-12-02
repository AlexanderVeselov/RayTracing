/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

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

#pragma once

#include "path_trace_integrator.hpp"
#include "acceleration_structure.hpp"
#include "scene/scene.hpp"
#include "GpuWrappers/cl_context.hpp"
#include "utils/camera_controller.hpp"
#include "utils/framebuffer.hpp"
#include <memory>
#include <ctime>

class Window;
class Render
{
public:
    Render(Window& window);
    ~Render();

    void    RenderFrame();
    double  GetCurtime()   const;
    double  GetDeltaTime() const;
    Window& GetWindow() const { return window_; }

    std::shared_ptr<CLContext> GetCLContext() const { return cl_context_; }

private:
    void FrameBegin();
    void FrameEnd();
    void DrawGUI();
    
private:
    // Window
    Window& window_;

    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    // Timing
    double start_frame_time_ = 0.0;
    double prev_frame_time_ = 0.0;
    std::shared_ptr<CLContext> cl_context_;
    // Estimator
    std::unique_ptr<PathTraceIntegrator> integrator_;
    // Acceleration structure
    std::unique_ptr<AccelerationStructure> acc_structure_;
    // Scene
    std::unique_ptr<CameraController> camera_controller_;
    std::unique_ptr<Scene>            scene_;
    std::unique_ptr<Framebuffer>      framebuffer_;

    struct GuiParams
    {
        float camera_aperture = 0.0f;
        float camera_focus_distance = 10.0f;
        int   max_bounces = 3u;
        bool  enable_denoiser = true;
        bool  enable_white_furnace = false;
        bool  enable_blue_noise = false;
    } gui_params_;

};
