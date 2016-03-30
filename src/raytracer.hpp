#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "camera.hpp"
#include "scene.hpp"
#include "cl_context.hpp"
#include <GLFW/glfw3.h>

class RayTracer
{
public:
    RayTracer();
    void MainLoop();
    void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    void CursorCallback(GLFWwindow *window, double xpos, double ypos);

private:
    void Update();
    void Render();
    unsigned int width, height;
    ClContext context;
    GLFWwindow* window;
    Camera camera;
    Scene  scene;

};

#endif // RAYTRACER_HPP
