#include "viewport.hpp"
#include <iostream>

Viewport::Viewport(size_t width, size_t height)
    : m_Width(width), m_Height(height), m_FrameCount(0)
{
    m_Pixels = new float3[m_Width * m_Height];
}

void Viewport::Draw() const
{
    //glDrawPixels(m_Width, m_Height, GL_RGBA, GL_FLOAT, m_Pixels);
}

void Viewport::Update(std::shared_ptr<ClContext> context)
{    
    //context.SetArgument(10, sizeof(cl_uint), &m_FrameCount);

    context->ExecuteKernel(m_Pixels, m_Width * m_Height);

    ++m_FrameCount;
    //if (m_CameraIsChanged())
    //{
    //    m_FrameCount = 0;
    //}

}
