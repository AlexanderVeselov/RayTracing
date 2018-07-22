cmake_minimum_required(VERSION 3.8)

project(RayTracing CXX)

set(SOURCES_CONTEXT
	src/Context/cl_context.cpp
	src/Context/cl_context.hpp
)

set(SOURCES_IO
	src/io/dds_loader.cpp
	src/io/hdr_loader.cpp
	src/io/image_loader.hpp
	src/io/inputsystem.cpp
	src/io/inputsystem.hpp
)

set(SRC_KERNELS
    src/kernels/kernel_bvh.cl
    src/kernels/kernel_grid.cl
)

set(SOURCES_MATHLIB
	src/mathlib/mathlib.cpp
	src/mathlib/mathlib.hpp
)

set(SOURCES_RENDERERS
	src/renderers/render.cpp
	src/renderers/render.hpp
)

set(SOURCES_SCENE
	src/scene/camera.cpp
	src/scene/camera.hpp
	src/scene/scene.cpp
	src/scene/scene.hpp
)

set(SOURCES_UTILS
	src/utils/cl_exception.hpp
	src/utils/shared_structs.hpp
	src/utils/viewport.hpp	
)

set(SOURCES
	${SOURCES_CONTEXT}
	${SOURCES_IO}
	${SRC_KERNELS}
	${SOURCES_MATHLIB}
	${SOURCES_RENDERERS}
	${SOURCES_SCENE}
	${SOURCES_UTILS}
	src/main.cpp
)

add_executable(RayTracing ${SOURCES})
find_package(OpenCL REQUIRED)
find_package(OpenGL REQUIRED)
target_include_directories(RayTracing PUBLIC "${RayTracing_SOURCE_DIR}/src")
target_link_libraries(RayTracing PUBLIC OpenCL::OpenCL OpenGL::GL)
set_target_properties(RayTracing PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY ${RayTracing_SOURCE_DIR}
)
