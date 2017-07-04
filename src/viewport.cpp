#include "viewport.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

Viewport::Viewport(const Camera& camera, size_t width, size_t height, size_t rectCount)
    : m_Camera(camera), m_Width(width), m_Height(height), m_RectCount(rectCount), m_CurrentRect(0), m_FrameCount(0)
{
    m_Pixels = new cl_float4[m_Width * m_Height];
}

void Viewport::Draw() const
{
    glDrawPixels(m_Width, m_Height, GL_RGBA, GL_FLOAT, m_Pixels);
}

void Viewport::Update(ClContext &context)
{
    //std::cout << "Thread: " << i << " " << m_CurrentRect << std::endl;
    
	context.SetArgument(4, sizeof(float3), &m_Camera.GetPosition());
	context.SetArgument(5, sizeof(float3), &m_Camera.GetFrontVector());
	context.SetArgument(6, sizeof(float3), &m_Camera.GetUpVector());

	context.SetArgument(10, sizeof(cl_uint), &m_FrameCount);
	//context.SetArgument(11, sizeof(cl_uint), &i);
	context.ExecuteKernel(m_Pixels, m_Width * m_Height / m_RectCount, m_Width * m_Height / m_RectCount * m_CurrentRect);
	++m_FrameCount;
	if (m_Camera.isChanged())
	{
		m_FrameCount = 0;
	}
    /*
    ++m_CurrentRect;
    if (m_CurrentRect >= m_RectCount)
    {
        ++m_FrameCount;
        if (m_Camera.isChanged())
            m_FrameCount = 0;
        m_CurrentRect = 0;
        cond.notify_all();
    }*/
}
