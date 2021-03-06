#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "mathlib/mathlib.hpp"

class Framebuffer
{
public:
    Framebuffer(std::uint32_t width, std::uint32_t height);
    ~Framebuffer();

private:
    std::uint32_t width_;
    std::uint32_t height_;

};

#endif // FRAMEBUFFER_HPP
