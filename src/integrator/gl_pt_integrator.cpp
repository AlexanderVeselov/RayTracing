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

#include "gl_pt_integrator.hpp"

namespace
{
char const* kVertexShaderSource =
"varying vec2 vTexcoord;"
"void main() {"
"    vTexcoord = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);"
"    gl_Position = vec4(vTexcoord * 2.0 - 1.0, 0.0, 1.0);"
"}";

char const* kFragmentShaderSource =
"varying vec2 vTexcoord;"
"void main() {"
"    gl_FragColor = vec4(vTexcoord, 0.0f, 1.0f);"
"}";
}

GLPathTraceIntegrator::GLPathTraceIntegrator(std::uint32_t width, std::uint32_t height,
    AccelerationStructure& acc_structure, std::uint32_t out_image)
    : Integrator(width, height, acc_structure)
    , framebuffer_(width, height)
    , graphics_pipeline_(kVertexShaderSource, kFragmentShaderSource)
    , out_image_(out_image)
{
}

void GLPathTraceIntegrator::UploadGPUData(Scene const& scene, AccelerationStructure const& acc_structure)
{
    // Create scene buffers
    auto const& triangles = scene.GetTriangles();
    auto const& materials = scene.GetMaterials();
    auto const& emissive_indices = scene.GetEmissiveIndices();
    auto const& lights = scene.GetLights();
    auto const& textures = scene.GetTextures();
    auto const& texture_data = scene.GetTextureData();
    auto const& env_image = scene.GetEnvImage();

    // Triangle buffer
    num_triangles_ = triangles.size();
    glCreateBuffers(1, &triangle_buffer_);
    glNamedBufferData(triangle_buffer_, triangles.size() * sizeof(Triangle), triangles.data(), GL_STATIC_DRAW);
}

void GLPathTraceIntegrator::SetCameraData(Camera const& camera)
{

}

void GLPathTraceIntegrator::EnableWhiteFurnace(bool enable)
{

}

void GLPathTraceIntegrator::SetMaxBounces(std::uint32_t max_bounces)
{

}

void GLPathTraceIntegrator::SetSamplerType(SamplerType sampler_type)
{

}

void GLPathTraceIntegrator::SetAOV(AOV aov)
{

}

void GLPathTraceIntegrator::EnableDenoiser(bool enable)
{

}

void GLPathTraceIntegrator::Integrate()
{
    glViewport(0, 0, width_, height_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_.GetFramebuffer());

    glClearColor(0.5f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    graphics_pipeline_.Use();
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glFinish();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glCopyImageSubData(framebuffer_.GetNativeTexture(), GL_TEXTURE_2D, 0, 0, 0, 0,
        out_image_, GL_TEXTURE_2D, 0, 0, 0, 0, width_, height_, 1);
}
