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

#include "framebuffer.hpp"

#include <vector>

Framebuffer::Framebuffer(std::uint32_t width, std::uint32_t height)
    : width_(width)
    , height_(height)
    , draw_pipeline_("fullscreen_quad.vert", "fullscreen_quad.frag")
{
    // Enable SRGB framebuffer
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Create framebuffer texture
    glCreateTextures(GL_TEXTURE_2D, 1, &render_texture_);
    glTextureStorage2D(render_texture_, 1, GL_RGBA32F, width_, height_);
}

void Framebuffer::Present()
{
    // Draw screen-aligned triangle
    glBindTextureUnit(0, render_texture_);
    draw_pipeline_.Use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
