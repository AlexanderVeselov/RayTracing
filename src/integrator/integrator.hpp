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
#include "integrator.hpp"
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

    Integrator(std::uint32_t width, std::uint32_t height) : width_(width), height_(height) {}
    virtual void Integrate() = 0;
    virtual void SetSceneData(Scene const& scene) = 0;
    virtual void SetCameraData(Camera const& camera) = 0;
    void RequestReset() { request_reset_ = true; }
    virtual void EnableWhiteFurnace(bool enable) = 0;
    virtual void SetMaxBounces(std::uint32_t max_bounces) = 0;
    virtual void SetSamplerType(SamplerType sampler_type) = 0;
    virtual void SetAOV(AOV aov) = 0;
    virtual void EnableDenoiser(bool enable) = 0;

protected:
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

};
