include (FindPackageHandleStandardArgs)

set(OCL_SDK_Light_DIR "${CMAKE_SOURCE_DIR}/3rdparty/OCL_SDK_Light")

find_library(OCL_SDK_Light_LIBRARY_RELEASE NAMES opencl PATHS "${OCL_SDK_Light_DIR}/lib/x86_64")
find_path(OCL_SDK_Light_INCLUDE_DIR NAMES CL/cl.h PATHS "${OCL_SDK_Light_DIR}/include")

find_package_handle_standard_args(OpenCL_Light DEFAULT_MSG OCL_SDK_Light_LIBRARY_RELEASE OCL_SDK_Light_INCLUDE_DIR)

add_library(OpenCL_Light UNKNOWN IMPORTED)
set_target_properties(OpenCL_Light PROPERTIES IMPORTED_LOCATION_DEBUG "${OCL_SDK_Light_LIBRARY_RELEASE}")
set_target_properties(OpenCL_Light PROPERTIES IMPORTED_LOCATION_RELEASE "${OCL_SDK_Light_LIBRARY_RELEASE}")
set_target_properties(OpenCL_Light PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO "${OCL_SDK_Light_LIBRARY_RELEASE}")
set_target_properties(OpenCL_Light PROPERTIES IMPORTED_LOCATION_MINSIZEREL "${OCL_SDK_Light_LIBRARY_RELEASE}")
set_target_properties(OpenCL_Light PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OCL_SDK_Light_INCLUDE_DIR}")
