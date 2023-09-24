/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

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

#include "gl_graphics_pipeline.hpp"
#include "gl_shader_utils.hpp"
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <cassert>

GraphicsPipeline::GraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc)
    : desc_(pipeline_desc)
{
    vertex_shader_ = CreateShader(desc_.vs_filename, GL_VERTEX_SHADER);
    fragment_shader_ = CreateShader(desc_.fs_filename, GL_FRAGMENT_SHADER);

    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader_);
    glAttachShader(shader_program_, fragment_shader_);
    glLinkProgram(shader_program_);

    GLint link_status;
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &link_status);

    if (link_status == false)
    {
        GLint log_length = 0;
        glGetProgramiv(shader_program_, GL_INFO_LOG_LENGTH, &log_length);

        // The maxLength includes the NULL character
        std::string info_log;
        info_log.resize(log_length);
        glGetProgramInfoLog(shader_program_, log_length, &log_length, &info_log[0]);

        throw std::runtime_error(info_log);
    }

    // Create the framebuffer
    if (HasColorAttachments() || HasDepthAttachment())
    {
        glCreateFramebuffers(1, &framebuffer_);
        assert(desc_.color_attachments.size() <= 16);

        // Bind color attachments
        for (auto i = 0; i < desc_.color_attachments.size(); ++i)
        {
            auto const& attachment = desc_.color_attachments[i];
            glNamedFramebufferTexture(framebuffer_, GL_COLOR_ATTACHMENT0 + i, attachment, 0);
        }

        if (desc_.depth_attachment != 0)
        {
            // Bind depth attachments
            glNamedFramebufferTexture(framebuffer_, GL_DEPTH_ATTACHMENT, desc_.depth_attachment, 0);
        }
    }
}

void GraphicsPipeline::Bind() const
{
    // If we have a framebuffer, bind it
    if (framebuffer_ != 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    }

    // Enable depth test
    if (desc_.depth_test_enabled)
    {
        glEnable(GL_DEPTH_TEST);
    }

    if (desc_.clear_color)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Clear color and/or depth
    if (desc_.clear_color || desc_.clear_depth)
    {
        GLbitfield clear_mask = 0;
        clear_mask |= desc_.clear_color ? GL_COLOR_BUFFER_BIT : 0;
        clear_mask |= desc_.clear_depth ? GL_DEPTH_BUFFER_BIT : 0;
        glClear(clear_mask);
    }

    // Use the shader program
    glUseProgram(shader_program_);
}

void GraphicsPipeline::Unbind() const
{
    // Unbind the shader program
    glUseProgram(0);

    // Disable depth test
    if (desc_.depth_test_enabled)
    {
        glDisable(GL_DEPTH_TEST);
    }

    // If we have a framebuffer, unbind it
    if (framebuffer_ != 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

GraphicsPipeline::~GraphicsPipeline()
{
    glDeleteProgram(shader_program_);
    glDeleteShader(fragment_shader_);
    glDeleteShader(vertex_shader_);
    if (framebuffer_ != 0)
    {
        glDeleteFramebuffers(1, &framebuffer_);
    }
}
