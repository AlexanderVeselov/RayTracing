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

#include "GpuWrappers/cl_context.hpp"
#include <memory>

class Scene;
class CameraController;
class AccelerationStructure;

class PathTraceIntegrator
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

    PathTraceIntegrator(std::uint32_t width, std::uint32_t height,
        CLContext& cl_context, AccelerationStructure& acc_structure, cl_GLuint interop_image);
    void Integrate();
    void SetSceneData(Scene const& scene);
    void SetCameraData(Camera const& camera);
    void RequestReset() { request_reset_ = true; }
    void ReloadKernels();
    void EnableWhiteFurnace(bool enable);
    void SetMaxBounces(std::uint32_t max_bounces);
    void SetSamplerType(SamplerType sampler_type);
    void SetAOV(AOV aov);
    void EnableDenoiser(bool enable);

private:
    struct Kernels
    {
        std::shared_ptr<CLKernel> reset;
        std::shared_ptr<CLKernel> raygen;
        std::shared_ptr<CLKernel> miss;
        std::shared_ptr<CLKernel> aov;
        std::shared_ptr<CLKernel> hit_surface;
        std::shared_ptr<CLKernel> accumulate_direct_samples;
        std::shared_ptr<CLKernel> clear_counter;
        std::shared_ptr<CLKernel> increment_counter;
        std::shared_ptr<CLKernel> temporal_accumulation;
        std::shared_ptr<CLKernel> resolve;
    };

    Kernels CreateKernels();
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

    // Render size
    std::uint32_t width_;
    std::uint32_t height_;
    Camera prev_camera_ = {};

    std::uint32_t max_bounces_ = 5u;
    SamplerType sampler_type_ = SamplerType::kRandom;
    AOV aov_ = AOV::kShadedColor;

    bool request_reset_ = false;
    // For debugging
    bool enable_white_furnace_ = false;
    bool enable_denoiser_ = false;

    CLContext& cl_context_;
    cl_GLuint gl_interop_image_;

    // Acceleration structure
    AccelerationStructure& acc_structure_;

    // Kernels
    Kernels kernels_;

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

    // Sampler buffers
    cl::Buffer sampler_sobol_buffer_;
    cl::Buffer sampler_scrambling_tile_buffer_;
    cl::Buffer sampler_ranking_tile_buffer_;

    std::unique_ptr<cl::Image> output_image_;

};
