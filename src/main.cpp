#define GLFW_DLL

#include "raytracer.hpp"

int main()
{
    RayTracer rt("meshes/city.obj");
    rt.Start();

    return 0;

}
