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
#include "integrator/cl_pt_integrator.hpp"
#include "integrator/gl_pt_integrator.hpp"
#include "mathlib/mathlib.hpp"
#include "utils/cl_exception.hpp"
#include "bvh.hpp"
#include "Utils/window.hpp"
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_win32.h>
#include <iostream>
#include <fstream>
#include <sstream>

Render::Render(Window& window, RenderBackend backend)
    : window_(window)
    , render_backend_(backend)
    , width_(window.GetWidth())
    , height_(window.GetHeight())
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplWin32_Init(window_.GetNativeHandle());

    if (render_backend_ == RenderBackend::kOpenCL)
    {
        std::vector<cl::Platform> all_platforms;
        cl::Platform::get(&all_platforms);
        if (all_platforms.empty())
        {
            throw std::runtime_error("No OpenCL platforms found");
        }

        cl_context_ = std::make_shared<CLContext>(all_platforms[0],
            window_.GetDisplayContext(), window_.GetGLContext());
    }

#ifndef NDEBUG
    //char const* scene_path = "assets/CornellBox_Dragon.obj";
    //char const* scene_path = "../Scenes/MtlTest/gi.obj";
    char const* scene_path = "../Scenes/MtlTest/mtltest.obj";
    //char const* scene_path = "assets/ShaderBalls.obj";
    //float scene_scale = 1.0f;
    float scene_scale = 0.01f;
    bool flip_yz = true;
#else
    char const* scene_path = "../Scenes/sponza/sponza.obj";
    float scene_scale = 0.005f;
    bool flip_yz = true;
#endif

    scene_ = std::make_unique<Scene>(scene_path, scene_scale, flip_yz);

    auto get_rand = [](float min, float max)
    {
        return min + (float)rand() / RAND_MAX * (max - min);
    };

    scene_->AddDirectionalLight({ -0.6f, -1.5f, 3.5f }, { 15.0f, 10.0f, 5.0f });

    //scene_->AddPointLight({ 0.0f, 0.0f, 1.5f }, { 2.0f, 2.0f, 2.0f });

    //for (int i = 0; i < 10; ++i)
    //{
    //    scene_->AddPointLight({ i - 5.0f, 0.0f, 2.0f },
    //        { get_rand(0.0f, 10.0f), get_rand(0.0f, 50.0f), get_rand(0.0f, 50.0f) });
    //}

    framebuffer_ = std::make_unique<Framebuffer>(width_, height_);
    camera_controller_ = std::make_unique<CameraController>(window_);

    // Create acc structure
    acc_structure_ = std::make_unique<Bvh>();
    // Build it right here
    acc_structure_->BuildCPU(scene_->GetTriangles());

    // TODO, NOTE: this is done after building the acc structure because it reorders triangles
    // Need to get rid of reordering
    scene_->Finalize();

    // Create integrator
    if (render_backend_ == RenderBackend::kOpenCL)
    {
        integrator_ = std::make_unique<CLPathTraceIntegrator>(width_, height_, *acc_structure_,
            *cl_context_, framebuffer_->GetGLImage());
    }
    else
    {
        integrator_ = std::make_unique<GLPathTraceIntegrator>(width_, height_, *acc_structure_,
            framebuffer_->GetGLImage());
    }

    // Upload scene data to the GPU
    integrator_->UploadGPUData(*scene_, *acc_structure_);
}

Render::~Render()
{
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Render::FrameEnd()
{
    camera_controller_->OnEndFrame();
    glFinish();
    window_.SwapBuffers();
    prev_frame_time_ = start_frame_time_;
}

void Render::ReloadKernels()
{
    if (cl_context_)
    {
        cl_context_->ReloadKernels();
    }
}

void Render::DrawGUI()
{
    ImGui::Begin("PerformanceStats", nullptr,
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::SetWindowSize(ImVec2(350, 50));
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
            integrator_->SetMaxBounces((std::uint32_t)gui_params_.max_bounces);
        }

        if (ImGui::Checkbox("Enable denoiser", &gui_params_.enable_denoiser))
        {
            integrator_->EnableDenoiser(gui_params_.enable_denoiser);
        }

        if (ImGui::Checkbox("Blue noise sampler", &gui_params_.enable_blue_noise))
        {
            integrator_->SetSamplerType(gui_params_.enable_blue_noise ?
                Integrator::SamplerType::kBlueNoise : Integrator::SamplerType::kRandom);
        }

        if (ImGui::Checkbox("Enable white furnace", &gui_params_.enable_white_furnace))
        {
            integrator_->EnableWhiteFurnace(gui_params_.enable_white_furnace);
        }

        static int aov_index = 0;
        const char* aov_names[] = { "Shaded Color", "Diffuse Albedo", "Depth", "Normal", "Motion Vectors" };
        if (ImGui::Combo("AOV", &aov_index, aov_names, 5))
        {
            integrator_->SetAOV((Integrator::AOV)aov_index);
        }
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Render::RenderFrame()
{
    FrameBegin();

    glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    bool need_to_reset = false;

    if (window_.IsKeyPressed('R'))
    {
        ReloadKernels();
        need_to_reset = true;
    }

    camera_controller_->Update((float)GetDeltaTime());
    integrator_->SetCameraData(camera_controller_->GetData());

    need_to_reset = need_to_reset || camera_controller_->IsChanged();

    if (need_to_reset)
    {
        integrator_->RequestReset();
    }

    integrator_->Integrate();
    framebuffer_->Present();

    // Draw GUI
    DrawGUI();

    FrameEnd();
}
