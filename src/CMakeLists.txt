set(GPU_WRAPPERS_SOURCES
    gpu_wrappers/cl_context.cpp
    gpu_wrappers/cl_context.hpp
    gpu_wrappers/gl_compute_pipeline.cpp
    gpu_wrappers/gl_compute_pipeline.hpp
    gpu_wrappers/gl_graphics_pipeline.cpp
    gpu_wrappers/gl_graphics_pipeline.hpp
    gpu_wrappers/gl_shader_utils.cpp
    gpu_wrappers/gl_shader_utils.hpp
)

set(INTEGRATOR_SOURCES
    integrator/integrator.cpp
    integrator/integrator.hpp
    integrator/cl_pt_integrator.cpp
    integrator/cl_pt_integrator.hpp
    integrator/gl_pt_integrator.cpp
    integrator/gl_pt_integrator.hpp
)

set(COMMON_KERNELS_SOURCES
    kernels/common/bxdf.h
    kernels/common/constants.h
    kernels/common/light.h
    kernels/common/material.h
    kernels/common/sampling.h
    kernels/common/shared_structures.h
    kernels/common/utils.h
)

set(CL_KERNELS_SOURCES
    kernels/cl/accumulate_direct_samples.cl
    kernels/cl/aov.cl
    kernels/cl/clear_counter.cl
    kernels/cl/denoiser.cl
    kernels/cl/hit_surface.cl
    kernels/cl/increment_counter.cl
    kernels/cl/miss.cl
    kernels/cl/raygeneration.cl
    kernels/cl/reset_radiance.cl
    kernels/cl/resolve_radiance.cl
    kernels/cl/trace_bvh.cl
)

set(GLSL_KERNELS_SOURCES
    kernels/glsl/accumulate_direct_samples.comp
    kernels/glsl/clear_counter.comp
    kernels/glsl/copy_image.comp
    kernels/glsl/fullscreen_quad.vert
    kernels/glsl/fullscreen_quad.frag
    kernels/glsl/hit_surface.comp
    kernels/glsl/increment_counter.comp
    kernels/glsl/initialize_hits.comp
    kernels/glsl/miss.comp
    kernels/glsl/raygeneration.comp
    kernels/glsl/reset_radiance.comp
    kernels/glsl/resolve_radiance.comp
    kernels/glsl/trace_bvh.comp
    kernels/glsl/visibility_buffer.vert
    kernels/glsl/visibility_buffer.frag
)

set(LOADERS_SOURCES
    loaders/dds_loader.cpp
    loaders/hdr_loader.cpp
    loaders/image_loader.cpp
    loaders/image_loader.hpp
)

set(MATHLIB_SOURCES
    mathlib/mathlib.cpp
    mathlib/mathlib.hpp
)

set(SCENE_SOURCES
    scene/scene.cpp
    scene/scene.hpp
)

set(UTILS_SOURCES
    utils/blue_noise_sampler.hpp
    utils/camera_controller.cpp
    utils/camera_controller.hpp
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
    render.cpp
    render.hpp
    main.cpp
)

set(SOURCES
    ${GPU_WRAPPERS_SOURCES}
    ${INTEGRATOR_SOURCES}
    ${CL_KERNELS_SOURCES}
    ${GLSL_KERNELS_SOURCES}
    ${COMMON_KERNELS_SOURCES}
    ${LOADERS_SOURCES}
    ${MATHLIB_SOURCES}
    ${SCENE_SOURCES}
    ${UTILS_SOURCES}
    ${MAIN_SOURCES}
)

source_group("gpu_wrappers" FILES ${GPU_WRAPPERS_SOURCES})
source_group("integrator" FILES ${INTEGRATOR_SOURCES})
source_group("kernels\\common" FILES ${COMMON_KERNELS_SOURCES})
source_group("kernels\\cl" FILES ${CL_KERNELS_SOURCES})
source_group("kernels\\glsl" FILES ${GLSL_KERNELS_SOURCES})
source_group("loaders" FILES ${LOADERS_SOURCES})
source_group("mathlib" FILES ${MATHLIB_SOURCES})
source_group("scene" FILES ${SCENE_SOURCES})
source_group("utils" FILES ${UTILS_SOURCES})

add_executable(RayTracingApp ${SOURCES})

target_include_directories(RayTracingApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(RayTracingApp PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/tinyobjloader ${CMAKE_SOURCE_DIR}/3rdparty/glm)

target_link_libraries(RayTracingApp PUBLIC glfw3::glfw3 OpenCL_Light OpenGL::GL GLEW::GLEW OpenGL::GLU imgui CLI11)
set_target_properties(RayTracingApp PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_command(TARGET RayTracingApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/3rdparty/glew-2.1.0/bin/x64/glew32.dll"
    $<TARGET_FILE_DIR:RayTracingApp>)
