#include "camera.hpp"
#include "math_functions.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

Camera::Camera(int width, int height, float3 position, float fov, float sensivity, float speed) :
	m_Width(width),
	m_Height(height),
	m_Position(position),
	m_Velocity(0.0f),
	m_Front(1.0f, 0.0f, 0.0f),                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
	m_Up(0.0f, 0.0f, 1.0f),
	m_Pitch(0.0f),
	m_Yaw(0.0f),
	m_Fov(fov),
	m_Speed(speed),
	m_Sensivity(sensivity),
    m_Changed(false)
{
}

void Camera::Update(GLFWwindow *window, float delta)
{
    m_Changed = false;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
	    glfwSetCursorPos(window, m_Width / 2, m_Height / 2);

	    m_Yaw   -= (static_cast<float>(xpos) - static_cast<float>(m_Width) / 2.0f) * m_Sensivity;
	    m_Pitch -= (static_cast<float>(ypos) - static_cast<float>(m_Height) / 2.0f) * m_Sensivity;
	    m_Pitch  = clamp(m_Pitch, radians(-89.9f), radians(89.9f));
        m_Changed = true;
    
    }

	m_Front = float3(cos(m_Yaw)*cos(m_Pitch), sin(m_Yaw)*cos(m_Pitch), sin(m_Pitch));
    float3 right = float3(sin(m_Yaw), -cos(m_Yaw), 0.0f);
    m_Up	         = cross(right, m_Front);

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
