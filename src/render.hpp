#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "camera.hpp"
#include "scene.hpp"
#include "devcontext.hpp"
#include "viewport.hpp"
#include <Windows.h>
#include <memory>
#include <ctime>

#define BVH_INTERSECTION

#ifdef BVH_INTERSECTION
enum RenderKernelArgument_t
{
    BUFFER_OUT = 0,
    BUFFER_SCENE,
    BUFFER_NODE,
    WIDTH,
    HEIGHT,
    CAM_ORIGIN,
    CAM_FRONT,
    CAM_UP,
    FRAME_COUNT,
    FRAME_SEED,
    TEXTURE0,
    
    // Not using now
    BUFFER_INDEX,
    BUFFER_CELL
};
#else
enum RenderKernelArgument_t
{
    BUFFER_OUT = 0,
    BUFFER_SCENE,
    BUFFER_INDEX,
    BUFFER_CELL,
    WIDTH,
    HEIGHT,
    CAM_ORIGIN,
    CAM_FRONT,
    CAM_UP,
    TEXTURE0,

    // Not using now
    BUFFER_NODE

};
#endif

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
    std::shared_ptr<CLContext> m_CLContext;
    // Kernels
    std::shared_ptr<CLKernel> m_RenderKernel;
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
