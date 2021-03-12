cmake_minimum_required(VERSION 3.8)

project(RayTracing CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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


add_executable(RayTracingApp ${SOURCES})
find_package(OpenCL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
target_include_directories(RayTracingApp PUBLIC "${RayTracing_SOURCE_DIR}/src")
target_link_libraries(RayTracingApp PUBLIC OpenCL::OpenCL OpenGL::GL GLEW::GLEW OpenGL::GLU)
set_target_properties(RayTracingApp PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${RayTracing_SOURCE_DIR}
)

add_custom_command(TARGET RayTracingApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/3rdparty/glew-2.1.0/bin/x64/glew32.dll"
    $<TARGET_FILE_DIR:RayTracingApp>)
