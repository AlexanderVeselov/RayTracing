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
#include "gpu_command_buffer.hpp"
#include "gpu_device.hpp"
#include "gpu_imgui.hpp"
#include "gpu_queue.hpp"
#include "gpu_swapchain.hpp"
#include "acc_structures/bvh.hpp"
#include "acc_structures/hardware_rt_acceleration_structure.hpp"
#include "managers/texture_manager.hpp"
#include "path_tracer.hpp"
#include "scene/scene.hpp"

#include <imgui.h>
#include <iostream>
#include <stdexcept>

Render::Render(Window& window, RenderBackend backend, std::string const& scene_path, float scene_scale, bool flip_yz)
    : window_(window)
    , render_backend_(backend)
    , scene_path_(scene_path)
    , scene_scale_(scene_scale)
    , flip_yz_(flip_yz)
    , width_(window.GetWidth())
    , height_(window.GetHeight())
{
    if (render_backend_ == RenderBackend::kVulkan)
    {
        rhi_api_type_ = gpu::ApiType::kVulkan;
    }
    else if (render_backend_ == RenderBackend::kD3D12)
    {
        rhi_api_type_ = gpu::ApiType::kD3D12;
    }

    rhi_api_.reset(gpu::Api::Create(rhi_api_type_));
    if (!rhi_api_)
    {
        throw std::runtime_error("Failed to create GpuApi");
    }

    rhi_api_->SetShaderPath("src/shaders");
    rhi_device_ = rhi_api_->CreateDevice();
    rhi_swapchain_ = rhi_device_->CreateSwapchain(window_.GetNativeHandle(), width_, height_, 3);
    rhi_imgui_renderer_ = rhi_device_->CreateImGuiRenderer(window_.GetGlfwWindow(), *rhi_swapchain_);
    texture_manager_ = std::make_unique<TextureManager>(*rhi_device_);
    scene_ = std::make_unique<Scene>(scene_path_.c_str(), scene_scale_, flip_yz_, *texture_manager_);
    scene_->AddDirectionalLight({ -0.6f, -1.5f, 3.5f }, { 30.0f, 20.0f, 10.0f });

    camera_controller_ = std::make_unique<CameraController>(window_);

    if (rhi_device_->SupportsRayQuery())
    {
        acc_structure_ = std::make_unique<HardwareRtAccelerationStructure>();
    }
    else
    {
        auto bvh = std::make_unique<Bvh>();
        bvh->BuildCPU(scene_->GetVertices(), scene_->GetIndices(), scene_->GetMeshInfos(), scene_->GetInstanceInfos());
        acc_structure_ = std::move(bvh);
    }

    // TODO, NOTE: this is done after building the acc structure because it
    // reorders triangles Need to get rid of reordering
    scene_->Finalize();
    texture_manager_->UploadPendingTextures();

    // Create path tracer
    path_tracer_ = std::make_unique<PathTracer>(width_, height_, *rhi_device_, *rhi_swapchain_);

    // Upload scene data to the GPU
    path_tracer_->UploadGPUData(*scene_, *acc_structure_, *texture_manager_);
}

Render::~Render()
{
    if (rhi_device_)
    {
        rhi_device_->WaitIdle();
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
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate);
        ImGui::Text("Press \"R\" to reload pipelines");
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
            path_tracer_->SetMaxBounces((uint32_t)gui_params_.max_bounces);
        }

        if (ImGui::Checkbox("Enable denoiser", &gui_params_.enable_denoiser))
        {
            path_tracer_->EnableDenoiser(gui_params_.enable_denoiser);
        }

        if (ImGui::Checkbox("Blue noise sampler", &gui_params_.enable_blue_noise))
        {
            path_tracer_->SetSamplerType(gui_params_.enable_blue_noise ? PathTracer::SamplerType::kBlueNoise
                                                                       : PathTracer::SamplerType::kRandom);
        }

        if (ImGui::Checkbox("Enable white furnace", &gui_params_.enable_white_furnace))
        {
            path_tracer_->EnableWhiteFurnace(gui_params_.enable_white_furnace);
        }

        static int aov_index = 0;
        const char* aov_names[] = { "Shaded Color", "Diffuse Albedo", "Depth", "Normal", "Motion Vectors" };
        if (ImGui::Combo("AOV", &aov_index, aov_names, 5))
        {
            path_tracer_->SetAOV((PathTracer::AOV)aov_index);
        }
    }
    ImGui::End();
}

void Render::HandlePipelineHotReload()
{
    bool const is_reload_key_pressed = window_.GetKey(KeyCode::kR);
    if (!is_reload_key_pressed || was_reload_key_pressed_)
    {
        was_reload_key_pressed_ = is_reload_key_pressed;
        return;
    }

    was_reload_key_pressed_ = true;

    gpu::PipelineReloadResult const result = rhi_device_->ReloadPipelines();
    if (result.success)
    {
        std::cout << "Reloaded " << result.reloaded_count << " pipelines" << std::endl;
        path_tracer_->RequestReset();
        return;
    }

    std::cerr << "Pipeline reload failed: " << result.error << std::endl;
}

void Render::RenderFrame()
{
    FrameBegin();

    rhi_imgui_renderer_->NewFrame();
    DrawGUI();

    bool need_to_reset = false;

    camera_controller_->Update((float)GetDeltaTime());
    path_tracer_->SetCameraData(camera_controller_->GetData());
    HandlePipelineHotReload();

    need_to_reset = need_to_reset || camera_controller_->IsChanged();

    if (need_to_reset)
    {
        path_tracer_->RequestReset();
    }

    gpu::Queue& queue = rhi_device_->GetQueue(gpu::QueueType::kGraphics);
    rhi_command_buffer_ = queue.CreateCommandBuffer();
    path_tracer_->SetCommandBuffer(*rhi_command_buffer_);
    path_tracer_->Integrate();
    rhi_imgui_renderer_->Render(*rhi_command_buffer_);
    rhi_command_buffer_->TransitionBarrier(rhi_swapchain_->GetCurrentImage(), gpu::ImageLayout::kRenderTarget,
        gpu::ImageLayout::kPresent);
    path_tracer_->SetCurrentSwapchainImageLayout(gpu::ImageLayout::kPresent);
    queue.Submit(std::move(rhi_command_buffer_));
    rhi_swapchain_->Present();

    FrameEnd();
}
