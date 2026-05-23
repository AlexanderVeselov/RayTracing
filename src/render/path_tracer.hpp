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

#include "gpu_buffer.hpp"
#include "gpu_types.hpp"
#include "shaders/shared_structures.h"

#include <array>
#include <memory>
#include <vector>

class Scene;
class TextureManager;
class AccelerationStructure;
class HardwareRtAccelerationStructure;

class PathTracer
{
public:
    enum class SamplerType
    {
        kRandom,
        kBlueNoise
    };

    enum AOV
    {
        kShadedColor,
        kDiffuseAlbedo,
        kDepth,
        kNormal,
        kMotionVectors
    };

    PathTracer(uint32_t width, uint32_t height, gpu::Device& device, gpu::Swapchain& swapchain);
    ~PathTracer();

    void Integrate();
    void SetCommandBuffer(gpu::CommandBuffer& command_buffer);
    void SetCurrentSwapchainImageLayout(gpu::ImageLayout layout);

    void UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure,
        TextureManager const& texture_manager);
    void SetCameraData(Camera const& camera);
    void RequestReset() { request_reset_ = true; }
    void EnableWhiteFurnace(bool enable);
    void SetMaxBounces(uint32_t max_bounces);
    void SetSamplerType(SamplerType sampler_type);
    void SetAOV(AOV aov);
    void EnableDenoiser(bool enable);

private:
    void CreatePipelines();
    void Reset();
    void AdvanceSampleCount();
    void GenerateRays();
    void IntersectRays(uint32_t bounce);
    void ComputeAOVs();
    void ShadeMissedRays(uint32_t bounce);
    void ShadeSurfaceHits(uint32_t bounce);
    void IntersectShadowRays();
    void AccumulateDirectSamples();
    void ClearOutgoingRayCounter(uint32_t bounce);
    void ClearShadowRayCounter();
    void Denoise();
    void CopyHistoryBuffers();
    void AccumulateRadiance();
    void Tonemap();

    gpu::BufferPtr CreateStagingBuffer(void const* data, size_t size, uint32_t stride);
    gpu::BufferPtr CreateStorageBuffer(size_t size, uint32_t stride);
    gpu::ImagePtr CreateFallbackTexture(gpu::CommandBufferPtr& upload_command_buffer);

    template <class T>
    gpu::BufferPtr CreateGpuBuffer(std::vector<T> const& cpu_buffer, gpu::CommandBufferPtr& upload_command_buffer,
        std::vector<gpu::BufferPtr>& staging_buffers, gpu::BufferFlags flags = gpu::BufferFlags::kShaderResource)
    {
        size_t allocation_size = std::max<size_t>(cpu_buffer.size() * sizeof(T), sizeof(T));
        gpu::BufferPtr buffer = device_.CreateBuffer(allocation_size, sizeof(T), flags);
        if (!cpu_buffer.empty())
        {
            gpu::BufferPtr staging_buffer = CreateStagingBuffer(cpu_buffer.data(), cpu_buffer.size() * sizeof(T),
                sizeof(T));
            upload_command_buffer->CopyBuffer(staging_buffer, 0, buffer, 0, cpu_buffer.size() * sizeof(T));
            staging_buffers.push_back(std::move(staging_buffer));
        }
        return buffer;
    }

    void UpdateCameraData();
    void UploadSceneInfo(gpu::CommandBuffer& upload_command_buffer, std::vector<gpu::BufferPtr>& staging_buffers);
    void RebuildDescriptorSets();

    gpu::Device& device_;
    gpu::Swapchain& swapchain_;
    TextureManager const* texture_manager_ = nullptr;
    HardwareRtAccelerationStructure const* hardware_rt_acc_structure_ = nullptr;
    bool use_hardware_rt_ = false;
    gpu::ImagePtr output_image_;
    gpu::CommandBuffer* command_buffer_ = nullptr;

    // Path tracing pipelines
    gpu::ComputePipelinePtr raygen_pipeline_;
    gpu::ComputePipelinePtr aov_pipeline_;
    gpu::ComputePipelinePtr miss_pipeline_;
    gpu::ComputePipelinePtr hit_surface_pipeline_;
    gpu::ComputePipelinePtr accumulate_direct_pipeline_;
    gpu::ComputePipelinePtr clear_counter_pipeline_;
    gpu::ComputePipelinePtr denoiser_pipeline_;
    gpu::ComputePipelinePtr copy_history_pipeline_;
    gpu::ComputePipelinePtr accumulate_radiance_pipeline_;
    gpu::ComputePipelinePtr tonemap_pipeline_;

    // BVH traversal pipelines
    gpu::ComputePipelinePtr trace_pipeline_;
    gpu::ComputePipelinePtr trace_shadow_pipeline_;

    // Descriptor sets
    gpu::DescriptorSetPtr raygen_set_;
    std::array<gpu::DescriptorSetPtr, 2> trace_sets_;
    gpu::DescriptorSetPtr trace_shadow_set_;
    gpu::DescriptorSetPtr aov_set_;
    std::array<gpu::DescriptorSetPtr, 2> miss_sets_;
    std::array<gpu::DescriptorSetPtr, 2> hit_surface_sets_;
    gpu::DescriptorSetPtr accumulate_direct_set_;
    std::array<gpu::DescriptorSetPtr, 2> clear_counter_sets_;
    gpu::DescriptorSetPtr clear_shadow_counter_set_;
    gpu::DescriptorSetPtr denoiser_set_;
    gpu::DescriptorSetPtr copy_history_set_;
    gpu::DescriptorSetPtr accumulate_radiance_set_;
    gpu::DescriptorSetPtr tonemap_set_;
    gpu::DescriptorSetPtr denoised_tonemap_set_;

    // Internal buffers and images
    std::array<gpu::BufferPtr, 2> rays_buffers_;
    std::array<gpu::BufferPtr, 2> pixel_indices_buffers_;
    std::array<gpu::BufferPtr, 2> ray_counter_buffers_;
    gpu::BufferPtr shadow_rays_buffer_;
    gpu::BufferPtr shadow_pixel_indices_buffer_;
    gpu::BufferPtr shadow_ray_counter_buffer_;
    gpu::BufferPtr hits_buffer_;
    gpu::BufferPtr shadow_hits_buffer_;
    gpu::ImagePtr throughputs_image_;
    gpu::ImagePtr radiance_image_;
    gpu::ImagePtr prev_radiance_image_;
    gpu::ImagePtr diffuse_albedo_image_;
    gpu::ImagePtr depth_image_;
    gpu::ImagePtr prev_depth_image_;
    gpu::ImagePtr normal_image_;
    gpu::ImagePtr motion_vectors_image_;
    gpu::ImagePtr direct_light_samples_image_;
    gpu::ImagePtr resolved_color_image_;

    // Scene buffers
    gpu::BufferPtr camera_cpu_buffer_;
    gpu::BufferPtr camera_buffer_;
    gpu::BufferPtr scene_info_buffer_;
    gpu::BufferPtr vertex_buffer_;
    gpu::BufferPtr index_buffer_;
    gpu::BufferPtr mesh_info_buffer_;
    gpu::BufferPtr instance_info_buffer_;
    gpu::BufferPtr material_buffer_;
    gpu::BufferPtr light_buffer_;
    gpu::ImagePtr fallback_texture_image_;
    gpu::SamplerPtr texture_sampler_;

    // Acceleration structure buffers
    gpu::BufferPtr rt_triangles_buffer_;
    gpu::BufferPtr nodes_buffer_;

    // Sample counter
    uint32_t sample_count_ = 0;

    uint32_t width_ = 0u;
    uint32_t height_ = 0u;
    Camera camera_ = {};
    Camera prev_camera_ = {};
    uint32_t max_bounces_ = 3u;
    SamplerType sampler_type_ = SamplerType::kRandom;
    AOV aov_ = AOV::kShadedColor;
    bool request_reset_ = false;
    bool enable_white_furnace_ = false;
    bool enable_denoiser_ = false;

    gpu::ImageLayout output_layout_ = gpu::ImageLayout::kUndefined;
    std::vector<gpu::ImageLayout> swapchain_image_layouts_;
    uint32_t triangle_count_ = 0u;
    uint32_t node_count_ = 0u;
    uint32_t light_count_ = 0u;
    uint32_t texture_count_ = 0u;
    uint32_t env_map_index_ = 0u;
};
