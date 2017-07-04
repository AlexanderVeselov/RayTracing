#ifndef m_CameraHPP
#define m_CameraHPP

#include "vectors.hpp"
#include <GLFW/glfw3.h>

class Camera
{
public:
	Camera(int width, int height, float3 position, float fov, float sensivity, float speed);
	void Update(GLFWwindow *window, float delta);
	void KeyCallback(int Key, int Action);
	void CursorCallback(float xpos, float ypos);

	// Getters...
	float3 GetPosition()         const { return m_Position; }
	float3 GetFrontVector() const { return m_Front; }
	float3 GetUpVector()    const { return m_Up; }

    bool isChanged()        const { return m_Changed; }

private:
	int m_Width;
    int m_Height;

	float3 m_Position;
    float3 m_Velocity;
    float3 m_Front;
    float3 m_Up;

	float m_Pitch;
    float m_Yaw;
    
	float m_Fov;
    float m_Speed;
	float m_Sensivity;
    bool m_Changed;

};

#endif // m_CameraHPP
