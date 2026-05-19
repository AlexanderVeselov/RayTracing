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

#pragma once

#include "acceleration_structure.hpp"
#include "gpu_api.hpp"
#include "integrator.hpp"
#include "utils/camera_controller.hpp"

#include <ctime>
#include <memory>
#include <string>

class Window;
class RhiIntegrator;
class Scene;
class TextureManager;
class Render
{
public:
    enum class RenderBackend
    {
        kVulkan,
        kD3D12
    };

    Render(Window& window, RenderBackend backend, std::string const& scene_path, float scene_scale, bool flip_yz);
    ~Render();

    void RenderFrame();
    double GetCurtime() const;
    double GetDeltaTime() const;
    Window& GetWindow() const { return window_; }

private:
    void FrameBegin();
    void FrameEnd();
    void DrawGUI();

private:
    // Window
    Window& window_;
    RenderBackend render_backend_;
    std::string scene_path_;
    float scene_scale_ = 1.0f;
    bool flip_yz_ = false;
    std::unique_ptr<Scene> scene_;

    // Render size
    uint32_t width_;
    uint32_t height_;

    // Timing
    double start_frame_time_ = 0.0;
    double prev_frame_time_ = 0.0;

    gpu::ApiType rhi_api_type_ = gpu::ApiType::kVulkan;
    std::unique_ptr<gpu::Api> rhi_api_;
    gpu::DevicePtr rhi_device_;
    gpu::SwapchainPtr rhi_swapchain_;
    gpu::ImGuiRendererPtr rhi_imgui_renderer_;
    gpu::CommandBufferPtr rhi_command_buffer_;
    RhiIntegrator* rhi_integrator_ = nullptr;
    std::unique_ptr<TextureManager> texture_manager_;

    // Integrator
    std::unique_ptr<Integrator> integrator_;
    // Acceleration structure
    std::unique_ptr<AccelerationStructure> acc_structure_;

    std::unique_ptr<CameraController> camera_controller_;

    struct GuiParams
    {
        float camera_aperture = 0.0f;
        float camera_focus_distance = 10.0f;
        int max_bounces = 3u;
        bool enable_denoiser = false;
        bool enable_white_furnace = false;
        bool enable_blue_noise = false;
    } gui_params_;
};
