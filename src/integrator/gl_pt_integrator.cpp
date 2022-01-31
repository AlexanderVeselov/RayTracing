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

#include "gpu_wrappers/cl_context.hpp"
#include "integrator.hpp"
#include <memory>

class Scene;
class CameraController;
class AccelerationStructure;

class GLPathTracingIntegrator : public Integrator
{
public:
    GLPathTracingIntegrator(std::uint32_t width, std::uint32_t height);
    virtual void Integrate() = 0;
    virtual void SetSceneData(Scene const& scene) = 0;
    virtual void SetCameraData(Camera const& camera) = 0;
    void RequestReset() { request_reset_ = true; }
    virtual void EnableWhiteFurnace(bool enable) = 0;
    virtual void SetMaxBounces(std::uint32_t max_bounces) = 0;
    virtual void SetSamplerType(SamplerType sampler_type) = 0;
    virtual void SetAOV(AOV aov) = 0;
    virtual void EnableDenoiser(bool enable) = 0;
};
