cmake_minimum_required(VERSION 3.8)

project(RayTracing CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

option(RAYTRACING_ENABLE_RHI "Enable GpuApi RHI renderers (Vulkan and D3D12)" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(glfw3 REQUIRED)
find_package(OpenCL_Light REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

if(RAYTRACING_ENABLE_RHI)
    set(GPU_API_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GpuApi)
endif()

add_subdirectory(src)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/CLI11)
