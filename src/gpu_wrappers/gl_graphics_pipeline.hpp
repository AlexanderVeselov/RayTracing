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

#include <GL/glew.h>
#include <vector>
#include <string>

class GraphicsPipeline
{
public:
    struct GraphicsPipelineDesc
    {
        char const* vs_filename;
        char const* fs_filename;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<GLuint> color_attachments;
        GLuint depth_attachment = 0u;
        bool clear_color = false;
        bool clear_depth = false;
        bool depth_test_enabled = false;
    };

    GraphicsPipeline(GraphicsPipelineDesc const& pipeline_desc);
    void Bind() const;
    void Unbind() const;
    GLuint GetProgram() const { return shader_program_; }
    ~GraphicsPipeline();
    bool HasDepthAttachment() const { return desc_.depth_attachment != 0u; }
    bool HasColorAttachments() const { return !desc_.color_attachments.empty(); }

private:
    GraphicsPipelineDesc desc_;
    GLuint framebuffer_ = 0u;
    GLuint vertex_shader_;
    GLuint fragment_shader_;
    GLuint shader_program_;

};
