set(INTEGRATOR_SOURCES
    integrator/integrator.cpp
    integrator/integrator.hpp
    integrator/rhi_integrator.cpp
    integrator/rhi_integrator.hpp
)

set(COMMON_KERNELS_SOURCES
    kernels/common/shared_structures.h
)

set(HLSL_KERNELS_SOURCES
    kernels/hlsl/accumulate_direct_samples.cs
    kernels/hlsl/aov.cs
    kernels/hlsl/clear_counter.cs
    kernels/hlsl/clear_sample_counter.cs
    kernels/hlsl/common.hlsli
    kernels/hlsl/copy_history.cs
    kernels/hlsl/denoiser.cs
    kernels/hlsl/frame_data.hlsli
    kernels/hlsl/hit_surface.cs
    kernels/hlsl/increment_counter.cs
    kernels/hlsl/light.hlsli
    kernels/hlsl/material.hlsli
    kernels/hlsl/miss.cs
    kernels/hlsl/raygeneration.cs
    kernels/hlsl/reset.cs
    kernels/hlsl/resolve.cs
    kernels/hlsl/trace_bvh.cs
    kernels/hlsl/trace_common.hlsli
    kernels/hlsl/trace_shadow_bvh.cs
)

set(LOADERS_SOURCES
    loaders/dds_loader.cpp
    loaders/hdr_loader.cpp
    loaders/image_loader.cpp
    loaders/image_loader.hpp
)

set(SCENE_SOURCES
    scene/scene.cpp
    scene/scene.hpp
)

set(UTILS_SOURCES
    utils/aabb.hpp
    utils/camera_controller.cpp
    utils/camera_controller.hpp
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
    ${INTEGRATOR_SOURCES}
    ${HLSL_KERNELS_SOURCES}
    ${COMMON_KERNELS_SOURCES}
    ${LOADERS_SOURCES}
    ${SCENE_SOURCES}
    ${UTILS_SOURCES}
    ${MAIN_SOURCES}
)

source_group("integrator" FILES ${INTEGRATOR_SOURCES})
source_group("kernels\\common" FILES ${COMMON_KERNELS_SOURCES})
source_group("kernels\\hlsl" FILES ${HLSL_KERNELS_SOURCES})
source_group("loaders" FILES ${LOADERS_SOURCES})
source_group("scene" FILES ${SCENE_SOURCES})
source_group("utils" FILES ${UTILS_SOURCES})

add_executable(RayTracingApp ${SOURCES})

target_include_directories(RayTracingApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(RayTracingApp PRIVATE
    ${PROJECT_SOURCE_DIR}/3rdparty/tinyobjloader
    ${CMAKE_SOURCE_DIR}/3rdparty/glm
    ${CMAKE_SOURCE_DIR}/3rdparty/stb
    ${CMAKE_SOURCE_DIR}/3rdparty/blue_noise_sampler)
target_compile_features(RayTracingApp PRIVATE cxx_std_17)

target_link_libraries(RayTracingApp PUBLIC glfw3::glfw3 CLI11 GpuApi)
set_target_properties(RayTracingApp PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_command(TARGET RayTracingApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/3rdparty/GpuApi/third_party/dxc_2022_12_16/bin/x64/dxcompiler.dll"
    $<TARGET_FILE_DIR:RayTracingApp>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/3rdparty/GpuApi/third_party/dxc_2022_12_16/bin/x64/dxil.dll"
    $<TARGET_FILE_DIR:RayTracingApp>)
