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

#include "gl_pt_integrator.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace
{
constexpr std::uint32_t kResetGroupSize = 32u;
constexpr std::uint32_t kRayGenerationGroupSize = 256u;

void BindFloat(GLuint program, char const* name, float value)
{
    GLuint uniform_index = glGetUniformLocation(program, name);
    assert(uniform_index != GL_INVALID_INDEX);
    glUniform1f(uniform_index, value);
}

void BindFloat3(GLuint program, char const* name, float3 value)
{
    GLuint uniform_index = glGetUniformLocation(program, name);
    assert(uniform_index != GL_INVALID_INDEX);
    glUniform3f(uniform_index, value.x, value.y, value.z);
}

void BindUint(GLuint program, char const* name, std::uint32_t value)
{
    GLuint uniform_index = glGetUniformLocation(program, name);
    assert(uniform_index != GL_INVALID_INDEX);
    glUniform1ui(uniform_index, value);
}

GLuint CreateBuffer(std::size_t size)
{
    GLuint buffer;
    glCreateBuffers(1, &buffer);
    glNamedBufferData(buffer, size, nullptr, GL_DYNAMIC_DRAW);
    return buffer;
}
}

GLPathTraceIntegrator::GLPathTraceIntegrator(std::uint32_t width, std::uint32_t height,
    AccelerationStructure& acc_structure, std::uint32_t out_image)
    : Integrator(width, height, acc_structure)
    , framebuffer_(width, height)
    , graphics_pipeline_("visibility_buffer.vert", "visibility_buffer.frag")
    , copy_pipeline_("copy_image.comp")
    , out_image_(out_image)
{
    std::uint32_t num_rays = width_ * height_;

    glCreateTextures(GL_TEXTURE_2D, 1, &radiance_image_);
    glTextureStorage2D(radiance_image_, 1, GL_RGBA32F, width_, height_);

    for (int i = 0; i < 2; ++i)
    {
        rays_buffer_[i] = CreateBuffer(num_rays * sizeof(Ray));
        pixel_indices_buffer_[i] = CreateBuffer(num_rays * sizeof(std::uint32_t));
        ray_counter_buffer_[i] = CreateBuffer(sizeof(std::uint32_t));
    }

    shadow_rays_buffer_ = CreateBuffer(num_rays * sizeof(Ray));
    shadow_pixel_indices_buffer_ = CreateBuffer(num_rays * sizeof(std::uint32_t));
    shadow_ray_counter_buffer_ = CreateBuffer(sizeof(std::uint32_t));
    hits_buffer_ = CreateBuffer(num_rays * sizeof(Hit));
    shadow_hits_buffer_ = CreateBuffer(num_rays * sizeof(std::uint32_t));
    throughputs_buffer_ = CreateBuffer(num_rays * sizeof(cl_float3));
    sample_counter_buffer_ = CreateBuffer(sizeof(std::uint32_t));

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
    reset_pipeline_ = std::make_unique<ComputePipeline>("reset_radiance.comp");
    raygen_pipeline_ = std::make_unique<ComputePipeline>("raygeneration.comp");
    intersect_pipeline_ = std::make_unique<ComputePipeline>("trace_bvh.comp");
    miss_pipeline_ = std::make_unique<ComputePipeline>("miss.comp");
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
    reset_pipeline_->Use();

    BindUint(reset_pipeline_->GetProgram(), "width", width_);
    BindUint(reset_pipeline_->GetProgram(), "height", height_);
    glBindImageTexture(0, radiance_image_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    std::uint32_t num_groups_x = (width_  + kResetGroupSize - 1) / kResetGroupSize;
    std::uint32_t num_groups_y = (height_ + kResetGroupSize - 1) / kResetGroupSize;
    glDispatchCompute(num_groups_x, num_groups_y, 1);
}

void GLPathTraceIntegrator::AdvanceSampleCount()
{

}

void GLPathTraceIntegrator::GenerateRays()
{
    raygen_pipeline_->Use();

    BindUint(raygen_pipeline_->GetProgram(), "width", width_);
    BindUint(raygen_pipeline_->GetProgram(), "height", height_);
    BindFloat3(raygen_pipeline_->GetProgram(), "camera.position", camera_.position);
    BindFloat3(raygen_pipeline_->GetProgram(), "camera.front", camera_.front);
    BindFloat3(raygen_pipeline_->GetProgram(), "camera.up", camera_.up);
    BindFloat(raygen_pipeline_->GetProgram(), "camera.fov", camera_.fov);
    BindFloat(raygen_pipeline_->GetProgram(), "camera.aspect_ratio", camera_.aspect_ratio);
    //BindFloat(raygen_pipeline_->GetProgram(), "camera.aperture", camera_.aperture);
    //BindFloat(raygen_pipeline_->GetProgram(), "camera.focus_distance", camera_.focus_distance);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sample_counter_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rays_buffer_[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ray_counter_buffer_[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, pixel_indices_buffer_[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, throughputs_buffer_);

    std::uint32_t num_groups = (width_ * height_ + kRayGenerationGroupSize - 1) / kRayGenerationGroupSize;
    glDispatchCompute(num_groups, 1, 1);

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
