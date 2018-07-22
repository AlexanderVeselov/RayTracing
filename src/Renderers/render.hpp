#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "context/cl_context.hpp"
#include "utils/viewport.hpp"
#include <Windows.h>
#include <memory>
#include <ctime>

#define BVH_INTERSECTION

class Render
{
public:
    void         Init(HWND hWnd);
    void         RenderFrame();
    void         Shutdown();

    const HWND   GetHWND()           const;
    double       GetCurtime()        const;
    double       GetDeltaTime()      const;
    unsigned int GetGlobalWorkSize() const;

    HDC          GetDisplayContext() const;
    HGLRC        GetGLContext()      const;

    std::shared_ptr<CLContext> GetCLContext() const;
    std::shared_ptr<CLKernel>  GetCLKernel()  const;

private:
    void InitGL();
    void SetupBuffers();
    void FrameBegin();
    void FrameEnd();
    
private:
    HWND m_hWnd;
    // Timing
    double m_StartFrameTime;
    double m_PreviousFrameTime;
    // Contexts
    HDC m_DisplayContext;
    HGLRC m_GLContext;
    std::shared_ptr<CLContext>  m_CLContext;
    // Kernels
    std::shared_ptr<CLKernel>   m_RenderKernel;
    // Scene
    std::shared_ptr<Camera>     m_Camera;
    std::shared_ptr<Scene>      m_Scene;
    std::shared_ptr<Viewport>   m_Viewport;
    // Buffers
    cl::Buffer m_OutputBuffer;
    cl::Image2D m_Texture0;

};

extern Render* render;

#endif // RAYTRACER_HPP
