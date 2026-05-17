cmake_minimum_required(VERSION 3.8)

project(RayTracing CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(glfw3 REQUIRED)

set(GPU_API_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GpuApi)

add_subdirectory(src)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/CLI11)
