#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "mathlib/mathlib.hpp"
#include "GpuWrappers/gl_shader.hpp"

class Framebuffer
{
public:
    Framebuffer(std::uint32_t width, std::uint32_t height);
    ~Framebuffer();

private:
    std::uint32_t width_;
    std::uint32_t height_;
    Shader vertex_shader_;
    Shader fragment_shader_;

};

#endif // FRAMEBUFFER_HPP
