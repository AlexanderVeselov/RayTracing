#include "camera.hpp"
#include "render.hpp"
#include "inputsystem.hpp"
#include "mathlib.hpp"
#include <Windows.h>
#include <iostream>

Camera::Camera(std::shared_ptr<Viewport> viewport)
    :
    m_Viewport(viewport),
    m_Origin(32.0f, 0.0f, 32.0f),
    m_Pitch(MATH_PIDIV2),
    m_Yaw(-MATH_PI),
    m_Speed(32.0f),
    m_Changed(false),
    m_Up(0.0f, 0.0f, 1.0f)
{
}

void Camera::Update()
{
    m_Changed = false;
    static POINT point = { 0, 0 };
    unsigned short x, y;
    input->GetMousePos(&x, &y);
    POINT mouseClient = { x, y };
    ScreenToClient(render->GetHWND(), &mouseClient);

    if (input->IsMousePressed(MK_RBUTTON) && mouseClient.x > 0 && mouseClient.y > 0 && mouseClient.x < m_Viewport->width && mouseClient.y < m_Viewport->height)
    {
        float sensivity = 0.00075f;
        m_Yaw += (x - point.x) * sensivity;
        m_Pitch += (y - point.y) * sensivity;
        float epsilon = 0.0001f;
        m_Pitch = clamp(m_Pitch, 0.0f + epsilon, MATH_PI - epsilon);
        input->SetMousePos(point.x, point.y);
        m_Changed = true;
    }
    else
    {
        point = { x, y };
    }

    int frontback = input->IsKeyDown('W') - input->IsKeyDown('S');
    int strafe = input->IsKeyDown('A') - input->IsKeyDown('D');

    if (frontback != 0 || strafe != 0)
    {
        m_Changed = true;
    }
    
    m_Origin += float3(std::cosf(m_Yaw) * std::sinf(m_Pitch) * frontback + std::cosf(m_Yaw - MATH_PIDIV2) * strafe,
        std::sinf(m_Yaw) * std::sinf(m_Pitch) * frontback + std::sinf(m_Yaw - MATH_PIDIV2) * strafe, std::cosf(m_Pitch) * frontback) * m_Speed * render->GetDeltaTime();
    m_Front = float3(std::cosf(m_Yaw) * std::sinf(m_Pitch), std::sinf(m_Yaw) * std::sinf(m_Pitch), std::cosf(m_Pitch));

    std::cout << m_Origin << std::endl;
    float3 right = cross(m_Front, m_Up).normalize();
    float3 up = cross(right, m_Front);

    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::CAM_ORIGIN, &m_Origin, sizeof(float3));
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::CAM_FRONT, &m_Front, sizeof(float3));
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::CAM_UP, &up, sizeof(float3));

}
