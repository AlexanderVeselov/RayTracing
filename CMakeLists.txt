cmake_minimum_required(VERSION 3.8)

project(RayTracing CXX)

set(CONTEXT_SOURCES
    src/context/cl_context.cpp
    src/context/cl_context.hpp
)

set(IO_SOURCES
    src/io/dds_loader.cpp
    src/io/hdr_loader.cpp
    src/io/image_loader.hpp
    src/io/inputsystem.cpp
    src/io/inputsystem.hpp
)

set(KERNELS_SOURCES
    src/kernels/kernel_bvh.cl
    src/kernels/kernel_grid.cl
)

set(MATHLIB_SOURCES
    src/mathlib/mathlib.cpp
    src/mathlib/mathlib.hpp
)

set(RENDERERS_SOURCES
    src/renderers/render.cpp
    src/renderers/render.hpp
)

set(SCENE_SOURCES
    src/scene/camera.cpp
    src/scene/camera.hpp
    src/scene/scene.cpp
    src/scene/scene.hpp
)

set(UTILS_SOURCES
    src/utils/cl_exception.hpp
    src/utils/shared_structs.hpp
    src/utils/viewport.hpp
)

set(MAIN_SOURCES
    src/main.cpp
)

set(SOURCES
    ${CONTEXT_SOURCES}
    ${IO_SOURCES}
    ${KERNELS_SOURCES}
    ${MATHLIB_SOURCES}
    ${RENDERERS_SOURCES}
    ${SCENE_SOURCES}
    ${UTILS_SOURCES}
    ${MAIN_SOURCES}
)

add_executable(RayTracing ${SOURCES})
find_package(OpenCL REQUIRED)
find_package(OpenGL REQUIRED)
target_include_directories(RayTracing PUBLIC "${RayTracing_SOURCE_DIR}/src")
target_link_libraries(RayTracing PUBLIC OpenCL::OpenCL OpenGL::GL OpenGL::GLU)
set_target_properties(RayTracing PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${RayTracing_SOURCE_DIR}
)
