#ifndef CL_CONTEXT_HPP
#define CL_CONTEXT_HPP

#include "scene.hpp"
#include <CL/cl.hpp>

class ClContext
{
public:
    ClContext(size_t global_work_size, size_t cell_resolution);
    void SetupBuffers(size_t width, size_t height, const Scene& scene);
    cl::CommandQueue queue;
    cl::Kernel kernel;
    cl::Buffer pixel_buffer, random_buffer, scene_buffer, index_buffer, cell_buffer;
    
    cl::Context context;
    int* random_array;

};

#endif // CL_CONTEXT_HPP
