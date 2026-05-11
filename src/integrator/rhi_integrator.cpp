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

#include "rhi_integrator.hpp"

#include "acceleration_structure.hpp"
#include "gpu_command_buffer.hpp"
#include "gpu_descriptor_set.hpp"
#include "gpu_device.hpp"
#include "gpu_image.hpp"
#include "gpu_pipeline.hpp"
#include "gpu_queue.hpp"
#include "gpu_swapchain.hpp"
#include "scene/scene.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace
{
struct RhiCameraData
{
    float camera_position_fov[4];
    float camera_front_aspect[4];
    float camera_up_aperture[4];
    float camera_lens[4];
    float prev_camera_position_fov[4];
    float prev_camera_front_aspect[4];
    float prev_camera_up_aperture[4];
    float prev_camera_lens[4];
    uint32_t render_size[4];
    uint32_t scene_counts[4];
    uint32_t render_params[4];
};

constexpr uint32_t kRenderFlagWhiteFurnace = 1u;
constexpr uint32_t kRenderFlagDenoiser = 2u;
} // namespace

RhiIntegrator::RhiIntegrator(uint32_t width, uint32_t height, AccelerationStructure& acc_structure,
    void* window_native_handle)
    : Integrator(width, height, acc_structure), api_(gpu::Api::Create(gpu::ApiType::kD3D12))
{
    if (!api_)
    {
        throw std::runtime_error("Failed to create GpuApi");
    }
    api_->SetShaderPath("src/kernels/hlsl");

    device_ = api_->CreateDevice();
    swapchain_ = device_->CreateSwapchain(window_native_handle, width_, height_, 3);
    output_image_ = device_->CreateImage(width_, height_, swapchain_->GetFormat(), 1, 1,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);

    uint32_t num_pixels = width_ * height_;
    for (uint32_t i = 0; i < 2; ++i)
    {
        rays_buffers_[i] = CreateStorageBuffer(num_pixels * sizeof(Ray), sizeof(Ray));
        pixel_indices_buffers_[i] =
            CreateStorageBuffer(num_pixels * sizeof(uint32_t), sizeof(uint32_t));
        ray_counter_buffers_[i] = CreateStorageBuffer(sizeof(uint32_t), sizeof(uint32_t));
    }

    shadow_rays_buffer_ = CreateStorageBuffer(num_pixels * sizeof(Ray), sizeof(Ray));
    shadow_pixel_indices_buffer_ =
        CreateStorageBuffer(num_pixels * sizeof(uint32_t), sizeof(uint32_t));
    shadow_ray_counter_buffer_ = CreateStorageBuffer(sizeof(uint32_t), sizeof(uint32_t));
    hits_buffer_ = CreateStorageBuffer(num_pixels * sizeof(Hit), sizeof(Hit));
    shadow_hits_buffer_ = CreateStorageBuffer(num_pixels * sizeof(uint32_t), sizeof(uint32_t));
    throughputs_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float3), sizeof(float3));
    sample_counter_buffer_ = CreateStorageBuffer(sizeof(uint32_t), sizeof(uint32_t));
    radiance_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float4), sizeof(float4));
    prev_radiance_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float4), sizeof(float4));
    diffuse_albedo_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float3), sizeof(float3));
    depth_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float), sizeof(float));
    prev_depth_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float), sizeof(float));
    normal_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float3), sizeof(float3));
    motion_vectors_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float4), sizeof(float4));
    direct_light_samples_buffer_ = CreateStorageBuffer(num_pixels * sizeof(float3), sizeof(float3));
    for (uint32_t bounce = 0; bounce < bounce_buffers_.size(); ++bounce)
    {
        bounce_buffers_[bounce] = CreateUploadBuffer(&bounce, sizeof(bounce), sizeof(bounce),
            gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    }

    CreateKernels();
}

RhiIntegrator::~RhiIntegrator()
{
    if (device_)
    {
        device_->GetQueue(gpu::QueueType::kGraphics).WaitIdle();
    }
}

void RhiIntegrator::UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure)
{
    auto const& triangles = scene.GetTriangles();
    auto const& nodes = acc_structure.GetNodes();
    auto const& materials = scene.GetMaterials();
    auto const& lights = scene.GetLights();
    auto const& textures = scene.GetTextures();
    auto const& texture_data = scene.GetTextureData();

    triangle_count_ = static_cast<uint32_t>(triangles.size());
    node_count_ = static_cast<uint32_t>(nodes.size());
    light_count_ = static_cast<uint32_t>(lights.size());
    texture_count_ = static_cast<uint32_t>(textures.size());

    triangle_buffer_ = CreateUploadBuffer(triangles.empty() ? nullptr : triangles.data(),
        triangles.size() * sizeof(Triangle), sizeof(Triangle),
        gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    node_buffer_ = CreateUploadBuffer(nodes.empty() ? nullptr : nodes.data(),
        nodes.size() * sizeof(LinearBVHNode), sizeof(LinearBVHNode),
        gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    material_buffer_ = CreateUploadBuffer(materials.empty() ? nullptr : materials.data(),
        materials.size() * sizeof(PackedMaterial), sizeof(PackedMaterial),
        gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    light_buffer_ =
        CreateUploadBuffer(lights.empty() ? nullptr : lights.data(), lights.size() * sizeof(Light),
            sizeof(Light), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    texture_buffer_ = CreateUploadBuffer(textures.empty() ? nullptr : textures.data(),
        textures.size() * sizeof(Texture), sizeof(Texture),
        gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    texture_data_buffer_ = CreateUploadBuffer(texture_data.empty() ? nullptr : texture_data.data(),
        texture_data.size() * sizeof(uint32_t), sizeof(uint32_t),
        gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);

    SetCameraData(camera_);
    RebuildDescriptorSets();
    Reset();
}

void RhiIntegrator::SetCameraData(Camera const& camera)
{
    if (prev_camera_.fov == 0.0f)
    {
        prev_camera_ = camera;
    }

    camera_ = camera;
    bool const had_camera_buffer = camera_buffer_ != nullptr;
    UpdateFrameData();

    if (triangle_buffer_ && !had_camera_buffer)
    {
        RebuildDescriptorSets();
    }

    prev_camera_ = camera;
}

void RhiIntegrator::SetSamplerType(SamplerType sampler_type)
{
    if (sampler_type == sampler_type_)
    {
        return;
    }

    sampler_type_ = sampler_type;
    RequestReset();
}

void RhiIntegrator::SetAOV(AOV aov)
{
    if (aov == aov_)
    {
        return;
    }

    aov_ = aov;
    UpdateFrameData();
    RequestReset();
}

void RhiIntegrator::EnableDenoiser(bool enable)
{
    if (enable == enable_denoiser_)
    {
        return;
    }

    enable_denoiser_ = enable;
    UpdateFrameData();
    RequestReset();
}

void RhiIntegrator::UpdateFrameData()
{
    RhiCameraData data = {};
    data.camera_position_fov[0] = camera_.position.x;
    data.camera_position_fov[1] = camera_.position.y;
    data.camera_position_fov[2] = camera_.position.z;
    data.camera_position_fov[3] = camera_.fov;

    data.camera_front_aspect[0] = camera_.front.x;
    data.camera_front_aspect[1] = camera_.front.y;
    data.camera_front_aspect[2] = camera_.front.z;
    data.camera_front_aspect[3] = camera_.aspect_ratio;

    data.camera_up_aperture[0] = camera_.up.x;
    data.camera_up_aperture[1] = camera_.up.y;
    data.camera_up_aperture[2] = camera_.up.z;
    data.camera_up_aperture[3] = camera_.aperture;

    data.camera_lens[0] = camera_.focus_distance;
    data.camera_lens[1] = 0.0f;
    data.camera_lens[2] = 0.0f;
    data.camera_lens[3] = 0.0f;

    data.prev_camera_position_fov[0] = prev_camera_.position.x;
    data.prev_camera_position_fov[1] = prev_camera_.position.y;
    data.prev_camera_position_fov[2] = prev_camera_.position.z;
    data.prev_camera_position_fov[3] = prev_camera_.fov;

    data.prev_camera_front_aspect[0] = prev_camera_.front.x;
    data.prev_camera_front_aspect[1] = prev_camera_.front.y;
    data.prev_camera_front_aspect[2] = prev_camera_.front.z;
    data.prev_camera_front_aspect[3] = prev_camera_.aspect_ratio;

    data.prev_camera_up_aperture[0] = prev_camera_.up.x;
    data.prev_camera_up_aperture[1] = prev_camera_.up.y;
    data.prev_camera_up_aperture[2] = prev_camera_.up.z;
    data.prev_camera_up_aperture[3] = prev_camera_.aperture;

    data.prev_camera_lens[0] = prev_camera_.focus_distance;
    data.prev_camera_lens[1] = 0.0f;
    data.prev_camera_lens[2] = 0.0f;
    data.prev_camera_lens[3] = 0.0f;

    data.render_size[0] = width_;
    data.render_size[1] = height_;
    data.render_size[2] = 0u;
    data.render_size[3] = 0u;

    data.scene_counts[0] = triangle_count_;
    data.scene_counts[1] = node_count_;
    data.scene_counts[2] = light_count_;
    data.scene_counts[3] = texture_count_;

    data.render_params[0] = static_cast<uint32_t>(aov_);
    data.render_params[1] = (enable_white_furnace_ ? kRenderFlagWhiteFurnace : 0u) |
                            (enable_denoiser_ ? kRenderFlagDenoiser : 0u);
    data.render_params[2] = 0u;
    data.render_params[3] = 0u;

    if (!camera_buffer_)
    {
        camera_buffer_ = CreateUploadBuffer(&data, sizeof(data), sizeof(data),
            gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kConstant);
        return;
    }

    void* mapped_data = camera_buffer_->Map();
    std::memcpy(mapped_data, &data, sizeof(data));
    camera_buffer_->Unmap();
}

void RhiIntegrator::CreateKernels()
{
    reset_pipeline_ = device_->CreateComputePipeline("reset.cs");
    raygen_pipeline_ = device_->CreateComputePipeline("raygeneration.cs");
    trace_pipeline_ = device_->CreateComputePipeline("trace_bvh.cs");
    trace_shadow_pipeline_ = device_->CreateComputePipeline("trace_shadow_bvh.cs");
    aov_pipeline_ = device_->CreateComputePipeline("aov.cs");
    miss_pipeline_ = device_->CreateComputePipeline("miss.cs");
    hit_surface_pipeline_ = device_->CreateComputePipeline("hit_surface.cs");
    accumulate_direct_pipeline_ = device_->CreateComputePipeline("accumulate_direct_samples.cs");
    clear_counter_pipeline_ = device_->CreateComputePipeline("clear_counter.cs");
    clear_sample_counter_pipeline_ = device_->CreateComputePipeline("clear_sample_counter.cs");
    increment_counter_pipeline_ = device_->CreateComputePipeline("increment_counter.cs");
    denoiser_pipeline_ = device_->CreateComputePipeline("denoiser.cs");
    resolve_pipeline_ = device_->CreateComputePipeline("resolve.cs");

    if (camera_buffer_ && triangle_buffer_)
    {
        RebuildDescriptorSets();
    }
}

void RhiIntegrator::Reset()
{
    if (reset_set_)
    {
        if (!enable_denoiser_)
        {
            SubmitCompute(clear_sample_counter_pipeline_, clear_sample_counter_set_, 1);
        }
        SubmitCompute(reset_pipeline_, reset_set_, DivideAndRoundUp(width_ * height_, 256));
    }
}

void RhiIntegrator::AdvanceSampleCount()
{
    SubmitCompute(increment_counter_pipeline_, increment_counter_set_, 1);
}

void RhiIntegrator::GenerateRays()
{
    SubmitCompute(raygen_pipeline_, raygen_set_, DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::IntersectRays(uint32_t bounce)
{
    SubmitCompute(
        trace_pipeline_, trace_sets_[bounce & 1], DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::ComputeAOVs()
{
    SubmitCompute(aov_pipeline_, aov_set_, DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::ShadeMissedRays(uint32_t bounce)
{
    SubmitCompute(miss_pipeline_, miss_sets_[bounce & 1], DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::ShadeSurfaceHits(uint32_t bounce)
{
    SubmitCompute(hit_surface_pipeline_, hit_surface_sets_[bounce & 1],
        DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::IntersectShadowRays()
{
    SubmitCompute(
        trace_shadow_pipeline_, trace_shadow_set_, DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::AccumulateDirectSamples()
{
    SubmitCompute(accumulate_direct_pipeline_, accumulate_direct_set_,
        DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::ClearOutgoingRayCounter(uint32_t bounce)
{
    SubmitCompute(clear_counter_pipeline_, clear_counter_sets_[(bounce + 1) & 1], 1);
}

void RhiIntegrator::ClearShadowRayCounter()
{
    SubmitCompute(clear_counter_pipeline_, clear_shadow_counter_set_, 1);
}

void RhiIntegrator::Denoise()
{
    SubmitCompute(denoiser_pipeline_, denoiser_set_, DivideAndRoundUp(width_ * height_, 256));
}

void RhiIntegrator::CopyHistoryBuffers()
{
    gpu::Queue& queue = device_->GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr cmd_buffer = queue.CreateCommandBuffer();
    cmd_buffer->CopyBuffer(radiance_buffer_.get(), 0, prev_radiance_buffer_.get(), 0,
        width_ * height_ * sizeof(float4));
    cmd_buffer->CopyBuffer(
        depth_buffer_.get(), 0, prev_depth_buffer_.get(), 0, width_ * height_ * sizeof(float));
    queue.Submit(std::move(cmd_buffer));
}

void RhiIntegrator::ResolveRadiance()
{
    gpu::ImagePtr swapchain_image = swapchain_->GetCurrentImage();
    gpu::Queue& queue = device_->GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr cmd_buffer = queue.CreateCommandBuffer();

    cmd_buffer->TransitionBarrier(
        output_image_, output_layout_, gpu::ImageLayout::kShaderReadWrite);
    cmd_buffer->BindPipeline(resolve_pipeline_);
    cmd_buffer->BindDescriptorSet(resolve_set_);
    cmd_buffer->Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    cmd_buffer->TransitionBarrier(
        output_image_, gpu::ImageLayout::kShaderReadWrite, gpu::ImageLayout::kCopySrc);
    output_layout_ = gpu::ImageLayout::kCopySrc;
    cmd_buffer->TransitionBarrier(
        swapchain_image, gpu::ImageLayout::kPresent, gpu::ImageLayout::kCopyDst);
    cmd_buffer->CopyImage(swapchain_image.get(), output_image_.get());
    cmd_buffer->TransitionBarrier(
        swapchain_image, gpu::ImageLayout::kCopyDst, gpu::ImageLayout::kPresent);

    queue.Submit(std::move(cmd_buffer));
    swapchain_->Present();
}

gpu::BufferPtr RhiIntegrator::CreateUploadBuffer(
    void const* data, size_t size, uint32_t stride, gpu::BufferFlags flags)
{
    size_t allocation_size = std::max<size_t>(size, stride);
    gpu::BufferPtr buffer = device_->CreateBuffer(allocation_size, stride, flags);
    if (data)
    {
        void* mapped_data = buffer->Map();
        std::memcpy(mapped_data, data, size);
        buffer->Unmap();
    }
    return buffer;
}

gpu::BufferPtr RhiIntegrator::CreateStorageBuffer(size_t size, uint32_t stride)
{
    size_t allocation_size = std::max<size_t>(size, stride);
    return device_->CreateBuffer(
        allocation_size, stride, gpu::BufferFlags::kShaderResource | gpu::BufferFlags::kStorage);
}

void RhiIntegrator::RebuildDescriptorSets()
{
    if (!camera_buffer_ || !triangle_buffer_)
    {
        return;
    }

    reset_set_ = reset_pipeline_->CreateDescriptorSet();
    reset_set_->BindBuffer(*radiance_buffer_, 0);

    raygen_set_ = raygen_pipeline_->CreateDescriptorSet();
    raygen_set_->BindBuffer(*camera_buffer_, 0);
    raygen_set_->BindBuffer(*rays_buffers_[0], 1);
    raygen_set_->BindBuffer(*ray_counter_buffers_[0], 2);
    raygen_set_->BindBuffer(*pixel_indices_buffers_[0], 3);
    raygen_set_->BindBuffer(*throughputs_buffer_, 4);
    raygen_set_->BindBuffer(*sample_counter_buffer_, 5);
    raygen_set_->BindBuffer(*diffuse_albedo_buffer_, 6);
    raygen_set_->BindBuffer(*depth_buffer_, 7);
    raygen_set_->BindBuffer(*normal_buffer_, 8);
    raygen_set_->BindBuffer(*motion_vectors_buffer_, 9);

    for (uint32_t i = 0; i < 2; ++i)
    {
        trace_sets_[i] = trace_pipeline_->CreateDescriptorSet();
        trace_sets_[i]->BindBuffer(*rays_buffers_[i], 0);
        trace_sets_[i]->BindBuffer(*ray_counter_buffers_[i], 1);
        trace_sets_[i]->BindBuffer(*hits_buffer_, 2);
        trace_sets_[i]->BindBuffer(*triangle_buffer_, 3);
        trace_sets_[i]->BindBuffer(*node_buffer_, 4);

        miss_sets_[i] = miss_pipeline_->CreateDescriptorSet();
        miss_sets_[i]->BindBuffer(*camera_buffer_, 0);
        miss_sets_[i]->BindBuffer(*ray_counter_buffers_[i], 1);
        miss_sets_[i]->BindBuffer(*pixel_indices_buffers_[i], 2);
        miss_sets_[i]->BindBuffer(*hits_buffer_, 3);
        miss_sets_[i]->BindBuffer(*throughputs_buffer_, 4);
        miss_sets_[i]->BindBuffer(*radiance_buffer_, 5);

        clear_counter_sets_[i] = clear_counter_pipeline_->CreateDescriptorSet();
        clear_counter_sets_[i]->BindBuffer(*ray_counter_buffers_[i], 0);
    }

    for (uint32_t i = 0; i < 2; ++i)
    {
        uint32_t ping = i & 1u;
        uint32_t pong = (i + 1u) & 1u;
        hit_surface_sets_[i] = hit_surface_pipeline_->CreateDescriptorSet();
        hit_surface_sets_[i]->BindBuffer(*camera_buffer_, 0);

        hit_surface_sets_[i]->BindBuffer(*rays_buffers_[ping], 1);
        hit_surface_sets_[i]->BindBuffer(*pixel_indices_buffers_[ping], 2);
        hit_surface_sets_[i]->BindBuffer(*ray_counter_buffers_[ping], 3);

        hit_surface_sets_[i]->BindBuffer(*rays_buffers_[pong], 4);
        hit_surface_sets_[i]->BindBuffer(*pixel_indices_buffers_[pong], 5);
        hit_surface_sets_[i]->BindBuffer(*ray_counter_buffers_[pong], 6);

        hit_surface_sets_[i]->BindBuffer(*hits_buffer_, 7);

        hit_surface_sets_[i]->BindBuffer(*shadow_rays_buffer_, 8);
        hit_surface_sets_[i]->BindBuffer(*shadow_pixel_indices_buffer_, 9);
        hit_surface_sets_[i]->BindBuffer(*shadow_ray_counter_buffer_, 10);
        hit_surface_sets_[i]->BindBuffer(*direct_light_samples_buffer_, 11);

        hit_surface_sets_[i]->BindBuffer(*throughputs_buffer_, 12);
        hit_surface_sets_[i]->BindBuffer(*radiance_buffer_, 13);

        hit_surface_sets_[i]->BindBuffer(*bounce_buffers_[i], 14);
        hit_surface_sets_[i]->BindBuffer(*sample_counter_buffer_, 15);

        hit_surface_sets_[i]->BindBuffer(*triangle_buffer_, 16);
        hit_surface_sets_[i]->BindBuffer(*material_buffer_, 17);
        hit_surface_sets_[i]->BindBuffer(*light_buffer_, 18);

        hit_surface_sets_[i]->BindBuffer(*texture_buffer_, 19);
        hit_surface_sets_[i]->BindBuffer(*texture_data_buffer_, 20);
    }

    trace_shadow_set_ = trace_shadow_pipeline_->CreateDescriptorSet();
    trace_shadow_set_->BindBuffer(*shadow_rays_buffer_, 0);
    trace_shadow_set_->BindBuffer(*shadow_ray_counter_buffer_, 1);
    trace_shadow_set_->BindBuffer(*shadow_hits_buffer_, 2);
    trace_shadow_set_->BindBuffer(*triangle_buffer_, 3);
    trace_shadow_set_->BindBuffer(*node_buffer_, 4);

    aov_set_ = aov_pipeline_->CreateDescriptorSet();
    aov_set_->BindBuffer(*camera_buffer_, 0);
    aov_set_->BindBuffer(*rays_buffers_[0], 1);
    aov_set_->BindBuffer(*ray_counter_buffers_[0], 2);
    aov_set_->BindBuffer(*pixel_indices_buffers_[0], 3);
    aov_set_->BindBuffer(*hits_buffer_, 4);
    aov_set_->BindBuffer(*diffuse_albedo_buffer_, 5);
    aov_set_->BindBuffer(*depth_buffer_, 6);
    aov_set_->BindBuffer(*normal_buffer_, 7);
    aov_set_->BindBuffer(*motion_vectors_buffer_, 8);
    aov_set_->BindBuffer(*triangle_buffer_, 9);
    aov_set_->BindBuffer(*material_buffer_, 10);
    aov_set_->BindBuffer(*texture_buffer_, 11);
    aov_set_->BindBuffer(*texture_data_buffer_, 12);

    accumulate_direct_set_ = accumulate_direct_pipeline_->CreateDescriptorSet();
    accumulate_direct_set_->BindBuffer(*shadow_ray_counter_buffer_, 0);
    accumulate_direct_set_->BindBuffer(*shadow_pixel_indices_buffer_, 1);
    accumulate_direct_set_->BindBuffer(*shadow_hits_buffer_, 2);
    accumulate_direct_set_->BindBuffer(*radiance_buffer_, 3);
    accumulate_direct_set_->BindBuffer(*direct_light_samples_buffer_, 4);

    clear_shadow_counter_set_ = clear_counter_pipeline_->CreateDescriptorSet();
    clear_shadow_counter_set_->BindBuffer(*shadow_ray_counter_buffer_, 0);

    clear_sample_counter_set_ = clear_sample_counter_pipeline_->CreateDescriptorSet();
    clear_sample_counter_set_->BindBuffer(*sample_counter_buffer_, 0);

    increment_counter_set_ = increment_counter_pipeline_->CreateDescriptorSet();
    increment_counter_set_->BindBuffer(*sample_counter_buffer_, 0);

    denoiser_set_ = denoiser_pipeline_->CreateDescriptorSet();
    denoiser_set_->BindBuffer(*camera_buffer_, 0);
    denoiser_set_->BindBuffer(*radiance_buffer_, 1);
    denoiser_set_->BindBuffer(*prev_radiance_buffer_, 2);
    denoiser_set_->BindBuffer(*depth_buffer_, 3);
    denoiser_set_->BindBuffer(*prev_depth_buffer_, 4);
    denoiser_set_->BindBuffer(*motion_vectors_buffer_, 5);

    resolve_set_ = resolve_pipeline_->CreateDescriptorSet();
    resolve_set_->BindBuffer(*camera_buffer_, 0);
    resolve_set_->BindImage(*output_image_, 1);
    resolve_set_->BindBuffer(*radiance_buffer_, 2);
    resolve_set_->BindBuffer(*sample_counter_buffer_, 3);
    resolve_set_->BindBuffer(*diffuse_albedo_buffer_, 4);
    resolve_set_->BindBuffer(*depth_buffer_, 5);
    resolve_set_->BindBuffer(*normal_buffer_, 6);
    resolve_set_->BindBuffer(*motion_vectors_buffer_, 7);
}

void RhiIntegrator::SubmitCompute(gpu::ComputePipelinePtr const& pipeline,
    gpu::DescriptorSetPtr const& descriptor_set, uint32_t groups_x, uint32_t groups_y,
    uint32_t groups_z)
{
    gpu::Queue& queue = device_->GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr cmd_buffer = queue.CreateCommandBuffer();
    cmd_buffer->BindPipeline(pipeline);
    cmd_buffer->BindDescriptorSet(descriptor_set);
    cmd_buffer->Dispatch(groups_x, groups_y, groups_z);
    queue.Submit(std::move(cmd_buffer));
}

uint32_t RhiIntegrator::DivideAndRoundUp(uint32_t value, uint32_t divisor)
{
    return (value + divisor - 1) / divisor;
}
