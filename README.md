# Realtime GPU Path Tracing engine based on OpenCL and OpenGL

[**Watch on YouTube**](https://youtu.be/UIjra1T7ilI)
[![Bistro scene](screenshots/Bistro.png)](https://youtu.be/UIjra1T7ilI)
![](screenshots/SanMiguel.png)

## Features
* Unidirectional wavefront path tracer
* OpenCL backend
* OpenGL backend (WIP)
* Lambert diffuse, GGX reflection BRDF
* Explicit point, directional light sampling
* Simple temporal reprojection filter
* Depth, normals, albedo, motion vectors AOV generation
* Hot kernel reloading
* OpenCL/OpenGL interop for presenting the image

## Pre-Requisites:
#### OpenCL SDKs
* [NVIDIA CUDA Toolkit](https://developer.nvidia.com/cuda-downloads) — for NVIDIA GPU
* [AMD OpenCL™ SDK](https://github.com/GPUOpen-LibrariesAndSDKs/OCL-SDK/releases/) — for AMD GPU/CPU
* [Intel® SDK for OpenCL™ Applications](https://software.intel.com/en-us/opencl-sdk) — for Intel CPU/GPU

#### [CMake](https://cmake.org/download/)
