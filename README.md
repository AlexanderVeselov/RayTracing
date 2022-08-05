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

## Pre-Requisites:
#### [CMake](https://cmake.org/download/)
