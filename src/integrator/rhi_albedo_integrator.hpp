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

#pragma once

#include "integrator.hpp"

#include "gpu_api.hpp"
#include "gpu_buffer.hpp"
#include "gpu_types.hpp"

#include <memory>

namespace gpu
{
class Device;
class Swapchain;
class Image;
class Buffer;
class ComputePipeline;
class DescriptorSet;
}

class RhiAlbedoIntegrator : public Integrator
{
public:
    RhiAlbedoIntegrator(std::uint32_t width, std::uint32_t height,
        AccelerationStructure& acc_structure, void* window_native_handle);
    ~RhiAlbedoIntegrator();

    void UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure) override;
    void SetCameraData(Camera const& camera) override;
    void SetSamplerType(SamplerType sampler_type) override;
    void SetAOV(AOV aov) override;
    void EnableDenoiser(bool enable) override;

protected:
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
    gpu::BufferPtr CreateUploadBuffer(void const* data, std::size_t size, std::uint32_t stride,
        gpu::BufferFlags flags);
    void RebuildDescriptorSet();
    static std::uint32_t DivideAndRoundUp(std::uint32_t value, std::uint32_t divisor);

    std::unique_ptr<gpu::Api> api_;
    gpu::DevicePtr device_;
    gpu::SwapchainPtr swapchain_;
    gpu::ImagePtr output_image_;
    gpu::ComputePipelinePtr pipeline_;
    gpu::DescriptorSetPtr descriptor_set_;
    gpu::BufferPtr camera_buffer_;
    gpu::BufferPtr triangle_buffer_;
    gpu::BufferPtr node_buffer_;
    gpu::BufferPtr material_buffer_;
    gpu::BufferPtr light_buffer_;
    gpu::BufferPtr texture_buffer_;
    gpu::BufferPtr texture_data_buffer_;
    gpu::ImageLayout output_layout_ = gpu::ImageLayout::kUndefined;
    std::uint32_t triangle_count_ = 0u;
    std::uint32_t node_count_ = 0u;
    std::uint32_t light_count_ = 0u;
    std::uint32_t texture_count_ = 0u;
    std::uint32_t texture_data_count_ = 0u;
};
