#define GLFW_DLL

#include "raytracer.hpp"

int main()
{
    RayTracer rt("meshes/box.obj");
    rt.Start();

    return 0;

}
