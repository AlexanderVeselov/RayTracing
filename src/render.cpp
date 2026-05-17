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

#include "render.hpp"

#include "Utils/window.hpp"
#include "bvh.hpp"
#include "gpu_command_buffer.hpp"
#include "gpu_device.hpp"
#include "gpu_imgui.hpp"
#include "gpu_queue.hpp"
#include "gpu_swapchain.hpp"
#include "integrator/rhi_integrator.hpp"

#include <fstream>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
bool IsRhiBackend(Render::RenderBackend backend)
{
    return backend == Render::RenderBackend::kVulkan || backend == Render::RenderBackend::kD3D12;
}
}  // namespace

Render::Render(Window& window, RenderBackend backend, Scene& scene)
    : window_(window), render_backend_(backend), scene_(scene), width_(window.GetWidth()), height_(window.GetHeight())
{
    if (render_backend_ == RenderBackend::kVulkan)
    {
        rhi_api_type_ = gpu::ApiType::kVulkan;
    }
    else if (render_backend_ == RenderBackend::kD3D12)
    {
        rhi_api_type_ = gpu::ApiType::kD3D12;
    }

    if (IsRhiBackend(render_backend_))
    {
        rhi_api_.reset(gpu::Api::Create(rhi_api_type_));
        if (!rhi_api_)
        {
            throw std::runtime_error("Failed to create GpuApi");
        }

        rhi_api_->SetShaderPath("src/kernels/hlsl");
        rhi_device_ = rhi_api_->CreateDevice();
        rhi_swapchain_ = rhi_device_->CreateSwapchain(window_.GetNativeHandle(), width_, height_, 3);
        rhi_imgui_renderer_ = rhi_device_->CreateImGuiRenderer(window_.GetGlfwWindow(), *rhi_swapchain_);
    }

    camera_controller_ = std::make_unique<CameraController>(window_);

    // Create acc structure
    acc_structure_ = std::make_unique<Bvh>();
    // Build it right here
    acc_structure_->BuildCPU(scene_.GetVertices(), scene_.GetIndices());

    // TODO, NOTE: this is done after building the acc structure because it
    // reorders triangles Need to get rid of reordering
    scene_.Finalize();

    // Create integrator
    if (IsRhiBackend(render_backend_))
    {
        auto rhi_integrator = std::make_unique<RhiIntegrator>(width_, height_, *rhi_device_, *rhi_swapchain_);
        rhi_integrator_ = rhi_integrator.get();
        integrator_ = std::move(rhi_integrator);
    }
    else
    {
        throw std::runtime_error("Unsupported render backend");
    }

    // Upload scene data to the GPU
    integrator_->UploadGPUData(scene_, *acc_structure_);
}

Render::~Render()
{
    if (rhi_device_)
    {
        rhi_device_->GetQueue(gpu::QueueType::kGraphics).WaitIdle();
    }
}

double Render::GetCurtime() const
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

double Render::GetDeltaTime() const
{
    return GetCurtime() - prev_frame_time_;
}

void Render::FrameBegin()
{
    start_frame_time_ = GetCurtime();
}

void Render::FrameEnd()
{
    camera_controller_->OnEndFrame();
    prev_frame_time_ = start_frame_time_;
}

void Render::DrawGUI()
{
    ImGui::Begin("PerformanceStats", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::SetWindowSize(ImVec2(350, 50));
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate);
        ImGui::Text("Press \"R\" to reload kernels");
    }
    ImGui::End();

    ImGui::Begin("Controls");
    {
        if (ImGui::SliderFloat("Camera aperture", &gui_params_.camera_aperture, 0.0, 1.0))
        {
            camera_controller_->SetAperture(gui_params_.camera_aperture);
        }

        if (ImGui::SliderFloat("Camera focus distance", &gui_params_.camera_focus_distance, 0.0, 100.0))
        {
            camera_controller_->SetFocusDistance(gui_params_.camera_focus_distance);
        }

        if (ImGui::SliderInt("Max bounces", &gui_params_.max_bounces, 0, 5))
        {
            integrator_->SetMaxBounces((uint32_t)gui_params_.max_bounces);
        }

        if (ImGui::Checkbox("Enable denoiser", &gui_params_.enable_denoiser))
        {
            integrator_->EnableDenoiser(gui_params_.enable_denoiser);
        }

        if (ImGui::Checkbox("Blue noise sampler", &gui_params_.enable_blue_noise))
        {
            integrator_->SetSamplerType(gui_params_.enable_blue_noise ? Integrator::SamplerType::kBlueNoise
                                                                      : Integrator::SamplerType::kRandom);
        }

        if (ImGui::Checkbox("Enable white furnace", &gui_params_.enable_white_furnace))
        {
            integrator_->EnableWhiteFurnace(gui_params_.enable_white_furnace);
        }

        static int aov_index = 0;
        const char* aov_names[] = {"Shaded Color", "Diffuse Albedo", "Depth", "Normal", "Motion Vectors"};
        if (ImGui::Combo("AOV", &aov_index, aov_names, 5))
        {
            integrator_->SetAOV((Integrator::AOV)aov_index);
        }
    }
    ImGui::End();
}

void Render::RenderFrame()
{
    FrameBegin();

    if (IsRhiBackend(render_backend_))
    {
        rhi_imgui_renderer_->NewFrame();
        DrawGUI();
    }

    bool need_to_reset = false;

    camera_controller_->Update((float)GetDeltaTime());
    integrator_->SetCameraData(camera_controller_->GetData());

    need_to_reset = need_to_reset || camera_controller_->IsChanged();

    if (need_to_reset)
    {
        integrator_->RequestReset();
    }

    if (IsRhiBackend(render_backend_))
    {
        gpu::Queue& queue = rhi_device_->GetQueue(gpu::QueueType::kGraphics);
        rhi_command_buffer_ = queue.CreateCommandBuffer();
        rhi_integrator_->SetCommandBuffer(*rhi_command_buffer_);
        integrator_->Integrate();
        rhi_imgui_renderer_->Render(*rhi_command_buffer_);
        rhi_command_buffer_->TransitionBarrier(rhi_swapchain_->GetCurrentImage(),
            gpu::ImageLayout::kRenderTarget,
            gpu::ImageLayout::kPresent);
        rhi_integrator_->SetCurrentSwapchainImageLayout(gpu::ImageLayout::kPresent);
        queue.Submit(std::move(rhi_command_buffer_));
        rhi_swapchain_->Present();
    }

    FrameEnd();
}
