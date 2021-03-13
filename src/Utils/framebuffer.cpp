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
        "varying vec2 vTexcoord;"
        "void main() {"
        "    gl_FragColor = vec4(vTexcoord, 0.0, 1.0);"
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

    GLuint render_texture;
    glGenTextures(1, &render_texture);
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);

}

void Framebuffer::Present()
{
    // Draw screen-aligned triangle
    draw_pipeline_.Use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
