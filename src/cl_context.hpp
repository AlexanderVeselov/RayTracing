#ifndef CL_m_ContextHPP
#define CL_m_ContextHPP

#include "scene.hpp"
#include <CL/cl.hpp>
#include <fstream>

class ClContext
{
public:
    ClContext(const cl::Platform& platform, const std::string& source, size_t width, size_t height, size_t cell_resolution);
    void SetupBuffers(const Scene& scene);

    void SetArgument(size_t index, size_t size, const void* argPtr);
    void WriteRandomBuffer(size_t size, const void* ptr);
    void ExecuteKernel(cl_float4* ptr, size_t size, size_t offset);

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

#endif // CL_m_ContextHPP
