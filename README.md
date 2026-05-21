# Realtime GPU Path Tracing Engine

Join my telegram channel, where I post everything related to graphics!
https://t.me/cg_lib

[**Watch demo on YouTube**](https://youtu.be/UIjra1T7ilI)
[![Bistro scene](screenshots/Bistro.png)](https://youtu.be/UIjra1T7ilI)
![](screenshots/SanMiguel.png)

## Features

- Realtime GPU wavefront path tracing using compute shaders
- Vulkan and D3D12 backends through `GpuApi`
- Hardware ray tracing through Ray Query when supported by the selected backend
- CPU-built BVH fallback with GPU traversal when Ray Query is unavailable
- Lambert diffuse and GGX reflection BRDFs
- Point, directional and environment light sampling
- Temporal reprojection filter
- Depth, normals, albedo and motion vector AOV generation
- Runtime compute pipeline hot reload

## Ray Tracing Paths

The application selects the acceleration-structure path automatically:

- If `Device::SupportsRayQuery()` is true, `HardwareRtAccelerationStructure`
  builds BLAS objects per mesh and a TLAS over scene instances. Tracing uses
  `trace_rayquery.cs` and `trace_shadow_rayquery.cs`.
- Otherwise `Bvh` builds a CPU BVH from scene meshes/instances, uploads the
  nodes and triangles, and tracing uses `trace_bvh.cs` and
  `trace_shadow_bvh.cs`.

Both paths keep the same high-level scene model: `SceneInstance` stores the
mesh index and transform, while ray-hit data stores `instance_id` and
`primitive_id`.

## Building

Requirements:

- CMake
- A C++17 compiler
- GLFW
- Vulkan SDK for the Vulkan backend
- Windows SDK with D3D12 for the D3D12 backend

Clone with submodules:

```bat
git clone --recursive https://github.com/AlexanderVeselov/RayTracing.git
cd RayTracing
```

Generate and build:

```bat
cmake -S . -B build
cmake --build build --config Release --target RayTracingApp
```

The Visual Studio project sets the debugger working directory to the repository
root. `dxcompiler.dll` and `dxil.dll` are copied next to `RayTracingApp` after
build.

## Running

Run `RayTracingApp` from the repository root:

```bat
build\src\Release\RayTracingApp.exe --backend vulkan --scene assets/ShaderBalls.obj
```

Command-line options:

- `--help` prints command-line help
- `-w <width>` sets window width, default `1280`
- `-h <height>` sets window height, default `720`
- `--scene <path>` sets OBJ scene path, default `assets/ShaderBalls.obj`
- `--scale <scale>` sets imported scene scale, default `1.0`
- `--flip_yz <0|1>` flips Y and Z axes during import, default `0`
- `--backend <vulkan|d3d12>` selects render backend, default `vulkan`

Press `R` while the app is running to reload registered compute pipelines. The
reload keeps the previous pipeline if shader compilation or reflected layout
compatibility checks fail.

`run_bistro.bat` downloads Amazon Lumberyard Bistro content into `assets`,
builds the project and runs the Bistro scene.
