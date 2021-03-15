cmake_minimum_required(VERSION 3.8)

project(RayTracing CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(OpenCL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_library(imgui STATIC
    3rdparty/imgui/imconfig.h
    3rdparty/imgui/imgui.cpp
    3rdparty/imgui/imgui.h
    3rdparty/imgui/imgui_demo.cpp
    3rdparty/imgui/imgui_draw.cpp
    3rdparty/imgui/imgui_internal.h
    3rdparty/imgui/imgui_tables.cpp
    3rdparty/imgui/imgui_widgets.cpp
    3rdparty/imgui/imstb_rectpack.h
    3rdparty/imgui/imstb_textedit.h
    3rdparty/imgui/imstb_truetype.h
    3rdparty/imgui/backends/imgui_impl_opengl3.cpp
    3rdparty/imgui/backends/imgui_impl_opengl3.h
    3rdparty/imgui/backends/imgui_impl_win32.cpp
    3rdparty/imgui/backends/imgui_impl_win32.h
)

target_compile_features(imgui PRIVATE cxx_std_17)
target_include_directories(imgui PUBLIC 3rdparty/imgui)
target_compile_options(imgui PRIVATE /WX)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_link_libraries(imgui GLEW::GLEW)

add_subdirectory(src)
