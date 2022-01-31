/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

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
#include "gpu_wrappers/cl_context.hpp"

class CLPathTraceIntegrator : public Integrator
{
public:
    CLPathTraceIntegrator(std::uint32_t width, std::uint32_t height,
        AccelerationStructure& acc_structure, CLContext& cl_context, unsigned int out_image);
    void Integrate() override;
    void UploadSceneData(Scene const& scene) override;
    void SetCameraData(Camera const& camera) override;
    void EnableWhiteFurnace(bool enable) override;
    void SetMaxBounces(std::uint32_t max_bounces) override;
    void SetSamplerType(SamplerType sampler_type) override;
    void SetAOV(AOV aov) override;
    void EnableDenoiser(bool enable) override;

private:
    void CreateKernels();
    void Reset();
    void AdvanceSampleCount();
    void GenerateRays();
    void IntersectRays(std::uint32_t bounce);
    void ComputeAOVs();
    void ShadeMissedRays(std::uint32_t bounce);
    void ShadeSurfaceHits(std::uint32_t bounce);
    void IntersectShadowRays();
    void AccumulateDirectSamples();
    void ClearOutgoingRayCounter(std::uint32_t bounce);
    void ClearShadowRayCounter();
    void Denoise();
    void CopyHistoryBuffers();
    void ResolveRadiance();

    CLContext& cl_context_;
    cl_GLuint gl_interop_image_;

    // Kernels
    std::shared_ptr<CLKernel> reset_kernel_;
    std::shared_ptr<CLKernel> raygen_kernel_;
    std::shared_ptr<CLKernel> miss_kernel_;
    std::shared_ptr<CLKernel> aov_kernel_;
    std::shared_ptr<CLKernel> hit_surface_kernel_;
    std::shared_ptr<CLKernel> accumulate_direct_samples_kernel_;
    std::shared_ptr<CLKernel> clear_counter_kernel_;
    std::shared_ptr<CLKernel> increment_counter_kernel_;
    std::shared_ptr<CLKernel> temporal_accumulation_kernel_;
    std::shared_ptr<CLKernel> resolve_kernel_;

    // Internal buffers
    cl::Buffer rays_buffer_[2]; // 2 buffers for incoming-outgoing rays
    cl::Buffer shadow_rays_buffer_;
    cl::Buffer pixel_indices_buffer_[2];
    cl::Buffer shadow_pixel_indices_buffer_;
    cl::Buffer ray_counter_buffer_[2];
    cl::Buffer shadow_ray_counter_buffer_;
    cl::Buffer hits_buffer_;
    cl::Buffer shadow_hits_buffer_;
    cl::Buffer throughputs_buffer_;
    cl::Buffer sample_counter_buffer_;
    cl::Buffer radiance_buffer_;
    cl::Buffer prev_radiance_buffer_;
    cl::Buffer diffuse_albedo_buffer_;
    cl::Buffer depth_buffer_;
    cl::Buffer prev_depth_buffer_;
    cl::Buffer normal_buffer_;
    cl::Buffer velocity_buffer_;
    cl::Buffer direct_light_samples_buffer_;

    // Scene buffers
    cl::Buffer triangle_buffer_;
    cl::Buffer material_buffer_;
    cl::Buffer texture_buffer_;
    cl::Buffer texture_data_buffer_;
    cl::Buffer emissive_buffer_;
    cl::Buffer analytic_light_buffer_;
    cl::Buffer scene_info_buffer_;
    cl::Image2D env_texture_;
    SceneInfo scene_info_;

    // Sampler buffers
    cl::Buffer sampler_sobol_buffer_;
    cl::Buffer sampler_scrambling_tile_buffer_;
    cl::Buffer sampler_ranking_tile_buffer_;

    std::unique_ptr<cl::Image> output_image_;

};
