#define GLFW_DLL

#include "raytracer.hpp"

int main()
{
    RayTracer rt("meshes/wall.obj");
    rt.Start();

    return 0;

}
