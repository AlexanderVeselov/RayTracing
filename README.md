# Realtime GPU Path Tracing Engine

Join my telegram channel, where I post everything related to graphics!
https://t.me/cg_lib

[**Watch demo on YouTube**](https://youtu.be/UIjra1T7ilI)
[![Bistro scene](screenshots/Bistro.png)](https://youtu.be/UIjra1T7ilI)
![](screenshots/SanMiguel.png)

## Features
* Realtime GPU wavefront path tracer using compute shaders
* RHI backends powered by [GpuApi](https://github.com/AlexanderVeselov/GpuApi): Vulkan and D3D12
* Vulkan validation layers in Debug builds
* BVH traversal on GPU
* Lambert diffuse and GGX reflection BRDFs
* Explicit point and directional light sampling
* Environment map sampling
* Temporal reprojection filter
* Depth, normals, albedo and motion vectors AOV generation

## Building
1. Clone the repository `git clone --recursive https://github.com/AlexanderVeselov/RayTracing.git`
2. Generate a solution using [CMake](https://cmake.org/download/)
3. Open the solution and build `RayTracingApp` project

The project builds the RHI renderers by default:

```bat
cmake -S . -B build
cmake --build build --config Release
```

## Running
* Run `RayTracingApp` executable
* You can provide the following optional arguments:
    * `--help` print command line help
    * `-w <width>` window width, default `1280`
    * `-h <height>` window height, default `720`
    * `--scene <path>` path to scene to be loaded, default `assets/ShaderBalls.obj`
    * `--scale <scale>` scale of the imported scene, default `1.0`
    * `--flip_yz <0|1>` flip Y and Z axis of the scene, default `0`
    * `--backend <vulkan|d3d12>` select render backend, default `vulkan`
* You can also run `run_bistro.bat`. It downloads Amazon Lumberyard Bistro content to the `assets` folder, builds the project and runs it with the scene.
