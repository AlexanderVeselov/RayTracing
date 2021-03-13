#ifndef RENDER_HPP
#define RENDER_HPP

#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "GpuWrappers/cl_context.hpp"
#include "utils/framebuffer.hpp"
#include <Windows.h>
#include <memory>
#include <ctime>

#define BVH_INTERSECTION

class Render
{
public:
    Render(std::uint32_t width, std::uint32_t weight);

    void          RenderFrame();

    const HWND    GetHWND()           const { return hwnd_; };
    double        GetCurtime()        const;
    double        GetDeltaTime()      const;
    std::uint32_t GetGlobalWorkSize() const;

    std::shared_ptr<CLContext> GetCLContext() const { return m_CLContext; };
    std::shared_ptr<CLKernel>  GetCLKernel()  const { return m_RenderKernel; };

private:
    void InitWindow();
    void InitGL();
    void SetupBuffers();
    void FrameBegin();
    void FrameEnd();
    
private:
    HWND hwnd_;

    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    // Timing
    double m_StartFrameTime;
    double m_PreviousFrameTime;
    // Contexts
    HGLRC m_GLContext;
    std::shared_ptr<CLContext>   m_CLContext;
    // Kernels
    std::shared_ptr<CLKernel>    m_RenderKernel;
    std::shared_ptr<CLKernel>    m_CopyKernel;
    // Scene
    std::shared_ptr<Camera>      m_Camera;
    std::shared_ptr<Scene>       m_Scene;
    std::shared_ptr<Framebuffer> m_Framebuffer;
    // Buffers
    cl::Buffer  m_OutputBuffer;
    cl::ImageGL m_OutputImage;
    cl::Image2D m_Texture0;

};

#endif // RENDER_HPP
