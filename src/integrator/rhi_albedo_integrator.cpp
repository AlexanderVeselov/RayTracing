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

#include "rhi_albedo_integrator.hpp"

#include "acceleration_structure.hpp"
#include "gpu_buffer.hpp"
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
    float camera_up_padding[4];
    std::uint32_t scene_counts[4];
};
}

RhiAlbedoIntegrator::RhiAlbedoIntegrator(std::uint32_t width, std::uint32_t height,
    AccelerationStructure& acc_structure, void* window_native_handle)
    : Integrator(width, height, acc_structure)
    , api_(gpu::Api::Create(gpu::ApiType::kD3D12))
{
    if (!api_)
    {
        throw std::runtime_error("Failed to create GpuApi");
    }

    device_ = api_->CreateDevice();
    swapchain_ = device_->CreateSwapchain(window_native_handle, width_, height_, 3);
    output_image_ = device_->CreateImage(width_, height_, swapchain_->GetFormat(), 1, 1,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);

    CreateKernels();
}

RhiAlbedoIntegrator::~RhiAlbedoIntegrator()
{
    if (device_)
    {
        device_->GetQueue(gpu::QueueType::kGraphics).WaitIdle();
    }
}

void RhiAlbedoIntegrator::UploadGPUData(
    Scene const& scene, AccelerationStructure const& acc_structure)
{
    auto const& triangles = scene.GetTriangles();
    auto const& nodes = acc_structure.GetNodes();
    auto const& materials = scene.GetMaterials();
    auto const& lights = scene.GetLights();
    auto const& textures = scene.GetTextures();
    auto const& texture_data = scene.GetTextureData();

    triangle_count_ = static_cast<std::uint32_t>(triangles.size());
    node_count_ = static_cast<std::uint32_t>(nodes.size());
    light_count_ = static_cast<std::uint32_t>(lights.size());
    texture_count_ = static_cast<std::uint32_t>(textures.size());
    texture_data_count_ = static_cast<std::uint32_t>(texture_data.size());

    triangle_buffer_ = CreateUploadBuffer(triangles.empty() ? nullptr : triangles.data(),
        triangles.size() * sizeof(Triangle),
        sizeof(Triangle), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    node_buffer_ = CreateUploadBuffer(nodes.empty() ? nullptr : nodes.data(),
        nodes.size() * sizeof(LinearBVHNode),
        sizeof(LinearBVHNode), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    material_buffer_ = CreateUploadBuffer(materials.empty() ? nullptr : materials.data(),
        materials.size() * sizeof(PackedMaterial),
        sizeof(PackedMaterial), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    light_buffer_ = CreateUploadBuffer(lights.empty() ? nullptr : lights.data(),
        lights.size() * sizeof(Light),
        sizeof(Light), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    texture_buffer_ = CreateUploadBuffer(textures.empty() ? nullptr : textures.data(),
        textures.size() * sizeof(Texture),
        sizeof(Texture), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);
    texture_data_buffer_ = CreateUploadBuffer(texture_data.empty() ? nullptr : texture_data.data(),
        texture_data.size() * sizeof(std::uint32_t),
        sizeof(std::uint32_t), gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kShaderResource);

    SetCameraData(camera_);
    RebuildDescriptorSet();
}

void RhiAlbedoIntegrator::SetCameraData(Camera const& camera)
{
    camera_ = camera;

    RhiCameraData data = {};
    data.camera_position_fov[0] = camera.position.x;
    data.camera_position_fov[1] = camera.position.y;
    data.camera_position_fov[2] = camera.position.z;
    data.camera_position_fov[3] = camera.fov;

    data.camera_front_aspect[0] = camera.front.x;
    data.camera_front_aspect[1] = camera.front.y;
    data.camera_front_aspect[2] = camera.front.z;
    data.camera_front_aspect[3] = camera.aspect_ratio;

    data.camera_up_padding[0] = camera.up.x;
    data.camera_up_padding[1] = camera.up.y;
    data.camera_up_padding[2] = camera.up.z;
    data.camera_up_padding[3] = 0.0f;

    data.scene_counts[0] = triangle_count_;
    data.scene_counts[1] = node_count_;
    data.scene_counts[2] = light_count_;
    data.scene_counts[3] = texture_count_;

    camera_buffer_ = CreateUploadBuffer(&data, sizeof(data), sizeof(data),
        gpu::BufferFlags::kCpuAccess | gpu::BufferFlags::kConstant);

    if (pipeline_ && triangle_buffer_ && node_buffer_ && material_buffer_)
    {
        RebuildDescriptorSet();
    }
}

void RhiAlbedoIntegrator::SetSamplerType(SamplerType sampler_type)
{
    sampler_type_ = sampler_type;
}

void RhiAlbedoIntegrator::SetAOV(AOV aov)
{
    aov_ = aov;
}

void RhiAlbedoIntegrator::EnableDenoiser(bool enable)
{
    enable_denoiser_ = enable;
}

void RhiAlbedoIntegrator::CreateKernels()
{
    pipeline_ = device_->CreateComputePipeline("src/kernels/hlsl/rhi_albedo.cs");
    if (camera_buffer_ && triangle_buffer_ && node_buffer_ && material_buffer_)
    {
        RebuildDescriptorSet();
    }
}

void RhiAlbedoIntegrator::Reset()
{
}

void RhiAlbedoIntegrator::AdvanceSampleCount()
{
}

void RhiAlbedoIntegrator::GenerateRays()
{
}

void RhiAlbedoIntegrator::IntersectRays(std::uint32_t)
{
}

void RhiAlbedoIntegrator::ComputeAOVs()
{
}

void RhiAlbedoIntegrator::ShadeMissedRays(std::uint32_t)
{
}

void RhiAlbedoIntegrator::ShadeSurfaceHits(std::uint32_t)
{
}

void RhiAlbedoIntegrator::IntersectShadowRays()
{
}

void RhiAlbedoIntegrator::AccumulateDirectSamples()
{
}

void RhiAlbedoIntegrator::ClearOutgoingRayCounter(std::uint32_t)
{
}

void RhiAlbedoIntegrator::ClearShadowRayCounter()
{
}

void RhiAlbedoIntegrator::Denoise()
{
}

void RhiAlbedoIntegrator::CopyHistoryBuffers()
{
}

void RhiAlbedoIntegrator::ResolveRadiance()
{
    if (!descriptor_set_)
    {
        return;
    }

    gpu::ImagePtr swapchain_image = swapchain_->GetCurrentImage();
    gpu::Queue& queue = device_->GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr cmd_buffer = queue.CreateCommandBuffer();

    cmd_buffer->TransitionBarrier(output_image_, output_layout_, gpu::ImageLayout::kShaderReadWrite);
    cmd_buffer->BindPipeline(pipeline_);
    cmd_buffer->BindDescriptorSet(descriptor_set_);
    cmd_buffer->Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    cmd_buffer->TransitionBarrier(output_image_, gpu::ImageLayout::kShaderReadWrite,
        gpu::ImageLayout::kCopySrc);
    output_layout_ = gpu::ImageLayout::kCopySrc;
    cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kPresent,
        gpu::ImageLayout::kCopyDst);
    cmd_buffer->CopyImage(swapchain_image.get(), output_image_.get());
    cmd_buffer->TransitionBarrier(swapchain_image, gpu::ImageLayout::kCopyDst,
        gpu::ImageLayout::kPresent);

    queue.Submit(std::move(cmd_buffer));
    swapchain_->Present();
}

gpu::BufferPtr RhiAlbedoIntegrator::CreateUploadBuffer(void const* data, std::size_t size,
    std::uint32_t stride, gpu::BufferFlags flags)
{
    std::size_t allocation_size = std::max<std::size_t>(size, stride);
    gpu::BufferPtr buffer = device_->CreateBuffer(allocation_size, stride, flags);
    if (data)
    {
        void* mapped_data = buffer->Map();
        std::memcpy(mapped_data, data, size);
        buffer->Unmap();
    }
    return buffer;
}

void RhiAlbedoIntegrator::RebuildDescriptorSet()
{
    descriptor_set_ = pipeline_->CreateDescriptorSet();
    descriptor_set_->BindImage(*output_image_, 0, 0);
    descriptor_set_->BindBuffer(*camera_buffer_, 1, 0);
    descriptor_set_->BindBuffer(*triangle_buffer_, 2, 0);
    descriptor_set_->BindBuffer(*node_buffer_, 3, 0);
    descriptor_set_->BindBuffer(*material_buffer_, 4, 0);
    descriptor_set_->BindBuffer(*light_buffer_, 5, 0);
    descriptor_set_->BindBuffer(*texture_buffer_, 6, 0);
    descriptor_set_->BindBuffer(*texture_data_buffer_, 7, 0);
}

std::uint32_t RhiAlbedoIntegrator::DivideAndRoundUp(std::uint32_t value, std::uint32_t divisor)
{
    return (value + divisor - 1) / divisor;
}
