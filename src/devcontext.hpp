#ifndef DEVCONTEXT_HPP
#define DEVCONTEXT_HPP

#include "scene.hpp"
#include <CL/cl.hpp>
#include <memory>

#define CL_ARG_PIXEL_BUFFER     0
#define CL_ARG_RANDOM_BUFFER    1
#define CL_ARG_WIDTH            2
#define CL_ARG_HEIGHT           3
#define CL_ARG_CAM_ORIGIN       4
#define CL_ARG_CAM_FRONT        5
#define CL_ARG_CAM_UP           6
#define CL_ARG_SCENE_BUFFER     7
#define CL_ARG_INDEX_BUFFER     8
#define CL_ARG_CELL_BUFFER      9

// OpenCL & OpenGL interloperability
// https://software.intel.com/en-us/articles/opencl-and-opengl-interoperability-tutorial

class ClContext
{
public:
    ClContext(const cl::Platform& platform, const std::string& source, size_t width, size_t height, size_t cell_resolution);
    void SetupBuffers(std::shared_ptr<Scene> scene);

    void SetArgument(size_t index, size_t size, const void* argPtr);
    void WriteRandomBuffer(size_t size, const void* ptr);
    void ExecuteKernel(float3* ptr, size_t size);

    bool IsValid() const { return m_Valid; }

private:
    size_t m_Width;
    size_t m_Height;
    bool m_Valid;

    cl::Context m_Context;
    cl::CommandQueue m_Queue;
    cl::Kernel m_Kernel;
    // Memory Buffers
    cl::Buffer m_PixelBuffer;
    cl::Buffer m_RandomBuffer;
    cl::Buffer m_SceneBuffer;
    cl::Buffer m_IndexBuffer;
    cl::Buffer m_CellBuffer;

};

#endif // DEVCONTEXT_HPP
