# Realtime GPU Path Tracing engine based on OpenCL and OpenGL

[**Watch on YouTube**](https://youtu.be/UIjra1T7ilI)
[![Bistro scene](screenshots/Bistro.png)](https://youtu.be/UIjra1T7ilI)
![](screenshots/SanMiguel.png)

## Features
* Unidirectional wavefront path tracer done entirely on GPU using compute shaders
* OpenCL backend
* OpenGL backend (WIP)
* Hybrid path tracing (rasterization of the primary visibility) in OpenGL mode
* Lambert diffuse, GGX reflection BRDF
* Explicit point, directional light sampling
* Simple temporal reprojection filter
* Depth, normals, albedo, motion vectors AOV generation
* Hot kernel reloading
* OpenCL/OpenGL interop for presenting the image

## Building
1. Clone the repository `git clone --recursive https://github.com/AlexanderVeselov/RayTracing.git`
2. Generate a solution using [CMake](https://cmake.org/download/)
3. Open the solution and build `RayTracingApp` project

## Running
* Run `RayTracingApp` executable
* You can provide the following optional arguments
    * `-w`, `-h` window width and height
    * `--scene <path>` path to scene to be loaded
    * `--scale <scale>` scale of the imported scene
    * `--flip_yz 0/1` flip Y and Z axis of the scene (some scenes have Y up and some have Z up)
    * `--opengl 0/1` use OpenGL-only mode
