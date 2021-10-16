/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

#include "camera.hpp"
#include "render.hpp"
#include "Utils/window.hpp"
#include <iostream>

Camera::Camera(Framebuffer& framebuffer, Render& render)
    : m_Render(render)
    , framebuffer_(framebuffer)
    , m_Origin(0.0f, -1.0f, 1.0f)
    , m_Pitch(MATH_PIDIV2)
    , m_Yaw(MATH_PIDIV2)
    , m_Speed(1.0f)
    , m_FrameCount(0)
    , m_Up(0.0f, 0.0f, 1.0f)
{
}

void Camera::Update()
{
    Window& window = m_Render.GetWindow();
    static int prev_x = 0, prev_y = 0;
    int x, y;
    window.GetMousePos(&x, &y);

    if (window.IsRightMouseButtonPressed())
    {
        float sensivity = 0.00075f;
        m_Yaw -= (x - prev_x) * sensivity;
        m_Pitch += (y - prev_y) * sensivity;
        float epsilon = 0.0001f;
        m_Pitch = clamp(m_Pitch, 0.0f + epsilon, MATH_PI - epsilon);
        window.SetMousePos(prev_x, prev_y);
        m_FrameCount = 0;
    }
    else
    {
        prev_x = x;
        prev_y = y;
    }

    int frontback = window.IsKeyPressed('W') - window.IsKeyPressed('S');
    int strafe = window.IsKeyPressed('A') - window.IsKeyPressed('D');

    if (frontback != 0 || strafe != 0)
    {
        m_FrameCount = 0;
    }
    
    m_Origin += float3(std::cosf(m_Yaw) * std::sinf(m_Pitch) * frontback - std::cosf(m_Yaw - MATH_PIDIV2) * strafe,
        std::sinf(m_Yaw) * std::sinf(m_Pitch) * frontback - std::sinf(m_Yaw - MATH_PIDIV2) * strafe, std::cosf(m_Pitch) * frontback) * m_Speed * m_Render.GetDeltaTime();
    m_Front = float3(std::cosf(m_Yaw) * std::sinf(m_Pitch), std::sinf(m_Yaw) * std::sinf(m_Pitch), std::cosf(m_Pitch));

    ++m_FrameCount;

}
