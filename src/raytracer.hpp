#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "camera.hpp"
#include "scene.hpp"
#include "cl_context.hpp"
#include "vectors.hpp"
#include "viewport.hpp"
#include <GLFW/glfw3.h>

class RayTracer
{
public:
    RayTracer(const char* filename);
    void Start();

private:
    void Update();
    void Render();
    //void testThread(size_t i);
    
    size_t m_Width;
    size_t m_Height;

    std::vector<ClContext> m_Contexts;
    GLFWwindow* m_Window;
    Camera m_Camera;
    Scene  m_Scene;
    Viewport m_Viewport;


};

#endif // RAYTRACER_HPP
