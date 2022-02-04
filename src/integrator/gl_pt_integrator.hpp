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
#include "gpu_wrappers/gl_framebuffer.hpp"
#include "gpu_wrappers/gl_graphics_pipeline.hpp"
#include "glm/matrix.hpp"

class GLPathTraceIntegrator : public Integrator
{
public:
    GLPathTraceIntegrator(std::uint32_t width, std::uint32_t height,
        AccelerationStructure& acc_structure, std::uint32_t out_image);
    void Integrate() override;
    void UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure) override;
    void SetCameraData(Camera const& camera) override;
    void EnableWhiteFurnace(bool enable) override;
    void SetMaxBounces(std::uint32_t max_bounces) override;
    void SetSamplerType(SamplerType sampler_type) override;
    void SetAOV(AOV aov) override;
    void EnableDenoiser(bool enable) override;

private:
    GLFramebuffer framebuffer_;
    GraphicsPipeline graphics_pipeline_;
    GLuint out_image_;
    GLuint triangle_buffer_;
    std::uint32_t num_triangles_;
    glm::mat4 view_proj_matrix_;
};
