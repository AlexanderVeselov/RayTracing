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
#include "io/inputsystem.hpp"
#include <Windows.h>
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
    static POINT point = { 0, 0 };
    unsigned short x, y;
    input->GetMousePos(&x, &y);
    POINT mouseClient = { x, y };
    ScreenToClient(m_Render.GetHWND(), &mouseClient);

    if (input->IsMousePressed(MK_RBUTTON) && mouseClient.x > 0 && mouseClient.y > 0
        && mouseClient.x < framebuffer_.GetWidth() && mouseClient.y < framebuffer_.GetHeight())
    {
        float sensivity = 0.00075f;
        m_Yaw -= (x - point.x) * sensivity;
        m_Pitch += (y - point.y) * sensivity;
        float epsilon = 0.0001f;
        m_Pitch = clamp(m_Pitch, 0.0f + epsilon, MATH_PI - epsilon);
        input->SetMousePos(point.x, point.y);
        m_FrameCount = 0;
    }
    else
    {
        point = { x, y };
    }

    int frontback = input->IsKeyDown('W') - input->IsKeyDown('S');
    int strafe = input->IsKeyDown('A') - input->IsKeyDown('D');

    if (frontback != 0 || strafe != 0)
    {
        m_FrameCount = 0;
    }
    
    m_Origin += float3(std::cosf(m_Yaw) * std::sinf(m_Pitch) * frontback - std::cosf(m_Yaw - MATH_PIDIV2) * strafe,
        std::sinf(m_Yaw) * std::sinf(m_Pitch) * frontback - std::sinf(m_Yaw - MATH_PIDIV2) * strafe, std::cosf(m_Pitch) * frontback) * m_Speed * m_Render.GetDeltaTime();
    m_Front = float3(std::cosf(m_Yaw) * std::sinf(m_Pitch), std::sinf(m_Yaw) * std::sinf(m_Pitch), std::cosf(m_Pitch));

    ++m_FrameCount;

}
