#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "devcontext.hpp"
#include <CL/cl.h>

class Viewport
{
public:
    Viewport(size_t width, size_t height);
    void Draw() const;
    void Update(std::shared_ptr<ClContext> context);
    size_t GetWidth() const { return m_Width; }
    size_t GetHeight() const { return m_Height; }
    float3* GetPixels() const { return m_Pixels; }

private:
    size_t m_Width;
    size_t m_Height;
    size_t m_FrameCount;
    float3* m_Pixels;

};

#endif // VIEWPORT_HPP
