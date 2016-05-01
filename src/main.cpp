#define GLFW_DLL

#include "raytracer.hpp"

int main()
{
    RayTracer rt("meshes/cube.obj");
    rt.Start();

    return 0;

}
