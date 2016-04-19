#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "camera.hpp"
#include "scene.hpp"
#include "cl_context.hpp"
#include "vectors.hpp"
#include <GLFW/glfw3.h>
#include <boost/thread/thread.hpp>

class RayTracer
{
public:
    RayTracer(const char* filename);
    void Start();

private:
    void Update();
    void Render();
    void testThread(size_t i);
    std::vector<cl_float4*> pixels_;
    
    size_t width_;
    size_t height_;

    std::vector<ClContext> contexts_;
    GLFWwindow* window_;
    Camera camera_;
    Scene  scene_;
    std::ofstream loop_example_;

};

#endif // RAYTRACER_HPP
