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

#include "path_tracer.hpp"

#include "gpu_command_buffer.hpp"
#include "gpu_descriptor_set.hpp"
#include "gpu_device.hpp"
#include "gpu_image.hpp"
#include "gpu_pipeline.hpp"
#include "gpu_queue.hpp"
#include "gpu_sampler.hpp"
#include "gpu_swapchain.hpp"

#include "acc_structures/acceleration_structure.hpp"
#include "acc_structures/bvh.hpp"
#include "acc_structures/hardware_rt_acceleration_structure.hpp"

#include "managers/texture_manager.hpp"
#include "scene/scene.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace
{
struct RhiCameraData
{
    Camera camera;
    Camera prev_camera;
};

struct RhiSceneInfoData
{
    uint32_t scene_counts[4];
};

constexpr uint32_t kRenderFlagWhiteFurnace = 1u;
constexpr uint32_t kMaxTextureCount = MAX_TEXTURES;

inline uint32_t DivideAndRoundUp(uint32_t value, uint32_t divisor)
{
    return (value + divisor - 1) / divisor;
}

}  // namespace

PathTracer::PathTracer(uint32_t width, uint32_t height, gpu::Device& device, gpu::Swapchain& swapchain)
    : width_(width), height_(height), device_(device), swapchain_(swapchain)
{
    use_hardware_rt_ = device_.SupportsRayQuery();

    output_image_ = device_.CreateImage(width_, height_, swapchain_.GetFormat(),
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);

    uint32_t num_pixels = width_ * height_;
    for (uint32_t i = 0; i < 2; ++i)
    {
        rays_buffers_[i] = CreateStorageBuffer(num_pixels * sizeof(Ray), sizeof(Ray));
        pixel_indices_buffers_[i] = CreateStorageBuffer(num_pixels * sizeof(uint32_t), sizeof(uint32_t));
        ray_counter_buffers_[i] = CreateStorageBuffer(sizeof(uint32_t), sizeof(uint32_t));
    }

    shadow_rays_buffer_ = CreateStorageBuffer(num_pixels * sizeof(Ray), sizeof(Ray));
    shadow_pixel_indices_buffer_ = CreateStorageBuffer(num_pixels * sizeof(uint32_t), sizeof(uint32_t));
    shadow_ray_counter_buffer_ = CreateStorageBuffer(sizeof(uint32_t), sizeof(uint32_t));
    hits_buffer_ = CreateStorageBuffer(num_pixels * sizeof(Hit), sizeof(Hit));
    shadow_hits_buffer_ = CreateStorageBuffer(num_pixels * sizeof(uint32_t), sizeof(uint32_t));

    // Create images
    throughputs_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    radiance_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    prev_radiance_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    diffuse_albedo_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA8_UNorm,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    depth_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kR32_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    prev_depth_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kR32_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    normal_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    motion_vectors_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    direct_light_samples_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);
    resolved_color_image_ = device_.CreateImage(width_, height_, gpu::ImageFormat::kRGBA16_Float,
        gpu::ImageFlags::kStorage | gpu::ImageFlags::kShaderResource);

    // Transition all images to kShaderReadWrite layout
    gpu::Queue& queue = device_.GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr transition_cmd_buffer = queue.CreateCommandBuffer();
    transition_cmd_buffer->TransitionBarrier({ throughputs_image_, radiance_image_, prev_radiance_image_,
                                                 diffuse_albedo_image_, depth_image_, prev_depth_image_, normal_image_,
                                                 motion_vectors_image_, direct_light_samples_image_,
                                                 resolved_color_image_ },
        gpu::ImageLayout::kUndefined, gpu::ImageLayout::kShaderReadWrite);
    queue.Submit(std::move(transition_cmd_buffer));
    queue.WaitIdle();

    camera_cpu_buffer_ = CreateStagingBuffer(nullptr, sizeof(RhiCameraData), sizeof(RhiCameraData));
    camera_buffer_ = device_.CreateBuffer(sizeof(RhiCameraData), sizeof(RhiCameraData),
        gpu::BufferFlags::kShaderResource | gpu::BufferFlags::kConstant);
    scene_info_buffer_ = device_.CreateBuffer(sizeof(RhiSceneInfoData), sizeof(RhiSceneInfoData),
        gpu::BufferFlags::kShaderResource | gpu::BufferFlags::kConstant);
    swapchain_image_layouts_.resize(swapchain_.GetImageCount(), gpu::ImageLayout::kUndefined);

    CreatePipelines();
}

PathTracer::~PathTracer() = default;

void PathTracer::Integrate()
{
    if (request_reset_)
    {
        Reset();
        request_reset_ = false;
    }

    GenerateRays();

    for (uint32_t bounce = 0; bounce <= max_bounces_; ++bounce)
    {
        IntersectRays(bounce);
        if (bounce == 0)
        {
            ComputeAOVs();
        }
        ShadeMissedRays(bounce);
        ClearOutgoingRayCounter(bounce);
        ClearShadowRayCounter();
        ShadeSurfaceHits(bounce);
        IntersectShadowRays();
        AccumulateDirectSamples();
    }

    AdvanceSampleCount();

    if (enable_denoiser_)
    {
        Denoise();
        CopyHistoryBuffers();
    }
    else
    {
        AccumulateRadiance();
    }

    Tonemap();
}

void PathTracer::SetCommandBuffer(gpu::CommandBuffer& command_buffer)
{
    command_buffer_ = &command_buffer;
}

void PathTracer::SetCurrentSwapchainImageLayout(gpu::ImageLayout layout)
{
    swapchain_image_layouts_[swapchain_.GetCurrentImageIndex()] = layout;
}

void PathTracer::UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure,
    TextureManager const& texture_manager)
{
    auto const& vertices = scene.GetVertices();
    auto const& indices = scene.GetIndices();
    auto const& mesh_infos = scene.GetMeshInfos();
    auto const& instance_infos = scene.GetInstanceInfos();
    auto const& materials = scene.GetMaterials();
    auto const& lights = scene.GetLights();
    auto const& scene_info = scene.GetSceneInfo();

    texture_manager_ = &texture_manager;
    hardware_rt_acc_structure_ = nullptr;
    use_hardware_rt_ = acc_structure.GetBackend() == AccelerationStructureBackend::kHardwareRt;
    triangle_count_ = static_cast<uint32_t>(indices.size() / 3);
    node_count_ = 0u;
    light_count_ = static_cast<uint32_t>(lights.size());
    texture_count_ = texture_manager.TextureCount();
    env_map_index_ = scene_info.environment_map_index;

    gpu::Queue& queue = device_.GetQueue(gpu::QueueType::kGraphics);
    gpu::CommandBufferPtr upload_command_buffer = queue.CreateCommandBuffer();
    std::vector<gpu::BufferPtr> staging_buffers;

    gpu::BufferFlags const geometry_buffer_flags = use_hardware_rt_
        ? gpu::BufferFlags::kShaderResource | gpu::BufferFlags::kAccelerationStructureBuildInput
        : gpu::BufferFlags::kShaderResource;

    vertex_buffer_ = CreateGpuBuffer(vertices, upload_command_buffer, staging_buffers, geometry_buffer_flags);
    index_buffer_ = CreateGpuBuffer(indices, upload_command_buffer, staging_buffers, geometry_buffer_flags);
    mesh_info_buffer_ = CreateGpuBuffer(mesh_infos, upload_command_buffer, staging_buffers);
    instance_info_buffer_ = CreateGpuBuffer(instance_infos, upload_command_buffer, staging_buffers);
    material_buffer_ = CreateGpuBuffer(materials, upload_command_buffer, staging_buffers);
    light_buffer_ = CreateGpuBuffer(lights, upload_command_buffer, staging_buffers);
    if (texture_count_ > kMaxTextureCount)
    {
        throw std::runtime_error("PathTracer::UploadGPUData: too many textures for shader descriptor array");
    }

    fallback_texture_image_ = CreateFallbackTexture(upload_command_buffer);

    gpu::SamplerDesc texture_sampler_desc = {};
    texture_sampler_desc.min_filter = gpu::SamplerFilter::kLinear;
    texture_sampler_desc.mag_filter = gpu::SamplerFilter::kLinear;
    texture_sampler_desc.address_u = gpu::SamplerAddressMode::kRepeat;
    texture_sampler_desc.address_v = gpu::SamplerAddressMode::kRepeat;
    texture_sampler_desc.address_w = gpu::SamplerAddressMode::kRepeat;
    texture_sampler_ = device_.GetSampler(texture_sampler_desc);

    if (use_hardware_rt_)
    {
        auto& hardware_rt_acc_structure = const_cast<
            HardwareRtAccelerationStructure&>(static_cast<HardwareRtAccelerationStructure const&>(acc_structure));
        hardware_rt_acc_structure.Build(device_, *upload_command_buffer, scene, vertex_buffer_, index_buffer_);
        hardware_rt_acc_structure_ = &hardware_rt_acc_structure;
    }
    else
    {
        auto const& bvh = static_cast<Bvh const&>(acc_structure);
        auto const& rt_triangles = bvh.GetTriangles();
        auto const& nodes = bvh.GetNodes();
        triangle_count_ = static_cast<uint32_t>(rt_triangles.size());
        node_count_ = static_cast<uint32_t>(nodes.size());
        rt_triangles_buffer_ = CreateGpuBuffer(rt_triangles, upload_command_buffer, staging_buffers);
        nodes_buffer_ = CreateGpuBuffer(nodes, upload_command_buffer, staging_buffers);
    }
    UploadSceneInfo(*upload_command_buffer, staging_buffers);

    queue.Submit(std::move(upload_command_buffer));
    queue.WaitIdle();

    SetCameraData(camera_);
    RebuildDescriptorSets();
    RequestReset();
}

void PathTracer::SetCameraData(Camera const& camera)
{
    if (prev_camera_.fov == 0.0f)
    {
        prev_camera_ = camera;
    }

    camera_ = camera;
    UpdateCameraData();

    prev_camera_ = camera;
}

void PathTracer::SetSamplerType(SamplerType sampler_type)
{
    if (sampler_type == sampler_type_)
    {
        return;
    }

    sampler_type_ = sampler_type;
    RequestReset();
}

void PathTracer::SetMaxBounces(uint32_t max_bounces)
{
    max_bounces_ = max_bounces;
    RequestReset();
}

void PathTracer::EnableWhiteFurnace(bool enable)
{
    if (enable == enable_white_furnace_)
    {
        return;
    }

    enable_white_furnace_ = enable;
    CreatePipelines();
    RequestReset();
}

void PathTracer::SetAOV(AOV aov)
{
    if (aov == aov_)
    {
        return;
    }

    aov_ = aov;
    RequestReset();
}

void PathTracer::EnableDenoiser(bool enable)
{
    if (enable == enable_denoiser_)
    {
        return;
    }

    enable_denoiser_ = enable;
    RequestReset();
}

void PathTracer::UpdateCameraData()
{
    RhiCameraData data = {};
    data.camera = camera_;
    data.prev_camera = prev_camera_;

    void* mapped_data = camera_cpu_buffer_->Map();
    std::memcpy(mapped_data, &data, sizeof(data));
    camera_cpu_buffer_->Unmap();
}

void PathTracer::UploadSceneInfo(gpu::CommandBuffer& upload_command_buffer,
    std::vector<gpu::BufferPtr>& staging_buffers)
{
    RhiSceneInfoData scene_info = {};
    scene_info.scene_counts[0] = triangle_count_;
    scene_info.scene_counts[1] = node_count_;
    scene_info.scene_counts[2] = light_count_;
    scene_info.scene_counts[3] = texture_count_;

    gpu::BufferPtr staging_buffer = CreateStagingBuffer(&scene_info, sizeof(scene_info), sizeof(scene_info));
    upload_command_buffer.CopyBuffer(staging_buffer, 0, scene_info_buffer_, 0, sizeof(scene_info));
    staging_buffers.push_back(std::move(staging_buffer));
}

void PathTracer::CreatePipelines()
{
    raygen_pipeline_ = device_.CreateComputePipeline("raygeneration.cs");
    trace_pipeline_ = device_.CreateComputePipeline(use_hardware_rt_ ? "trace_rayquery.cs" : "trace_bvh.cs");
    trace_shadow_pipeline_ = device_.CreateComputePipeline(use_hardware_rt_ ? "trace_shadow_rayquery.cs"
                                                                            : "trace_shadow_bvh.cs");
    aov_pipeline_ = device_.CreateComputePipeline("aov.cs");
    miss_pipeline_ = device_.CreateComputePipeline("miss.cs");
    hit_surface_pipeline_ = device_.CreateComputePipeline("hit_surface.cs");
    accumulate_direct_pipeline_ = device_.CreateComputePipeline("accumulate_direct_samples.cs");
    clear_counter_pipeline_ = device_.CreateComputePipeline("clear_counter.cs");
    denoiser_pipeline_ = device_.CreateComputePipeline("denoiser.cs");
    copy_history_pipeline_ = device_.CreateComputePipeline("copy_history.cs");
    accumulate_radiance_pipeline_ = device_.CreateComputePipeline("accumulate_radiance.cs");
    tonemap_pipeline_ = device_.CreateComputePipeline("tonemap.cs");
}

void PathTracer::Reset()
{
    if (!enable_denoiser_)
    {
        sample_count_ = 0;
    }
}

void PathTracer::AdvanceSampleCount()
{
    sample_count_++;
}

void PathTracer::GenerateRays()
{
    assert(raygen_pipeline_ && raygen_set_);
    assert(command_buffer_ && "PathTracer::GenerateRays(): command buffer is not initialized");

    struct RaygenRootConstants
    {
        uint32_t sample_count;
        uint32_t width;
        uint32_t height;
    } root_constants = { sample_count_, width_, height_ };

    command_buffer_->CopyBuffer(camera_cpu_buffer_, 0, camera_buffer_, 0, sizeof(RhiCameraData));
    command_buffer_->BindPipeline(raygen_pipeline_);
    command_buffer_->BindDescriptorSet(raygen_set_);
    command_buffer_->SetRootConstants(&root_constants, sizeof(root_constants));
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(rays_buffers_[0]);
    command_buffer_->StorageBarrier(ray_counter_buffers_[0]);
    command_buffer_->StorageBarrier(pixel_indices_buffers_[0]);
    command_buffer_->StorageBarrier(throughputs_image_);
    command_buffer_->StorageBarrier(radiance_image_);
    command_buffer_->StorageBarrier(diffuse_albedo_image_);
    command_buffer_->StorageBarrier(depth_image_);
    command_buffer_->StorageBarrier(normal_image_);
    command_buffer_->StorageBarrier(motion_vectors_image_);
}

void PathTracer::IntersectRays(uint32_t bounce)
{
    assert(trace_pipeline_ && trace_sets_[bounce & 1]);
    assert(command_buffer_ && "PathTracer::IntersectRays(): command buffer is not initialized");

    command_buffer_->BindPipeline(trace_pipeline_);
    command_buffer_->BindDescriptorSet(trace_sets_[bounce & 1]);
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(hits_buffer_);
}

void PathTracer::ComputeAOVs()
{
    assert(aov_pipeline_ && aov_set_);
    assert(command_buffer_ && "PathTracer::ComputeAOVs(): command buffer is not initialized");

    struct AovRootConstants
    {
        uint32_t width;
    } root_constants = { width_ };

    command_buffer_->BindPipeline(aov_pipeline_);
    command_buffer_->BindDescriptorSet(aov_set_);
    command_buffer_->SetRootConstants(&root_constants, sizeof(root_constants));
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(diffuse_albedo_image_);
    command_buffer_->StorageBarrier(depth_image_);
    command_buffer_->StorageBarrier(normal_image_);
    command_buffer_->StorageBarrier(motion_vectors_image_);
}

void PathTracer::ShadeMissedRays(uint32_t bounce)
{
    assert(miss_pipeline_ && miss_sets_[bounce & 1]);
    assert(command_buffer_ && "PathTracer::ShadeMissedRays(): command buffer is not initialized");

    struct MissRootConstants
    {
        uint32_t width;
        uint32_t env_map_index;
        uint32_t white_furnace;
    } root_constants = { width_, env_map_index_, enable_white_furnace_ ? kRenderFlagWhiteFurnace : 0u };

    command_buffer_->BindPipeline(miss_pipeline_);
    command_buffer_->BindDescriptorSet(miss_sets_[bounce & 1]);
    command_buffer_->SetRootConstants(&root_constants, sizeof(root_constants));
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(radiance_image_);
}

void PathTracer::ShadeSurfaceHits(uint32_t bounce)
{
    assert(hit_surface_pipeline_ && hit_surface_sets_[bounce & 1]);
    assert(command_buffer_ && "PathTracer::ShadeSurfaceHits(): command buffer is not initialized");

    struct ShadeRootConstants
    {
        uint32_t sample_count;
        uint32_t bounce;
        uint32_t width;
        uint32_t white_furnace;
    } root_constants = { sample_count_, bounce, width_, enable_white_furnace_ ? kRenderFlagWhiteFurnace : 0u };

    command_buffer_->BindPipeline(hit_surface_pipeline_);
    command_buffer_->BindDescriptorSet(hit_surface_sets_[bounce & 1]);
    command_buffer_->SetRootConstants(&root_constants, sizeof(root_constants));
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(rays_buffers_[(bounce + 1) & 1]);
    command_buffer_->StorageBarrier(pixel_indices_buffers_[(bounce + 1) & 1]);
    command_buffer_->StorageBarrier(ray_counter_buffers_[(bounce + 1) & 1]);
    command_buffer_->StorageBarrier(shadow_rays_buffer_);
    command_buffer_->StorageBarrier(shadow_pixel_indices_buffer_);
    command_buffer_->StorageBarrier(shadow_ray_counter_buffer_);
    command_buffer_->StorageBarrier(direct_light_samples_image_);
    command_buffer_->StorageBarrier(throughputs_image_);
    command_buffer_->StorageBarrier(radiance_image_);
}

void PathTracer::IntersectShadowRays()
{
    assert(trace_shadow_pipeline_ && trace_shadow_set_);
    assert(command_buffer_ && "PathTracer::IntersectShadowRays(): command buffer is not initialized");

    command_buffer_->BindPipeline(trace_shadow_pipeline_);
    command_buffer_->BindDescriptorSet(trace_shadow_set_);
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(shadow_hits_buffer_);
}

void PathTracer::AccumulateDirectSamples()
{
    assert(accumulate_direct_pipeline_ && accumulate_direct_set_);
    assert(command_buffer_ && "PathTracer::AccumulateDirectSamples(): command buffer is not initialized");

    command_buffer_->BindPipeline(accumulate_direct_pipeline_);
    command_buffer_->BindDescriptorSet(accumulate_direct_set_);
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(radiance_image_);
}

void PathTracer::ClearOutgoingRayCounter(uint32_t bounce)
{
    assert(clear_counter_pipeline_ && clear_counter_sets_[(bounce + 1) & 1]);
    assert(command_buffer_ && "PathTracer::ClearOutgoingRayCounter(): command buffer is not initialized");

    command_buffer_->BindPipeline(clear_counter_pipeline_);
    command_buffer_->BindDescriptorSet(clear_counter_sets_[(bounce + 1) & 1]);
    command_buffer_->Dispatch(1, 1, 1);
    command_buffer_->StorageBarrier(ray_counter_buffers_[(bounce + 1) & 1]);
}

void PathTracer::ClearShadowRayCounter()
{
    assert(clear_counter_pipeline_ && clear_shadow_counter_set_);
    assert(command_buffer_ && "PathTracer::ClearShadowRayCounter(): command buffer is not initialized");

    command_buffer_->BindPipeline(clear_counter_pipeline_);
    command_buffer_->BindDescriptorSet(clear_shadow_counter_set_);
    command_buffer_->Dispatch(1, 1, 1);
    command_buffer_->StorageBarrier(shadow_ray_counter_buffer_);
}

void PathTracer::Denoise()
{
    assert(denoiser_pipeline_ && denoiser_set_);
    assert(command_buffer_ && "PathTracer::Denoise(): command buffer is not initialized");

    struct DenoiserRootConstants
    {
        uint32_t width;
        uint32_t height;
    } root_constants = { width_, height_ };

    command_buffer_->BindPipeline(denoiser_pipeline_);
    command_buffer_->BindDescriptorSet(denoiser_set_);
    command_buffer_->SetRootConstants(&root_constants, sizeof(root_constants));
    command_buffer_->Dispatch(DivideAndRoundUp(width_ * height_, 256), 1, 1);
    command_buffer_->StorageBarrier(radiance_image_);
}

void PathTracer::CopyHistoryBuffers()
{
    assert(copy_history_pipeline_ && copy_history_set_);
    assert(command_buffer_ && "PathTracer::CopyHistoryBuffers(): command buffer is not initialized");

    command_buffer_->BindPipeline(copy_history_pipeline_);
    command_buffer_->BindDescriptorSet(copy_history_set_);
    command_buffer_->Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    command_buffer_->StorageBarrier(prev_radiance_image_);
    command_buffer_->StorageBarrier(prev_depth_image_);
}

void PathTracer::AccumulateRadiance()
{
    assert(accumulate_radiance_pipeline_ && accumulate_radiance_set_);
    assert(command_buffer_ && "PathTracer::AccumulateRadiance(): command buffer is not initialized");

    command_buffer_->BindPipeline(accumulate_radiance_pipeline_);
    command_buffer_->BindDescriptorSet(accumulate_radiance_set_);
    command_buffer_->SetRootConstants(&sample_count_, sizeof(sample_count_));
    command_buffer_->Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    command_buffer_->StorageBarrier(resolved_color_image_);
}

void PathTracer::Tonemap()
{
    gpu::DescriptorSetPtr const& descriptor_set = enable_denoiser_ ? denoised_tonemap_set_ : tonemap_set_;
    assert(tonemap_pipeline_ && descriptor_set);
    assert(command_buffer_ && "PathTracer::Tonemap(): command buffer is not initialized");

    gpu::ImagePtr swapchain_image = swapchain_.GetCurrentImage();
    uint32_t const swapchain_image_index = swapchain_.GetCurrentImageIndex();

    command_buffer_->TransitionBarrier(output_image_, output_layout_, gpu::ImageLayout::kShaderReadWrite);
    command_buffer_->BindPipeline(tonemap_pipeline_);
    command_buffer_->BindDescriptorSet(descriptor_set);
    command_buffer_->Dispatch(DivideAndRoundUp(width_, 8), DivideAndRoundUp(height_, 8), 1);
    command_buffer_->TransitionBarrier(output_image_, gpu::ImageLayout::kShaderReadWrite, gpu::ImageLayout::kCopySrc);
    output_layout_ = gpu::ImageLayout::kCopySrc;
    command_buffer_->TransitionBarrier(swapchain_image, swapchain_image_layouts_[swapchain_image_index],
        gpu::ImageLayout::kCopyDst);
    command_buffer_->CopyImage(swapchain_image, output_image_);
    command_buffer_->TransitionBarrier(swapchain_image, gpu::ImageLayout::kCopyDst, gpu::ImageLayout::kRenderTarget);
    swapchain_image_layouts_[swapchain_image_index] = gpu::ImageLayout::kRenderTarget;
}

gpu::BufferPtr PathTracer::CreateStagingBuffer(void const* data, size_t size, uint32_t stride)
{
    size_t allocation_size = std::max<size_t>(size, stride);
    gpu::BufferPtr buffer = device_.CreateBuffer(allocation_size, stride, gpu::BufferFlags::kCpuAccess);
    if (data)
    {
        void* mapped_data = buffer->Map();
        std::memcpy(mapped_data, data, size);
        buffer->Unmap();
    }
    return buffer;
}

gpu::BufferPtr PathTracer::CreateStorageBuffer(size_t size, uint32_t stride)
{
    size_t allocation_size = std::max<size_t>(size, stride);
    return device_.CreateBuffer(allocation_size, stride,
        gpu::BufferFlags::kShaderResource | gpu::BufferFlags::kStorage);
}

gpu::ImagePtr PathTracer::CreateFallbackTexture(gpu::CommandBufferPtr& upload_command_buffer)
{
    uint32_t const fallback_data = 0xFFFFFFFFu;
    gpu::ImagePtr image = device_.CreateImage(1, 1, gpu::ImageFormat::kRGBA8_UNorm, gpu::ImageFlags::kShaderResource);
    upload_command_buffer->TransitionBarrier(image, gpu::ImageLayout::kUndefined, gpu::ImageLayout::kCopyDst);
    upload_command_buffer->UploadImage(image, &fallback_data, sizeof(fallback_data));
    upload_command_buffer->TransitionBarrier(image, gpu::ImageLayout::kCopyDst, gpu::ImageLayout::kShaderRead);

    return image;
}

void PathTracer::RebuildDescriptorSets()
{
    assert(texture_manager_ && "PathTracer::RebuildDescriptorSets: texture manager is not set");

    std::vector<gpu::ImageDescriptor> texture_descriptors;
    texture_descriptors.reserve(kMaxTextureCount);
    for (gpu::ImagePtr const& texture_image : texture_manager_->GetImages())
    {
        texture_descriptors.push_back({ texture_image.get(), {} });
    }
    while (texture_descriptors.size() < kMaxTextureCount)
    {
        texture_descriptors.push_back({ fallback_texture_image_.get(), {} });
    }

    raygen_set_ = raygen_pipeline_->CreateDescriptorSet();
    raygen_set_->BindBuffer(*camera_buffer_, 0);
    raygen_set_->BindBuffer(*rays_buffers_[0], 1);
    raygen_set_->BindBuffer(*ray_counter_buffers_[0], 2);
    raygen_set_->BindBuffer(*pixel_indices_buffers_[0], 3);
    raygen_set_->BindImage(*throughputs_image_, 4);
    raygen_set_->BindImage(*radiance_image_, 5);
    raygen_set_->BindImage(*diffuse_albedo_image_, 6);
    raygen_set_->BindImage(*depth_image_, 7);
    raygen_set_->BindImage(*normal_image_, 8);
    raygen_set_->BindImage(*motion_vectors_image_, 9);

    for (uint32_t i = 0; i < 2; ++i)
    {
        trace_sets_[i] = trace_pipeline_->CreateDescriptorSet();
        trace_sets_[i]->BindBuffer(*rays_buffers_[i], 0);
        trace_sets_[i]->BindBuffer(*ray_counter_buffers_[i], 1);
        trace_sets_[i]->BindBuffer(*hits_buffer_, 2);
        if (use_hardware_rt_)
        {
            assert(hardware_rt_acc_structure_);
            trace_sets_[i]->BindAccelerationStructure(hardware_rt_acc_structure_->GetTopLevelAS(), 3);
        }
        else
        {
            trace_sets_[i]->BindBuffer(*rt_triangles_buffer_, 3);
            trace_sets_[i]->BindBuffer(*nodes_buffer_, 4);
        }

        miss_sets_[i] = miss_pipeline_->CreateDescriptorSet();
        miss_sets_[i]->BindBuffer(*ray_counter_buffers_[i], 1);
        miss_sets_[i]->BindBuffer(*pixel_indices_buffers_[i], 2);
        miss_sets_[i]->BindBuffer(*hits_buffer_, 3);
        miss_sets_[i]->BindImage(*throughputs_image_, 4);
        miss_sets_[i]->BindImage(*radiance_image_, 5);
        miss_sets_[i]->BindBuffer(*rays_buffers_[i], 6);
        miss_sets_[i]->BindImageArray(texture_descriptors, 7);
        miss_sets_[i]->BindSampler(*texture_sampler_, 8);
        miss_sets_[i]->BindBuffer(*scene_info_buffer_, 24);

        clear_counter_sets_[i] = clear_counter_pipeline_->CreateDescriptorSet();
        clear_counter_sets_[i]->BindBuffer(*ray_counter_buffers_[i], 0);
    }

    for (uint32_t i = 0; i < 2; ++i)
    {
        uint32_t ping = i & 1u;
        uint32_t pong = (i + 1u) & 1u;
        hit_surface_sets_[i] = hit_surface_pipeline_->CreateDescriptorSet();

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
        hit_surface_sets_[i]->BindImage(*direct_light_samples_image_, 11);

        hit_surface_sets_[i]->BindImage(*throughputs_image_, 12);
        hit_surface_sets_[i]->BindImage(*radiance_image_, 13);

        hit_surface_sets_[i]->BindBuffer(*vertex_buffer_, 16);
        hit_surface_sets_[i]->BindBuffer(*index_buffer_, 17);
        hit_surface_sets_[i]->BindBuffer(*mesh_info_buffer_, 18);
        hit_surface_sets_[i]->BindBuffer(*instance_info_buffer_, 19);
        hit_surface_sets_[i]->BindBuffer(*material_buffer_, 20);
        hit_surface_sets_[i]->BindBuffer(*light_buffer_, 21);

        hit_surface_sets_[i]->BindImageArray(texture_descriptors, 22);
        hit_surface_sets_[i]->BindSampler(*texture_sampler_, 23);
        hit_surface_sets_[i]->BindBuffer(*scene_info_buffer_, 24);
    }

    trace_shadow_set_ = trace_shadow_pipeline_->CreateDescriptorSet();
    trace_shadow_set_->BindBuffer(*shadow_rays_buffer_, 0);
    trace_shadow_set_->BindBuffer(*shadow_ray_counter_buffer_, 1);
    trace_shadow_set_->BindBuffer(*shadow_hits_buffer_, 2);
    if (use_hardware_rt_)
    {
        assert(hardware_rt_acc_structure_);
        trace_shadow_set_->BindAccelerationStructure(hardware_rt_acc_structure_->GetTopLevelAS(), 3);
    }
    else
    {
        trace_shadow_set_->BindBuffer(*rt_triangles_buffer_, 3);
        trace_shadow_set_->BindBuffer(*nodes_buffer_, 4);
    }

    aov_set_ = aov_pipeline_->CreateDescriptorSet();
    aov_set_->BindBuffer(*camera_buffer_, 0);
    aov_set_->BindBuffer(*rays_buffers_[0], 1);
    aov_set_->BindBuffer(*ray_counter_buffers_[0], 2);
    aov_set_->BindBuffer(*pixel_indices_buffers_[0], 3);
    aov_set_->BindBuffer(*hits_buffer_, 4);
    aov_set_->BindImage(*diffuse_albedo_image_, 5);
    aov_set_->BindImage(*depth_image_, 6);
    aov_set_->BindImage(*normal_image_, 7);
    aov_set_->BindImage(*motion_vectors_image_, 8);
    aov_set_->BindBuffer(*vertex_buffer_, 9);
    aov_set_->BindBuffer(*index_buffer_, 10);
    aov_set_->BindBuffer(*mesh_info_buffer_, 11);
    aov_set_->BindBuffer(*instance_info_buffer_, 12);
    aov_set_->BindBuffer(*material_buffer_, 13);
    aov_set_->BindImageArray(texture_descriptors, 14);
    aov_set_->BindSampler(*texture_sampler_, 15);
    aov_set_->BindBuffer(*scene_info_buffer_, 24);

    accumulate_direct_set_ = accumulate_direct_pipeline_->CreateDescriptorSet();
    accumulate_direct_set_->BindBuffer(*shadow_ray_counter_buffer_, 0);
    accumulate_direct_set_->BindBuffer(*shadow_pixel_indices_buffer_, 1);
    accumulate_direct_set_->BindBuffer(*shadow_hits_buffer_, 2);
    accumulate_direct_set_->BindImage(*radiance_image_, 3);
    accumulate_direct_set_->BindImage(*direct_light_samples_image_, 4);

    clear_shadow_counter_set_ = clear_counter_pipeline_->CreateDescriptorSet();
    clear_shadow_counter_set_->BindBuffer(*shadow_ray_counter_buffer_, 0);

    denoiser_set_ = denoiser_pipeline_->CreateDescriptorSet();
    denoiser_set_->BindImage(*radiance_image_, 1);
    denoiser_set_->BindImage(*prev_radiance_image_, 2);
    denoiser_set_->BindImage(*depth_image_, 3);
    denoiser_set_->BindImage(*prev_depth_image_, 4);
    denoiser_set_->BindImage(*motion_vectors_image_, 5);

    copy_history_set_ = copy_history_pipeline_->CreateDescriptorSet();
    copy_history_set_->BindImage(*radiance_image_, 0);
    copy_history_set_->BindImage(*prev_radiance_image_, 1);
    copy_history_set_->BindImage(*depth_image_, 2);
    copy_history_set_->BindImage(*prev_depth_image_, 3);

    accumulate_radiance_set_ = accumulate_radiance_pipeline_->CreateDescriptorSet();
    accumulate_radiance_set_->BindImage(*resolved_color_image_, 0);
    accumulate_radiance_set_->BindImage(*radiance_image_, 1);

    tonemap_set_ = tonemap_pipeline_->CreateDescriptorSet();
    tonemap_set_->BindImage(*output_image_, 0);
    tonemap_set_->BindImage(*resolved_color_image_, 1);

    denoised_tonemap_set_ = tonemap_pipeline_->CreateDescriptorSet();
    denoised_tonemap_set_->BindImage(*output_image_, 0);
    denoised_tonemap_set_->BindImage(*radiance_image_, 1);
}
