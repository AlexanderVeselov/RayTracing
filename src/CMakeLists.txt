set(RENDER_SOURCES
    render/integrator.cpp
    render/integrator.hpp
    render/render.cpp
    render/render.hpp
    render/rhi_integrator.cpp
    render/rhi_integrator.hpp
)

set(SHADER_SOURCES
    shaders/shared_structures.h
    shaders/accumulate_direct_samples.cs
    shaders/aov.cs
    shaders/clear_counter.cs
    shaders/common.hlsli
    shaders/copy_history.cs
    shaders/denoiser.cs
    shaders/frame_data.hlsli
    shaders/hit_surface.cs
    shaders/light.hlsli
    shaders/material.hlsli
    shaders/miss.cs
    shaders/raygeneration.cs
    shaders/reset.cs
    shaders/resolve.cs
    shaders/tonemap.cs
    shaders/trace_bvh.cs
    shaders/trace_common.hlsli
    shaders/trace_rayquery.cs
    shaders/trace_shadow_bvh.cs
    shaders/trace_shadow_rayquery.cs
)

set(LOADERS_SOURCES
    loaders/hdr_loader.cpp
    loaders/image_loader.cpp
    loaders/image_loader.hpp
    loaders/obj_loader.cpp
    loaders/obj_loader.hpp
)

set(MANAGERS_SOURCES
    managers/texture_manager.cpp
    managers/texture_manager.hpp
)

set(SCENE_SOURCES
    scene/scene.cpp
    scene/scene.hpp
)

set(ACC_STRUCTURES_SOURCES
    acc_structures/acceleration_structure.hpp
    acc_structures/bvh.cpp
    acc_structures/bvh.hpp
    acc_structures/hardware_rt_acceleration_structure.cpp
    acc_structures/hardware_rt_acceleration_structure.hpp
)

set(UTILS_SOURCES
    utils/aabb.hpp
    utils/camera_controller.cpp
    utils/camera_controller.hpp
    utils/window.cpp
    utils/window.hpp
)

set(SOURCES
    ${RENDER_SOURCES}
    ${SHADER_SOURCES}
    ${LOADERS_SOURCES}
    ${MANAGERS_SOURCES}
    ${SCENE_SOURCES}
    ${ACC_STRUCTURES_SOURCES}
    ${UTILS_SOURCES}
    main.cpp
)

source_group("render" FILES ${RENDER_SOURCES})
source_group("shaders" FILES ${SHADER_SOURCES})
source_group("loaders" FILES ${LOADERS_SOURCES})
source_group("managers" FILES ${MANAGERS_SOURCES})
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
