include (FindPackageHandleStandardArgs)

set(glfw3_DIR "${CMAKE_SOURCE_DIR}/3rdparty/glfw")

find_library(glfw3_LIBRARY_DEBUG NAMES glfw glfw3 PATHS "${glfw3_DIR}/lib/x64/Debug")
find_library(glfw3_LIBRARY_RELEASE NAMES glfw glfw3 PATHS "${glfw3_DIR}/lib/x64/Release")
find_path(glfw3_INCLUDE_DIR NAMES GLFW/glfw3.h PATHS "${glfw3_DIR}/include")

find_package_handle_standard_args(glfw3 DEFAULT_MSG glfw3_LIBRARY_DEBUG glfw3_INCLUDE_DIR)

add_library(glfw3::glfw3 UNKNOWN IMPORTED)
set_target_properties(glfw3::glfw3 PROPERTIES IMPORTED_LOCATION_DEBUG "${glfw3_LIBRARY_DEBUG}")
set_target_properties(glfw3::glfw3 PROPERTIES IMPORTED_LOCATION_RELEASE "${glfw3_LIBRARY_RELEASE}")
set_target_properties(glfw3::glfw3 PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO "${glfw3_LIBRARY_RELEASE}")
set_target_properties(glfw3::glfw3 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${glfw3_INCLUDE_DIR}")
