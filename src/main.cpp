#define GLFW_DLL

#include "raytracer.hpp"

int main()
{
    RayTracer rt("meshes/room.obj");
    rt.Start();

    return 0;

}
