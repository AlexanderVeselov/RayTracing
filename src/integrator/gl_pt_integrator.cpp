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
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

GLPathTraceIntegrator::GLPathTraceIntegrator(std::uint32_t width, std::uint32_t height,
    AccelerationStructure& acc_structure, std::uint32_t out_image)
    : Integrator(width, height, acc_structure)
    , framebuffer_(width, height)
    , graphics_pipeline_("visibility_buffer.vert", "visibility_buffer.frag")
    , copy_pipeline_("copy_image.comp")
    , out_image_(out_image)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &radiance_image_);
    glTextureStorage2D(radiance_image_, 1, GL_RGBA32F, width_, height_);

    CreateKernels();
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
    glm::vec3 position = glm::vec3(camera.position.x, camera.position.y, camera.position.z);
    glm::vec3 front = glm::vec3(camera.front.x, camera.front.y, camera.front.z);
    glm::mat4 view_matrix = glm::lookAt(position, position + front, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 proj_matrix = glm::perspectiveFov(camera.fov, (float)width_, (float)height_, 0.1f, 100.0f);

    view_proj_matrix_ = proj_matrix * view_matrix;
}

void GLPathTraceIntegrator::CreateKernels()
{
    //reset_pipeline_ = std::make_unique<ComputePipeline>("reset_radiance.comp");
    raygen_pipeline_ = std::make_unique<ComputePipeline>("raygeneration.comp");
    //miss_pipeline_ = std::make_unique<ComputePipeline>("miss.comp");
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

void GLPathTraceIntegrator::Reset()
{

}

void GLPathTraceIntegrator::AdvanceSampleCount()
{

}

void GLPathTraceIntegrator::GenerateRays()
{
    /*
    glViewport(0, 0, width_, height_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_.GetFramebuffer());

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    graphics_pipeline_.Use();

    GLuint uniform_index = glGetUniformLocation(graphics_pipeline_.GetProgram(), "g_ViewProjection");
    assert(uniform_index != GL_INVALID_INDEX);
    glUniformMatrix4fv(uniform_index, 1, GL_FALSE, &view_proj_matrix_[0][0]);

    //GLuint storage_index = glGetProgramResourceIndex(graphics_pipeline_.GetProgram(),
    //    GL_SHADER_STORAGE_BLOCK, "TriangleBuffer");
    //assert(storage_index != GL_INVALID_INDEX);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangle_buffer_);

    glDrawArrays(GL_TRIANGLES, 0, num_triangles_ * 3);
    glDisable(GL_DEPTH_TEST);

    glFinish();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    copy_pipeline_.Use();
    std::uint32_t num_groups_x = (width_ + 31) / 32;
    std::uint32_t num_groups_y = (height_ + 31) / 32;
    glBindImageTexture(0, framebuffer_.GetNativeTexture(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, out_image_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(num_groups_x, num_groups_y, 1);

    //glCopyImageSubData(framebuffer_.GetNativeTexture(), GL_TEXTURE_2D, 0, 0, 0, 0,
    //    out_image_, GL_TEXTURE_2D, 0, 0, 0, 0, width_, height_, 1);
    */
}

void GLPathTraceIntegrator::IntersectRays(std::uint32_t bounce)
{

}

void GLPathTraceIntegrator::ComputeAOVs()
{

}

void GLPathTraceIntegrator::ShadeMissedRays(std::uint32_t bounce)
{

}

void GLPathTraceIntegrator::ShadeSurfaceHits(std::uint32_t bounce)
{

}

void GLPathTraceIntegrator::IntersectShadowRays()
{

}

void GLPathTraceIntegrator::AccumulateDirectSamples()
{

}

void GLPathTraceIntegrator::ClearOutgoingRayCounter(std::uint32_t bounce)
{

}

void GLPathTraceIntegrator::ClearShadowRayCounter()
{

}

void GLPathTraceIntegrator::Denoise()
{

}

void GLPathTraceIntegrator::CopyHistoryBuffers()
{

}

void GLPathTraceIntegrator::ResolveRadiance()
{

}
