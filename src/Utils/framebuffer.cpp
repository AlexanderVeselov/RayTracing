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

#include "framebuffer.hpp"

#include <vector>

namespace
{
    char const* kVertexShaderSource =
        "varying vec2 vTexcoord;"
        "void main() {"
        "    vTexcoord = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);"
        "    gl_Position = vec4(vTexcoord * 2.0 - 1.0, 0.0, 1.0);"
        "}";

    char const* kFragmentShaderSource =
        "uniform sampler2D input_sampler;"
        "varying vec2 vTexcoord;"
        "void main() {"
        "    gl_FragColor = texture(input_sampler, vTexcoord);"
        "}";
}

Framebuffer::Framebuffer(std::uint32_t width, std::uint32_t height)
    : width_(width)
    , height_(height)
    , draw_pipeline_(kVertexShaderSource, kFragmentShaderSource)
{
    // Enable SRGB framebuffer
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Create framebuffer texture
    glCreateTextures(GL_TEXTURE_2D, 1, &render_texture_);
    glTextureStorage2D(render_texture_, 1, GL_RGBA32F, width_, height_);
    //glTextureSubImage2D(render_texture_, 0, 0, 0, width_, height_, GL_RGBA, GL_FLOAT, nullptr);

    //glGenTextures(1, &render_texture_);
    //glBindTexture(GL_TEXTURE_2D, render_texture_);
    ////regular sampler params
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ////need to set GL_NEAREST
    ////(not GL_NEAREST_MIPMAP_* which would cause CL_INVALID_GL_OBJECT later)
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, 0);
    //glBindTexture(GL_TEXTURE_2D, 0);


}

void Framebuffer::Present()
{
    // Draw screen-aligned triangle
    glBindTextureUnit(0, render_texture_);
    draw_pipeline_.Use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
