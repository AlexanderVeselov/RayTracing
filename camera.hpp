#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera
{
public:
	Camera(glm::vec3 Position, float Sensivity, float Speed, int Width, int Height, float Fov);
	void Update(GLFWwindow *window, float delta);
	void KeyCallback(int Key, int Action);
	void CursorCallback(float xpos, float ypos);

	// Getters...
	glm::vec3 GetPos() const { return position; }
	glm::vec3 GetVelocity() const { return velocity; }
	glm::vec3 GetFrontVec() const { return front; }
	glm::vec3 GetUpVec() const { return up; }
    bool      Changed() const { return bCameraChanged; }

private:
	glm::vec3 position, velocity, front, right, up;
	float pitch, yaw;

	float sensivity, speed;
	int width, height;
	float fov;
	int key, keyAction;
    bool bCameraChanged;

};

#endif // CAMERA_HPP
