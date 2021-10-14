#ifndef RENDER_HPP
#define RENDER_HPP

#include "path_trace_integrator.hpp"
#include "acceleration_structure.hpp"
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
    ~Render();

    void          RenderFrame();

    const HWND    GetHWND()           const { return hwnd_; };
    double        GetCurtime()        const;
    double        GetDeltaTime()      const;

    std::shared_ptr<CLContext> GetCLContext() const { return cl_context_; };

private:
    void InitWindow();
    void InitGL();
    void FrameBegin();
    void FrameEnd();
    void DrawGUI();
    
private:
    HWND hwnd_;

    // Render size
    std::uint32_t width_;
    std::uint32_t height_;

    // Timing
    double start_frame_time_ = 0.0;
    double prev_frame_time_ = 0.0;
    // Contexts
    HGLRC gl_context_;
    std::shared_ptr<CLContext>   cl_context_;
    // Estimator
    std::unique_ptr<PathTraceIntegrator> integrator_;
    // Acceleration structure
    std::unique_ptr<AccelerationStructure> acc_structure_;
    // Scene
    std::shared_ptr<Camera>      camera_;
    std::unique_ptr<Scene>       scene_;
    std::unique_ptr<Framebuffer> framebuffer_;

    struct GuiParams
    {
        float camera_aperture = 0.0f;
        float camera_focus_distance = 10.0f;
        int max_bounces = 3u;
        bool enable_white_furnace = false;
        bool enable_blue_noise = false;
    } gui_params_;

};

#endif // RENDER_HPP
