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

#include "integrator.hpp"

#include "gpu_api.hpp"
#include "gpu_buffer.hpp"
#include "gpu_types.hpp"

#include <array>
#include <memory>
#include <vector>

class RhiIntegrator : public Integrator
{
  public:
    RhiIntegrator(std::uint32_t width, std::uint32_t height, AccelerationStructure& acc_structure,
        void* window_native_handle, gpu::ApiType api_type);
    ~RhiIntegrator();

    void UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure) override;
    void SetCameraData(Camera const& camera) override;
    void SetSamplerType(SamplerType sampler_type) override;
    void SetAOV(AOV aov) override;
    void EnableDenoiser(bool enable) override;

  protected:
    void BeginFrame() override;
    void EndFrame() override;
    void CreateKernels() override;
    void Reset() override;
    void AdvanceSampleCount() override;
    void GenerateRays() override;
    void IntersectRays(std::uint32_t bounce) override;
    void ComputeAOVs() override;
    void ShadeMissedRays(std::uint32_t bounce) override;
    void ShadeSurfaceHits(std::uint32_t bounce) override;
    void IntersectShadowRays() override;
    void AccumulateDirectSamples() override;
    void ClearOutgoingRayCounter(std::uint32_t bounce) override;
    void ClearShadowRayCounter() override;
    void Denoise() override;
    void CopyHistoryBuffers() override;
    void ResolveRadiance() override;

  private:
    gpu::BufferPtr CreateStagingBuffer(void const* data, std::size_t size, std::uint32_t stride);
    gpu::BufferPtr CreateGpuBuffer(void const* data, std::size_t size, std::uint32_t stride,
        gpu::BufferFlags flags, gpu::CommandBuffer& upload_command_buffer,
        std::vector<gpu::BufferPtr>& staging_buffers);
    gpu::BufferPtr CreateStorageBuffer(std::size_t size, std::uint32_t stride);

    void UpdateFrameData();
    void RebuildDescriptorSets();

    std::unique_ptr<gpu::Api> api_;
    gpu::DevicePtr device_;
    gpu::SwapchainPtr swapchain_;
    gpu::ImagePtr output_image_;
    gpu::CommandBufferPtr command_buffer_;

    gpu::ComputePipelinePtr reset_pipeline_;
    gpu::ComputePipelinePtr raygen_pipeline_;
    gpu::ComputePipelinePtr trace_pipeline_;
    gpu::ComputePipelinePtr trace_shadow_pipeline_;
    gpu::ComputePipelinePtr aov_pipeline_;
    gpu::ComputePipelinePtr miss_pipeline_;
    gpu::ComputePipelinePtr hit_surface_pipeline_;
    gpu::ComputePipelinePtr accumulate_direct_pipeline_;
    gpu::ComputePipelinePtr clear_counter_pipeline_;
    gpu::ComputePipelinePtr clear_sample_counter_pipeline_;
    gpu::ComputePipelinePtr increment_counter_pipeline_;
    gpu::ComputePipelinePtr denoiser_pipeline_;
    gpu::ComputePipelinePtr copy_history_pipeline_;
    gpu::ComputePipelinePtr resolve_pipeline_;

    gpu::DescriptorSetPtr reset_set_;
    gpu::DescriptorSetPtr raygen_set_;
    std::array<gpu::DescriptorSetPtr, 2> trace_sets_;
    gpu::DescriptorSetPtr trace_shadow_set_;
    gpu::DescriptorSetPtr aov_set_;
    std::array<gpu::DescriptorSetPtr, 2> miss_sets_;
    std::array<gpu::DescriptorSetPtr, 2> hit_surface_sets_;
    gpu::DescriptorSetPtr accumulate_direct_set_;
    std::array<gpu::DescriptorSetPtr, 2> clear_counter_sets_;
    gpu::DescriptorSetPtr clear_shadow_counter_set_;
    gpu::DescriptorSetPtr clear_sample_counter_set_;
    gpu::DescriptorSetPtr increment_counter_set_;
    gpu::DescriptorSetPtr denoiser_set_;
    gpu::DescriptorSetPtr copy_history_set_;
    gpu::DescriptorSetPtr resolve_set_;

    gpu::BufferPtr camera_cpu_buffer_;
    gpu::BufferPtr camera_buffer_;
    std::array<gpu::BufferPtr, 2> rays_buffers_;
    std::array<gpu::BufferPtr, 2> pixel_indices_buffers_;
    std::array<gpu::BufferPtr, 2> ray_counter_buffers_;
    gpu::BufferPtr shadow_rays_buffer_;
    gpu::BufferPtr shadow_pixel_indices_buffer_;
    gpu::BufferPtr shadow_ray_counter_buffer_;
    gpu::BufferPtr hits_buffer_;
    gpu::BufferPtr shadow_hits_buffer_;
    gpu::ImagePtr throughputs_image_;
    gpu::BufferPtr sample_counter_buffer_;
    gpu::ImagePtr radiance_image_;
    gpu::ImagePtr prev_radiance_image_;
    gpu::ImagePtr diffuse_albedo_image_;
    gpu::ImagePtr depth_image_;
    gpu::ImagePtr prev_depth_image_;
    gpu::ImagePtr normal_image_;
    gpu::ImagePtr motion_vectors_image_;
    gpu::ImagePtr direct_light_samples_image_;
    std::array<gpu::BufferPtr, 2> bounce_buffers_;

    gpu::BufferPtr triangle_buffer_;
    gpu::BufferPtr node_buffer_;
    gpu::BufferPtr material_buffer_;
    gpu::BufferPtr light_buffer_;
    gpu::BufferPtr texture_buffer_;
    gpu::BufferPtr texture_data_buffer_;
    gpu::BufferPtr env_map_buffer_;

    gpu::ImageLayout output_layout_ = gpu::ImageLayout::kUndefined;
    std::vector<gpu::ImageLayout> swapchain_image_layouts_;
    std::uint32_t triangle_count_ = 0u;
    std::uint32_t node_count_ = 0u;
    std::uint32_t light_count_ = 0u;
    std::uint32_t texture_count_ = 0u;
    std::uint32_t env_map_width_ = 0u;
    std::uint32_t env_map_height_ = 0u;
};
