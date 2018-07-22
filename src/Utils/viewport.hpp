#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "mathlib/mathlib.hpp"

struct Viewport
{
public:
    Viewport(size_t x, size_t y, size_t width, size_t height)
        : x(x), y(y), width(width), height(height)
    {
        pixels = new float3[width * height];
    }

    ~Viewport() { if (pixels) delete[] pixels; }

    unsigned int x, y;
    unsigned int width, height;
    float3* pixels;

};

#endif // VIEWPORT_HPP
