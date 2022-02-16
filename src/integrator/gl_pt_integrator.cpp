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
#include "acceleration_structure.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace
{
constexpr std::uint32_t kMaxTextures = 512u;
constexpr std::uint32_t kResetGroupSize = 32u;
constexpr std::uint32_t kRayGenerationGroupSize = 256u;
constexpr std::uint32_t kIntersectGroupSize = 32u;
constexpr std::uint32_t kMissGroupSize = 32u;
constexpr std::uint32_t kShadeGroupSize = 32u;
constexpr std::uint32_t kAccumulateDirectSamplesGroupSize = 256u;
constexpr std::uint32_t kResolveGroupSize = 32u;

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
        ray_counter_buffer_[i] = CreateBuffer(sizeof(std::uint32_t));
        pixel_indices_buffer_[i] = CreateBuffer(num_rays * sizeof(std::uint32_t));
    }

    shadow_rays_buffer_ = CreateBuffer(num_rays * sizeof(Ray));
    shadow_ray_counter_buffer_ = CreateBuffer(sizeof(std::uint32_t));
    shadow_pixel_indices_buffer_ = CreateBuffer(num_rays * sizeof(std::uint32_t));
    hits_buffer_ = CreateBuffer(num_rays * sizeof(Hit));
    shadow_hits_buffer_ = CreateBuffer(num_rays * sizeof(std::uint32_t));
    throughputs_buffer_ = CreateBuffer(num_rays * sizeof(float) * 4);
    sample_counter_buffer_ = CreateBuffer(sizeof(std::uint32_t));
    direct_light_samples_buffer_ = CreateBuffer(num_rays * sizeof(float) * 4);

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

    // Additional compressed triangle buffer
    {
        std::vector<RTTriangle> rt_triangles;
        for (auto const& triangle : triangles)
        {
            rt_triangles.emplace_back(triangle.v1.position, triangle.v2.position, triangle.v3.position);
        }

        glCreateBuffers(1, &rt_triangle_buffer_);
        glNamedBufferData(rt_triangle_buffer_, rt_triangles.size() * sizeof(RTTriangle), rt_triangles.data(), GL_STATIC_DRAW);
    }

    glCreateBuffers(1, &material_buffer_);
    glNamedBufferData(material_buffer_, materials.size() * sizeof(PackedMaterial), materials.data(), GL_STATIC_DRAW);

    if (!emissive_indices.empty())
    {
        glCreateBuffers(1, &emissive_buffer_);
        glNamedBufferData(emissive_buffer_, emissive_indices.size() * sizeof(std::uint32_t), emissive_indices.data(), GL_STATIC_DRAW);
    }

    if (!lights.empty())
    {
        glCreateBuffers(1, &analytic_light_buffer_);
        glNamedBufferData(analytic_light_buffer_, lights.size() * sizeof(Light), lights.data(), GL_STATIC_DRAW);
    }

    // Upload texture data
    textures_.resize(textures.size());
    texture_handles_.resize(kMaxTextures);
    assert(textures.size() <= kMaxTextures);
    for (auto i = 0; i < textures.size(); ++i)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &textures_[i]);
        glTextureStorage2D(textures_[i], 1, GL_RGBA8, textures[i].width, textures[i].height);
        glTextureSubImage2D(textures_[i], 0, 0, 0, textures[i].width, textures[i].height, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.data());
        texture_handles_[i] = glGetTextureHandleARB(textures_[i]);
        glMakeTextureHandleResidentARB(texture_handles_[i]);
    }

    // Just pad the remaining handles with zeroth handle
    for (auto i = textures.size(); i < kMaxTextures; ++i)
    {
        texture_handles_[i] = texture_handles_[0];
    }

    // Create texture handle buffer
    glCreateBuffers(1, &texture_handle_buffer_);
    glNamedBufferData(texture_handle_buffer_, texture_handles_.size() * sizeof(GLuint64),
        texture_handles_.data(), GL_STATIC_DRAW);

    // Scene info
    scene_info_ = scene.GetSceneInfo();

    // Create environment map
    glCreateTextures(GL_TEXTURE_2D, 1, &env_image_);
    glTextureStorage2D(env_image_, 1, GL_RGBA32F, env_image.width, env_image.height);
    glTextureSubImage2D(env_image_, 0, 0, 0, env_image.width, env_image.height, GL_RGBA, GL_FLOAT, env_image.data.data());

    // Upload BVH data
    auto const& nodes = acc_structure.GetNodes();
    glCreateBuffers(1, &nodes_buffer_);
    glNamedBufferData(nodes_buffer_, nodes.size() * sizeof(LinearBVHNode), nodes.data(), GL_STATIC_DRAW);
}

void GLPathTraceIntegrator::SetCameraData(Camera const& camera)
{
    glm::vec3 position = glm::vec3(camera.position.x, camera.position.y, camera.position.z);
    glm::vec3 front = glm::vec3(camera.front.x, camera.front.y, camera.front.z);
    glm::mat4 view_matrix = glm::lookAt(position, position + front, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 proj_matrix = glm::perspectiveFov(camera.fov, (float)width_, (float)height_, 0.1f, 100.0f);

    camera_ = camera;
    view_proj_matrix_ = proj_matrix * view_matrix;
}

void GLPathTraceIntegrator::CreateKernels()
{
    std::vector<std::string> definitions;
    if (enable_white_furnace_)
    {
        definitions.push_back("ENABLE_WHITE_FURNACE");
    }

    if (sampler_type_ == SamplerType::kBlueNoise)
    {
        definitions.push_back("BLUE_NOISE_SAMPLER");
    }

    if (enable_denoiser_)
    {
        definitions.push_back("ENABLE_DENOISER");
    }

    accumulate_direct_samples_pipeline_ = std::make_unique<ComputePipeline>("accumulate_direct_samples.comp");
    clear_counter_pipeline_ = std::make_unique<ComputePipeline>("clear_counter.comp");
    hit_surface_pipeline_ = std::make_unique<ComputePipeline>("hit_surface.comp", definitions);
    increment_counter_pipeline_ = std::make_unique<ComputePipeline>("increment_counter.comp");
    miss_pipeline_ = std::make_unique<ComputePipeline>("miss.comp", definitions);
    raygen_pipeline_ = std::make_unique<ComputePipeline>("raygeneration.comp");
    reset_pipeline_ = std::make_unique<ComputePipeline>("reset_radiance.comp");
    resolve_pipeline_ = std::make_unique<ComputePipeline>("resolve_radiance.comp", definitions);
    intersect_pipeline_ = std::make_unique<ComputePipeline>("trace_bvh.comp");

    std::vector<std::string> trace_shadow_definitions = { "SHADOW_RAYS" };
    intersect_shadow_pipeline_ = std::make_unique<ComputePipeline>("trace_bvh.comp", trace_shadow_definitions);
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
    if (!enable_denoiser_)
    {
        // Reset frame index
        clear_counter_pipeline_->Use();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sample_counter_buffer_);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    reset_pipeline_->Use();
    reset_pipeline_->BindConstant("width", width_);
    reset_pipeline_->BindConstant("height", height_);
    glBindImageTexture(0, radiance_image_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    std::uint32_t num_groups_x = (width_  + kResetGroupSize - 1) / kResetGroupSize;
    std::uint32_t num_groups_y = (height_ + kResetGroupSize - 1) / kResetGroupSize;
    glDispatchCompute(num_groups_x, num_groups_y, 1);
}

void GLPathTraceIntegrator::AdvanceSampleCount()
{
    increment_counter_pipeline_->Use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sample_counter_buffer_);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GLPathTraceIntegrator::GenerateRays()
{
    raygen_pipeline_->Use();

    raygen_pipeline_->BindConstant("width", width_);
    raygen_pipeline_->BindConstant("height", height_);
    raygen_pipeline_->BindConstant("camera.position", camera_.position);
    raygen_pipeline_->BindConstant("camera.front", camera_.front);
    raygen_pipeline_->BindConstant("camera.up", camera_.up);
    raygen_pipeline_->BindConstant("camera.fov", camera_.fov);
    raygen_pipeline_->BindConstant("camera.aspect_ratio", camera_.aspect_ratio);
    //raygen_pipeline_->BindConstant("camera.aperture", camera_.aperture);
    //raygen_pipeline_->BindConstant("camera.focus_distance", camera_.focus_distance);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, sample_counter_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rays_buffer_[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ray_counter_buffer_[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, pixel_indices_buffer_[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, throughputs_buffer_);

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
    std::uint32_t max_num_rays = width_ * height_;
    std::uint32_t incoming_idx = bounce & 1;

    intersect_pipeline_->Use();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rays_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ray_counter_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rt_triangle_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, nodes_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, hits_buffer_);

    ///@TODO: use indirect dispatch
    std::uint32_t num_groups = (max_num_rays + kIntersectGroupSize - 1) / kIntersectGroupSize;
    glDispatchCompute(num_groups, 1, 1);
}

void GLPathTraceIntegrator::ComputeAOVs()
{

}

void GLPathTraceIntegrator::ShadeMissedRays(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;
    std::uint32_t incoming_idx = bounce & 1;

    miss_pipeline_->Use();
    miss_pipeline_->BindConstant("width", width_);

    glBindTextureUnit(0, env_image_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rays_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ray_counter_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, hits_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, pixel_indices_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, throughputs_buffer_);
    glBindImageTexture(0, radiance_image_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    std::uint32_t num_groups = (max_num_rays + kMissGroupSize - 1) / kMissGroupSize;
    glDispatchCompute(num_groups, 1, 1);
}

void GLPathTraceIntegrator::ShadeSurfaceHits(std::uint32_t bounce)
{
    std::uint32_t max_num_rays = width_ * height_;

    std::uint32_t incoming_idx = bounce & 1;
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    hit_surface_pipeline_->Use();
    hit_surface_pipeline_->BindConstant("bounce", bounce);
    hit_surface_pipeline_->BindConstant("width", width_);
    hit_surface_pipeline_->BindConstant("scene_info.analytic_light_count", scene_info_.analytic_light_count);
    //hit_surface_pipeline_->BindConstant("scene_info.emissive_count", scene_info_.emissive_count);
    //hit_surface_pipeline_->BindConstant("scene_info.environment_map_index", scene_info_.environment_map_index);
    //hit_surface_pipeline_->BindConstant("scene_info.padding", scene_info_.padding);
    glBindImageTexture(0, radiance_image_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, sample_counter_buffer_);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, texture_handle_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rays_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ray_counter_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, pixel_indices_buffer_[incoming_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, rays_buffer_[outgoing_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ray_counter_buffer_[outgoing_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, pixel_indices_buffer_[outgoing_idx]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, shadow_rays_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, shadow_ray_counter_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, shadow_pixel_indices_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9,  direct_light_samples_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, hits_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, throughputs_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, triangle_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, analytic_light_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, emissive_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, material_buffer_);

    std::uint32_t num_groups = (max_num_rays + kShadeGroupSize - 1) / kShadeGroupSize;
    glDispatchCompute(num_groups, 1, 1);
}

void GLPathTraceIntegrator::IntersectShadowRays()
{
    std::uint32_t max_num_rays = width_ * height_;

    intersect_shadow_pipeline_->Use();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shadow_rays_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shadow_ray_counter_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rt_triangle_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, nodes_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, shadow_hits_buffer_);

    ///@TODO: use indirect dispatch
    std::uint32_t num_groups = (max_num_rays + kIntersectGroupSize - 1) / kIntersectGroupSize;
    glDispatchCompute(num_groups, 1, 1);
}

void GLPathTraceIntegrator::AccumulateDirectSamples()
{
    std::uint32_t max_num_rays = width_ * height_;

    accumulate_direct_samples_pipeline_->Use();
    accumulate_direct_samples_pipeline_->BindConstant("width", width_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shadow_hits_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shadow_ray_counter_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, shadow_pixel_indices_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, direct_light_samples_buffer_);
    glBindImageTexture(4, radiance_image_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    std::uint32_t num_groups = (max_num_rays + kAccumulateDirectSamplesGroupSize - 1) / kAccumulateDirectSamplesGroupSize;
    glDispatchCompute(num_groups, 1, 1);
}

void GLPathTraceIntegrator::ClearOutgoingRayCounter(std::uint32_t bounce)
{
    std::uint32_t outgoing_idx = (bounce + 1) & 1;

    clear_counter_pipeline_->Use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ray_counter_buffer_[outgoing_idx]);
    glDispatchCompute(1, 1, 1);
}

void GLPathTraceIntegrator::ClearShadowRayCounter()
{
    clear_counter_pipeline_->Use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shadow_ray_counter_buffer_);
    glDispatchCompute(1, 1, 1);
}

void GLPathTraceIntegrator::Denoise()
{

}

void GLPathTraceIntegrator::CopyHistoryBuffers()
{

}

void GLPathTraceIntegrator::ResolveRadiance()
{
    resolve_pipeline_->Use();
    resolve_pipeline_->BindConstant("width", width_);
    resolve_pipeline_->BindConstant("height", height_);
    glBindImageTexture(0, radiance_image_, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, out_image_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, sample_counter_buffer_);

    std::uint32_t num_groups_x = (width_ + kResolveGroupSize - 1) / kResolveGroupSize;
    std::uint32_t num_groups_y = (height_ + kResolveGroupSize - 1) / kResolveGroupSize;
    glDispatchCompute(num_groups_x, num_groups_y, 1);
}
