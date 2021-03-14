#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "mathlib/mathlib.hpp"
#include "GpuWrappers/gl_graphics_pipeline.hpp"

class Framebuffer
{
public:
    Framebuffer(std::uint32_t width, std::uint32_t height);
    void Present();
    std::uint32_t GetWidth() const { return width_; }
    std::uint32_t GetHeight() const { return height_; }
    GLuint GetGLImage() const { return render_texture_; }

private:
    std::uint32_t width_;
    std::uint32_t height_;
    GraphicsPipeline draw_pipeline_;
    GLuint render_texture_;
};

#endif // FRAMEBUFFER_HPP
