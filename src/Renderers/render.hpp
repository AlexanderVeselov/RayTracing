#ifndef RENDER_HPP
#define RENDER_HPP

#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "context/cl_context.hpp"
#include "utils/framebuffer.hpp"
#include <Windows.h>
#include <memory>
#include <ctime>

#define BVH_INTERSECTION

class Render
{
public:
    Render(std::uint32_t width, std::uint32_t weight);

    void         RenderFrame();

    //const HWND   GetHWND()           const;
    double       GetCurtime()        const;
    double       GetDeltaTime()      const;
    //unsigned int GetGlobalWorkSize() const;

    //std::shared_ptr<CLContext> GetCLContext() const;
    //std::shared_ptr<CLKernel>  GetCLKernel()  const;

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
    // Scene
    std::shared_ptr<Camera>      m_Camera;
    std::shared_ptr<Scene>       m_Scene;
    std::shared_ptr<Framebuffer> m_Framebuffer;
    // Buffers
    //cl::Buffer m_OutputBuffer;
    //cl::Image2D m_Texture0;

};

#endif // RENDER_HPP
