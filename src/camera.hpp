#ifndef CAMERA_HPP
#define CAMERA_HPP

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
	float3 getPos()         const { return position_; }
	float3 getFrontVector() const { return front_; }
	float3 getUpVector()    const { return up_; }

    bool isChanged()        const { return changed_; }

private:
	int width_;
    int height_;

	float3 position_;
    float3 velocity_;
    float3 front_;
    float3 up_;

	float pitch_;
    float yaw_;
    
	float fov_;
    float speed_;
	float sensivity_;
    bool changed_;

};

#endif // CAMERA_HPP
