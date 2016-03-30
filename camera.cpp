#include "camera.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

//#define M_PI 3.141592f

Camera::Camera(glm::vec3 Position, float Sensivity, float Speed, int Width, int Height, float Fov) :
	position(Position),
	velocity(0.0f),
	front(1.0f, 0.0f, 0.0f),
	up(0.0f, 0.0f, 1.0f),
	pitch(0),
	yaw(0.0f),
	sensivity(Sensivity),
	speed(Speed),
	width(Width),
	height(Height),
	fov(Fov),
	key(0),
	keyAction(0)
{
}

void Camera::Update(GLFWwindow *window, float delta)
{
	switch(key)
	{
	case GLFW_KEY_W:
		velocity = front*speed*delta;
		break;
	case GLFW_KEY_S:
		velocity = -front*speed*delta;
		break;
	case GLFW_KEY_A:
		velocity = -right*speed*delta;
		break;
	case GLFW_KEY_D:
		velocity = right*speed*delta;
		break;
	}

	if (keyAction == GLFW_RELEASE)
	{
		velocity = glm::vec3(0);
	}

	position += velocity;
	
	//std::cout << "(" << position.x << ", " << position.y << ", " << position.z << ") ";
    	
}

void Camera::KeyCallback(int Key, int Action)
{
	key = Key;
	keyAction = Action;
}

void Camera::CursorCallback(float xpos, float ypos)
{
    if (abs(xpos - width/2) > 0 || abs(ypos - height/2) > 0)
    {
        bCameraChanged = true;
    }
    else
    {
        bCameraChanged = false;
    }

	yaw -= (xpos - width / 2) * sensivity;
	pitch -= (ypos - height / 2) * sensivity;
	pitch = glm::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));

	front = glm::vec3(cos(yaw)*cos(pitch), sin(yaw)*cos(pitch), sin(pitch));
	right = glm::vec3(sin(yaw), -cos(yaw), 0);
	up	  = glm::cross(right, front);

}
