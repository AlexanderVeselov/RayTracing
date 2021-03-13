#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "mathlib/mathlib.hpp"
#include "GpuWrappers/gl_graphics_pipeline.hpp"

class Framebuffer
{
public:
    Framebuffer(std::uint32_t width, std::uint32_t height);
    ~Framebuffer();

    void Present();

private:
    std::uint32_t width_;
    std::uint32_t height_;
    GraphicsPipeline draw_pipeline_;

};

#endif // FRAMEBUFFER_HPP
