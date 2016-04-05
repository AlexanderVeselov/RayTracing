#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "camera.hpp"
#include "scene.hpp"
#include "cl_context.hpp"
#include <GLFW/glfw3.h>

class RayTracer
{
public:
    RayTracer(const char* filename);
    void Start();

private:
    void Update();
    void Render();

    size_t width_;
    size_t height_;

    ClContext context_;
    GLFWwindow* window_;
    Camera camera_;
    Scene  scene_;

};

#endif // RAYTRACER_HPP
