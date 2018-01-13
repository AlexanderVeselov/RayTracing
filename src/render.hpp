#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "camera.hpp"
#include "scene.hpp"
#include "devcontext.hpp"
#include "viewport.hpp"
#include <Windows.h>
#include <memory>
#include <ctime>

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
    TEXTURE0

};

class Render
{
public:
    void   Init(HWND hWnd);
    void   RenderFrame();
    void   Shutdown();

    const  HWND GetHWND() const { return m_hWnd; }
    double GetCurtime() { return (double)clock() / (double)CLOCKS_PER_SEC; }
    double GetDeltaTime() { return GetCurtime() - m_PreviousFrameTime; }

    HDC    GetDisplayContext() const { return m_DisplayContext; }
    HGLRC  GetGLContext() const { return m_GLContext; }
    std::shared_ptr<CLContext> GetCLContext() const { return m_CLContext; }
    std::shared_ptr<CLKernel> GetCLKernel() const { return m_RenderKernel; }

private:
    // Private methods
    void InitGL();
    void SetupBuffers();
    void FrameBegin();
    void FrameEnd();

    // Class fields
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
    cl::Buffer m_SceneBuffer;
    cl::Buffer m_IndexBuffer;
    cl::Buffer m_CellBuffer;
    cl::Image2D m_Texture0;

};

extern Render* render;

#endif // RAYTRACER_HPP
