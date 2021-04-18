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
