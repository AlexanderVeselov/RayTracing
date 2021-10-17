set(GPU_WRAPPERS_SOURCES
    GpuWrappers/cl_context.cpp
    GpuWrappers/cl_context.hpp
    GpuWrappers/gl_graphics_pipeline.cpp
    GpuWrappers/gl_graphics_pipeline.hpp
)

set(IO_SOURCES
    io/dds_loader.cpp
    io/hdr_loader.cpp
    io/image_loader.cpp
    io/image_loader.hpp
)

set(KERNELS_SOURCES
    kernels/accumulate_direct_samples.cl
    kernels/bxdf.h
    kernels/constants.h
    kernels/shared_structures.h
    kernels/clear_counter.cl
    kernels/hit_surface.cl
    kernels/increment_counter.cl
    kernels/light.h
    kernels/material.h
    kernels/miss.cl
    kernels/raygeneration.cl
    kernels/reset_radiance.cl
    kernels/resolve_radiance.cl
    kernels/sampling.h
    kernels/trace_bvh.cl
    kernels/utils.h
)

set(MATHLIB_SOURCES
    mathlib/mathlib.cpp
    mathlib/mathlib.hpp
)

set(SCENE_SOURCES
    scene/camera.cpp
    scene/camera.hpp
    scene/scene.cpp
    scene/scene.hpp
)

set(UTILS_SOURCES
    utils/blue_noise_sampler.hpp
    utils/cl_exception.hpp
    utils/framebuffer.cpp
    utils/framebuffer.hpp
    utils/window.cpp
    utils/window.hpp
)

set(MAIN_SOURCES
    acceleration_structure.hpp
    bvh.cpp
    bvh.hpp
    path_trace_integrator.cpp
    path_trace_integrator.hpp
    render.cpp
    render.hpp
    main.cpp
)

set(SOURCES
    ${GPU_WRAPPERS_SOURCES}
    ${IO_SOURCES}
    ${KERNELS_SOURCES}
    ${MATHLIB_SOURCES}
    ${SCENE_SOURCES}
    ${UTILS_SOURCES}
    ${MAIN_SOURCES}
)

source_group("GpuWrappers" FILES ${GPU_WRAPPERS_SOURCES})
source_group("Io" FILES ${IO_SOURCES})
source_group("Kernels" FILES ${KERNELS_SOURCES})
source_group("Mathlib" FILES ${MATHLIB_SOURCES})
source_group("Scene" FILES ${SCENE_SOURCES})
source_group("Utils" FILES ${UTILS_SOURCES})

add_executable(RayTracingApp ${SOURCES})

target_include_directories(RayTracingApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(RayTracingApp PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/tinyobjloader)

target_link_libraries(RayTracingApp PUBLIC OpenCL::OpenCL OpenGL::GL GLEW::GLEW OpenGL::GLU imgui)
set_target_properties(RayTracingApp PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_command(TARGET RayTracingApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/3rdparty/glew-2.1.0/bin/x64/glew32.dll"
    $<TARGET_FILE_DIR:RayTracingApp>)
