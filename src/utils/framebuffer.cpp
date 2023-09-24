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

#include "framebuffer.hpp"

#include <vector>

Framebuffer::Framebuffer(std::uint32_t width, std::uint32_t height)
    : width_(width)
    , height_(height)
{
    // Enable SRGB framebuffer
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Create framebuffer texture
    glCreateTextures(GL_TEXTURE_2D, 1, &render_texture_);
    glTextureStorage2D(render_texture_, 1, GL_RGBA32F, width_, height_);

    GraphicsPipeline::GraphicsPipelineDesc pipeline_desc;
    pipeline_desc.width = width_;
    pipeline_desc.height = height_;
    pipeline_desc.vs_filename = "fullscreen_quad.vert";
    pipeline_desc.fs_filename = "fullscreen_quad.frag";
    draw_pipeline_ = std::make_unique<GraphicsPipeline>(pipeline_desc);
}

void Framebuffer::Present()
{
    // Draw screen-aligned triangle
    draw_pipeline_->Bind();
    glBindTextureUnit(0, render_texture_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    draw_pipeline_->Unbind();
}
