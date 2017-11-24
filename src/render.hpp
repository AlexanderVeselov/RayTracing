#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "camera.hpp"
#include "scene.hpp"
#include "devcontext.hpp"
#include "viewport.hpp"
#include <Windows.h>
#include <memory>

class Render
{
public:
    void Init(HWND hWnd);
    void RenderFrame();
    void Shutdown();
    const HWND GetHWND() const { return m_hWnd; }
    double GetCurtime() { return GetTickCount() * 0.001; }
    double GetDeltaTime() { return GetCurtime() - m_PreviousFrameTime; }

private:
    void InitGL();

    HWND m_hWnd;
    std::vector<std::shared_ptr<ClContext> > m_Contexts;
    std::shared_ptr<Camera> m_Camera;
    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Viewport> m_Viewport;
    HGLRC m_GlContext;
    HDC m_DC;
    int* m_RandomArray;

    double m_PreviousFrameTime;

};

extern Render* render;

#endif // RAYTRACER_HPP
