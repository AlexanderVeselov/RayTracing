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
    std::vector<std::uint32_t> tex_data(width_ * height_, 0xFFFFFFFF);

    //glCreateTextures(GL_TEXTURE_2D, 1, &render_texture_);
    //glTextureStorage2D(render_texture_, 1, GL_RGBA8, width_, height_);
    //glTextureSubImage2D(render_texture_, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());

    glGenTextures(1, &render_texture_);
    glBindTexture(GL_TEXTURE_2D, render_texture_);
    //regular sampler params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //need to set GL_NEAREST
    //(not GL_NEAREST_MIPMAP_* which would cause CL_INVALID_GL_OBJECT later)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);


}

void Framebuffer::Present()
{
    // Draw screen-aligned triangle
    glBindTextureUnit(0, render_texture_);
    draw_pipeline_.Use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
