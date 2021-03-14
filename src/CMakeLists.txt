set(GPU_WRAPPERS_SOURCES
    GpuWrappers/cl_context.cpp
    GpuWrappers/cl_context.hpp
    GpuWrappers/gl_graphics_pipeline.cpp
    GpuWrappers/gl_graphics_pipeline.hpp
)

set(IO_SOURCES
    io/dds_loader.cpp
    io/hdr_loader.cpp
    io/image_loader.hpp
    io/inputsystem.cpp
    io/inputsystem.hpp
)

set(KERNELS_SOURCES
    kernels/kernel_bvh.cl
    kernels/kernel_grid.cl
    kernels/kernel_copy.cl
)

set(MATHLIB_SOURCES
    mathlib/mathlib.cpp
    mathlib/mathlib.hpp
)

set(RENDERERS_SOURCES
    renderers/render.cpp
    renderers/render.hpp
)

set(SCENE_SOURCES
    scene/camera.cpp
    scene/camera.hpp
    scene/scene.cpp
    scene/scene.hpp
)

set(UTILS_SOURCES
    utils/cl_exception.hpp
    utils/shared_structs.hpp
    utils/framebuffer.cpp
    utils/framebuffer.hpp
)

set(MAIN_SOURCES
    main.cpp
)

set(SOURCES
    ${GPU_WRAPPERS_SOURCES}
    ${IO_SOURCES}
    ${KERNELS_SOURCES}
    ${MATHLIB_SOURCES}
    ${RENDERERS_SOURCES}
    ${SCENE_SOURCES}
    ${UTILS_SOURCES}
    ${MAIN_SOURCES}
)

add_executable(RayTracingApp ${SOURCES})

target_include_directories(RayTracingApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(RayTracingApp PUBLIC OpenCL::OpenCL OpenGL::GL GLEW::GLEW OpenGL::GLU)
set_target_properties(RayTracingApp PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_command(TARGET RayTracingApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/3rdparty/glew-2.1.0/bin/x64/glew32.dll"
    $<TARGET_FILE_DIR:RayTracingApp>)
