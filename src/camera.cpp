#include "camera.hpp"
#include "math_functions.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

Camera::Camera(int width, int height, float3 position, float fov, float sensivity, float speed) :
	width_(width),
	height_(height),
	position_(position),
	velocity_(0.0f),
	front_(1.0f, 0.0f, 0.0f),                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
	up_(0.0f, 0.0f, 1.0f),
	pitch_(0.0f),
	yaw_(0.0f),
	fov_(fov),
	speed_(speed),
	sensivity_(sensivity),
    changed_(false)
{
}

void Camera::Update(GLFWwindow *window, float delta)
{
    changed_ = false;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
	    glfwSetCursorPos(window, width_ / 2, height_ / 2);

	    yaw_   -= (static_cast<float>(xpos) - static_cast<float>(width_) / 2.0f) * sensivity_;
	    pitch_ -= (static_cast<float>(ypos) - static_cast<float>(height_) / 2.0f) * sensivity_;
	    pitch_  = clamp(pitch_, radians(-89.9f), radians(89.9f));
        changed_ = true;
    
    }

	front_ = float3(cos(yaw_)*cos(pitch_), sin(yaw_)*cos(pitch_), sin(pitch_));
    float3 right = float3(sin(yaw_), -cos(yaw_), 0.0f);
    up_	         = cross(right, front_);

    if (glfwGetKey(window, GLFW_KEY_W))
    {
        velocity_ += front_ * speed_ * delta;
        changed_ = true;
    }

    if (glfwGetKey(window, GLFW_KEY_S))
    {
		velocity_ -= front_ * speed_ * delta;
        changed_ = true;
    }

    if (glfwGetKey(window, GLFW_KEY_A))
    {
		velocity_ -= right * speed_ * delta;
        changed_ = true;
    }

    if (glfwGetKey(window, GLFW_KEY_D))
    {
		velocity_ += right * speed_ * delta;
        changed_ = true;
    }

	position_ += velocity_;
    velocity_ *= 0.5f;
	    	
}
