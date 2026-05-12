# Realtime GPU Path Tracing Engine

Join my telegram channel, where I post everything related to graphics!
https://t.me/cg_lib

[**Watch demo on YouTube**](https://youtu.be/UIjra1T7ilI)
[![Bistro scene](screenshots/Bistro.png)](https://youtu.be/UIjra1T7ilI)
![](screenshots/SanMiguel.png)

## Features
* Realtime GPU wavefront path tracer using compute shaders
* OpenCL backend
* OpenGL backend with OpenCL/OpenGL interop presentation
* Optional RHI backends powered by [GpuApi](https://github.com/AlexanderVeselov/GpuApi): Vulkan and D3D12
* Vulkan validation layers in Debug RHI builds
* Hybrid path tracing in OpenGL mode with rasterized primary visibility
* BVH traversal on GPU
* Lambert diffuse and GGX reflection BRDFs
* Explicit point and directional light sampling
* Environment map sampling
* Temporal reprojection filter
* Depth, normals, albedo and motion vectors AOV generation
* Hot kernel reloading

## Building
1. Clone the repository `git clone --recursive https://github.com/AlexanderVeselov/RayTracing.git`
2. Generate a solution using [CMake](https://cmake.org/download/)
3. Open the solution and build `RayTracingApp` project

By default the project builds only the OpenCL/OpenGL renderers. Vulkan and D3D12 are part of the new RHI path and are disabled by default:

```bat
cmake -S . -B build
cmake --build build --config Release
```

Enable the RHI backends explicitly when Vulkan/D3D12 support is needed:

```bat
cmake -S . -B build_rhi -DRAYTRACING_ENABLE_RHI=ON
cmake --build build_rhi --config Release
```

CMake options:

* `RAYTRACING_ENABLE_RHI=OFF` - default. Builds OpenCL/OpenGL only and does not link [GpuApi](https://github.com/AlexanderVeselov/GpuApi).
* `RAYTRACING_ENABLE_RHI=ON` - enables [GpuApi](https://github.com/AlexanderVeselov/GpuApi), HLSL kernels, Vulkan and D3D12 backends, and copies DXC runtime DLLs after build.

## Running
* Run `RayTracingApp` executable
* You can provide the following optional arguments:
    * `--help` print command line help
    * `-w <width>` window width, default `1280`
    * `-h <height>` window height, default `720`
    * `--scene <path>` path to scene to be loaded, default `assets/ShaderBalls.obj`
    * `--scale <scale>` scale of the imported scene, default `1.0`
    * `--flip_yz <0|1>` flip Y and Z axis of the scene, default `0`
    * `--backend <opencl|opengl>` select render backend in the default build
    * `--backend <opencl|opengl|vulkan|d3d12>` select render backend when built with `RAYTRACING_ENABLE_RHI=ON`
* You can also run `run_bistro.bat`. It downloads Amazon Lumberyard Bistro content to the `assets` folder, builds the project and runs it with the scene.
