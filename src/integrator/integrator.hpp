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

#pragma once

#include "gpu_wrappers/cl_context.hpp"
#include <memory>

class Scene;
class CameraController;
class AccelerationStructure;

class Integrator
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

    Integrator(std::uint32_t width, std::uint32_t height, AccelerationStructure& acc_structure)
        : width_(width), height_(height), acc_structure_(acc_structure) {}
    void Integrate();
    virtual void UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure) = 0;
    virtual void SetCameraData(Camera const& camera) = 0;
    void RequestReset() { request_reset_ = true; }
    virtual void EnableWhiteFurnace(bool enable) = 0;
    virtual void SetMaxBounces(std::uint32_t max_bounces) = 0;
    virtual void SetSamplerType(SamplerType sampler_type) = 0;
    virtual void SetAOV(AOV aov) = 0;
    virtual void EnableDenoiser(bool enable) = 0;

protected:
    virtual void Reset() = 0;
    virtual void AdvanceSampleCount() = 0;
    virtual void GenerateRays() = 0;
    virtual void IntersectRays(std::uint32_t bounce) = 0;
    virtual void ComputeAOVs() = 0;
    virtual void ShadeMissedRays(std::uint32_t bounce) = 0;
    virtual void ShadeSurfaceHits(std::uint32_t bounce) = 0;
    virtual void IntersectShadowRays() = 0;
    virtual void AccumulateDirectSamples() = 0;
    virtual void ClearOutgoingRayCounter(std::uint32_t bounce) = 0;
    virtual void ClearShadowRayCounter() = 0;
    virtual void Denoise() = 0;
    virtual void CopyHistoryBuffers() = 0;
    virtual void ResolveRadiance() = 0;

    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    // Acceleration structure
    AccelerationStructure& acc_structure_;

    Camera camera_ = {};
    Camera prev_camera_ = {};

    std::uint32_t max_bounces_ = 3u;
    SamplerType sampler_type_ = SamplerType::kRandom;
    AOV aov_ = AOV::kShadedColor;

    bool request_reset_ = false;
    // For debugging
    bool enable_white_furnace_ = false;
    bool enable_denoiser_ = false;

};
