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
    m_Speed(0.1f),
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

    if (input->IsMousePressed(MK_RBUTTON) && mouseClient.x > 0 && mouseClient.y > 0 && mouseClient.x < m_Viewport->GetWidth() && mouseClient.y < m_Viewport->GetHeight())
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
        std::sinf(m_Yaw) * std::sinf(m_Pitch) * frontback + std::sinf(m_Yaw - MATH_PIDIV2) * strafe, std::cosf(m_Pitch) * frontback) * m_Speed;
    m_Front = float3(std::cosf(m_Yaw) * std::sinf(m_Pitch), std::sinf(m_Yaw) * std::sinf(m_Pitch), std::cosf(m_Pitch));

}

/*
void Camera::Update()
{
    m_Changed = false;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        unsigned short xpos, ypos;
        input->GetMousePos(&xpos, &ypos);
        input->SetMousePos()
        glfwSetCursorPos(window, m_Width / 2, m_Height / 2);

        m_Yaw   -= (static_cast<float>(xpos) - static_cast<float>(m_Width) / 2.0f) * m_Sensivity;
        m_Pitch -= (static_cast<float>(ypos) - static_cast<float>(m_Height) / 2.0f) * m_Sensivity;
        m_Pitch  = clamp(m_Pitch, radians(-89.9f), radians(89.9f));
        m_Changed = true;
    
    }

    m_Front = float3(cos(m_Yaw)*cos(m_Pitch), sin(m_Yaw)*cos(m_Pitch), sin(m_Pitch));
    float3 right = float3(sin(m_Yaw), -cos(m_Yaw), 0.0f);
    m_Up             = cross(right, m_Front);

    if (glfwGetKey(window, GLFW_KEY_W))
    {
        m_Velocity += m_Front * m_Speed * delta;
        m_Changed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_S))
    {
        m_Velocity -= m_Front * m_Speed * delta;
        m_Changed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_A))
    {
        m_Velocity -= right * m_Speed * delta;
        m_Changed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_D))
    {
        m_Velocity += right * m_Speed * delta;
        m_Changed = true;
    }

    m_Position += m_Velocity;
    m_Velocity *= 0.5f;
            
}
*/
