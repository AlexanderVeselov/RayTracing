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

#include "shaders/shared_structures.h"
#include <memory>

class Scene;
class CameraController;
class AccelerationStructure;
class TextureManager;

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

    Integrator(uint32_t width, uint32_t height) : width_(width), height_(height) {}
    virtual ~Integrator() = default;

    void Integrate();
    virtual void UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure,
        TextureManager const& texture_manager) = 0;
    virtual void SetCameraData(Camera const& camera) = 0;
    void RequestReset() { request_reset_ = true; }
    void EnableWhiteFurnace(bool enable);
    void SetMaxBounces(uint32_t max_bounces);
    virtual void SetSamplerType(SamplerType sampler_type) = 0;
    virtual void SetAOV(AOV aov) = 0;
    virtual void EnableDenoiser(bool enable) = 0;

protected:
    virtual void BeginFrame() {}
    virtual void EndFrame() {}
    virtual void CreateKernels() = 0;
    virtual void Reset() = 0;
    virtual void AdvanceSampleCount() = 0;
    virtual void GenerateRays() = 0;
    virtual void IntersectRays(uint32_t bounce) = 0;
    virtual void ComputeAOVs() = 0;
    virtual void ShadeMissedRays(uint32_t bounce) = 0;
    virtual void ShadeSurfaceHits(uint32_t bounce) = 0;
    virtual void IntersectShadowRays() = 0;
    virtual void AccumulateDirectSamples() = 0;
    virtual void ClearOutgoingRayCounter(uint32_t bounce) = 0;
    virtual void ClearShadowRayCounter() = 0;
    virtual void Denoise() = 0;
    virtual void CopyHistoryBuffers() = 0;
    virtual void ResolveRadiance() = 0;

    // Render size
    uint32_t width_;
    uint32_t height_;

    Camera camera_ = {};
    Camera prev_camera_ = {};

    uint32_t max_bounces_ = 3u;
    SamplerType sampler_type_ = SamplerType::kRandom;
    AOV aov_ = AOV::kShadedColor;

    bool request_reset_ = false;
    // For debugging
    bool enable_white_furnace_ = false;
    bool enable_denoiser_ = false;
};
